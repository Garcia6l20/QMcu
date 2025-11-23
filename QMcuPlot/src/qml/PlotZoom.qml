import QtCore
import QtQuick
import QMcu.Plot

Item {
    id: root
    anchors.fill: parent

    required property Plot plot

    property int autoScaleMargin: 16
    property color rubberBandColor: "steelblue"

    property int xScaleZoom: 1
    property int yScaleZoom: 1

    Rectangle {
        id: rubberBand
        z: 9999
        visible: rubberBandHandler.active
        border.color: root.rubberBandColor
        border.width: 1
        color: root.rubberBandColor
        opacity: 0.3
        transform: Scale {
            origin.x: 0
            origin.y: 0
            xScale: root.xScaleZoom
            yScale: root.yScaleZoom
        }
    }

    // --- Wheel zoom ---
    WheelHandler {
        id: wheelHandler
        orientation: Qt.Vertical
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: event => {
            const delta = 1 - event.angleDelta.y / 120 * 0.1;
            // console.log("delta", delta);
            if (event.modifiers & Qt.ControlModifier)
                root.plot.zoomX(delta, event.x);
            else
                root.plot.zoomY(delta, event.y);
        }
    }

    // --- Pinch zoom (touch) / untested ---
    PinchHandler {
        id: pinchHandler
        acceptedDevices: PointerDevice.TouchScreen
        onScaleChanged: {
            root.plot.zoom(1 / scale);
        }
    }

    // --- Panning ---
    DragHandler {
        id: panHandler
        target: null
        // acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchScreen
        acceptedModifiers: Qt.NoModifier
        acceptedButtons: Qt.LeftButton
        onTranslationChanged: event => {
            // console.debug(`panHandler: ${event}`);
            root.plot.pan(event.x, event.y);
        }
    }

    // --- RubberBand ---
    DragHandler {
        id: rubberBandHandler
        target: null
        // acceptedDevices: PointerDevice.Mouse
        acceptedModifiers: Qt.ControlModifier
        acceptedButtons: Qt.LeftButton
        // grabPermissions: PointerHandler.TakeOverForbidden

        property point start: Qt.point(0, 0)

        onActiveChanged: {
            // console.debug(`rubberBandHandler: ${active}`);
            if (active) {
                start = centroid.position;
                rubberBand.x = start.x;
                rubberBand.y = start.y;
                rubberBand.width = 0;
                rubberBand.height = 0;
                rubberBand.visible = true;
            } else if (rubberBand.visible) {
                rubberBand.visible = false;
                if (rubberBand.width > 4 && rubberBand.height > 4) {
                    const x = (centroid.position.x >= start.x) ? start.x : centroid.position.x;
                    const y = (centroid.position.y >= start.y) ? start.y : centroid.position.y;
                    root.plot.zoomIn(Qt.rect(x, y, rubberBand.width, rubberBand.height));
                }
            }
        }

        onTranslationChanged: {
            // console.debug(`rectZoomHandler: ${centroid.position}`);
            const p = centroid.position;
            if (p.x - start.x >= 0) {
                root.xScaleZoom = 1;
                rubberBand.width = p.x - start.x;
            } else {
                root.xScaleZoom = -1;
                rubberBand.width = start.x - p.x;
            }

            if (p.y - start.y >= 0) {
                root.yScaleZoom = 1;
                rubberBand.height = p.y - start.y;
            } else {
                root.yScaleZoom = -1;
                rubberBand.height = start.y - p.y;
            }
        }
    }

    // --- Auto-scale ---
    TapHandler {
        acceptedButtons: Qt.LeftButton
        onDoubleTapped: root.plot.autoScale(root.autoScaleMargin)
    }

    // --- Zoom reset ---
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: root.plot.zoomReset()
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
