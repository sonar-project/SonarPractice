import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../js/MediaKind.js" as MediaKind

Pane {
    id: root

    // Mirrors MediaFile struct fields exposed to QML
    required property int mediaId
    required property int songId
    required property string filePath
    required property string fileType
    required property string mediaKind
    required property string sourceType
    required property bool isManaged
    required property bool canBePracticed

    signal openRequested()
    signal practiceRequested()

    padding: 0

    readonly property string displayName: {
        if (root.sourceType === MediaKind.Url) {
            try {
                var url = new URL(root.filePath)
                return url.hostname + url.pathname
            } catch (e) {
                return root.filePath
            }
        }
        var parts = root.filePath.split(/[/\\]/)
        return parts[parts.length - 1]
    }

    background: Rectangle {
        radius: 10
        color: Theme.assetTileBackground
        border.color: MediaKind.accentColor(root.mediaKind)
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                implicitHeight: 36
                implicitWidth: 36
                radius: 8
                color: Qt.rgba(MediaKind.accentColor(root.mediaKind), 0.25)

                MediaKindIcon {
                    anchors.centerIn: parent
                    kind: root.mediaKind
                    size: 22
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: root.displayName
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    color: Theme.textPrimary
                    elide: Text.ElideMiddle
                }
            }

            TapHandler {
                onTapped: root.openRequested()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Label {
                text: root.sourceType === MediaKind.Url ? qsTr("URL")
                     : root.isManaged ? qsTr("Managed") : qsTr("Linked")
                font.pixelSize: 10
                color: Theme.textSecondary
            }

            Item { Layout.fillWidth: true }

            Button {
                visible: root.canBePracticed
                text: qsTr("Practice")
                flat: true
                font.pixelSize: 11
                palette.buttonText: Theme.accent
                onClicked: root.practiceRequested()
            }
        }
    }
}
