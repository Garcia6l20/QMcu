import QtCore
import QtQuick
import QtCharts
import QMcuDebug

LineSeries {
    id: root

    property alias proxy: rec.proxy

    BufferRecorder {
        id: rec
        series: root
    }
}
