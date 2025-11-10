import QtCore
import QtQuick
import QtGraphs
import QMcuDebug

LineSeries {
    id: root

    property alias proxy: rec.proxy

    BufferRecorder {
        id: rec
        series: root
    }
}
