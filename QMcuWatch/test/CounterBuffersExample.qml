import QtQuick

import QMcuDebug
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window
import QtCharts

ApplicationWindow {
    id: root
    visible: true
    minimumWidth: 800
    minimumHeight: 600

    title: `Counter Buffers watch example ${dbg.targetArchitecture}`

    Material.theme: Material.Light

    Debugger {
        id: dbg
        executable: "QMcuWatchCounterExample"
    }

    VariableProxy {
        id: counterBuffer
        name: "counterBuffer"
        rate: 3 // Hz
        running: true
    }

    VariableProxy {
        id: counter
        name: "counter"
        rate: 3 // Hz
        running: true
    }

    property string counterStr: "<unknown>"
    header: RowLayout {
        Label {
            text: `Running: ${dbg.launched}`
        }
    }

    ColumnLayout {
        anchors.fill: parent
        ChartView {
            id: mainChart
            title: "Counter plot"
            Layout.fillHeight: true
            Layout.fillWidth: true
            antialiasing: true
            theme: ChartView.ChartThemeDark

            ScrollRecorderSeries {
                id: counterSeries
                xMode: ScrollRecorder.Index
                useOpenGL: true
                proxy: counter
                sampleCount: 50
            }
            BufferRecorderSeries {
                id: counterBufferSeries
                useOpenGL: true
                proxy: counterBuffer
            }
            // Highlight marker
            ScatterSeries {
                id: highlight
                markerShape: ScatterSeries.MarkerShapeCircle
                markerSize: 14
                color: "red"
                visible: count > 0
                Connections {
                    target: rubberBand
                    function onPointSelected(pt, s, distance) {
                        highlight.color = Qt.lighter(s.color, 1.5);
                        // highlight.color = s.color;
                        highlight.name = `${s.name}[${pt.x}] = ${pt.y}`;
                        highlight.clear();
                        highlight.append(pt.x, pt.y);
                    }
                    function onUnselected() {
                        highlight.clear();
                    }
                }
            }
            RubberBand {
                id: rubberBand
            }
            AutoScale {
                id: scaler
                series: [counterSeries, counterBufferSeries,]
                yMargin: 10
            }
        }
    }

    BusyIndicator {
        running: !dbg.running || dbg.launching
        anchors.centerIn: parent
    }
}
