import QtQuick
import QtQuick.Controls

SpinBox {
    id: control

    editable: true
    implicitHeight: 32

    palette.text: Theme.textPrimary
    palette.base: Theme.inputBackground
    palette.button: Theme.panelBackgroundNested
    palette.buttonText: Theme.textPrimary
    palette.highlight: Theme.highlight
    palette.highlightedText: Theme.textOnAccent

    background: Rectangle {
        implicitHeight: 32
        color: Theme.inputBackground
        border.color: control.enabled
                              ? (control.activeFocus ? Theme.borderFocus : Theme.borderSubtle)
                              : Theme.border
        border.width: 1
        radius: 4
    }
}
