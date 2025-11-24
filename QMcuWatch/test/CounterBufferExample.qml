import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import QtGraphs

import QMcu.Debug
import QMcu.Plot
import QMcu.Utils

ApplicationWindow {
    id: root
    visible: true
    minimumWidth: 800
    minimumHeight: 600

    title: `Counter watch example ${dbg.targetArchitecture}`

    Material.theme: Material.Light

    // LoggingErrorDialog {}

    Debugger {
        id: dbg
        executable: "QMcuWatchCounterExample"
    }

    Timer {
        running: dbg.ready
        interval: 500
        onTriggered: {
            console.debug("starting process...");
            dbg.launched = true;
        }
    }

    VariableProxyGroup {
        id: proxies
    }

    Timer {
        interval: 250
        repeat: true
        running: dbg.launched && play.checked
        onTriggered: {
            proxies.update();
            plot.update();
        }
    }

    ColumnLayout {
        anchors.fill: parent

        PlayButton {
            id: play
            checked: true
        }

        PlotView {
            id: plot
            title: "Counter Plot"
            Layout.fillHeight: true
            Layout.fillWidth: true

            axisX: ValueAxis {
                min: 0
                max: counterBuffer.size
            }

            axisY: ValueAxis {
                min: -5
                max: 500
            }

            PlotLineSeries {
                BufferPlotProvider {
                    ArrayProxy {
                        id: counterBuffer
                        name: "counterBuffer"
                        group: proxies
                    }
                }
            }
        }
    }

    BusyIndicator {
        running: !dbg.launched
        visible: !dbg.launched // disable it in order to prevent event grabbing
        anchors.centerIn: parent
    }
}
