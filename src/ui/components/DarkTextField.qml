import QtQuick
import QtQuick.Controls

TextField {
    id: control

    color: Theme.textPrimary
    selectByMouse: true
    padding: 8
    implicitHeight: 32

    palette.base: Theme.inputBackground
    palette.text: Theme.textPrimary
    palette.highlight: Theme.highlight
    palette.highlightedText: Theme.textOnAccent

    background: Rectangle {
        implicitHeight: 32
        color: control.enabled ? Theme.inputBackground : Theme.inputBackgroundDisabled
        border.color: control.enabled
                              ? (control.activeFocus ? Theme.borderFocus : Theme.borderSubtle)
                              : Theme.border
        border.width: 1
        radius: 4
    }
}
