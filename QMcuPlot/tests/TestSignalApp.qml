import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import QtCharts

import QMcuPlot
import QPlotTest

ApplicationWindow {
    id: root
    width: 640
    height: 480
    color: "black"
    visible: true

    Material.theme: Material.Dark

    Timer {
        interval: 40
        repeat: true
        running: true
        onTriggered: {
            plot.draw();
        }
    }

    FileDialog {
        id: saveImgDial
        currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
        fileMode: FileDialog.SaveFile
        nameFilters: ["Png files (*.png)", "Jpeg files (*.jpeg)"]
        defaultSuffix: ".png"
        onAccepted: {
            plot.grabToImage(image => {
                image.saveToFile(selectedFile);
            });
        }
    }

    Shortcut {
        sequence: StandardKey.Print
        onActivated: saveImgDial.open()
    }

    RowLayout {
        anchors.fill: parent
        PlotView {
            id: plot
            Layout.fillHeight: true
            Layout.fillWidth: true
            // anchors.fill: parent

            axisX: ValueAxis {
                min: 0
                max: 8000 - 1
            }

            axisY: ValueAxis {
                min: -12.5
                max: +12.5
            }

            LinePlotSeries {
                name: "1 Hz"
                dataProvider: TestSignal {
                    amplitude: 10
                    frequency: 1
                }
            }
            LinePlotSeries {
                name: "2 Hz"
                dataProvider: TestSignal {
                    amplitude: -8
                    frequency: 2
                }
            }
            LinePlotSeries {
                name: "3 Hz"
                dataProvider: TestSignal {
                    amplitude: 9
                    frequency: 3
                }
            }
            LinePlotSeries {
                name: "4 Hz"
                dataProvider: TestSignal {
                    amplitude: -7
                    frequency: 4
                }
            }
            LinePlotSeries {
                name: "5 Hz"
                dataProvider: TestSignal {
                    amplitude: -9
                    frequency: 5
                }
            }
            LinePlotSeries {
                name: "6 Hz"
                dataProvider: TestSignal {
                    amplitude: 8
                    frequency: 6
                }
            }
            LinePlotSeries {
                name: "7 Hz"
                dataProvider: TestSignal {
                    amplitude: -7
                    frequency: 7
                }
            }
            LinePlotSeries {
                name: "8 Hz"
                dataProvider: TestSignal {
                    amplitude: 6
                    frequency: 8
                }
            }
            LinePlotSeries {
                name: "9 Hz"
                dataProvider: TestSignal {
                    amplitude: 7
                    frequency: 9
                }
            }
            LinePlotSeries {
                name: "10 Hz"
                dataProvider: TestSignal {
                    amplitude: -6
                    frequency: 10
                }
            }
        }
    }
}
