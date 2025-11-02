import QMcuDebug 1.0
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

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            MenuSeparator {}
            Action {
                text: qsTr("&Quit")
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: !dbg.ready || dbg.launching
    }

    ColumnLayout {
        anchors.fill: parent
        Label {
            text: `Ready: ${dbg.ready}`
        }
        Label {
            text: `Launched: ${dbg.launched}`
        }
        Label {
            text: `Running: ${dbg.running}`
        }
    }

    Connections {
        target: dbg
        function onTargetLoadingCompleted() {
            for (const watch of config.watches) {
                console.debug(`Setting up watch for '${watch.variable}'`);
                var v = dbg.variable(watch.variable);
                console.debug(`Got variable '${v.type.name} ${v.name}'`);
            }
        }
    }

    Component.onCompleted: {
        console.debug(config);
        dbg.rate = config.rate;
        dbg.load(config.executable);
    }
}
