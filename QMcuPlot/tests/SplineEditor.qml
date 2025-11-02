import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCharts
import QMcuPlot
import Qt.labs.platform

ApplicationWindow {
    id: root
    width: 900
    height: 600
    visible: true
    title: "Spline Editor with SplineSeries"

    property list<point> points: [Qt.point(0, 0), Qt.point(0.3, 0.1), Qt.point(0.5, 0.5), Qt.point(0.7, 0.9), Qt.point(1, 1)]
    property int selectedIndex: -1
    property bool closed: false
    property real tension: 0.5 // 0..1

    function clamp(v, a, b) {
        return Math.max(a, Math.min(b, v));
    }

    function addPoint(pt) {
        console.log(root.points);
        root.points.push(pt);
        root.points.sort((l, r) => {
            if (l.x < r.x) {
                return -1;
            } else {
                return 1;
            }
        });
        console.log(root.points);
        root.selectedIndex = root.points.length - 1;
        chartView.updateSeries();
    }

    function removeSelected() {
        if (root.selectedIndex >= 0 && root.selectedIndex < root.points.length) {
            root.points.splice(root.selectedIndex, 1);
            root.selectedIndex = -1;
            chartView.updateSeries();
        }
    }

    CurveInterpolator {
        id: ci
        points: root.points
    }

    FileIO {
        id: fio
    }

    FileDialog {
        id: fileDialog
        // currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
        nameFilters: ["JSON Files (*.json)"]
        onAccepted: {
            if (fileMode == FileDialog.OpenFile) {
                fio.source = fileDialog.currentFile;
                if (!fio.read()) {
                    console.log(`${fio.source} cannot be read !`);
                    return;
                }
                const data = JSON.parse(fio.text);
                let points = [];
                for (const i of data) {
                    points.push(Qt.point(i.x, i.y));
                }
                root.points = points;
                chartView.updateSeries();
                console.log(`${fio.source} loaded !`);
            } else {
                fio.source = fileDialog.currentFile;
                fio.text = JSON.stringify(root.points);
                if (!fio.write()) {
                    console.log(`${fio.source} cannot be written !`);
                    return;
                }
                console.log(`${fio.source} written !`);
            }
        }
    }

    header: RowLayout {
        RowLayout {
            Label {
                text: "Output size:"
            }
            SpinBox {
                id: nOutputData
                from: 3
                to: 1024 * 10
                value: 32
                onValueChanged: chartView.updateSeries()
            }
        }
        Button {
            text: "Open"
            onClicked: {
                fileDialog.fileMode = FileDialog.OpenFile;
                fileDialog.open();
            }
        }
        Button {
            text: "Save"
            onClicked: {
                fileDialog.fileMode = FileDialog.SaveFile;
                fileDialog.open();
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent

        ChartView {
            id: chartView
            Layout.fillHeight: true
            Layout.fillWidth: true
            antialiasing: true

            ValueAxis {
                id: axisX
                min: 0
                max: 1
                titleText: "X"
            }
            ValueAxis {
                id: axisY
                min: 0
                max: 1
                titleText: "Y"
            }

            SplineSeries {
                id: splineSeries
                // visible: false
                name: "expected"
                axisX: axisX
                axisY: axisY
                color: "blue"
            }

            ScatterSeries {
                id: pointsSeries
                name: "points"
                axisX: axisX
                axisY: axisY
                color: "red"
                markerSize: 10
            }

            ScatterSeries {
                id: controlSeries
                name: "controls"
                axisX: axisX
                axisY: axisY
                color: "yellow"
                markerSize: 7
            }

            LineSeries {
                id: outputSeries
                name: "output"
                axisX: axisX
                axisY: axisY
                color: "green"
                style: Qt.DashLine
            }

            function updateSeries() {
                splineSeries.clear();
                for (var i = 0; i < root.points.length; i++)
                    splineSeries.append(root.points[i].x, root.points[i].y);

                pointsSeries.clear();
                for (var i = 0; i < root.points.length; i++)
                    pointsSeries.append(root.points[i].x, root.points[i].y);

                controlSeries.clear();
                for (var i = 0; i < ci.controlPoints.length; i++)
                    controlSeries.append(ci.controlPoints[i].x, ci.controlPoints[i].y);

                outputSeries.clear();

                let x = [];
                for (let ii = 0; ii < nOutputData.value; ++ii) {
                    x.push(ii / (nOutputData.value - 1));
                }
                const y = ci.eval(x);
                for (var i = 0; i < y.length; i++)
                    outputSeries.append(x[i], y[i]);

                outputText.text = JSON.stringify(y, null, 1);
            }

            Component.onCompleted: chartView.updateSeries()

            MouseArea {
                anchors.fill: parent
                drag.target: null
                focus: true

                property bool dragging: false
                onDoubleClicked: mouse => {
                    const point = chartView.mapToValue(Qt.point(mouse.x, mouse.y));
                    root.addPoint(point);
                }

                onPressed: mouse => {
                    var closest = -1, minD = Infinity;
                    const point = chartView.mapToValue(Qt.point(mouse.x, mouse.y));
                    console.debug(`point: ${point} (mouse: ${mouse.x}, ${mouse.y})`);
                    for (var i = 0; i < root.points.length; i++) {
                        var dx = root.points[i].x - point.x, dy = root.points[i].y - point.y, d2 = dx * dx + dy * dy;
                        if (d2 < minD) {
                            minD = d2;
                            closest = i;
                        }
                    }
                    if (minD < 7.5) {
                        if (closest == 0) {
                            console.debug("first point is fixed !");
                            return;
                        } else if (closest == root.points.length - 1) {
                            console.debug("last point is fixed !");
                            return;
                        }
                        console.debug(`selecting: ${closest}`);
                        root.selectedIndex = closest;
                        focus = true
                        dragging = true;
                    }
                }
                onReleased: mouse => {
                    dragging = false;
                }
                onPositionChanged: mouse => {
                    if (dragging && root.selectedIndex >= 0) {
                        const point = chartView.mapToValue(Qt.point(mouse.x, mouse.y));
                        root.points[root.selectedIndex].x = root.clamp(point.x, 0, chartView.width);
                        root.points[root.selectedIndex].y = root.clamp(point.y, 0, chartView.height);
                        chartView.updateSeries();
                    }
                }
                Keys.onPressed: event => {
                    if (event.key === Qt.Key_Delete) {
                        console.log("Deleting...");
                        root.removeSelected();
                        event.accepted = true;
                    }
                }
            }
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.minimumHeight: 100
            Layout.maximumHeight: 200
            TextArea {
                id: outputText
                visible: true
                wrapMode: Text.WordWrap
                text: ""
            }
        }
    }
}
