import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    width: 1080
    height: 780
    minimumWidth: 820
    minimumHeight: 620
    visible: true
    title: "Astro Manager for Linux"

    pageStack.initialPage: Kirigami.Page {
        title: "Astro Manager for Linux"
        padding: 0

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            TabBar {
                id: navigation
                Layout.fillWidth: true
                TabButton { text: "Dashboard" }
                TabButton { text: "Spatial Audio" }
                TabButton { text: "Hardware EQ" }
                TabButton { text: "Microphone" }
                TabButton { text: "Stream Port" }
                TabButton { text: "Profiles" }
                TabButton { text: "Diagnostics" }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: navigation.currentIndex
                DashboardPage { }
                SpatialPage { }
                HardwareEqPage { }
                MicrophonePage { }
                StreamPage { }
                ProfilesPage { }
                DiagnosticsPage { }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: saveRow.implicitHeight + Kirigami.Units.largeSpacing * 2
                visible: hardware.dirty
                color: Kirigami.Theme.neutralBackgroundColor

                RowLayout {
                    id: saveRow
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.largeSpacing
                    Label {
                        Layout.fillWidth: true
                        text: hardware.neutralized
                              ? "Unsaved headset changes · onboard output EQ is temporarily flat while Linux spatial audio is active"
                              : "Unsaved headset changes"
                        wrapMode: Text.WordWrap
                    }
                    Button {
                        text: "Revert"
                        enabled: !hardware.busy
                        onClicked: hardware.revertToSaved()
                    }
                    Button {
                        text: "Save to Headset"
                        highlighted: true
                        enabled: !hardware.busy
                        onClicked: hardware.saveToHeadset()
                    }
                }
            }
        }
    }
}
