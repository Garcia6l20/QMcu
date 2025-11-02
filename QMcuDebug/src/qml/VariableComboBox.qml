import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

ComboBox {
    id: root
    required property QtObject dbg

    model: dbg.globals

    textRole: "display"
}
