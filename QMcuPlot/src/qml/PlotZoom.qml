import QtCore
import QtQuick
import QMcuPlot

Item {
    id: root
    anchors.fill: parent

    property int xScaleZoom: 0
    property int yScaleZoom: 0

    property int autoScaleMargin: 16

    property color rubberBandColor: "steelblue"

    required property Plot plot

    Rectangle {
        id: rubberBand
        z: 9999
        border.color: root.rubberBandColor
        border.width: 1
        color: root.rubberBandColor
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

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        property bool panning: false
        property point panStart: Qt.point(0, 0)

        onWheel: wheel => {
            const delta = 1 - wheel.angleDelta.y / 120 * 0.1;
            if (wheel.modifiers & Qt.ControlModifier) {
                root.plot.zoomX(delta, wheel.x);
            } else {
                root.plot.zoomY(delta, wheel.y);
            }
        }
        onPressed: mouse => {
            // console.debug(`PlotZoom press: ${mouse.buttons}`);
            if (mouse.buttons == Qt.RightButton) {
                root.plot.zoomReset();
                mouse.accepted = false;
            } else {
                if (mouse.modifiers & Qt.ControlModifier) {
                    rubberBand.x = mouseX;
                    rubberBand.y = mouseY;
                    rubberBand.width = 0;
                    rubberBand.height = 0;
                    rubberBand.visible = true;
                } else {
                    panning = true;
                    panStart = Qt.point(mouse.x, mouse.y);
                }
                mouse.accepted = true;
            }
        }
        onMouseXChanged: mouse => {
            if (panning) {
                const panX = mouse.x - panStart.x;
                const panY = mouse.y - panStart.y;
                panStart = Qt.point(mouse.x, mouse.y);
                root.plot.pan(panX, panY);
            } else {
                if (mouseX - rubberBand.x >= 0) {
                    root.xScaleZoom = 1;
                    rubberBand.width = mouseX - rubberBand.x;
                } else {
                    root.xScaleZoom = -1;
                    rubberBand.width = rubberBand.x - mouseX;
                }
            }
        }
        onMouseYChanged: mouse => {
            if (panning) {
                const panX = mouse.x - panStart.x;
                const panY = mouse.y - panStart.y;
                panStart = Qt.point(mouse.x, mouse.y);
                root.plot.pan(panX, panY);
            } else {
                if (mouseY - rubberBand.y >= 0) {
                    root.yScaleZoom = 1;
                    rubberBand.height = mouseY - rubberBand.y;
                } else {
                    root.yScaleZoom = -1;
                    rubberBand.height = rubberBand.y - mouseY;
                }
            }
        }
        onReleased: mouse => {
            if (panning) {
                panning = false;
                return;
            }
            // console.debug(`PlotZoom release: ${mouse.buttons}`);
            rubberBand.visible = false;
            if (rubberBand.width < 0.05 || rubberBand.height < 0.05) {
                mouse.accepted = false;
            } else {
                // console.debug("zooming...");
                var x = (mouseX >= rubberBand.x) ? rubberBand.x : mouseX;
                var y = (mouseY >= rubberBand.y) ? rubberBand.y : mouseY;
                root.plot.zoomIn(Qt.rect(x, y, rubberBand.width, rubberBand.height));
            }
        }
        onDoubleClicked: {
            root.plot.autoScale(root.autoScaleMargin);
        }
    }

    focus: true

    property double keyZoomFactor: 1.025

    Keys.onRightPressed: event => {
        if (event.modifiers & Qt.ControlModifier) {
            root.plot.zoomX(root.keyZoomFactor, width / 2);
        } else {
            root.plot.pan(1, 0);
        }
    }
    Keys.onLeftPressed: event => {
        if (event.modifiers & Qt.ControlModifier) {
            root.plot.zoomX(1 / root.keyZoomFactor, width / 2);
        } else {
            root.plot.pan(-1, 0);
        }
    }
    Keys.onUpPressed: event => {
        if (event.modifiers & Qt.ControlModifier) {
            root.plot.zoomY(root.keyZoomFactor, height / 2);
        } else {
            root.plot.pan(0, -1);
        }
    }
    Keys.onDownPressed: event => {
        if (event.modifiers & Qt.ControlModifier) {
            root.plot.zoomY(1 / root.keyZoomFactor, height / 2);
        } else {
            root.plot.pan(0, 1);
        }
    }
}
