import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property string name
    property alias from: dial.from
    property alias to: dial.to
    property alias value: dial.value
    property int defaultValue: 0xBADBEEF
    property int precision: 1

    Component.onCompleted: {
        if (defaultValue === 0xBADBEEF) {
            defaultValue = value;
        }
    }

    Label {
        Layout.alignment: Qt.AlignHCenter
        text: root.name
    }
    Dial {
        id: dial
        from: -1
        to: 1
        value: 0

        Label {
            anchors.centerIn: parent
            text: `${dial.value.toFixed(root.precision)}`
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: mouse => {
            if (mouse.button === Qt.RightButton)
                dial.value = root.defaultValue;
        }
    }
}
