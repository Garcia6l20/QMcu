import QMcuPlot 1.0
import QPlotTest 1.0
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Dialogs
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

ApplicationWindow {
    id: root
    width: 640
    height: 480
    color: "black"
    visible: true

    Material.theme: Material.Dark

    TestSignal {
        id: s1
    }

    Timer {
        interval: 20
        repeat: true
        running: true
        onTriggered: {
            plot.update();
        }
    }

    Rectangle {
        id: background
        anchors.fill: parent
        // color: Material.backgroundColor
        // color: '#c7853a'
        color: '#797673'
    }

    property rect ps1ViewRect: Qt.rect(0, 0, 1, 1)

    RowLayout {
        anchors.fill: parent
        PlotView {
            id: plot
            Layout.fillHeight: true
            Layout.fillWidth: true
            // anchors.fill: parent

            LinePlotSeries {
                id: ps1
                dataProvider: s1
                yScale: yUnit
            }
        }

        ColumnLayout {

            Component {
                id: customDial
                Dial {
                    from: -1
                    to: 1
                    value: 0
                }
            }
            RowLayout {
                TestDial {
                    name: "Y Pan"

                    from: -0.5
                    to: 0.5

                    onMoved: {
                        ps1ViewRect.y = value;
                        ps1.setViewRect(root.ps1ViewRect);
                    }
                }
                TestDial {
                    name: "Y Zoom"
                    from: 0.1   // minimal height (zoomed in)
                    to: 1.0     // max height (fully zoomed out)
                    value: ps1ViewRect.height  // start at full view

                    onMoved: {
                        const centerY = ps1ViewRect.y + ps1ViewRect.height / 2;
                        const halfH = value / 2;

                        // update ps1ViewRect to new zoomed height, keep centerY
                        ps1ViewRect = Qt.rect(ps1ViewRect.x, centerY - halfH, ps1ViewRect.width, halfH * 2);

                        ps1.setViewRect(ps1ViewRect);
                        console.log(`Y: ${ps1ViewRect.y.toFixed(2)}, Height: ${ps1ViewRect.height.toFixed(2)}`);
                    }
                }
            }
            RowLayout {
                TestDial {
                    name: "X Pan"

                    from: -1
                    to: 1

                    onMoved: {
                        ps1ViewRect.x = value;
                        ps1.setViewRect(root.ps1ViewRect);
                    }
                }
                TestDial {
                    name: "X Zoom"
                    from: 0.1   // minimal height (zoomed in)
                    to: 2.0     // max height (fully zoomed out)
                    value: ps1ViewRect.width  // start at full view

                    onMoved: {
                        const centerX = ps1ViewRect.x + ps1ViewRect.width / 2;
                        const halfW = value / 2;

                        // update ps1ViewRect to new zoomed height, keep centerY
                        ps1ViewRect = Qt.rect(centerX - halfW, ps1ViewRect.y, halfW * 2, ps1ViewRect.height);

                        ps1.setViewRect(ps1ViewRect);
                        console.log(`X: ${ps1ViewRect.x.toFixed(2)}, Width: ${ps1ViewRect.width.toFixed(2)}`);
                    }
                }
            }
        }
        // Potentiometer {
        //     from: yUnit.valueMin
        //     to: yUnit.valueMax
        //     // color: "#00E676"
        //     onValueChanged: console.log("Gain:", value.toFixed(2), "dB")
        // }
    }
}
