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

    TestCounter {
        id: counter1
        size: 101
        shift: 1
        low: 0
        high: 100
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

    PlotView {
        id: plotView
        anchors.fill: parent
        grid.ticks: 3

        axisX: ValueAxis {
            min: 0
            max: counter1.size - 1
        }

        axisY: ValueAxis {
            min: counter1.low - 10
            max: counter1.high + 10
        }

        LinePlotSeries {
            id: ps1
            lineColor: "cyan"
            dataProvider: counter1
            lineStyle: LinePlotSeries.Halo
        }
        LinePlotSeries {
            id: ps2
            lineColor: "magenta"
            dataProvider: counter2
            lineStyle: LinePlotSeries.Halo
        }
    }
}
