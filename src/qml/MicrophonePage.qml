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
        Kirigami.Heading { Layout.leftMargin: Kirigami.Units.largeSpacing; level: 1; text: "Microphone" }
        GroupBox {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            title: "Levels"
            ColumnLayout {
                anchors.fill: parent
                LevelRow {
                    Layout.fillWidth: true; label: "Microphone level"; value: hardware.state.microphoneLevel || 0
                    controlEnabled: hardware.connected && !hardware.busy
                    onCommitted: value => hardware.setSlider(4, value)
                }
                LevelRow {
                    Layout.fillWidth: true; label: "Sidetone"; value: hardware.state.sidetone || 0
                    controlEnabled: hardware.connected && !hardware.busy
                    onCommitted: value => hardware.setSlider(5, value)
                }
                LevelRow {
                    Layout.fillWidth: true; label: "Alert volume"; value: hardware.state.alertVolume || 0
                    controlEnabled: hardware.connected && !hardware.busy
                    onCommitted: value => hardware.setAlertVolume(value)
                }
            }
        }
        GroupBox {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            title: "Processing"
            GridLayout {
                anchors.fill: parent
                columns: 2
                Label { text: "Noise gate" }
                ComboBox {
                    Layout.fillWidth: true
                    model: ["Streaming", "Night", "Home", "Tournament"]
                    currentIndex: hardware.state.noiseGate || 0
                    enabled: hardware.connected && !hardware.busy
                    onActivated: hardware.setNoiseGate(currentIndex)
                }
                Label { text: "Microphone EQ" }
                ComboBox {
                    Layout.fillWidth: true
                    model: ["Profile 1", "Profile 2", "Profile 3"]
                    currentIndex: hardware.state.microphoneEq || 0
                    enabled: hardware.connected && !hardware.busy
                    onActivated: hardware.setMicrophoneEq(currentIndex)
                }
            }
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            type: Kirigami.MessageType.Information
            text: "The dock protocol exposes three microphone-EQ values, but their official acoustic names are not published. They remain neutrally labeled to avoid inventing behavior."
        }
        Item { Layout.preferredHeight: Kirigami.Units.largeSpacing }
    }
}
