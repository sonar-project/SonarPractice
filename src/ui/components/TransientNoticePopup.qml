import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    parent: Overlay.overlay
    modal: false
    focus: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 16
    width: Math.min(Overlay.overlay.width - 48, 420)
    x: Math.max(24, (Overlay.overlay.width - width) / 2)
    y: 24

    property int autoCloseMs: 3000

    function show(message) {
        if (!message || message.length === 0)
            return
        noticeText.text = message
        open()
        autoCloseTimer.restart()
    }

    background: Rectangle {
        radius: 10
        color: Theme.toolbarBackground
        border.color: Theme.borderAccent
        border.width: 1
    }

    contentItem: RowLayout {
        spacing: 12

        Label {
            id: noticeText
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            font.pixelSize: 14
            color: Theme.textPrimary
        }

        Button {
            text: qsTr("OK")
            flat: true
            onClicked: root.close()
        }
    }

    Timer {
        id: autoCloseTimer
        interval: root.autoCloseMs
        repeat: false
        onTriggered: root.close()
    }

    onClosed: autoCloseTimer.stop()
}
