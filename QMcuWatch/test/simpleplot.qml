import QMcu.Debug
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import QtGraphs

SimplePlot {
    id: root

    title: `Embedded watch example`

    Debugger {
        executable: "/home/sylvain/projects/b2r/build/nucleo-debug/bin/b2r.elf"
    }

    StLinkProbe {
        serial: "004900423433510B37363934"
        speed: 24000
    }

    AutoScale {
        yMargin: 0.25
    }

    ScrollRecorder {

        readonly property int ra: 200
        readonly property int rb: 40
        readonly property int vddValue: 3300
        readonly property int adcFullScale: 0x0FFF

        readonly property double adcScale: vddValue / adcFullScale

        name: "usbpd_bsp_pwd.vbus_adc_value"
        factor: (adcScale * (ra + rb) / rb) / 1000.
        rate: 4
        sampleCount: 50
    }
}
