import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../js/MediaKind.js" as MediaKind

// Exercise card in the dashboard: a tap -> signal activated()-> DashboardPage.songSelected.
Pane {
    id: root

    property int songId: 0
    property string title: ""
    property int baseBpm: 0
    property string artistName: ""
    property string tuningName: ""
    property int tuningId: 0
    property string displayTitle: ""
    property bool isLinkedGroup: false
    property int linkedMediaCount: 0
    property int linkGroupId: 0
    property bool isContainerMember: false
    property int hubSongId: 0
    property var assetSummary: []

    property bool isFavorite: false

    readonly property var mediaKinds: {
        if (!root.assetSummary || root.assetSummary.length === 0)
            return []

        var order = [
            MediaKind.GuitarPro,
            MediaKind.Video,
            MediaKind.Audio,
            MediaKind.Document,
            MediaKind.Image,
            MediaKind.Unknown
        ]
        var present = {}
        for (var i = 0; i < root.assetSummary.length; ++i) {
            var entry = root.assetSummary[i]
            var kind = entry ? (entry.kind !== undefined ? entry.kind : entry["kind"]) : ""
            if (kind)
                present[kind] = true
        }

        var result = []
        for (var j = 0; j < order.length; ++j) {
            if (present[order[j]])
                result.push(order[j])
        }
        return result
    }

    signal activated()

    padding: 0

    background: Rectangle {
        radius: 12
        color: cardTap.pressed ? Theme.cardBackgroundPressed : Theme.cardBackground
        border.color: root.isContainerMember ? Theme.borderAccentMuted : Theme.borderSubtle
        border.width: root.isContainerMember ? 2 : 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: root.displayTitle.length > 0 ? root.displayTitle : root.title
                font.pixelSize: 16
                font.weight: Font.DemiBold
                color: Theme.textHeading
                elide: Text.ElideRight
            }

            Rectangle {
                visible: root.isLinkedGroup && root.linkedMediaCount > 1 && !root.isContainerMember
                radius: 6
                color: Theme.accentFill
                border.color: Theme.accent
                implicitHeight: linkedBadge.implicitHeight + 8
                implicitWidth: linkedBadge.implicitWidth + 12

                Label {
                    id: linkedBadge
                    anchors.centerIn: parent
                    text: qsTr("Group")
                    font.pixelSize: 10
                    font.weight: Font.Medium
                    color: Theme.accent
                }
            }

            Label {
                visible: root.isContainerMember
                text: qsTr("File")
                font.pixelSize: 10
                color: Theme.textSecondary
            }

            Label {
                visible: root.isFavorite
                text: "\u2605"
                color: Theme.warning
                font.pixelSize: 14
            }
        }

        Label {
            visible: root.artistName.length > 0
            Layout.fillWidth: true
            text: root.artistName
            font.pixelSize: 12
            color: Theme.textSecondary
            elide: Text.ElideRight
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: root.mediaKinds.length > 0 ? 48 : 0

            Row {
                anchors.centerIn: parent
                spacing: 10

                Repeater {
                    model: root.mediaKinds

                    delegate: MediaKindIcon {
                        required property var modelData

                        kind: modelData
                        size: 32
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Label {
                visible: root.baseBpm > 0
                text: qsTr("%1 BPM").arg(root.baseBpm)
                font.pixelSize: 12
                color: Theme.textTertiary
            }

            Label {
                visible: root.tuningName.length > 0
                text: root.tuningName
                font.pixelSize: 12
                color: Theme.textTertiary
            }

            Item { Layout.fillWidth: true }
        }
    }

    TapHandler {
        id: cardTap
        target: root
        onTapped: root.activated()
    }
}
