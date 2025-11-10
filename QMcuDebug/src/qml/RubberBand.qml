import QtCore
import QtQuick
import QtGraphs

Item {
    id: root
    anchors.fill: parent

    property int xScaleZoom: 0
    property int yScaleZoom: 0

    property GraphsView chart: parent instanceof GraphsView ? parent : null

    property var series: []

    function getSeries() {
        let series = [];
        for (let ii = 0; ii < chart.count; ++ii) {
            let s = chart.series(ii);
            if (s instanceof XYSeries) {
                console.log(`Using series: ${s}`);
                series.push(chart.series(ii));
            } else {
                console.log(`Skipping series: ${s}`);
            }
        }
        return series;
    }

    Component.onCompleted: {
        if (!chart)
            console.error(`RubberBand must be used inside a GraphsView`);
        series = getSeries();
    }

    Rectangle {
        id: recZoom
        z: 9999
        border.color: "steelblue"
        border.width: 1
        color: "steelblue"
        opacity: 0.3
        visible: false
        transform: Scale {
            origin.x: 0
            origin.y: 0
            xScale: root.xScaleZoom
            yScale: root.yScaleZoom
        }
    }

    function dist(p1, p2) {
        return Math.hypot(p1.x - p2.x, p1.y - p2.y);
    }

    function closestSeriesPoint(mouse) {
        const mousePt = Qt.point(mouse.x, mouse.y);
        let minDist = Infinity;
        let closestSeries = root.series[0];
        let closest = closestSeries.at(0);
        for (let line of root.series) {
            for (let i = 0; i < line.count; ++i) {
                const p = chart.mapToPosition(line.at(i));
                const d = dist(p, mousePt);
                if (d < closestMinDist && d < minDist) {
                    minDist = d;
                    closestSeries = line;
                    closest = p;
                }
            }
        }
        if (minDist !== Infinity) {
            return {
                series: closestSeries,
                point: closest,
                dist: minDist
            };
        } else {
            return null;
        }
    }

    property var closest: null
    property double closestMinDist: 15

    signal pointSelected(pt: point, series: XYSeries, distance: double)
    signal unselected

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        property bool rClick: false
        onPressed: mouse => {
            // console.debug(`RubberBand press: ${mouse.buttons}`);
            // console.debug(`Closest series: ${root.series}`);
            const had_closest = root.closest !== null;
            root.closest = root.closestSeriesPoint(mouse);
            if (root.closest !== null) {
                console.debug(`Closest point: ${root.closest.point} (from: ${root.closest.series.name}, dist: ${root.closest.dist})`);
                root.pointSelected(root.chart.mapToValue(root.closest.point, root.closest.series), root.closest.series, root.closest.dist);
                mouse.accepted = false;
                return;
            } else if (had_closest) {
                root.unselected();
            }
            if (mouse.buttons == Qt.RightButton) {
                root.chart.zoomReset();
                rClick = true;
            } else {
                recZoom.x = mouseX;
                recZoom.y = mouseY;
                recZoom.visible = true;
            }
            mouse.accepted = true;
        }
        onMouseXChanged: mouse => {
            if (rClick) {
                mouse.accepted = false;
            } else {
                if (mouseX - recZoom.x >= 0) {
                    root.xScaleZoom = 1;
                    recZoom.width = mouseX - recZoom.x;
                } else {
                    root.xScaleZoom = -1;
                    recZoom.width = recZoom.x - mouseX;
                }
            }
        }
        onMouseYChanged: mouse => {
            if (rClick) {
                mouse.accepted = false;
            } else {
                if (mouseY - recZoom.y >= 0) {
                    root.yScaleZoom = 1;
                    recZoom.height = mouseY - recZoom.y;
                } else {
                    root.yScaleZoom = -1;
                    recZoom.height = recZoom.y - mouseY;
                }
            }
        }
        onReleased: mouse => {
            console.debug(`RubberBand release: ${mouse.buttons}`);
            if (rClick) {
                rClick = false;
                root.chart.zoomReset();
            } else {
                recZoom.visible = false;
                if (recZoom.width < 0.05 || recZoom.height < 0.05) {
                    mouse.accepted = false;
                } else {
                    // console.debug("zooming...");
                    var x = (mouseX >= recZoom.x) ? recZoom.x : mouseX;
                    var y = (mouseY >= recZoom.y) ? recZoom.y : mouseY;
                    root.chart.zoomIn(Qt.rect(x, y, recZoom.width, recZoom.height));
                }
            }
        }
    }
}
