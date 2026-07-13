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
        Kirigami.Heading {
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            level: 1
            text: "A50 Gen 4"
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            implicitHeight: summary.implicitHeight + Kirigami.Units.largeSpacing * 2
            radius: Kirigami.Units.smallSpacing
            color: hardware.connected ? Kirigami.Theme.positiveBackgroundColor
                                      : Kirigami.Theme.negativeBackgroundColor
            ColumnLayout {
                id: summary
                anchors.fill: parent
                anchors.margins: Kirigami.Units.largeSpacing
                RowLayout {
                    Layout.fillWidth: true
                    Kirigami.Icon {
                        source: hardware.connected ? "audio-headphones" : "network-disconnect"
                        implicitWidth: Kirigami.Units.iconSizes.large
                        implicitHeight: Kirigami.Units.iconSizes.large
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Kirigami.Heading {
                            level: 2
                            text: hardware.connected ? "Dock connected" : "Controls disconnected"
                        }
                        Label {
                            text: hardware.connected
                                  ? (hardware.state.headsetOn ? "Headset on" : "Headset off")
                                    + (hardware.state.docked ? " · docked" : " · wireless")
                                  : "Connect the dock in PC mode"
                        }
                    }
                    BusyIndicator { running: hardware.busy; visible: running }
                }
                Label {
                    visible: hardware.errorMessage.length > 0
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: hardware.errorMessage
                    font.bold: true
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            columns: 3
            columnSpacing: Kirigami.Units.largeSpacing
            rowSpacing: Kirigami.Units.largeSpacing

            GroupBox {
                Layout.fillWidth: true
                title: "Battery"
                ColumnLayout {
                    anchors.fill: parent
                    Kirigami.Heading { level: 1; text: hardware.connected ? hardware.state.battery + "%" : "—" }
                    ProgressBar { Layout.fillWidth: true; from: 0; to: 100; value: hardware.state.battery || 0 }
                    Label { text: hardware.state.charging ? "Charging" : "Not charging" }
                }
            }
            GroupBox {
                Layout.fillWidth: true
                title: "Firmware"
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    Label { text: "Base"; opacity: 0.7 }
                    Label { text: hardware.state.baseFirmware || "—"; font.family: "monospace" }
                    Label { text: "Headset"; opacity: 0.7 }
                    Label { text: hardware.state.headsetFirmware || "—"; font.family: "monospace" }
                }
            }
            GroupBox {
                Layout.fillWidth: true
                title: "Current path"
                ColumnLayout {
                    anchors.fill: parent
                    Kirigami.Heading { level: 3; text: controller.statusTitle }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: controller.statusDetail }
                }
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            visible: hardware.neutralized
            type: Kirigami.MessageType.Information
            text: "Linux spatial EQ has priority. The active onboard output EQ is temporarily neutralized without overwriting its saved preset."
        }
        Item { Layout.preferredHeight: Kirigami.Units.largeSpacing }
    }
}
