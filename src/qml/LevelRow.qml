import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root
    property string label
    property int value: 0
    property bool controlEnabled: true
    signal committed(int value)
    Label { Layout.preferredWidth: 145; text: root.label }
    Slider {
        id: slider
        Layout.fillWidth: true
        from: 0; to: 100; stepSize: 1
        value: root.value
        enabled: root.controlEnabled
        onPressedChanged: if (!pressed) root.committed(Math.round(value))
    }
    Label { Layout.preferredWidth: 45; horizontalAlignment: Text.AlignRight; text: Math.round(slider.value) + "%" }
}
