import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import QMcu.Utils

Dialog {
    id: root
    standardButtons: Dialog.Ok
    modal: true
    anchors.centerIn: parent

    property var currentLevel: undefined
    property url icon: ""
    property string category: ""
    property string text: ""
    property color iconColor: "red"

    property list<logEntry> entries

    header: ColumnLayout {
        RowLayout {
            Layout.topMargin: 8
            spacing: 10

            Item {}
            Image {
                id: iconItem
                source: root.icon
                width: 24
                height: 24
                sourceSize.width: width
                sourceSize.height: height
                visible: false
                fillMode: Image.PreserveAspectFit
            }
            MultiEffect {
                source: iconItem
                width: iconItem.width
                height: iconItem.height
                colorization: 1.0
                brightness: 1.0
                colorizationColor: "red"
            }

            Label {
                text: root.title
                font.bold: true
                font.pixelSize: 16
                Layout.fillWidth: true
                horizontalAlignment: Qt.AlignHCenter
            }
            Item {}
        }

        Label {
            text: root.category
            font.bold: true
            font.pixelSize: 14
            Layout.fillWidth: true
            horizontalAlignment: Qt.AlignHCenter
        }
    }
    contentItem: Label {
        text: root.text
    }

    onAccepted: {
        console.debug("Accepted");
        if (root.currentLevel == LogInterceptor.Fatal) {
            Qt.exit(-1);
        }
        delayedProcess.running = true
    }
    onRejected: {
        console.debug("Rejected");
        Qt.exit(-1);
    }

    Timer {
        id: delayedProcess
        interval: 20
        repeat: false
        onTriggered: root.processEntries()
    }

    function processEntries() {
        if (visible) {
            return;
        }
        if (entries.length) {
            const entry = entries.shift();
            switch (entry.level) {
            case LogInterceptor.Fatal:
                icon = "qrc:/qmcu/utils/icons/circle-xmark-solid.svg";
                iconColor = "red";
                title = "Fatal error";
                standardButtons = Dialog.Ok;
                break;
            case LogInterceptor.Critical:
                icon = "qrc:/qmcu/utils/icons/circle-xmark-solid.svg";
                iconColor = "red";
                title = "Error";
                standardButtons = Dialog.Ignore | Dialog.Close;
                break;
            case LogInterceptor.Warning:
                icon = "qrc:/qmcu/utils/icons/circle-exclamation-solid.svg";
                iconColor = "yellow";
                title = "Warning";
                standardButtons = Dialog.Ignore | Dialog.Close;
                break;
            }
            category = entry.category;
            text = entry.message;
            open();
            visible = true;
        }
    }

    Connections {
        target: LogInterceptor
        function onFatal(entry) {
            root.entries.push(entry);
            root.processEntries();
        }
        function onCritical(entry) {
            root.entries.push(entry);
            root.processEntries();
        }
        function onWarning(entry) {
            root.entries.push(entry);
            root.processEntries();
        }
    }
}
