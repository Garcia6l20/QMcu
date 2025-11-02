import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

ListView {
    required property QtObject dbg

    model: dbg.globals

    delegate: Label {
        required property string name
        required property QtObject type
        text: type.name + " " + name
    }
}
