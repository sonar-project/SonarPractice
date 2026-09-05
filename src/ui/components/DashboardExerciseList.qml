pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    signal songActivated(int songId, string title, int baseBpm, int mediaId)

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
        text: qsTr("No exercises match the filters.")
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
            required property int mediaId
            required property var assetSummary
            required property bool isFavorite

            width: exerciseGrid.cellWidth - 12
            height: 150

            SongCard {
                anchors.fill: parent
                title: cardHost.title
                baseBpm: cardHost.baseBpm
                artistName: cardHost.artistName
                tuningName: cardHost.tuningName
                displayTitle: cardHost.displayTitle
                isLinkedGroup: cardHost.isLinkedGroup
                linkedMediaCount: cardHost.linkedMediaCount
                isContainerMember: cardHost.isContainerMember
                assetSummary: cardHost.assetSummary
                isFavorite: cardHost.isFavorite

                onActivated: root.songActivated(
                    cardHost.hubSongId > 0 ? cardHost.hubSongId : cardHost.songId,
                    cardHost.displayTitle.length > 0 ? cardHost.displayTitle : cardHost.title,
                    cardHost.baseBpm,
                    cardHost.mediaId)

                onFavoriteToggled: favorite => songModel.setFavorite(cardHost.songId, favorite)
            }
        }
    }
}
