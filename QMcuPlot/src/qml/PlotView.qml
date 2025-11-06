import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Effects

import QMcuPlot

Rectangle {
    id: root

    property color backgroundColor: Material.background
    property color primary: Material.foreground

    color: Qt.darker(backgroundColor, 1.5)

    default property alias series: plot.series
    property alias grid: plot.grid

    property alias axisX: plot.axisX
    property alias axisY: plot.axisY

    property string title

    GridLayout {
        anchors.fill: parent
        columns: 2

        Item {}

        Label {
            visible: root.title !== undefined
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            text: root.title
            font.bold: true
            font.pixelSize: 20
            color: root.primary
        }

        Item {}

        RowLayout {
            Item {
                Layout.fillWidth: true
            }
            Repeater {
                model: plot.series
                delegate: RowLayout {
                    Rectangle {
                        width: 6
                        height: 6
                        radius: 3
                        border.color: modelData.lineColor
                        border.width: 1
                        color: Qt.darker(modelData.lineColor, 2)
                    }
                    Text {
                        text: modelData.name
                        color: root.primary
                    }
                }
            }
            Item {
                Layout.fillWidth: true
            }
        }

        Item {
            implicitWidth: 48
            Layout.fillHeight: true
            // Label {
            //     text: "Y"
            //     anchors.centerIn: parent
            // }

            Repeater {
                model: root.grid.ticks
                delegate: Item {

                    width: parent.width
                    y: parent.height * (modelData + 1) / (root.grid.ticks + 1)

                    Text {
                        id: yTickLbl
                        text: `tick#${modelData}`
                        font.bold: true
                        color: root.primary
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    function sync() {
                        const infos = plot.unitPointsAt(Qt.point(0, y));
                        const info = infos[0];
                        // console.debug(`v-tick#${modelData} ${y} => ${info.mouseUnitPoint.y}`);
                        yTickLbl.text = parseFloat(info.mouseUnitPoint.y.toFixed(2));
                    }

                    onYChanged: sync()

                    Connections {
                        target: plot
                        function onTransformsChanged() {
                            sync();
                        }
                    }
                }
            }
        }

        Rectangle {
            id: wrapper
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: root.backgroundColor
            border.color: root.primary
            border.width: 2

            Layout.topMargin: 8
            Layout.rightMargin: 8
            Layout.bottomMargin: 4
            Layout.leftMargin: 4

            // enable layering to allow masking
            layer.enabled: true
            layer.smooth: true

            radius: 64

            property var overlayPoints: []

            Plot {
                id: plot
                anchors.fill: parent

                // enable layering to allow masking
                layer.enabled: true
                layer.smooth: true

                grid.color: Qt.darker(root.primary, 1.5)

                Rectangle {
                    id: overlay
                    anchors.fill: parent
                    z: 10 // ensures it’s drawn on top of the plot content

                    Repeater {
                        model: wrapper.overlayPoints
                        delegate: Item {
                            required property plotPointInfo modelData

                            x: modelData.seriesLocalPoint.x
                            y: modelData.seriesLocalPoint.y

                            Rectangle {
                                anchors.centerIn: parent
                                radius: 4
                                width: 2 * radius
                                height: 2 * radius
                                border.color: Qt.darker(parent.modelData.series.lineColor, 4)
                                border.width: 1
                                color: Qt.darker(parent.modelData.series.lineColor, 2)
                            }
                        }
                    }
                }
            }

            RowLayout {
                id: infos
                anchors.bottom: wrapper.bottom
                anchors.bottomMargin: 4
                width: wrapper.width
                spacing: 8

                Item {
                    width: wrapper.radius
                }

                Label {
                    text: wrapper.overlayPoints.length ? "X: " + parseFloat(wrapper.overlayPoints[0].mouseUnitPoint.x.toFixed(2)) : ""
                }

                Item {
                    Layout.fillWidth: true
                }

                Repeater {
                    model: wrapper.overlayPoints
                    delegate: Label {
                        required property plotPointInfo modelData
                        text: `${modelData.series.name}: ${parseFloat(modelData.seriesUnitPoint.y.toFixed(2))}`
                        color: modelData.series.lineColor
                    }
                }

                Item {
                    width: wrapper.radius
                }
            }
            // apply shadow on wrapper
            // MultiEffect {
            //     source: wrapper
            //     anchors.fill: wrapper

            //     shadowEnabled: true
            //     shadowColor: Qt.rgba(0.0, 1.0, 0.0, 0.6)
            //     shadowHorizontalOffset: 0
            //     shadowVerticalOffset: 0
            //     shadowBlur: 0.4

            //     brightness: 0.2
            //     contrast: 1.3
            //     saturation: 1.1
            // }
            // clip plot content into its wrapper bound
            MultiEffect {
                source: plot
                anchors.fill: wrapper
                maskEnabled: true
                maskSource: wrapper
                // maskSource: Rectangle {
                //     anchors.fill: wrapper
                //     anchors.centerIn: wrapper
                //     radius: wrapper.radius
                //     layer.enabled: true
                //     visible: false
                //     color: "black"
                // }
                maskThresholdMin: 0.5
                maskSpreadAtMin: 1.0
            }

            PlotZoom {
                id: zoom
                plot: plot
                rubberBandColor: Qt.lighter(root.backgroundColor, 7.5)
            }

            MouseArea {
                anchors.fill: parent
                // acceptedButtons: Qt.LeftButton | Qt.RightButton
                propagateComposedEvents: true
                hoverEnabled: true
                function updateOverlay(mouse) {
                    wrapper.overlayPoints = plot.valuesAt(Qt.point(mouse.x, mouse.y));
                    // for (let info of wrapper.overlayPoints) {
                    //     console.debug(`seriesDataPoint: ${info.seriesDataPoint}`);
                    //     console.debug(`mouseDataPoint: ${info.mouseDataPoint}`);
                    //     console.debug(`seriesUnitPoint: ${info.seriesUnitPoint}`);
                    //     console.debug(`mouseUnitPoint: ${info.mouseUnitPoint}`);
                    //     console.debug(`seriesLocalPoint: ${info.seriesLocalPoint}`);
                    //     console.debug(`mouseLocalPoint: ${info.mouseLocalPoint}`);
                    // }
                }
                onPositionChanged: mouse => updateOverlay(mouse)
                onPressed: mouse => {
                    mouse.accepted = false;
                }
                onExited: {
                    wrapper.overlayPoints = [];
                }
            }
        }

        Item {}

        Item {
            implicitHeight: 64
            Layout.fillWidth: true
            // Label {
            //     text: "X"
            //     anchors.centerIn: parent
            // }

            Repeater {
                model: root.grid.ticks
                delegate: Item {
                    id: tickDelegate

                    required property int modelData

                    height: parent.height
                    x: parent.width * (modelData + 1) / (root.grid.ticks + 1)

                    Text {
                        id: xTickLbl
                        text: `tick#${tickDelegate.modelData}`
                        font.bold: true
                        color: root.primary
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    function sync() {
                        const infos = plot.unitPointsAt(Qt.point(x, 0));
                        const info = infos[0];
                        // console.debug(`v-tick#${modelData} ${x} => ${info.mouseUnitPoint.x}`);
                        xTickLbl.text = parseFloat(info.mouseUnitPoint.x.toFixed(2));
                        // xTickLbl.color = Qt.lighter(info.series.lineColor, 1.5);
                    }

                    onXChanged: sync()

                    Connections {
                        target: plot
                        function onTransformsChanged() {
                            sync();
                        }
                    }
                }
            }
        }
    }

    function draw() {
        plot.draw();
    }

    function update() {
        plot.update();
    }

    function pan(x, y) {
        plot.pan(x, y);
    }

    function panX(x) {
        plot.panX(x);
    }

    function panY(y) {
        plot.panY(y);
    }

    function zoom(x, y) {
        plot.zoom(x, y);
    }

    function zoomX(x) {
        plot.zoomX(x);
    }

    function zoomY(y) {
        plot.zoomY(y);
    }

    function resetZoom() {
        plot.resetZoom();
    }
}
