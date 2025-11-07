import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Effects
import QtCharts

import QMcuPlot
import QPlotTest

ApplicationWindow {
    width: 640
    height: 480
    visible: true

    Material.theme: Material.Dark
    Material.background: Qt.rgba(0.03, 0.1, 0.03)
    Material.foreground: Qt.lighter(Material.background, 8)

    Rectangle {
        anchors.fill: parent
        anchors.margins: 16
        color: Qt.rgba(0.2, 0.58, 0.2, 0.5)

        Rectangle {
            id: container
            anchors.fill: parent
            anchors.margins: 32

            border.color: Material.foreground
            border.width: 2
            radius: 64

            color: Material.background

            Plot {
                id: plot
                anchors.fill: parent
                grid.color: "red"
                grid.ticks: 10

                // layer.enabled: true
            }
            // Rectangle {
            //     id: rectMask
            //     radius: container.radius
            //     anchors.fill: parent
            //     anchors.margins: parent.border.width
            //     layer.enabled: true
            //     layer.samplerName: "maskSource"
            //     layer.effect: ShaderEffect {
            //         property variant source: plot
            //         fragmentShader: "/qmcu/plot/qt-shaders/opacitymask.frag.qsb"
            //     }
            // }
        }
    }
}
