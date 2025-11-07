import QMcuPlot 1.0
import QPlotTest 1.0
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Dialogs
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtCharts

ApplicationWindow {
    width: 640
    height: 480
    color: "black"
    visible: true

    Material.theme: Material.Dark
    Material.background: Qt.rgba(0.03, 0.1, 0.03)
    Material.foreground: Qt.lighter(Material.background, 8)

    TestCounter {
        id: counter1
        size: 101
        shift: 1
        low: 0
        high: 200
    }

    TestCounter {
        id: counter2
        initial: 50
        size: 101
        increment: -1
        shift: 4
        low: 0
        high: 100
    }

    Timer {
        interval: 250
        repeat: true
        running: true
        onTriggered: plotView.draw()
    }

    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            Item {
                Layout.fillWidth: true
            }
            Button {
                text: "Save"
                onClicked: {
                    plotView.grabToImage(function (result) {
                        result.saveToFile("counters.png");
                    });
                }
            }
            Item {
                Layout.fillWidth: true
            }
        }
        PlotView {
            id: plotView
            Layout.fillHeight: true
            Layout.fillWidth: true
            grid.ticks: 3

            axisX: ValueAxis {
                min: 0
                max: counter1.size - 1
            }

            axisY: ValueAxis {
                min: counter1.low - 10
                max: counter1.high + 10
            }

            PlotLineSeries {
                id: ps1
                name: "counter1"
                lineColor: "cyan"
                dataProvider: counter1
                lineStyle: PlotLineSeries.Halo
            }
            PlotLineSeries {
                id: ps2
                name: "counter2"
                lineColor: "magenta"
                dataProvider: counter2
                lineStyle: PlotLineSeries.Halo
            }
        }
    }
}
