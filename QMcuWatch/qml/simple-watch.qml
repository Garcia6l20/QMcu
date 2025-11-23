import QMcu.Debug 1.0
import QtCore 6.9
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Dialogs
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

ApplicationWindow {
    id: root
    visible: true

    title: "Stm32 watch"

    Material.theme: Material.Light

    Settings {
        id: settings

        property string executablePath

        category: "global"
    }

    readonly property QtObject dbg: debuggerInstance

    FileDialog {
        id: openExecutableFile
        onAccepted: {
            root.loadExecutable(selectedFile);
        }
    }

    Shortcut {
        sequence: "CTRL+o"
        onActivated: openExecutableFile.open()
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&Open...")

                onTriggered: openExecutableFile.open()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Quit")
            }
        }
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            VariableComboBox {
                id: varCombo
                focus: true

                Layout.fillWidth: true
                onCurrentIndexChanged: dbg.watcher.variable = varCombo.valueAt(varCombo.currentIndex)
                // text: Shared.settings.comDevice
                dbg: root.dbg
            }
            SpinBox {
                id: rate
                from: 1
                to: 10000
                value: 10
                stepSize: 10

                readonly property double kHz: value / 1000.
                readonly property double hz: value
            }
            Button {
                id: startButton
                enabled: dbg.ready
                text: dbg.ready && dbg.launched ? qsTr("Stop") : qsTr("Start")
                onClicked: dbg.watcher.running = !dbg.watcher.running
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: !dbg.ready || dbg.launching
    }

    ColumnLayout {
        anchors.fill: parent
        Label { text: `Ready: ${dbg.ready}` }
        Label { text: `Launched: ${dbg.launched}` }
        Label { text: `Running: ${dbg.running}` }
    }

    function loadExecutable(file) {
        console.log(`loading ${file}...`);
        settings.executablePath = file;
        settings.sync();
        dbg.load(file);
    }

    Component.onCompleted: {
        if (settings.executablePath) {
            openExecutableFile.currentFile = settings.executablePath;
            loadExecutable(settings.executablePath);
        }
    }
}
