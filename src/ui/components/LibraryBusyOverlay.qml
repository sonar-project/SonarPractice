import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    parent: Overlay.overlay
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose
    padding: 24
    anchors.centerIn: parent
    width: Math.min(Overlay.overlay.width - 48, 420)

    required property bool active
    property string statusText: qsTr("Please wait…")
    property int fileCount: 0

    onActiveChanged: syncOpenState()

    function syncOpenState() {
        if (active)
            open()
        else
            close()
    }

    Component.onCompleted: syncOpenState()
    onOpened: forceActiveFocus()

    background: Rectangle {
        radius: 12
        color: Theme.toolbarBackground
        border.color: Theme.borderSubtle
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            BusyIndicator {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                running: root.active
            }

            Label {
                Layout.fillWidth: true
                text: root.statusText
                font.pixelSize: 16
                font.weight: Font.DemiBold
                color: Theme.textHeading
                wrapMode: Text.WordWrap
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            indeterminate: true
            visible: root.active
        }

        Label {
            Layout.fillWidth: true
            visible: root.fileCount > 0
            text: qsTr("%1 files").arg(root.fileCount)
            font.pixelSize: 12
            color: Theme.textSecondary
        }
    }
}
