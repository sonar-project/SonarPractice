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
    width: Math.min(Overlay.overlay.width - 48, 520)

    property bool processing: (audioConfigController?.loading ?? false)
                              || (audioConfigController?.pitchAnalyzing ?? false)
    readonly property real innerWidth: Math.max(0, width - leftPadding - rightPadding)

    readonly property string headline: {
        const song = audioConfigController?.songTitle ?? ""
        if (song.length > 0)
            return song
        return audioConfigController?.displayName ?? ""
    }

    readonly property string fileLabel: {
        const song = audioConfigController?.songTitle ?? ""
        const file = audioConfigController?.displayName ?? ""
        if (song.length > 0 && file.length > 0 && song !== file)
            return file
        return ""
    }

    onProcessingChanged: syncOpenState()

    function syncOpenState() {
        if (processing)
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
                running: root.processing
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    Layout.fillWidth: true
                    text: (audioConfigController?.pitchAnalyzing ?? false)
                          ? qsTr("Detecting pitch…")
                          : qsTr("Processing audio…")
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    color: Theme.textHeading
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.headline.length > 0
                    text: root.headline
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    color: Theme.textPrimary
                    elide: Text.ElideMiddle
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.fileLabel.length > 0
                    text: root.fileLabel
                    font.pixelSize: 12
                    color: Theme.textSecondary
                    elide: Text.ElideMiddle
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredWidth: contentColumn.width
            Layout.preferredHeight: processingProgressBar.implicitHeight

            ProgressBar {
                id: processingProgressBar
                anchors.left: parent.left
                anchors.right: parent.right
                indeterminate: (audioConfigController?.processingProgress ?? -1) < 0
                from: 0
                to: 100
                value: Math.max(0, audioConfigController?.processingProgress ?? 0)
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.preferredWidth: contentColumn.width
            text: {
                const stage = audioConfigController?.processingStage ?? ""
                if (stage.length > 0)
                    return stage
                return qsTr("Please wait…")
            }
            font.pixelSize: 13
            color: Theme.textHint
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.fillWidth: true
            Layout.preferredWidth: contentColumn.width
            visible: (audioConfigController?.processingProgress ?? -1) >= 0
            text: qsTr("%1% complete").arg(audioConfigController?.processingProgress ?? 0)
            font.pixelSize: 12
            color: Theme.textSecondary
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredWidth: contentColumn.width

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Cancel")
                visible: audioConfigController?.loading ?? false
                enabled: root.processing && (audioConfigController?.loading ?? false)
                onClicked: audioConfigController?.cancelProcessing()
            }
        }
    }
}
