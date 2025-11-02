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

    title: `Embedded watch example`

    Material.theme: Material.Light

    Debugger {
        id: dbg
        executable: "/home/sylvain/projects/b2r/build/nucleo-debug/bin/b2r.elf"
    }

    StLinkProbe {
        serial: "004900423433510B37363934"
        speed: 4000
    }

    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            CheckBox {
                id: voltageEnabled
                text: "Voltage"
                checked: true
            }
            CheckBox {
                id: adcEnabled
                text: "ADC"
                checked: false
            }
            Button {
                id: run
                checkable: true
                checked: true
                text: checked ? "Pause" : "Run"
            }
        }
        ChartView {
            id: mainChart
            title: "Live plot"
            Layout.fillHeight: true
            Layout.fillWidth: true
            antialiasing: true
            theme: ChartView.ChartThemeDark

            LineSeries {
                id: adcSeries
                visible: adcEnabled.checked
                bestFitLineVisible: true
            }

            LineSeries {
                id: voltageSeries
                visible: voltageEnabled.checked
                bestFitLineVisible: true
            }

            RubberBand {}
        }
    }

    AutoScale {
        id: scaler
        series: [adcSeries, voltageSeries,]
    }

    ScrollRecorder {
        target: "usbpd_bsp_pwd.vbus_adc_value"
        sampleCount: 50
        rate: 2 // Hz
        series: adcSeries
        running: run.checked
    }
    ScrollRecorder {
        target: "usbpd_bsp_pwd.vbus_value"
        sampleCount: 50
        rate: 2 // Hz
        series: voltageSeries
        running: run.checked
    }
}
