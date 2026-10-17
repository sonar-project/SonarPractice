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
    property int initialMediaId: 0
    /** Reminder whose condition (bars/BPM) should seed the training panel. */
    property int initialSourceReminderId: 0

    signal backRequested
    signal assetOpenRequested(int mediaId)
    signal practiceRequested(int mediaId)
    signal openCalendarRequested
    signal audioConfigRequested(int mediaId)
    signal guitarProPlayerRequested(int audioMediaId, int practiceAssetId)
    /** Hub asks Main to stop the timer, save journal, and handle reminder prompt. */
    signal finishTrainingRequested()

    // Resolve group title once (fallback to song title):
    readonly property string groupTitle: {
        const info = linkGroupService.groupInfoForSong(root.songId);
        return (info && info.title && info.title.length > 0) ? info.title : root.songTitle;
    }

    /** Set after Open, until the score tempo is known and the practice BPM is applied. */
    property bool applyPracticeTempoOnLoad: false
    /** Song whose base tempo was last copied into the training spin. */
    property int _appliedTempoSongId: 0
    property int _appliedTempoBpm: 0

    function practiceBaseBpm() {
        const gp = guitarProPreviewController
        const mediaId = songInfo.guitarProMediaId
        if (mediaId > 0 && gp.mediaFileId === mediaId && gp.scoreBpm > 0)
            return gp.scoreBpm
        if (songInfo.effectiveBaseBpm > 0)
            return songInfo.effectiveBaseBpm
        return root.songBaseBpm > 0 ? root.songBaseBpm : 100
    }

    function applyPracticeTempo() {
        const target = practiceTracker.targetBpm
        if (target <= 0)
            return
        const base = root.practiceBaseBpm()
        const gp = guitarProPreviewController
        gp.tempoPercent = Math.max(gp.minTempoPercent,
                                   Math.min(gp.maxTempoPercent, Math.round(target * 100 / base)))
    }

    /** Free play: load GP without training loop; copy hub bar range and tempo into the player. */
    function openGuitarProInternally(mediaId) {
        if (mediaId <= 0)
            return
        const gp = guitarProPreviewController
        const tempoKnown = gp.loaded && gp.mediaFileId === mediaId && gp.scoreBpm > 0
        root.applyPracticeTempoOnLoad = !tempoKnown
        root.applyPracticeTempo()
        gp.loopStartBar = practiceTracker.startBar
        gp.loopEndBar = Math.max(practiceTracker.startBar, practiceTracker.endBar)
        gp.loopEnabled = false
        gp.usePlayer = true
        gp.load(mediaId)
        root.requestGuitarProPlayer()
    }

    function requestGuitarProPlayer() {
        const assetId = songInfo.commitPendingAsset()
        if (assetId > 0)
            songInfo.activePracticeAssetId = assetId
        root.guitarProPlayerRequested(songInfo.audioMediaId, assetId)
    }

    background: Rectangle {
        color: Theme.windowBackground
    }

    Connections {
        target: guitarProPreviewController
        function onScoreBpmChanged() {
            root.applySelectedSongTempo()
            if (!root.applyPracticeTempoOnLoad || guitarProPreviewController.scoreBpm <= 0)
                return
            root.applyPracticeTempo()
            root.applyPracticeTempoOnLoad = false
        }
        function onLoadedChanged() {
            if (!root.applyPracticeTempoOnLoad || !guitarProPreviewController.loaded)
                return
            // Score BPM is known now. Recompute before the score is pushed to the player.
            root.applyPracticeTempo()
            root.applyPracticeTempoOnLoad = false
        }
    }

    /**
     * Copy the selected song's base tempo into the training spin and the reminder form.
     * Opening a reminder keeps that reminder's tempo until another song is chosen.
     * A spin or min-BPM the user already changed is left alone.
     */
    function applySelectedSongTempo() {
        const songId = songInfo.selectedSongId
        if (songId <= 0)
            return
        const bpm = songInfo.effectiveBaseBpm
        if (bpm <= 0)
            return
        if (songId === root._appliedTempoSongId && bpm === root._appliedTempoBpm)
            return

        const songChanged = root._appliedTempoSongId !== songId
        const previousBpm = root._appliedTempoBpm
        const userEditedSpin = !songChanged && practiceTracker.targetBpm !== previousBpm
        const openingReminder = root._appliedTempoSongId === 0
                && practiceTracker.sourceReminderId > 0
        root._appliedTempoSongId = songId
        root._appliedTempoBpm = bpm
        if (openingReminder || practiceTracker.timerRunning)
            return
        if (!userEditedSpin)
            practiceTracker.targetBpm = bpm
        if (reminderPanel.shouldAdoptSongBpm(previousBpm, songChanged))
            reminderPanel.applySelectedSongBpm(bpm)
    }

    /** Title shown in the header: the selected Guitar Pro song, otherwise the hub song. */
    readonly property string visibleTitle: {
        if (!songInfo)
            return root.songTitle
        const selected = (songInfo.selectedSongTitle || "").trim()
        return selected.length > 0 ? selected : root.songTitle
    }

    header: TopBar {
        title: root.visibleTitle
        showBack: true
        sessionLocked: root.timerActive
        sessionLabel: qsTr("Timer running")
        onBackRequested: root.backRequested()
        onStopSessionRequested: root.stopTrainingSession()
    }

    function stopTrainingSession() {
        if (!practiceTracker.timerRunning)
            return;
        // Keep asset id current before Main syncs player params and saves.
        if (songInfo.activePracticeAssetId > 0)
            practiceTracker.assetId = songInfo.activePracticeAssetId;
        root.finishTrainingRequested();
    }

    function resolveInitialPracticeAssetId() {
        if (root.initialPracticeAssetId > 0)
            return root.initialPracticeAssetId;
        if (root.initialEditingReminderId <= 0 && root.initialSourceReminderId <= 0)
            return 0;
        const reminderId = root.initialEditingReminderId > 0
                         ? root.initialEditingReminderId
                         : root.initialSourceReminderId;
        const payload = reminderController.reminderEditPayload(reminderId);
        return payload.practiceAssetId ?? 0;
    }

    function syncSongContext() {
        root._appliedTempoSongId = 0
        root._appliedTempoBpm = 0
        journalEditor.ready = false;
        practiceTracker.songId = root.songId;
        const assetId = root.resolveInitialPracticeAssetId();
        // Clear stale asset from a previous hub visit before loading defaults.
        practiceTracker.assetId = assetId;
        const sourceReminderId = root.initialSourceReminderId > 0
                               ? root.initialSourceReminderId
                               : root.initialEditingReminderId;
        practiceTracker.loadTrainingDefaults(root.songBaseBpm, sourceReminderId);
        mediaFileModel.songId = root.songId;
        mediaFileModel.reload();
        practiceTracker.reloadJournal();
        practiceTracker.reloadJournalNote();
        journalEditor.syncFromTracker();
        journalEditor.ready = true;
        reminderController.songId = root.songId;
        guitarProPreviewController.clear();
        if (assetId > 0) {
            songInfo.requestedActivePracticeAssetId = assetId;
            root.initialPracticeAssetId = 0;
        }
        if (root.initialMediaId > 0) {
            songInfo.requestedActiveMediaId = root.initialMediaId;
            root.initialMediaId = 0;
        }
        songInfo.restoreLastSelection();
        if (root.initialEditingReminderId > 0)
            reminderPanel.loadReminderForEdit(root.initialEditingReminderId);
        else
            reminderPanel.applyTrainingDefaults();
        root.initialSourceReminderId = 0;
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

            selectedSongTitle: songInfo.selectedSongTitle
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
                    songBaseBpm: root.songBaseBpm
                    onAssetOpenRequested: mediaId => root.assetOpenRequested(mediaId)
                    onPracticeRequested: mediaId => root.practiceRequested(mediaId)
                    onAudioConfigRequested: mediaId => root.audioConfigRequested(mediaId)
                    onGuitarProInternalOpenRequested: mediaId => root.openGuitarProInternally(mediaId)
                    onActivePracticeAssetIdChanged: {
                        practiceTracker.assetId = songInfo.activePracticeAssetId;
                    }
                    onSelectedSongIdChanged: root.applySelectedSongTempo()
                    onEffectiveBaseBpmChanged: root.applySelectedSongTempo()
                }

                TrainingPanel {
                    id: trainingsPanel
                    Layout.fillWidth: true
                    practiceAssetId: songInfo.activePracticeAssetId
                    practiceMaterialReady: songInfo.practiceMaterialReady
                    guitarProMediaId: songInfo.guitarProMediaId
                    songBaseBpm: songInfo.effectiveBaseBpm > 0
                                 ? songInfo.effectiveBaseBpm
                                 : root.songBaseBpm
                    onInternalPlayerRequested: root.requestGuitarProPlayer()
                    onStopSaveRequested: root.stopTrainingSession()
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
