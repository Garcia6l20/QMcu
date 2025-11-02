import QtCore 6.9
import QtQuick 2.15
import QtCharts 2.15

Item {
    id: root
    visible: false

    property ChartView chart: parent instanceof ChartView ? parent : null

    Component.onCompleted: {
        if (!chart) {
            console.error(`AutoScale must be used inside a ChartView`);
        }

        // listen to series additions/removals
        chart.seriesAdded.connect(updateSeriesConnections);
        chart.seriesRemoved.connect(updateSeriesConnections);

        updateSeriesConnections();
    }

    // Store connections so we can disconnect later if needed
    property var seriesConnections: []

    function updateSeriesConnections() {
        // disconnect old
        for (let c of seriesConnections) {
            c.disconnect();
        }
        seriesConnections = [];

        // iterate over current series
        for (let ii = 0; ii < chart.count; ++ii) {
            let s = chart.series(ii);

            let xAxis = s.axisX;
            let yAxis = s.axisY;

            if (xAxis) {
                let connX = xAxis.minChanged.connect(() => onAxisChanged(s));
                seriesConnections.push({
                    disconnect: () => xAxis.minChanged.disconnect(connX)
                });
                let connXMax = xAxis.maxChanged.connect(() => onAxisChanged(s));
                seriesConnections.push({
                    disconnect: () => xAxis.maxChanged.disconnect(connXMax)
                });
            }

            if (yAxis) {
                let connY = yAxis.minChanged.connect(() => onAxisChanged(s));
                seriesConnections.push({
                    disconnect: () => yAxis.minChanged.disconnect(connY)
                });
                let connYMax = yAxis.maxChanged.connect(() => onAxisChanged(s));
                seriesConnections.push({
                    disconnect: () => yAxis.maxChanged.disconnect(connYMax)
                });
            }
        }
    }

    function onAxisChanged(series) {
        // TODO: implement your auto-scale logic here
        console.log("Axis changed for series", series.name, "x:", series.axisX ? series.axisX.min + ".." + series.axisX.max : "?", "y:", series.axisY ? series.axisY.min + ".." + series.axisY.max : "?");
    }
}
