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

    Debugger {
        executable: "/home/sylvain/projects/b2r/build/nucleo-debug/bin/b2r.elf"
    }

    StLinkProbe {
        serial: "004900423433510B37363934"
        speed: 24000
    }

    readonly property int ra: 200
    readonly property int rb: 40
    readonly property int vddValue: 3300
    readonly property int adcFullScale: 0x0FFF

    readonly property double adcScale: vddValue / adcFullScale

    readonly property double adcFactor: (adcScale * (ra + rb) / rb) / 1000.

    VariableProxy {
        id: vBus
        name: "usbpd_bsp_pwd.vbus_adc_value"
        running: run.checked
        rate: 3 // Hz
        transform: value => value * root.adcFactor
    }

    header: RowLayout {
        Button {
            id: run
            checkable: true
            checked: true
            text: checked ? "Pause" : "Run"
        }
        Label {
            text: `VBus: ${vBus.value.toFixed(3)} V`
        }
    }

    ChartView {
        id: mainChart
        anchors.fill: parent
        title: "Live plot"
        antialiasing: true
        theme: ChartView.ChartThemeDark

        ScrollRecorderSeries {
            id: vBusSeries
            useOpenGL: true
            proxy: vBus
            sampleCount: 50
            pointsVisible: true

            property point selectedPoint: Qt.point(NaN, NaN)
        }

        // Highlight marker
        ScatterSeries {
            id: vBusSeriesHighLight

            property var pt: rubberBand.closest !== null ? mainChart.mapToValue(rubberBand.closest.point, vBusSeries) : null
            markerShape: ScatterSeries.MarkerShapeCircle
            markerSize: 14
            color: "red"
            visible: pt !== null
            onPtChanged: {
                if (pt !== null) {
                    this.name = `${pt.y.toFixed(3)} V`
                    console.debug(`${pt} selected`);
                    this.clear();
                    this.append(pt.x, pt.y);
                } else {
                    this.clear();
                }
            }
        }

        AutoScale {
            yMargin: 0.25
            series: [vBusSeries, vBusSeriesHighLight]
        }
        RubberBand {
            id: rubberBand
        }
    }
}
