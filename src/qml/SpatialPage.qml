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
            level: 1
            text: "Spatial Audio"
        }
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            wrapMode: Text.WordWrap
            text: "Choose the physical A50 USB channel and whether program audio stays direct or is virtualized through 5.1/7.1 HRTF processing."
        }

        GroupBox {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            title: "Physical output"
            RowLayout {
                anchors.fill: parent
                ButtonGroup { id: outputGroup }
                Button {
                    Layout.fillWidth: true
                    text: "A50 Chat" + (controller.chatAvailable ? "  · connected" : "  · unavailable")
                    checkable: true
                    checked: controller.outputEndpoint === 0
                    ButtonGroup.group: outputGroup
                    onClicked: controller.selectOutputEndpoint(0)
                }
                Button {
                    Layout.fillWidth: true
                    text: "A50 Game" + (controller.gameAvailable ? "  · connected" : "  · unavailable")
                    checkable: true
                    checked: controller.outputEndpoint === 1
                    ButtonGroup.group: outputGroup
                    onClicked: controller.selectOutputEndpoint(1)
                }
            }
        }

        GroupBox {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            title: "Listening mode"
            ColumnLayout {
                anchors.fill: parent
                RowLayout {
                    Layout.fillWidth: true
                    ButtonGroup { id: modeGroup }
                    Button {
                        Layout.fillWidth: true; text: "Direct Stereo"; checkable: true
                        checked: controller.mode === 0; ButtonGroup.group: modeGroup
                        onClicked: controller.activateMode(0)
                    }
                    Button {
                        Layout.fillWidth: true; text: "Spatial 5.1"; checkable: true
                        checked: controller.mode === 1; ButtonGroup.group: modeGroup
                        onClicked: controller.activateMode(1)
                    }
                    Button {
                        Layout.fillWidth: true; text: "Spatial 7.1"; checkable: true
                        checked: controller.mode === 2; ButtonGroup.group: modeGroup
                        onClicked: controller.activateMode(2)
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    visible: controller.mode !== 0
                    Label { text: "HRTF profile" }
                    ComboBox {
                        id: profileCombo
                        Layout.fillWidth: true
                        textRole: "name"; valueRole: "id"; model: controller.profileOptions
                        Component.onCompleted: sync()
                        onActivated: controller.selectProfile(currentValue)
                        Connections { target: controller; function onStateChanged() { profileCombo.sync() } }
                        function sync() {
                            for (let i = 0; i < count; ++i)
                                if (valueAt(i) === controller.profileId) currentIndex = i
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    visible: controller.mode !== 0
                    Label { text: "Linux equalizer" }
                    ComboBox {
                        id: eqCombo
                        Layout.fillWidth: true
                        textRole: "name"; valueRole: "id"; model: controller.eqOptions
                        Component.onCompleted: sync()
                        onActivated: controller.selectEqPreset(currentValue)
                        Connections { target: controller; function onStateChanged() { eqCombo.sync() } }
                        function sync() {
                            for (let i = 0; i < count; ++i)
                                if (valueAt(i) === controller.eqPresetId) currentIndex = i
                        }
                    }
                }
                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: controller.mode !== 0
                    type: Kirigami.MessageType.Information
                    text: "Linux EQ is authoritative in spatial modes. Astro Manager temporarily runs the dock output EQ flat and restores it in Direct mode."
                }
            }
        }

        GroupBox {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            title: "Channel calibration"
            ColumnLayout {
                anchors.fill: parent
                Label {
                    Layout.fillWidth: true; wrapMode: Text.WordWrap
                    text: controller.mode === 0 ? "Select a spatial mode to test speaker positions."
                          : "Keep the dock in PC mode and use white-star/source-passthrough on the headset."
                }
                Flow {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    Repeater {
                        model: controller.channelOptions
                        Button {
                            required property var modelData
                            text: modelData
                            enabled: controller.spatialAvailable && !controller.busy
                            onClicked: controller.playChannel(modelData)
                        }
                    }
                }
            }
        }
        Item { Layout.preferredHeight: Kirigami.Units.largeSpacing }
    }
}
