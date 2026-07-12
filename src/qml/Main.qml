import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    width: 760
    height: 760
    minimumWidth: 620
    minimumHeight: 620
    visible: true
    title: "Astro Spatial"

    pageStack.initialPage: Kirigami.Page {
        title: "Astro Spatial"
        padding: Kirigami.Units.largeSpacing

        ScrollView {
            anchors.fill: parent
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: Kirigami.Units.largeSpacing

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: statusColumn.implicitHeight + Kirigami.Units.largeSpacing * 2
                    radius: Kirigami.Units.smallSpacing
                    color: controller.errorMessage.length > 0
                           ? Kirigami.Theme.negativeBackgroundColor
                           : controller.connected
                             ? Kirigami.Theme.positiveBackgroundColor
                             : Kirigami.Theme.neutralBackgroundColor

                    ColumnLayout {
                        id: statusColumn
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.largeSpacing
                        spacing: Kirigami.Units.smallSpacing

                        RowLayout {
                            Layout.fillWidth: true
                            Kirigami.Icon {
                                source: controller.connected ? "audio-headphones" : "network-disconnect"
                                implicitWidth: Kirigami.Units.iconSizes.medium
                                implicitHeight: Kirigami.Units.iconSizes.medium
                            }
                            Kirigami.Heading {
                                Layout.fillWidth: true
                                level: 2
                                text: controller.statusTitle
                            }
                            BusyIndicator {
                                running: controller.busy
                                visible: running
                            }
                        }
                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: controller.statusDetail
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: controller.errorMessage.length > 0
                            wrapMode: Text.WordWrap
                            text: controller.errorMessage
                            font.bold: true
                        }
                    }
                }

                Kirigami.Heading { level: 2; text: "Physical output" }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: "Choose which USB playback channel receives both Direct and spatialized audio. This does not process the microphone."
                }
                RowLayout {
                    Layout.fillWidth: true
                    ButtonGroup { id: outputGroup }
                    Button {
                        Layout.fillWidth: true
                        text: "A50 Chat" + (controller.chatAvailable ? "  • connected" : "  • unavailable")
                        checkable: true
                        checked: controller.outputEndpoint === 0
                        ButtonGroup.group: outputGroup
                        onClicked: controller.selectOutputEndpoint(0)
                    }
                    Button {
                        Layout.fillWidth: true
                        text: "A50 Game" + (controller.gameAvailable ? "  • connected" : "  • unavailable")
                        checkable: true
                        checked: controller.outputEndpoint === 1
                        ButtonGroup.group: outputGroup
                        onClicked: controller.selectOutputEndpoint(1)
                    }
                }

                Kirigami.Heading { level: 2; text: "Listening mode" }
                RowLayout {
                    Layout.fillWidth: true
                    ButtonGroup { id: modeGroup }
                    Button {
                        Layout.fillWidth: true
                        text: "Direct Stereo"
                        checkable: true
                        checked: controller.mode === 0
                        ButtonGroup.group: modeGroup
                        onClicked: controller.activateMode(0)
                    }
                    Button {
                        Layout.fillWidth: true
                        text: "Spatial 5.1"
                        checkable: true
                        checked: controller.mode === 1
                        ButtonGroup.group: modeGroup
                        onClicked: controller.activateMode(1)
                    }
                    Button {
                        Layout.fillWidth: true
                        text: "Spatial 7.1"
                        checkable: true
                        checked: controller.mode === 2
                        ButtonGroup.group: modeGroup
                        onClicked: controller.activateMode(2)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: controller.mode !== 0
                    Label { text: "Profile" }
                    ComboBox {
                        id: profileCombo
                        Layout.fillWidth: true
                        textRole: "name"
                        valueRole: "id"
                        model: controller.profileOptions
                        Component.onCompleted: syncSelection()
                        onModelChanged: syncSelection()
                        onActivated: controller.selectProfile(currentValue)
                        Connections {
                            target: controller
                            function onStateChanged() { profileCombo.syncSelection() }
                        }
                        function syncSelection() {
                            for (let i = 0; i < count; ++i) {
                                if (valueAt(i) === controller.profileId) {
                                    currentIndex = i
                                    return
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: controller.mode !== 0
                    Label { text: "Equalizer" }
                    ComboBox {
                        id: eqCombo
                        Layout.fillWidth: true
                        textRole: "name"
                        valueRole: "id"
                        model: controller.eqOptions
                        Component.onCompleted: syncSelection()
                        onModelChanged: syncSelection()
                        onActivated: controller.selectEqPreset(currentValue)
                        Connections {
                            target: controller
                            function onStateChanged() { eqCombo.syncSelection() }
                        }
                        function syncSelection() {
                            for (let i = 0; i < count; ++i) {
                                if (valueAt(i) === controller.eqPresetId) {
                                    currentIndex = i
                                    return
                                }
                            }
                        }
                    }
                }
                Label {
                    Layout.fillWidth: true
                    visible: controller.mode !== 0 && controller.eqPresetId === "bass-heavy"
                    wrapMode: Text.WordWrap
                    opacity: 0.75
                    text: "Bass Heavy adds a strong low shelf and sub-bass punch while reducing the upper-mid and treble edge. The safety limiter remains last in the chain."
                }

                Kirigami.Separator { Layout.fillWidth: true }

                Kirigami.Heading { level: 2; text: "Channel calibration" }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: controller.mode === 0
                          ? "Select a spatial mode to test virtual speaker positions."
                          : "Tap each channel and confirm that its direction matches the label. Keep the dock in PC mode and hardware Dolby on the white-star passthrough setting."
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

                Kirigami.Separator { Layout.fillWidth: true }

                RowLayout {
                    Layout.fillWidth: true
                    Kirigami.Heading {
                        Layout.fillWidth: true
                        level: 2
                        text: "Signal-path verification"
                    }
                    Button {
                        text: "Run checks"
                        onClicked: controller.runSelfTest()
                    }
                    Button {
                        text: "Reapply"
                        onClicked: controller.refresh()
                    }
                }
                Label {
                    Layout.fillWidth: true
                    visible: !controller.limiterInstalled
                    color: Kirigami.Theme.negativeTextColor
                    wrapMode: Text.WordWrap
                    text: "Surround safety limiter missing: install lsp-plugins-lv2 from the Arch/CachyOS repositories."
                }
                TextArea {
                    Layout.fillWidth: true
                    visible: controller.verificationReport.length > 0
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    text: controller.verificationReport
                    font.family: "monospace"
                    background: Rectangle {
                        color: Kirigami.Theme.alternateBackgroundColor
                        radius: Kirigami.Units.smallSpacing
                    }
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    opacity: 0.75
                    text: "Direct can be bit-identical only for 48 kHz / 16-bit sources at unity software volume. Surround and its equalizer intentionally change samples. Linux cannot verify the proprietary wireless hop after the USB dock."
                }
            }
        }
    }
}
