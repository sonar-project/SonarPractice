pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../components"

Page {
    id: root

    readonly property bool useWideLayout: width > 900

    required property bool sessionLocked

    signal songSelected(int songId, string title, int baseBpm, int practiceAssetId)
    signal reminderEditRequested(int songId, string title, int baseBpm, int reminderId, int practiceAssetId)

    background: Rectangle { color: Theme.windowBackground }

    header: TopBar {
        title: qsTr("Repertoire")
        showBack: false
        sessionLocked: root.sessionLocked
        sessionLabel: qsTr("Timer running")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Label {
            text: qsTr("Launch hub")
            font.pixelSize: 22
            font.weight: Font.Bold
            color: Theme.textHeading
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Exercises on the left, reminders in the center, calendar on the right — tap a card to open media and training.")
            font.pixelSize: 13
            color: Theme.textSecondary
        }

        Label {
            Layout.fillWidth: true
            visible: importService.statusMessage.length > 0
            wrapMode: Text.WordWrap
            text: importService.statusMessage
            font.pixelSize: 12
            color: Theme.textHint
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            DarkTextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search by title, artist, or mood…")
                text: songModel.searchText
                onTextEdited: songModel.searchText = text

                Keys.onEscapePressed: {
                    text = ""
                    songModel.searchText = ""
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            CheckBox {
                text: qsTr("Hide groups")
                checked: songModel.hideContainers
                onToggled: songModel.hideContainers = checked
            }

            CheckBox {
                text: qsTr("Show groups only")
                checked: songModel.containersOnly
                onToggled: songModel.containersOnly = checked
            }

            CheckBox {
                text: qsTr("Show all files")
                checked: songModel.expandAllGroups
                enabled: !songModel.hideContainers && !songModel.containersOnly
                onToggled: songModel.expandAllGroups = checked
            }
        }

        Loader {
            id: contentLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: root.useWideLayout ? wideLayout : narrowLayout
        }

        Label {
            visible: songModel.catalogReady && songModel.totalCount === 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("No exercises yet. Import media via Import → File… in the menu bar.")
            font.pixelSize: 13
            color: Theme.textMuted
        }
    }

    Component {
        id: wideLayout

        RowLayout {
            anchors.fill: parent
            spacing: 16

            DashboardExerciseList {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 280

                onSongActivated: (songId, title, baseBpm) =>
                    root.songSelected(songId, title, baseBpm, 0)
            }

            DashboardReminderPanel {
                onOpenSessionRequested: (songId, title, baseBpm, practiceAssetId) =>
                    root.songSelected(songId, title, baseBpm, practiceAssetId)
                onEditReminderRequested: (songId, title, baseBpm, reminderId, practiceAssetId) =>
                    root.reminderEditRequested(songId, title, baseBpm, reminderId, practiceAssetId)
            }

            PracticeCalendarPanel {}
        }
    }

    Component {
        id: narrowLayout

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            DashboardExerciseList {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: Math.max(200, root.height * 0.45)

                onSongActivated: (songId, title, baseBpm) =>
                    root.songSelected(songId, title, baseBpm, 0)
            }

            DashboardReminderPanel {
                Layout.fillWidth: true
                panelWidth: parent.width

                onOpenSessionRequested: (songId, title, baseBpm, practiceAssetId) =>
                    root.songSelected(songId, title, baseBpm, practiceAssetId)
                onEditReminderRequested: (songId, title, baseBpm, reminderId, practiceAssetId) =>
                    root.reminderEditRequested(songId, title, baseBpm, reminderId, practiceAssetId)
            }

            PracticeCalendarPanel {
                Layout.fillWidth: true
                sidePanelWidth: parent.width
            }
        }
    }
}

