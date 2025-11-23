import QtCore
import QtQuick
import QtGraphs
import QMcu.Debug

LineSeries {
    id: root

    property alias proxy: rec.proxy

    BufferRecorder {
        id: rec
        series: root
    }
}
