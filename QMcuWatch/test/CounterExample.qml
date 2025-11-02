import QMcuDebug 1.0
import QtCore 6.9
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Dialogs
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtCharts 2.15

ApplicationWindow {
    id: root
    visible: true
    minimumWidth: 800
    minimumHeight: 600

    title: `Counter watch example ${dbg.targetArchitecture}`

    Material.theme: Material.Light

    Debugger {
        id: dbg
        executable: "QMcuWatchCounterExample"
    }

    VariableProxy {
        id: counter
        name: "counter"
        rate: 3 // Hz
        running: true
    }
    VariableProxy {
        id: noiseCounter
        name: "noiseCounter"
        rate: 3 // Hz
        running: true
        transform: value => value * 2
    }

    property string counterStr: "<unknown>"
    header: RowLayout {
        Label {
            text: `Counter value: ${counter.value}`
        }
        Label {
            text: `Running: ${dbg.launched}`
        }
    }

    ColumnLayout {
        anchors.fill: parent
        ChartView {
            id: mainChart
            title: "Counter plot"
            Layout.fillHeight: true
            Layout.fillWidth: true
            antialiasing: true
            theme: ChartView.ChartThemeDark

            ScrollRecorderSeries {
                id: counterSeries
                useOpenGL: true
                proxy: counter
                sampleCount: 50
            }
            ScrollRecorderSeries {
                id: noiseCounterSeries
                useOpenGL: true
                proxy: noiseCounter
                sampleCount: 50
            }
            RubberBand {}
            AutoScale {
                id: scaler
                series: [counterSeries, noiseCounterSeries,]
                yMargin: 10
            }
        }
    }

    BusyIndicator {
        running: !dbg.running || dbg.launching
        anchors.centerIn: parent
    }
}
