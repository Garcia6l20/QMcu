import QMcuDebug
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import QtGraphs
import QMcuPlot

ApplicationWindow {
    id: root
    visible: true
    minimumWidth: 800
    minimumHeight: 600

    title: `Counter watch example ${dbg.targetArchitecture}`

    Material.theme: Material.Light

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
        running: dbg.launched
        onTriggered: {
            proxies.update();
            plot.update();
        }
    }

    RowLayout {
        anchors.fill: parent
        PlotView {
            id: plot
            title: "Counter Plot"
            Layout.fillHeight: true
            Layout.fillWidth: true

            axisX: ValueAxis {
                min: 0
                max: counterScrollProvider.sampleCount
            }

            axisY: ValueAxis {
                min: -5
                max: 500
            }

            PlotLineSeries {
                ScrollPlotProvider {
                    id: counterScrollProvider
                    VariableProxy {
                        name: "counter"
                        group: proxies
                    }
                }
            }
            PlotLineSeries {
                ScrollPlotProvider {
                    VariableProxy {
                        name: "noiseCounter"
                        group: proxies
                    }
                }
            }
        }
    }

    BusyIndicator {
        running: !dbg.launched
        anchors.centerIn: parent
    }
}
