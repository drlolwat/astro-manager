import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ScrollView {
    contentWidth: availableWidth
    ColumnLayout {
        width: parent.width
        spacing: Kirigami.Units.largeSpacing
        Item { Layout.preferredHeight: Kirigami.Units.smallSpacing }
        Kirigami.Heading { Layout.leftMargin: Kirigami.Units.largeSpacing; level: 1; text: "Stream Port & Game/Voice" }
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            wrapMode: Text.WordWrap
            text: "Control the dock's stream-output mix and the balance restored when the headset starts."
        }
        GroupBox {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            title: "Stream Port Mix"
            ColumnLayout {
                anchors.fill: parent
                LevelRow { Layout.fillWidth: true; label: "Microphone"; value: hardware.state.streamMic || 0; controlEnabled: hardware.connected && !hardware.busy; onCommitted: value => hardware.setSlider(0, value) }
                LevelRow { Layout.fillWidth: true; label: "Chat"; value: hardware.state.streamChat || 0; controlEnabled: hardware.connected && !hardware.busy; onCommitted: value => hardware.setSlider(1, value) }
                LevelRow { Layout.fillWidth: true; label: "Game"; value: hardware.state.streamGame || 0; controlEnabled: hardware.connected && !hardware.busy; onCommitted: value => hardware.setSlider(2, value) }
                LevelRow { Layout.fillWidth: true; label: "Aux"; value: hardware.state.streamAux || 0; controlEnabled: hardware.connected && !hardware.busy; onCommitted: value => hardware.setSlider(3, value) }
            }
        }
        GroupBox {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            title: "Game / Voice balance"
            ColumnLayout {
                anchors.fill: parent
                Label {
                    Layout.fillWidth: true
                    text: "Live rocker position: " + Math.round((hardware.state.balance || 0) / 255 * 100) + "% Chat"
                }
                Slider {
                    id: defaultBalance
                    Layout.fillWidth: true
                    from: 0; to: 255; stepSize: 1
                    value: hardware.state.defaultBalance || 128
                    enabled: hardware.connected && !hardware.busy
                    onPressedChanged: if (!pressed) hardware.setDefaultBalance(Math.round(value))
                }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "100% Game" }
                    Item { Layout.fillWidth: true }
                    Label { text: Math.round(defaultBalance.value / 255 * 100) + "% Chat default" }
                    Item { Layout.fillWidth: true }
                    Label { text: "100% Chat" }
                }
            }
        }
        Item { Layout.preferredHeight: Kirigami.Units.largeSpacing }
    }
}
