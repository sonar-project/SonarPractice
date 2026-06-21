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

    property bool importBusy: importService?.busy ?? false
    readonly property real innerWidth: Math.max(0, width - leftPadding - rightPadding)

    onImportBusyChanged: syncOpenState()

    function syncOpenState() {
        if (importBusy)
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
                running: importService?.busy ?? false
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("Import in progress…")
                font.pixelSize: 18
                font.weight: Font.DemiBold
                color: Theme.textHeading
                elide: Text.ElideRight
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredWidth: contentColumn.width
            Layout.preferredHeight: importProgress.implicitHeight

            ProgressBar {
                id: importProgress
                anchors.left: parent.left
                anchors.right: parent.right
                indeterminate: (importService?.progressTotal ?? 0) <= 0
                from: 0
                to: Math.max(importService?.progressTotal ?? 1, 1)
                value: importService?.progressCurrent ?? 0
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.preferredWidth: contentColumn.width
            // Coverage details in the policy conditions and text format
            visible: (importService?.progressTotal ?? 0) > 0
            text: qsTr("%1 of %2 files").arg(importService?.progressCurrent ?? 0)
                                               .arg(importService?.progressTotal ?? 0)
            font.pixelSize: 13
            color: Theme.textHint
        }

        Label {
            Layout.fillWidth: true
            Layout.preferredWidth: contentColumn.width
            visible: (importService?.progressFileName ?? "").length > 0
            // width: contentColumn.width
            wrapMode: Text.WrapAnywhere
            maximumLineCount: 3
            text: importService?.progressFileName ?? ""
            font.pixelSize: 12
            color: Theme.textSecondary
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredWidth: contentColumn.width

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Cancel")
                onClicked: importService?.cancelImport()
            }
        }
    }
}
