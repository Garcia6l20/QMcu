import QMcuDebug
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import QtGraphs

ApplicationWindow {
    id: root
    visible: true

    default property alias content: recorderContainer.data

    readonly property list<ScrollRecorder> scrollRecorders: content.filter(item => item instanceof ScrollRecorder)

    readonly property AutoScale scaler: {
        const tmp = content.filter(item => item instanceof AutoScale);
        if (tmp.length > 0) {
            return tmp[0];
        } else {
            console.log("creating default AutoScale...");
            return Qt.createQmlObject("AutoScale {}", root, "DefaultAutoScale");
        }
    }

    title: `Embedded watch example`

    Material.theme: Material.Light

    header: RowLayout {
        Repeater {
            id: enabledRepeater
            model: scrollRecorders
            CheckBox {
                text: modelData.name
                checked: true
            }
        }
        Button {
            id: run
            checkable: true
            checked: true
            text: checked ? "Pause" : "Run"
        }
    }

    GraphsView {
        id: mainChart
        anchors.fill: parent
        title: "Live plot"
        Layout.fillHeight: true
        Layout.fillWidth: true
        antialiasing: true

        RubberBand {}
    }

    Item {
        id: recorderContainer
        visible: false
    }

    Component.onCompleted: {
        let index = 0;
        for (let rec of root.scrollRecorders) {
            var line = mainChart.seriesList.append(L);
            rec.series = line;
            scaler.series.push(line);

            line.visible = Qt.binding(() => enabledRepeater.itemAt(index).checked);
            rec.running = Qt.binding(() => run.checked);

            ++index;
        }
    }
}
