import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
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
        id: importSummaryDialog
        title: qsTr("Import complete")
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok
        width: Math.min(root.width * 0.7, 420)

        property string summaryText: ""

        Label {
            width: importSummaryDialog.availableWidth
            wrapMode: Text.WordWrap
            text: importSummaryDialog.summaryText
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
            importSummaryDialog.open();
        }
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

    function pushPracticeHub(songId, title, baseBpm, editingReminderId, practiceAssetId) {
        if (!root.servicesReady || practiceTracker.timerRunning)
            return;
        stackView.push(practiceHubComponent, {
            songId: songId,
            songTitle: title,
            songBaseBpm: baseBpm,
            initialEditingReminderId: editingReminderId > 0 ? editingReminderId : 0,
            initialPracticeAssetId: practiceAssetId > 0 ? practiceAssetId : 0
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
            sessionLocked: practiceTracker.timerRunning

            onSongSelected: (songId, title, baseBpm, practiceAssetId) => root.pushPracticeHub(songId, title, baseBpm, 0, practiceAssetId || 0)
            onReminderEditRequested: (songId, title, baseBpm, reminderId, practiceAssetId) => root.pushPracticeHub(songId, title, baseBpm, reminderId, practiceAssetId || 0)
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
            onAudioConfigRequested: mediaId => root.pushAudioConfig(mediaId, songTitle)
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
        id: libraryComponent

        LibraryPage {
            sessionLocked: practiceTracker.timerRunning

            onBackRequested: {
                if (!practiceTracker.timerRunning)
                    stackView.pop();
            }
            onSongSelected: (songId, title, baseBpm) => root.pushPracticeHub(songId, title, baseBpm, 0, 0)
        }
    }
}
