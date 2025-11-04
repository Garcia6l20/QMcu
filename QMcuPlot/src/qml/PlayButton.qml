import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

RoundButton {
    implicitWidth: 40
    implicitHeight: 40
    radius: width / 2
    icon.width: 24
    icon.height: 24
    flat: true
    checkable: true
    icon.source: checked ? "qrc:/qmcu/plot/images/pause.svg" : "qrc:/qmcu/plot/images/play.svg"
}
