import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Dialogs
import QtGraphs

import QMcu.Plot
import QMcu.Utils

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
        graphsView.updateSeries();
    }

    function removeSelected() {
        if (root.selectedIndex >= 0 && root.selectedIndex < root.points.length) {
            root.points.splice(root.selectedIndex, 1);
            root.selectedIndex = -1;
            graphsView.updateSeries();
        }
    }

    CurveInterpolator {
        id: ci
        points: root.points
    }

    FileDialog {
        id: fileDialog
        // currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
        nameFilters: ["JSON Files (*.json)"]
        onAccepted: {
            if (fileMode == FileDialog.OpenFile) {
                const text = FileIO.readText(fileDialog.currentFile);
                if (!text.length) {
                    console.log(`${fileDialog.currentFile} cannot be read !`);
                    return;
                }
                const data = JSON.parse(text);
                let points = [];
                for (const i of data) {
                    points.push(Qt.point(i.x, i.y));
                }
                root.points = points;
                graphsView.updateSeries();
                console.log(`${fileDialog.currentFile} loaded !`);
            } else {
                if (!FileIO.writeText(fileDialog.currentFile, JSON.stringify(root.points))) {
                    console.log(`${fileDialog.currentFile} cannot be written !`);
                    return;
                }
                console.log(`${fileDialog.currentFile} written !`);
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
                onValueChanged: graphsView.updateSeries()
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

        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            focus: true

            TapHandler {
                onDoubleTapped: {
                    const pa = graphsView.plotArea;
                    const pt = Qt.point(point.position.x, point.position.y);
                    root.addPoint(Qt.point((pt.x - pa.x) / pa.width, 1 - ((pt.y - pa.y) / pa.height)));
                }
            }

            Keys.onPressed: event => {
                if (event.key === Qt.Key_Delete && pointsSeries.selectedPoints.length) {
                    console.log("Deleting...");
                    root.points.splice(pointsSeries.selectedPoints[0], 1);
                    graphsView.updateSeries();
                    event.accepted = true;
                }
            }
            GraphsView {
                id: graphsView
                anchors.fill: parent
                antialiasing: true

                axisX: ValueAxis {
                    min: 0
                    max: 1
                    titleText: "X"
                }
                axisY: ValueAxis {
                    min: 0
                    max: 1
                    titleText: "Y"
                }

                SplineSeries {
                    id: splineSeries
                    name: "expected"
                    color: "blue"
                }

                ScatterSeries {
                    id: pointsSeries
                    name: "points"
                    pointDelegate: Rectangle {
                        property bool pointSelected
                        radius: 6
                        width: radius * 2
                        height: radius * 2
                        border.width: pointSelected ? 1 : 0
                        border.color: "purple"
                        color: "red"
                        onXChanged: {}
                    }
                    draggable: true
                    selectable: true
                    onPointReplaced: ii => {
                        if (ii != 0 && ii < this.count - 1) {
                            root.points[ii] = this.at(ii);
                        }
                        graphsView.updateSeries();
                    }
                }

                ScatterSeries {
                    id: controlSeries
                    name: "controls"
                    pointDelegate: Rectangle {
                        radius: 3
                        width: radius * 2
                        height: radius * 2
                        color: "yellow"
                    }
                }

                LineSeries {
                    id: outputSeries
                    name: "output"
                    color: "green"
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

                Component.onCompleted: graphsView.updateSeries()
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
