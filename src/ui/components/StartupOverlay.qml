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
    width: Math.min(Overlay.overlay.width - 48, 480)

    property bool startupBusy: startupController.initializing && !startupController.catalogReady
    readonly property real innerWidth: Math.max(0, width - leftPadding - rightPadding)

    onStartupBusyChanged: syncOpenState()

    function syncOpenState() {
        if (startupBusy)
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
        clip: true
    }

    contentItem: ColumnLayout {
        id: contentColumn
        width: root.innerWidth
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredWidth: contentColumn.width
            spacing: 16

            BusyIndicator {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                running: root.startupBusy
            }

            Label {
                Layout.fillWidth: true
                text: startupController.initStatusText.length > 0
                      ? startupController.initStatusText
                      : qsTr("Loading library…")
                font.pixelSize: 18
                font.weight: Font.DemiBold
                color: Theme.textHeading
                elide: Text.ElideRight
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            indeterminate: true
        }
    }
}
