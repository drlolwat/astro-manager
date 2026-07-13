import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ScrollView {
    id: page
    contentWidth: availableWidth
    property int selectedSlot: hardware.state.activeEqPreset || 1
    property var presets: hardware.state.eqPresets || []
    property var preset: presets.length >= selectedSlot ? presets[selectedSlot - 1] : ({"name":"", "bands":[]})
    Connections {
        target: hardware
        function onStateChanged() {
            if (hardware.state.activeEqPreset)
                page.selectedSlot = hardware.state.activeEqPreset
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: Kirigami.Units.largeSpacing
        Item { Layout.preferredHeight: Kirigami.Units.smallSpacing }
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            Kirigami.Heading { Layout.fillWidth: true; level: 1; text: "Hardware EQ" }
            Switch {
                text: "Audition onboard EQ"
                visible: controller.mode !== 0
                onToggled: hardware.auditionHardwareEq(checked)
            }
        }
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            wrapMode: Text.WordWrap
            text: "These are the three five-band presets stored by the dock. Changes preview live but are not persistent until Save to Headset."
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            visible: hardware.neutralized
            type: Kirigami.MessageType.Information
            text: "Spatial audio is active, so the dock's output gains are temporarily flat. Your edited values are retained and will be restored in Direct mode or saved explicitly."
        }
        TabBar {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            currentIndex: page.selectedSlot - 1
            Repeater {
                model: 3
                TabButton {
                    text: page.presets.length > index ? (index + 1) + " · " + page.presets[index].name : "Slot " + (index + 1)
                    onClicked: {
                        page.selectedSlot = index + 1
                        hardware.setActiveEqPreset(index + 1)
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            Label { text: "Preset name" }
            TextField {
                id: nameField
                Layout.fillWidth: true
                text: page.preset.name || ""
                enabled: hardware.connected && !hardware.busy
                onEditingFinished: hardware.setEqPresetName(page.selectedSlot, text)
            }
        }
        Repeater {
            model: page.preset.bands || []
            delegate: GroupBox {
                required property var modelData
                required property int index
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                title: "Band " + (index + 1) + ((index === 0 || index === 4) ? " · shelf" : " · bell")
                GridLayout {
                    anchors.fill: parent
                    columns: 6
                    Label { text: "Gain" }
                    SpinBox {
                        from: -7; to: 7; value: modelData.gain
                        editable: true
                        enabled: hardware.connected && !hardware.busy
                        textFromValue: function(value) { return (value > 0 ? "+" : "") + value + " dB" }
                        valueFromText: function(text) { return parseInt(text) }
                        onValueModified: hardware.setEqGain(page.selectedSlot, index + 1, value)
                    }
                    Label { text: "Frequency" }
                    SpinBox {
                        from: 80; to: 15000; stepSize: 10; value: modelData.frequency
                        editable: true
                        enabled: hardware.connected && !hardware.busy
                        textFromValue: function(value) { return value + " Hz" }
                        valueFromText: function(text) { return parseInt(text) }
                        onValueModified: hardware.setEqBand(page.selectedSlot, index + 1, value,
                            (index === 0 || index === 4) ? 0 : bandwidth.value)
                    }
                    Label { text: "Width" }
                    SpinBox {
                        id: bandwidth
                        from: 1; to: 30; value: Math.round((modelData.bandwidth || 1) * 10)
                        enabled: index !== 0 && index !== 4 && hardware.connected && !hardware.busy
                        textFromValue: function(value) { return (value / 10).toFixed(1) }
                        valueFromText: function(text) { return Math.round(parseFloat(text) * 10) }
                        onValueModified: hardware.setEqBand(page.selectedSlot, index + 1,
                            modelData.frequency, value / 10)
                    }
                }
            }
        }
        Item { Layout.preferredHeight: Kirigami.Units.largeSpacing }
    }
}
