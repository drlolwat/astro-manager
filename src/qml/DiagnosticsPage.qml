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
        Kirigami.Heading { Layout.leftMargin: Kirigami.Units.largeSpacing; level: 1; text: "Diagnostics" }
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            Button { text: "Refresh device"; enabled: !hardware.busy; onClicked: hardware.refresh() }
            Button { text: "Run audio checks"; enabled: !controller.busy; onClicked: controller.runSelfTest() }
            Button { text: "Reapply audio path"; enabled: !controller.busy; onClicked: controller.refresh() }
            BusyIndicator { running: hardware.busy || controller.busy; visible: running }
            Item { Layout.fillWidth: true }
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            visible: hardware.errorMessage.length > 0
            type: Kirigami.MessageType.Error
            text: hardware.errorMessage
        }
        GroupBox {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            title: "Signal path"
            ColumnLayout {
                anchors.fill: parent
                Label {
                    Layout.fillWidth: true
                    visible: !controller.limiterInstalled
                    color: Kirigami.Theme.negativeTextColor
                    wrapMode: Text.WordWrap
                    text: "Safety limiter missing: install lsp-plugins-lv2."
                }
                TextArea {
                    Layout.fillWidth: true
                    implicitHeight: 220
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    text: controller.verificationReport || "Run audio checks for a host-side verification report."
                    font.family: "monospace"
                }
            }
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            type: Kirigami.MessageType.Warning
            text: "USB bit-perfect can be verified only in Direct mode for 48 kHz S16 audio at unity software volume. Linux cannot verify the proprietary wireless hop or post-USB headset processing."
        }
        GroupBox {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            title: "Protocol scope"
            Label {
                anchors.fill: parent
                wrapMode: Text.WordWrap
                text: "Daily controls use the decoded A50 Gen 4 protocol on USB interface 6. Firmware versions are read-only. Firmware flashing and unverified raw commands are deliberately unavailable."
            }
        }
        Item { Layout.preferredHeight: Kirigami.Units.largeSpacing }
    }
}
