// src/ui/pages/PracticeHubPage.qml

/*
  Connector for SongFileInfo and SongReminder
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root
    required property int songId
    required property string songTitle
    required property int songBaseBpm
    readonly property bool timerActive: practiceTracker.timerRunning
    
    property int initialEditingReminderId: 0
    property int initialPracticeAssetId: 0

    signal backRequested
    signal assetOpenRequested(int mediaId)
    signal practiceRequested(int mediaId)
    signal openCalendarRequested
    signal audioConfigRequested(int mediaId)

    // Resolve group title once (fallback to song title):
    readonly property string groupTitle: {
        const info = linkGroupService.groupInfoForSong(root.songId);
        return (info && info.title && info.title.length > 0) ? info.title : root.songTitle;
    }

    background: Rectangle {
        color: Theme.windowBackground
    }

    header: TopBar {
        title: root.songTitle
        showBack: true
        sessionLocked: root.timerActive
        sessionLabel: qsTr("Timer running")
        onBackRequested: root.backRequested()
    }

    function resolveInitialPracticeAssetId() {
        if (root.initialPracticeAssetId > 0)
            return root.initialPracticeAssetId;
        if (root.initialEditingReminderId <= 0)
            return 0;
        const payload = reminderController.reminderEditPayload(root.initialEditingReminderId);
        return payload.practiceAssetId ?? 0;
    }

    function syncSongContext() {
        journalEditor.ready = false;
        practiceTracker.songId = root.songId;
        practiceTracker.loadTrainingDefaults(root.songBaseBpm);
        mediaFileModel.songId = root.songId;
        mediaFileModel.reload();
        practiceTracker.reloadJournal();
        practiceTracker.reloadJournalNote();
        journalEditor.syncFromTracker();
        journalEditor.ready = true;
        reminderController.songId = root.songId;
        const assetId = root.resolveInitialPracticeAssetId();
        if (assetId > 0) {
            songInfo.requestedActivePracticeAssetId = assetId;
            root.initialPracticeAssetId = 0;
        }
        if (root.initialEditingReminderId > 0)
            reminderPanel.loadReminderForEdit(root.initialEditingReminderId);
        else
            reminderPanel.applyTrainingDefaults();
    }

    Component.onCompleted: root.syncSongContext()
    onSongIdChanged: root.syncSongContext()

    StackView.onDeactivated: songInfo.releaseKeyboardFocus()

    function releaseKeyboardFocus() {
        songInfo.releaseKeyboardFocus();
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        SongReminderPanel {
            id: reminderPanel
            songId: root.songId
            songTitle: root.songTitle
            groupTitle: root.groupTitle
            practiceAssetId: songInfo.activePracticeAssetId

            guitarProLabel: songInfo.activeLabelGuitarPro
            audioLabel: songInfo.activeLabelAudio
            videoLabel: songInfo.activeLabelVideo
            imageLabel: songInfo.activeLabelImage
            documentLabel: songInfo.activeLabelDocument

            onOpenCalendarRequested: root.openCalendarRequested()
            onCommitAssetRequested: {
                const assetId = songInfo.commitPendingAsset();
                reminderController.practiceAssetId = assetId;
            }
        }

        ScrollView {
            id: contentScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ColumnLayout {
                width: contentScroll.availableWidth
                spacing: 12

                SongInfoPanel {
                    id: songInfo
                    Layout.fillWidth: true
                    songId: root.songId
                    songTitle: root.songTitle
                    songBaseBpm: root.songBaseBpm
                    onAssetOpenRequested: mediaId => root.assetOpenRequested(mediaId)
                    onPracticeRequested: mediaId => root.practiceRequested(mediaId)
                    onAudioConfigRequested: mediaId => root.audioConfigRequested(mediaId)
                    onActivePracticeAssetIdChanged: {
                        practiceTracker.assetId = songInfo.activePracticeAssetId;
                    }
                }

                TrainingPanel {
                    id: trainingsPanel
                    Layout.fillWidth: true
                    practiceAssetId: songInfo.activePracticeAssetId
                    practiceMaterialReady: songInfo.practiceMaterialReady
                }

                Label {
                    Layout.fillWidth: true
                    visible: practiceTracker.statusMessage.length > 0
                    wrapMode: Text.WordWrap
                    text: practiceTracker.statusMessage
                    font.pixelSize: 11
                    color: practiceTracker.statusIsError ? Theme.error : Theme.success
                }

                JournalMarkdownEditor {
                    id: journalEditor
                    Layout.fillWidth: true
                    Layout.preferredHeight: 260
                    Layout.minimumHeight: 200
                }

                JournalTableView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 500
                    Layout.minimumHeight: 120
                }
            }
        }
    }
}
