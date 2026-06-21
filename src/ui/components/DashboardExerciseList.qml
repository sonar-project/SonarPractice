pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    signal songActivated(int songId, string title, int baseBpm)

    spacing: 8

    Label {
        text: qsTr("Exercises")
        font.pixelSize: 18
        font.weight: Font.DemiBold
        color: Theme.textHeading
    }

    Label {
        visible: !songModel.catalogReady
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Loading library…")
        font.pixelSize: 13
        color: Theme.textMuted
    }

    Label {
        visible: songModel.catalogReady && songModel.totalCount > 0
                 && songModel.count === 0
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("No exercises match the search.")
        font.pixelSize: 13
        color: Theme.textMuted
    }

    GridView {
        id: exerciseGrid
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        visible: songModel.catalogReady
        boundsBehavior: Flickable.StopAtBounds
        cellWidth: 280
        cellHeight: 162
        model: songModel.catalogReady ? songModel : null

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        delegate: Item {
            id: cardHost

            required property int songId
            required property string title
            required property int baseBpm
            required property string artistName
            required property string tuningName
            required property int tuningId
            required property string displayTitle
            required property bool isLinkedGroup
            required property int linkedMediaCount
            required property int linkGroupId
            required property bool isContainerMember
            required property int hubSongId
            required property var assetSummary

            width: exerciseGrid.cellWidth - 12
            height: 150

            SongCard {
                anchors.fill: parent
                songId: cardHost.songId
                title: cardHost.title
                baseBpm: cardHost.baseBpm
                artistName: cardHost.artistName
                tuningName: cardHost.tuningName
                tuningId: cardHost.tuningId
                displayTitle: cardHost.displayTitle
                isLinkedGroup: cardHost.isLinkedGroup
                linkedMediaCount: cardHost.linkedMediaCount
                linkGroupId: cardHost.linkGroupId
                isContainerMember: cardHost.isContainerMember
                hubSongId: cardHost.hubSongId
                assetSummary: cardHost.assetSummary

                onActivated: root.songActivated(
                    cardHost.hubSongId > 0 ? cardHost.hubSongId : cardHost.songId,
                    cardHost.displayTitle.length > 0 ? cardHost.displayTitle : cardHost.title,
                    cardHost.baseBpm)
            }
        }
    }
}
