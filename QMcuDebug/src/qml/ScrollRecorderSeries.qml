import QtCore
import QtQuick
import QtGraphs
import QMcuDebug

LineSeries {
    id: root

    property alias proxy: rec.proxy
    property alias sampleCount: rec.sampleCount
    property alias factor: rec.factor
    property alias xMode: rec.xMode

    ScrollRecorder {
        id: rec
        series: root
    }
}
