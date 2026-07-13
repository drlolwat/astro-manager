import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ScrollView {
    id: page
    contentWidth: availableWidth
    property string selectedId: ""

    FileDialog {
        id: importDialog
        title: "Import Astro Manager profile"
        nameFilters: ["Astro Manager profiles (*.json)"]
        onAccepted: controller.importUnifiedProfile(selectedFile)
    }
    FileDialog {
        id: exportDialog
        title: "Export Astro Manager profile"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Astro Manager profiles (*.json)"]
        onAccepted: controller.exportUnifiedProfile(page.selectedId, selectedFile)
    }
    Dialog {
        id: createDialog
        title: "Create unified profile"
        modal: true
        standardButtons: Dialog.Save | Dialog.Cancel
        anchors.centerIn: parent
        onAccepted: {
            controller.createUnifiedProfile(profileName.text)
            profileName.clear()
        }
        contentItem: ColumnLayout {
            Label { text: "Capture the current spatial and headset settings." }
            TextField { id: profileName; Layout.fillWidth: true; placeholderText: "Profile name" }
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
            Kirigami.Heading { Layout.fillWidth: true; level: 1; text: "Unified Profiles" }
            Button { text: "Import"; onClicked: importDialog.open() }
            Button { text: "Capture current"; highlighted: true; enabled: hardware.connected; onClicked: createDialog.open() }
        }
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            wrapMode: Text.WordWrap
            text: "A unified profile includes spatial mode, Chat/Game output, Linux EQ, all three hardware EQ slots, microphone settings, Stream Port Mix, and Game/Voice default. Applying previews headset values; it never saves them automatically."
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            visible: controller.profileError.length > 0
            type: Kirigami.MessageType.Error
            text: controller.profileError
        }
        Label {
            Layout.leftMargin: Kirigami.Units.largeSpacing
            visible: controller.savedProfiles.length === 0
            text: "No saved profiles yet."
            opacity: 0.7
        }
        Repeater {
            model: controller.savedProfiles
            delegate: GroupBox {
                required property var modelData
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                RowLayout {
                    anchors.fill: parent
                    ColumnLayout {
                        Layout.fillWidth: true
                        Kirigami.Heading { level: 3; text: modelData.name }
                        Label { text: "Updated " + modelData.modified; opacity: 0.7 }
                    }
                    Button {
                        text: "Apply"
                        enabled: hardware.connected && !hardware.busy
                        onClicked: controller.applyUnifiedProfile(modelData.id)
                    }
                    Button {
                        text: "Export"
                        onClicked: { page.selectedId = modelData.id; exportDialog.open() }
                    }
                    Button {
                        text: "Delete"
                        onClicked: controller.deleteUnifiedProfile(modelData.id)
                    }
                }
            }
        }
        Item { Layout.preferredHeight: Kirigami.Units.largeSpacing }
    }
}
