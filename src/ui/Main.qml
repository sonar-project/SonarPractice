import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtCore

pragma ComponentBehavior: Bound

import com.sonarp.sonarpractice

ApplicationWindow {
    id: root;

    width: 1280
    height: 800
    minimumWidth: 1024
    minimumHeight: 720
    visible: true
    title: "SonarPractice"

    color: Theme.windowBackground

    palette.window: Theme.windowBackground
    palette.windowText: Theme.textPrimary
    palette.base: Theme.inputBackground
    palette.text: Theme.textPrimary
    palette.button: Theme.panelBackgroundNested
    palette.buttonText: Theme.textPrimary
    palette.highlight: Theme.highlight
    palette.highlightedText: Theme.textOnAccent
    palette.mid: Theme.border
    palette.dark: Theme.borderSubtle
    palette.light: Theme.panelBackground
    palette.placeholderText: Theme.textPlaceholder
    palette.toolTipBase: Theme.panelBackground
    palette.toolTipText: Theme.textPrimary

    property bool shellReady: startupController.shellReady
    property bool catalogReady: startupController.catalogReady
    property bool servicesReady: startupController.catalogReady

    /** Last Practice Hub visit, so the dashboard can return after Back / calendar. */
    property var practiceReturn: null

    function rememberPracticeHub(songId, title, baseBpm, editingReminderId, practiceAssetId, mediaId, sourceReminderId) {
        root.practiceReturn = {
            "songId": songId,
            "title": title,
            "baseBpm": baseBpm,
            "editingReminderId": editingReminderId,
            "practiceAssetId": practiceAssetId,
            "mediaId": mediaId,
            "sourceReminderId": sourceReminderId
        };
    }

    function reopenPracticeHub() {
        if (!root.practiceReturn)
            return;
        const ctx = root.practiceReturn;
        root.pushPracticeHub(ctx.songId, ctx.title, ctx.baseBpm,
                             ctx.editingReminderId, ctx.practiceAssetId,
                             ctx.mediaId, ctx.sourceReminderId);
    }

    function urlToLocalPath(url) {
        const resolved = Qt.resolvedUrl(url);
        const stringForm = resolved.toString();
        if (stringForm.startsWith("file://"))
            return decodeURIComponent(stringForm.slice(7));
        return stringForm;
    }

    function importPaths(paths) {
        if (!root.servicesReady || !paths || paths.length === 0)
            return;
        if (importService.usesOrganizer())
            importOrganizer.begin(paths);
        else
            importService.importPaths(paths);
    }

    function openFiles() {
        openFileDialog.nameFilters = appSettings.fileDialogNameFilters;
        Qt.callLater(() => {
            openFileDialog.selectedNameFilter.index = 0;
            openFileDialog.open();
        });
    }

    function openFolder() {
        const folderPath = startupController.browseImportFolder("");
        if (folderPath.length > 0)
            root.importPaths([folderPath]);
    }

    function openSettings() {
        stackView.push(configComponent, {
            "firstRun": false
        });
    }

    /** Copy live player tempo/loop into the practice tracker before journal save. */
    function syncPracticeParamsFromPlayer() {
        const gp = guitarProPreviewController;
        if (!gp.usePlayer || !gp.loaded)
            return;
        if (gp.playbackBpm > 0)
            practiceTracker.targetBpm = gp.playbackBpm;
        if (gp.loopEnabled) {
            if (gp.loopStartBar > 0)
                practiceTracker.startBar = gp.loopStartBar;
            if (gp.loopEndBar >= practiceTracker.startBar)
                practiceTracker.endBar = gp.loopEndBar;
        }
    }

    function practiceSongTitle() {
        for (let i = stackView.depth - 1; i >= 0; --i) {
            const page = stackView.get(i)
            if (page && page.visibleTitle && String(page.visibleTitle).length > 0)
                return String(page.visibleTitle)
        }
        if (root.practiceReturn && root.practiceReturn.title)
            return root.practiceReturn.title
        return ""
    }

    function applyReminderAfterPractice(updateTempoAndLoop) {
        const reminderId = practiceTracker.sourceReminderId;
        if (reminderId <= 0)
            return;
        const gp = guitarProPreviewController;
        reminderController.applySessionToReminder(
            reminderId,
            practiceTracker.startBar,
            practiceTracker.endBar,
            practiceTracker.targetBpm,
            gp.countInEnabled,
            gp.countInOneTime,
            gp.metronomeEnabled,
            gp.transposeSemitones,
            updateTempoAndLoop,
            root.practiceSongTitle(),
            (gp.loaded && gp.selectedTrackIndex >= 0) ? gp.selectedTrackIndex : -1);
    }

    /**
     * Stops the training timer, saves the journal, and optionally prompts to
     * update the source reminder when tempo changed.
     * @param popPlayer When true, pop the Guitar Pro player page after finish.
     */
    function finishTrainingSession(popPlayer) {
        if (!practiceTracker.timerRunning)
            return;

        guitarProPreviewController.pause();
        audioConfigController.pausePlayback();
        root.syncPracticeParamsFromPlayer();

        const assetId = practiceTracker.assetId;
        practiceTracker.stopAndSaveWithAssetId(assetId);

        const reminderId = practiceTracker.sourceReminderId;
        if (reminderId <= 0) {
            if (popPlayer)
                stackView.pop();
            return;
        }

        const payload = reminderController.reminderEditPayload(reminderId);
        const savedBpm = payload.conditionMinBpm ?? 0;
        const currentBpm = practiceTracker.targetBpm;
        if (currentBpm > 0 && savedBpm > 0 && currentBpm !== savedBpm) {
            tempoAdoptDialog.popPlayer = !!popPlayer;
            tempoAdoptDialog.newBpm = currentBpm;
            tempoAdoptDialog.previewLabel = reminderController.practiceLabel(
                root.practiceSongTitle(),
                practiceTracker.startBar,
                practiceTracker.endBar,
                currentBpm);
            tempoAdoptDialog.open();
            return;
        }

        root.applyReminderAfterPractice(false);
        if (popPlayer)
            stackView.pop();
    }

    function openLibrary() {
        if (!root.shellReady || practiceTracker.timerRunning)
            return;
        stackView.push(libraryComponent);
    }

    menuBar: MenuBar {
        visible: root.shellReady

        Menu {
            title: qsTr("Import")

            Action {
                text: qsTr("File…")
                shortcut: StandardKey.Open
                enabled: {
                    if (!startupController.shellReady)
                        return false;
                    return !practiceTracker.timerRunning && !importService.busy && !startupController.initializing;
                }
                onTriggered: root.openFiles()
            }

            Action {
                text: qsTr("Folder…")
                enabled: {
                    if (!startupController.shellReady)
                        return false;
                    return !practiceTracker.timerRunning && !importService.busy && !startupController.initializing;
                }
                onTriggered: root.openFolder()
            }
        }

        Menu {
            title: qsTr("Library")

            Action {
                text: qsTr("Link files…")
                enabled: {
                    if (!startupController.shellReady)
                        return false;
                    return !practiceTracker.timerRunning;
                }
                onTriggered: root.openLibrary()
            }
        }

        Menu {
            title: qsTr("Settings")

            Action {
                text: qsTr("Configuration…")
                onTriggered: root.openSettings()
            }
        }

        Menu {
            title: qsTr("Help")

            Action {
                text: qsTr("About SonarPractice")
                onTriggered: aboutDialog.open()
            }
        }
    }

    AboutDialog {
        id: aboutDialog
    }

    Dialog {
        id: tempoAdoptDialog
        anchors.centerIn: parent
        modal: true
        title: qsTr("Adopt tempo for reminder?")
        standardButtons: Dialog.Yes | Dialog.No
        closePolicy: Popup.NoAutoClose
        // Keep width independent of implicitWidth. The wrapped label's width is
        // availableWidth, and Fusion Dialog.implicitWidth reads that label back.
        width: Math.min(root.width * 0.7, 480)

        property bool popPlayer: false
        property int newBpm: 0
        property string previewLabel: ""

        contentItem: Label {
            width: tempoAdoptDialog.availableWidth
            wrapMode: Text.WordWrap
            text: qsTr("Update the reminder to the new tempo and loop?\n\n%1")
                    .arg(tempoAdoptDialog.previewLabel)
            color: Theme.textPrimary
        }

        onAccepted: {
            root.applyReminderAfterPractice(true);
            if (tempoAdoptDialog.popPlayer)
                stackView.pop();
        }
        onRejected: {
            root.applyReminderAfterPractice(false);
            if (tempoAdoptDialog.popPlayer)
                stackView.pop();
        }
    }

    Dialog {
        id: importSummaryDialog
        title: qsTr("Import complete")
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok
        width: Math.min(root.width * 0.85, 560)
        height: failureDetailsVisible
                ? Math.min(root.height * 0.8, 520)
                : implicitHeight

        property string summaryText: ""
        property var reasonLines: []
        property var detailLines: []
        property bool failureDetailsVisible: detailLines.length > 0

        contentItem: ColumnLayout {
            spacing: 12
            width: importSummaryDialog.availableWidth

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: importSummaryDialog.summaryText
            }

            Label {
                Layout.fillWidth: true
                visible: importSummaryDialog.reasonLines.length > 0
                text: qsTr("Failure reasons:")
                font.weight: Font.DemiBold
            }

            Label {
                Layout.fillWidth: true
                visible: importSummaryDialog.reasonLines.length > 0
                wrapMode: Text.WordWrap
                text: importSummaryDialog.reasonLines.join("\n")
            }

            Label {
                Layout.fillWidth: true
                visible: importSummaryDialog.failureDetailsVisible
                text: qsTr("Failed files:")
                font.weight: Font.DemiBold
            }

            ScrollView {
                id: failureScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: importSummaryDialog.failureDetailsVisible ? 220 : 0
                visible: importSummaryDialog.failureDetailsVisible
                clip: true

                TextArea {
                    width: failureScroll.availableWidth
                    readOnly: true
                    wrapMode: TextArea.WrapAnywhere
                    selectByMouse: true
                    text: importSummaryDialog.detailLines.join("\n")
                }
            }

            Label {
                Layout.fillWidth: true
                visible: importSummaryDialog.failureDetailsVisible
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                opacity: 0.8
                text: qsTr("Details were also written to the error log:\n%1")
                        .arg(errorLog?.logFilePath ?? "")
            }
        }
    }

    Connections {
        target: root.servicesReady ? importService : null

        function onImportFinished() {
            const imported = importService.lastImportedCount;
            const skipped = importService.lastSkippedCount;
            const failed = importService.lastFailedCount;

            let lines = [];
            if (imported > 0)
                lines.push(qsTr("%1 file(s) imported").arg(imported));
            if (skipped > 0)
                lines.push(qsTr("%1 skipped (duplicate or already present)").arg(skipped));
            if (failed > 0)
                lines.push(qsTr("%1 failed").arg(failed));
            if (lines.length === 0)
                lines.push(qsTr("No files imported."));

            importSummaryDialog.summaryText = lines.join("\n");
            importSummaryDialog.reasonLines = failed > 0
                    ? (importService.lastFailureReasonSummary || [])
                    : [];
            importSummaryDialog.detailLines = failed > 0
                    ? (importService.lastFailureDetails || [])
                    : [];
            importSummaryDialog.open();
        }
    }

    ImportOrganizerDialog {
        id: importOrganizerDialog
        parent: Overlay.overlay
        organizer: root.servicesReady ? importOrganizer : null
    }

    ImportProgressOverlay {
        id: importProgressOverlay
    }

    AudioProcessingOverlay {
        id: audioProcessingOverlay
    }

    StartupOverlay {
        id: startupOverlay
    }

    TransientNoticePopup {
        id: transientNoticePopup
    }

    Connections {
        target: audioConfigController
        function onTransientNoticeRequested(message) {
            const top = stackView.currentItem;
            if (top && top.objectName === "audioConfigPage")
                return;
            transientNoticePopup.show(message);
        }
    }

    Connections {
        target: errorLog
        function onUserNoticeRequested(message) {
            transientNoticePopup.show(message);
        }
    }

    FileDialog {
        id: openFileDialog
        title: qsTr("Import file")
        fileMode: FileDialog.OpenFiles
        currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]

        onAccepted: {
            const paths = selectedFiles.map(url => root.urlToLocalPath(url));
            root.importPaths(paths);
        }
    }

    function pushAudioConfig(mediaFileId, songTitle) {
        if (!root.servicesReady)
            return;
        const previousItem = stackView.currentItem;
        if (previousItem && previousItem.objectName !== "audioConfigPage"
                && typeof previousItem.releaseKeyboardFocus === "function") {
            previousItem.releaseKeyboardFocus();
        }
        audioConfigController.songTitle = songTitle;
        stackView.push(audioConfigComponent, {
            mediaFileId: mediaFileId,
            songTitle: songTitle
        });
    }

    function pushGuitarProPlayer(songTitle, audioMediaId, practiceAssetId) {
        if (!root.servicesReady)
            return;
        const current = stackView.currentItem;
        if (current && current.objectName === "guitarProPlayerPage")
            return;
        if (current && typeof current.releaseKeyboardFocus === "function")
            current.releaseKeyboardFocus();
        stackView.push(guitarProPlayerComponent, {
            songTitle: songTitle || "",
            backingMediaId: audioMediaId > 0 ? audioMediaId : 0,
            practiceAssetId: practiceAssetId > 0 ? practiceAssetId : 0
        });
    }

    function pushPracticeHub(songId, title, baseBpm, editingReminderId, practiceAssetId, mediaId, sourceReminderId) {
        if (!root.servicesReady || practiceTracker.timerRunning)
            return;
        root.rememberPracticeHub(songId, title, baseBpm, editingReminderId, practiceAssetId, mediaId, sourceReminderId);
        stackView.push(practiceHubComponent, {
            songId: songId,
            songTitle: title,
            songBaseBpm: baseBpm,
            initialEditingReminderId: editingReminderId > 0 ? editingReminderId : 0,
            initialPracticeAssetId: practiceAssetId > 0 ? practiceAssetId : 0,
            initialMediaId: mediaId > 0 ? mediaId : 0,
            initialSourceReminderId: sourceReminderId > 0 ? sourceReminderId : 0
        });
    }

    function popToDashboard() {
        if (!root.servicesReady || practiceTracker.timerRunning)
            return;
        while (stackView.depth > 1)
            stackView.pop();
        reminderController.reloadDayReminders();
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: startupController.needsConfiguration ? configComponent : mainShellComponent

        popEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 180
            }
        }
        popExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 180
            }
        }
        pushEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 180
            }
        }
        pushExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 180
            }
        }
    }

    Component {
        id: configComponent

        ConfigPage {
            firstRun: true

            onConfigurationSaved: {
                if (firstRun)
                    stackView.replace(mainShellComponent);
                else
                    stackView.pop();
            }

            onCancelled: stackView.pop()
        }
    }

    Component {
        id: mainShellComponent

        Item {
            Loader {
                anchors.fill: parent
                active: root.shellReady
                sourceComponent: dashboardComponent
            }
        }
    }

    Component {
        id: dashboardComponent

        DashboardPage {
            showReturnToPractice: root.practiceReturn !== null
            onReturnToPracticeRequested: root.reopenPracticeHub()

            sessionLocked: practiceTracker.timerRunning

            onSongSelected: (songId, title, baseBpm, practiceAssetId, mediaId, sourceReminderId) => root.pushPracticeHub(songId, title, baseBpm, 0, practiceAssetId || 0, mediaId || 0, sourceReminderId || 0)
            onReminderEditRequested: (songId, title, baseBpm, reminderId, practiceAssetId) => root.pushPracticeHub(songId, title, baseBpm, reminderId, practiceAssetId || 0, 0, reminderId)
            onFinishTrainingRequested: root.finishTrainingSession(false)
        }
    }

    Component {
        id: practiceHubComponent

        PracticeHubPage {
            onBackRequested: {
                if (!practiceTracker.timerRunning)
                    stackView.pop();
            }
            onAssetOpenRequested: mediaId => practiceSession.openAsset(mediaId)
            onPracticeRequested: mediaId => practiceSession.startPractice(mediaId)
            onOpenCalendarRequested: root.popToDashboard()
            onAudioConfigRequested: mediaId => root.pushAudioConfig(mediaId, visibleTitle)
            onGuitarProPlayerRequested: (audioMediaId, practiceAssetId) =>
                root.pushGuitarProPlayer(visibleTitle, audioMediaId, practiceAssetId)
            onFinishTrainingRequested: root.finishTrainingSession(false)
        }
    }

    Component {
        id: audioConfigComponent

        AudioConfigPage {
            onBackRequested: {
                audioConfigController.stopPlayback();
                stackView.pop();
            }
        }
    }

    Component {
        id: guitarProPlayerComponent

        GuitarProPlayerPage {
            onBackRequested: {
                guitarProPreviewController.pause();
                audioConfigController.pausePlayback();
                stackView.pop();
            }
            onStopSessionRequested: root.finishTrainingSession(true)
        }
    }

    Component {
        id: libraryComponent

        LibraryPage {
            sessionLocked: practiceTracker.timerRunning

            onFinishTrainingRequested: root.finishTrainingSession(false)

            onBackRequested: {
                if (!practiceTracker.timerRunning)
                    stackView.pop();
            }
            onSongSelected: (songId, title, baseBpm) => root.pushPracticeHub(songId, title, baseBpm, 0, 0, 0)
        }
    }
}
