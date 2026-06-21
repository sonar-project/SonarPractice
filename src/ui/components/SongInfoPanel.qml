/*
  This component is loaded by the dashboard and contains the exercise data.
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../js/MediaKind.js" as MediaKind

GroupBox {
    id: root

    required property int songId
    required property string songTitle
    required property int songBaseBpm

    signal assetOpenRequested(int mediaId)
    signal practiceRequested(int mediaId)
    signal audioConfigRequested(int mediaId)
    signal mediaSelected(int mediaId)

    /** Media confirmed via the active-material radio — required to start the practice timer. */
    property int activeMediaId: 0
    /** practice_asset_id for timer/journal — only set when practice material is confirmed via radio. */
    property int activePracticeAssetId: 0
    readonly property bool practiceMaterialReady: root.activePracticeAssetId > 0 && root.activeMediaId > 0
    /** Internal reentrance guard for applyMediaSelection(). */
    property bool _applyingSelection: false
    /** Media file to pre-select when opening from a reminder or journal restore. */
    property int requestedActiveMediaId: 0
    /** practice_asset_id to pre-select when opening from a reminder. */
    property int requestedActivePracticeAssetId: 0

    // Properties for PracticeHub and SongReminder
    property string activeLabelGuitarPro: ""
    property string activeLabelAudio: ""
    property string activeLabelVideo: ""
    property string activeLabelImage: ""
    property string activeLabelDocument: ""

    /**
     * Cache that maps MediaKind -> kindRow delegate instance.
     * Populated by each delegate's Component.onCompleted.
     */
    property var _kindRowCache: ({})
    property bool _clearingPracticeMaterial: false
    property string activeMediaDisplayName: ""

    function _updateActiveMediaDisplayName() {
        for (const kind of root.kindOrder) {
            const files = mediaFileModel.filesForKind(kind);
            for (let i = 0; i < files.length; ++i) {
                if (files[i].mediaId === root.activeMediaId) {
                    root.activeMediaDisplayName = files[i].displayName;
                    return;
                }
            }
        }
        root.activeMediaDisplayName = "";
    }

    readonly property bool mediaSelectionLocked: practiceTracker.timerRunning

    // get index from combobox for Reminder labels
    function activeLabelForKind(kind) {
        const kindRow = root._kindRowCache[kind];

        if (!kindRow) {
            return "";
        }

        const entry = kindRow.selectedEntry();
        return entry ? entry.displayName : "";
    }

    function releaseKeyboardFocus() {
        for (const kind in root._kindRowCache) {
            const kindRow = root._kindRowCache[kind];
            if (!kindRow) {
                continue;
            }
            if (kindRow.kindCombo) {
                kindRow.kindCombo.popup.close();
                kindRow.kindCombo.focus = false;
            }
            if (kindRow.audioPresetCombo) {
                kindRow.audioPresetCombo.popup.close();
                kindRow.audioPresetCombo.focus = false;
            }
        }
    }

    /** Index of mediaId in the file list for a given kind. Returns 0 if not found. */
    function indexForMediaId(kind, mediaId) {
        const files = mediaFileModel.filesForKind(kind);
        for (let i = 0; i < files.length; ++i) {
            if (files[i].mediaId === mediaId)
                return i;
        }
        return 0;
    }

    // Legacy helper kept for any GuitarPro-only call-sites.
    function gpIndexForMediaId(mediaId) {
        return root.indexForMediaId(MediaKind.GuitarPro, mediaId);
    }

    property var _pendingAsset: ({
            guitarProId: 0,
            audioId: 0,
            videoId: 0,
            imageId: 0,
            documentId: 0
        })

    function _commitPracticeMaterial(confirmedMediaId) {
        const a = root._pendingAsset;
        const hasAny = a.guitarProId > 0 || a.audioId > 0 || a.videoId > 0 || a.imageId > 0 || a.documentId > 0;
        if (!hasAny || confirmedMediaId <= 0) {
            root._clearPracticeMaterial();
            return;
        }
        root.activeMediaId = confirmedMediaId;
        root.activePracticeAssetId = practiceAssetController.upsertCompositeAsset(
            root.songId, a.guitarProId, a.audioId, a.videoId, a.imageId, a.documentId);
        root._updateActiveMediaDisplayName();
    }

    function _clearPracticeMaterial() {
        if (root._clearingPracticeMaterial)
            return;
        root._clearingPracticeMaterial = true;
        root.activeMediaId = 0;
        root.activePracticeAssetId = 0;
        root.activeMediaDisplayName = "";
        for (const kind of root.kindOrder) {
            const kindRow = root._kindRowCache[kind];
            if (kindRow)
                kindRow.clearPracticeMaterialRadio();
        }
        root._clearingPracticeMaterial = false;
    }

    function _activateRadioForMediaId(mediaId) {
        if (mediaId <= 0)
            return;
        for (const kind of root.kindOrder) {
            const kindRow = root._kindRowCache[kind];
            if (!kindRow)
                continue;
            const entry = kindRow.selectedEntry();
            if (entry && entry.mediaId === mediaId) {
                kindRow.setPracticeMaterialActive(true);
                return;
            }
        }
    }

    function applyMediaSelection(entry, kind) {
        if (root._applyingSelection)
            return;
        const newMediaId = entry ? entry.mediaId : 0;

        root._applyingSelection = true;

        // Update local slot
        const a = root._pendingAsset;
        if (kind === MediaKind.GuitarPro)
            a.guitarProId = newMediaId;
        else if (kind === MediaKind.Audio)
            a.audioId = newMediaId;
        else if (kind === MediaKind.Video)
            a.videoId = newMediaId;
        else if (kind === MediaKind.Image)
            a.imageId = newMediaId;
        else if (kind === MediaKind.Document)
            a.documentId = newMediaId;

        root._pendingAsset = a;

        root._applyingSelection = false;
        root._updateKindLabels();

        const kindRow = root._kindRowCache[kind];
        if (kindRow && kindRow.isPracticeMaterialActive()) {
            if (entry)
                root._commitPracticeMaterial(entry.mediaId);
            else {
                kindRow.clearPracticeMaterialRadio();
                root._clearPracticeMaterial();
            }
        }
    }

    // Called by the ReminderPanel shortly before saving (independent of radio/timer state).
    function commitPendingAsset() {
        const a = root._pendingAsset;
        const hasAny = a.guitarProId > 0 || a.audioId > 0 || a.videoId > 0 || a.imageId > 0 || a.documentId > 0;
        if (!hasAny)
            return root.activePracticeAssetId > 0 ? root.activePracticeAssetId : 0;
        return practiceAssetController.upsertCompositeAsset(
            root.songId, a.guitarProId, a.audioId, a.videoId, a.imageId, a.documentId);
    }

    // update reminder labels
    function _updateKindLabels() {
        root.activeLabelGuitarPro = activeLabelForKind(MediaKind.GuitarPro);
        root.activeLabelAudio = activeLabelForKind(MediaKind.Audio);
        root.activeLabelVideo = activeLabelForKind(MediaKind.Video);
        root.activeLabelImage = activeLabelForKind(MediaKind.Image);
        root.activeLabelDocument = activeLabelForKind(MediaKind.Document);
    }

    // Legacy helper kept for call-sites that only deal with GuitarPro.
    function applyGuitarProSelection(entry) {
        root.applyMediaSelection(entry, MediaKind.GuitarPro);
    }

    /**
     * Sets the combo for a given kind to the correct index for `file`
     * using the cached kindRow delegate instance.
     *
     * NOTE: filesForKind() returns a freshly-allocated QVariantList on every
     * call (see MediaFileListModel.h), so `file` here is never the same JS
     * object instance as the entries in kindRow.filteredFiles even when it
     * represents the same underlying media row. Comparing by reference
     * (Array.indexOf) would therefore always fail; compare by mediaId.
     */
    function _setComboForFile(kind, file) {
        const kindRow = root._kindRowCache[kind];
        if (!kindRow || !file)
            return;
        const idx = kindRow.filteredFiles.findIndex(f => f.mediaId === file.mediaId);
        if (idx >= 0)
            kindRow.kindCombo.currentIndex = idx + 1; // +1 for the "--" placeholder
    }

    /** Forces every kindRow's cached file list to re-read from mediaFileModel. */
    function _refreshAllKindRowCaches() {
        for (const kind of root.kindOrder) {
            const kindRow = root._kindRowCache[kind];
            if (kindRow)
                kindRow._refreshKindFiles();
        }
        root._updateActiveMediaDisplayName();
    }

    function restoreLastSelection() {
        Qt.callLater(function () {
            root._refreshAllKindRowCaches();

            const assetId = root.requestedActivePracticeAssetId > 0 ? root.requestedActivePracticeAssetId : practiceAssetController.lastMediaFileIdForSong(root.songId);

            if (assetId > 0) {
                const asset = practiceAssetController.assetById(assetId);
                if (asset && Object.keys(asset).length > 0) {
                    root._pendingAsset = {
                        guitarProId: asset.guitarProId ?? 0,
                        audioId: asset.audioId ?? 0,
                        videoId: asset.videoId ?? 0,
                        imageId: asset.imageId ?? 0,
                        documentId: asset.documentId ?? 0
                    };
                    _restoreComboForMediaId(MediaKind.GuitarPro, asset.guitarProId ?? 0);
                    _restoreComboForMediaId(MediaKind.Audio, asset.audioId ?? 0);
                    _restoreComboForMediaId(MediaKind.Video, asset.videoId ?? 0);
                    _restoreComboForMediaId(MediaKind.Image, asset.imageId ?? 0);
                    _restoreComboForMediaId(MediaKind.Document, asset.documentId ?? 0);
                    const primaryMediaId = practiceAssetController.mediaFileIdForAsset(assetId);
                    if (primaryMediaId > 0) {
                        Qt.callLater(function () {
                            root._activateRadioForMediaId(primaryMediaId);
                        });
                    }
                    root._updateKindLabels();
                    return;
                }
            }

            root._clearPracticeMaterial();
            root._updateKindLabels();
        });
    }

    function _restoreComboForMediaId(kind, mediaId) {
        if (mediaId <= 0)
            return;
        const kindRow = root._kindRowCache[kind];
        if (!kindRow)
            return;
        const idx = kindRow.filteredFiles.findIndex(f => f.mediaId === mediaId);
        if (idx >= 0)
            kindRow.kindCombo.currentIndex = idx + 1;
    }

    onActiveMediaIdChanged: {
        if (root.activeMediaId <= 0) {
            root.activeMediaDisplayName = "";
        }
    }

    onRequestedActiveMediaIdChanged: root.restoreLastSelection()
    onRequestedActivePracticeAssetIdChanged: {
        if (root.requestedActivePracticeAssetId > 0) {
            const asset = practiceAssetController.assetById(root.requestedActivePracticeAssetId);
            if (asset && Object.keys(asset).length > 0) {
                root._pendingAsset = {
                    guitarProId: asset.guitarProId ?? 0,
                    audioId: asset.audioId ?? 0,
                    videoId: asset.videoId ?? 0,
                    imageId: asset.imageId ?? 0,
                    documentId: asset.documentId ?? 0
                };
                _restoreComboForMediaId(MediaKind.GuitarPro, asset.guitarProId ?? 0);
                _restoreComboForMediaId(MediaKind.Audio, asset.audioId ?? 0);
                _restoreComboForMediaId(MediaKind.Video, asset.videoId ?? 0);
                _restoreComboForMediaId(MediaKind.Image, asset.imageId ?? 0);
                _restoreComboForMediaId(MediaKind.Document, asset.documentId ?? 0);
                const primaryMediaId = practiceAssetController.mediaFileIdForAsset(root.requestedActivePracticeAssetId);
                if (primaryMediaId > 0) {
                    Qt.callLater(function () {
                        root._activateRadioForMediaId(primaryMediaId);
                    });
                }
            }
        }
    }

    onSongIdChanged: root.restoreLastSelection()

    readonly property var kindOrder: [MediaKind.GuitarPro, MediaKind.Video, MediaKind.Audio, MediaKind.Document, MediaKind.Image]

    title: qsTr("Song information")
    padding: 12

    background: Rectangle {
        radius: 10
        color: Theme.panelBackground
        border.color: Theme.border
    }

    label: Label {
        text: root.title
        font.pixelSize: 14
        font.weight: Font.DemiBold
        color: Theme.textPrimary
    }

    ColumnLayout {
        width: parent.width
        spacing: 10

        Label {
            Layout.fillWidth: true
            text: root.songTitle
            font.pixelSize: 20
            font.weight: Font.Bold
            color: Theme.textTitle
        }

        Label {
            text: root.songBaseBpm > 0 ? qsTr("Base tempo: %1 BPM").arg(root.songBaseBpm) : qsTr("No tempo set")
            font.pixelSize: 13
            color: Theme.textTertiary
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: mediaFileModel.count > 0 ? qsTr("%1 media — select a file and open it with the button.").arg(mediaFileModel.count) : qsTr("No media found for this exercise.")
            font.pixelSize: 12
            color: Theme.textSecondary
        }

        Label {
            text: qsTr("Select a file and mark it as active material.")
            color: Theme.textSecondary
        }

        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: qsTr("Search media...")
            background: Rectangle {
                radius: 4
                border.color: Theme.border
                color: Theme.inputBackground
            }
        }

        Column {
            Layout.fillWidth: true
            spacing: 8

            ButtonGroup {
                id: practiceMaterialGroup
            }

            Repeater {
                model: root.kindOrder

                delegate: Item {
                    id: kindRow
                    required property string modelData

                    /**
                     * NOT readonly: filesForKind() is a plain Q_INVOKABLE, so a
                     * `readonly property var: mediaFileModel.filesForKind(modelData)`
                     * binding would never re-evaluate automatically (QML only tracks
                     * NOTIFY-backed property reads, not invokable calls). We therefore
                     * populate it once below and explicitly refresh it via _refreshKindFiles()
                     * whenever mediaFileModel reports a relevant change.
                     */
                    property var kindFiles: mediaFileModel.filesForKind(modelData)

                    function _refreshKindFiles() {
                        kindRow.kindFiles = mediaFileModel.filesForKind(kindRow.modelData);
                        if (kindCombo.currentIndex > 0)
                            kindRow.applyComboSelection();
                    }

                    property int _comboSelectionRetries: 0

                    function applyComboSelection() {
                        if (kindCombo.currentIndex <= 0) {
                            kindRow._comboSelectionRetries = 0;
                            root.applyMediaSelection(null, kindRow.modelData);
                            return;
                        }
                        const entry = kindRow.filteredFiles[kindCombo.currentIndex - 1];
                        if (!entry) {
                            if (kindRow._comboSelectionRetries < 5) {
                                kindRow._comboSelectionRetries++;
                                Qt.callLater(kindRow.applyComboSelection);
                            }
                            return;
                        }
                        kindRow._comboSelectionRetries = 0;
                        root.applyMediaSelection(entry, kindRow.modelData);
                        kindRow.markSelectionChanged();
                        if (kindRow.isAudio)
                            kindRow.syncAudioPresetContext();
                    }

                    Connections {
                        target: mediaFileModel
                        function onFilesForKindChanged(kind) {
                            if (kind === kindRow.modelData)
                                kindRow._refreshKindFiles();
                        }
                        function onSongIdChanged() {
                            kindRow._refreshKindFiles();
                        }
                    }

                    /**
                     * filteredFiles lives here on kindRow (not on kindCombo) so that
                     * restoreLastSelection() can reach it via _kindRowCache.
                     */
                    readonly property var filteredFiles: kindFiles.filter(f => f.displayName.toLowerCase().includes(searchField.text.toLowerCase()))

                    readonly property bool isGuitarPro: modelData === MediaKind.GuitarPro
                    readonly property bool isAudio: modelData === MediaKind.Audio
                    readonly property bool isInternalAudio: modelData === MediaKind.Audio || modelData === MediaKind.Video
                    readonly property bool isInternalVideo: modelData === MediaKind.Video

                    // Expose kindCombo so root._setComboForFile() can set currentIndex.
                    property alias kindCombo: kindCombo
                    property alias audioPresetCombo: audioPresetCombo

                    function isPracticeMaterialActive() {
                        return timerRadioButton.checked;
                    }

                    function setPracticeMaterialActive(active) {
                        timerRadioButton.checked = active;
                    }

                    function clearPracticeMaterialRadio() {
                        timerRadioButton.checked = false;
                    }

                    function selectedEntry() {
                        if (kindCombo.currentIndex <= 0)
                            return null; // "--" placeholder
                        const idx = kindCombo.currentIndex - 1;
                        if (idx >= filteredFiles.length)
                            return null;
                        return filteredFiles[idx];
                    }

                    width: parent.width
                    height: kindRowColumn.implicitHeight
                    visible: kindFiles.length > 0

                    // Register / unregister this delegate in the root cache.
                    Component.onCompleted: root._kindRowCache[modelData] = kindRow
                    Component.onDestruction: delete root._kindRowCache[modelData]

                    function openSelectedEntry() {
                        const entry = selectedEntry();
                        if (!entry)
                            return;
                        if (entry.canBePracticed)
                            root.practiceRequested(entry.mediaId);
                        else
                            root.assetOpenRequested(entry.mediaId);
                    }

                    function playAudioEntry(entry) {
                        if (!entry)
                            return;
                        audioConfigController.playMediaFile(entry.mediaId);
                    }

                    function isAudioEntryPlaying(entry) {
                        return entry && audioConfigController.mediaFileId === entry.mediaId && audioConfigController.playing;
                    }

                    function syncAudioPresetContext() {
                        if (!kindRow.isAudio)
                            return;
                        const entry = kindRow.selectedEntry();
                        if (entry)
                            audioConfigController.preparePresetsForMedia(entry.mediaId);
                    }

                    function loadAudioPreset() {
                        const entry = kindRow.selectedEntry();
                        if (!entry)
                            return;
                        audioConfigController.loadPresetForMedia(entry.mediaId, audioPresetCombo.currentIndex);
                    }

                    function markSelectionChanged() {
                        selectionGuard.restart();
                    }

                    Timer {
                        id: selectionGuard
                        interval: 200
                    }

                    ColumnLayout {
                        id: kindRowColumn
                        width: parent.width
                        spacing: 6

                        RowLayout {
                            id: kindRowLayout
                            Layout.fillWidth: true
                            spacing: 8

                            MediaKindIcon {
                                kind: kindRow.modelData
                                size: 18
                                Layout.alignment: Qt.AlignVCenter
                            }

                            ComboBox {
                                id: kindCombo
                                Layout.minimumWidth: 700
                                enabled: !root.mediaSelectionLocked && kindRow.kindFiles.length > 0

                                // Model is built from kindRow.filteredFiles (defined above).
                                model: ["--"].concat(kindRow.filteredFiles.map(f => f.displayName))

                                onCurrentIndexChanged: kindRow.applyComboSelection()

                                popup.onClosed: kindRow.markSelectionChanged()

                                Keys.onReturnPressed: event => event.accepted = true
                                Keys.onEnterPressed: event => event.accepted = true
                            }

                            RadioButton {
                                id: timerRadioButton
                                text: qsTr("active material")
                                enabled: !root.mediaSelectionLocked && kindRow.selectedEntry() !== null
                                ButtonGroup.group: practiceMaterialGroup
                                onToggled: {
                                    if (checked) {
                                        const entry = kindRow.selectedEntry();
                                        if (entry)
                                            root._commitPracticeMaterial(entry.mediaId);
                                    } else {
                                        const entry = kindRow.selectedEntry();
                                        if (!root._clearingPracticeMaterial
                                                && entry
                                                && entry.mediaId === root.activeMediaId) {
                                            root._clearPracticeMaterial();
                                        }
                                    }
                                }
                            }

                            Button {
                                text: qsTr("Open")
                                enabled: !root.mediaSelectionLocked && kindRow.selectedEntry() !== null
                                focusPolicy: Qt.TabFocus
                                visible: kindRow.modelData !== MediaKind.Audio && kindRow.modelData !== MediaKind.Video

                                Keys.onReturnPressed: event => event.accepted = true
                                Keys.onEnterPressed: event => event.accepted = true

                                onClicked: {
                                    if (selectionGuard.running) {
                                        return;
                                    }
                                    kindRow.openSelectedEntry();
                                }
                            }

                            Button {
                                text: kindRow.isAudioEntryPlaying(kindRow.selectedEntry()) ? qsTr("Pause") : qsTr("Play")
                                enabled: kindRow.selectedEntry() !== null && !kindRow.isInternalVideo
                                        && !audioConfigController.loading
                                visible: kindRow.isInternalAudio && !kindRow.isInternalVideo
                                focusPolicy: Qt.TabFocus

                                onClicked: {
                                    if (selectionGuard.running) {
                                        return;
                                    }
                                    kindRow.playAudioEntry(kindRow.selectedEntry());
                                }
                            }

                            Button {
                                text: qsTr("Configure")
                                enabled: kindRow.selectedEntry() !== null && !kindRow.isInternalVideo
                                visible: kindRow.isInternalAudio && !kindRow.isInternalVideo
                                focusPolicy: Qt.TabFocus

                                onClicked: {
                                    if (selectionGuard.running) {
                                        return;
                                    }
                                    const entry = kindRow.selectedEntry();
                                    if (entry) {
                                        root.audioConfigRequested(entry.mediaId);
                                    }
                                }
                            }

                            Button {
                                text: qsTr("Open externally")
                                enabled: kindRow.selectedEntry() !== null && !kindRow.isAudioEntryPlaying(kindRow.selectedEntry())
                                visible: kindRow.isInternalAudio
                                flat: false
                                focusPolicy: Qt.TabFocus

                                onClicked: {
                                    if (selectionGuard.running) {
                                        return;
                                    }
                                    kindRow.openSelectedEntry();
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            visible: kindRow.isInternalAudio

                            AudioPresetIcon {
                                size: 18
                                Layout.alignment: Qt.AlignVCenter
                                visible: kindRow.isAudio && !kindRow.isInternalVideo
                            }

                            ComboBox {
                                id: audioPresetCombo
                                Layout.preferredWidth: kindCombo.width
                                Layout.fillWidth: false
                                enabled: count > 0 && kindRow.selectedEntry() !== null
                                model: audioConfigController.presetNames
                                currentIndex: audioConfigController.selectedPresetIndex
                                visible: kindRow.isAudio && !kindRow.isInternalVideo

                                onActivated: audioConfigController.selectedPresetIndex = currentIndex

                                Connections {
                                    target: audioConfigController
                                    function onSelectedPresetIndexChanged() {
                                        if (audioPresetCombo.currentIndex !== audioConfigController.selectedPresetIndex)
                                            audioPresetCombo.currentIndex = audioConfigController.selectedPresetIndex;
                                    }
                                    function onPresetNamesChanged() {
                                        if (audioPresetCombo.count === 0) {
                                            audioPresetCombo.currentIndex = -1;
                                            return;
                                        }
                                        const index = audioConfigController.selectedPresetIndex;
                                        audioPresetCombo.currentIndex = (index >= 0 && index < audioPresetCombo.count) ? index : 0;
                                    }
                                    function onMediaFileIdChanged() {
                                        if (!kindRow.isAudio)
                                            return;
                                        const entry = kindRow.selectedEntry();
                                        if (entry && entry.mediaId === audioConfigController.mediaFileId)
                                            kindRow.syncAudioPresetContext();
                                    }
                                }
                            }

                            Button {
                                text: qsTr("Load preset")
                                enabled: audioPresetCombo.count > 0 && kindRow.selectedEntry() !== null && !audioConfigController.loading
                                focusPolicy: Qt.TabFocus
                                visible: kindRow.isAudio && !kindRow.isInternalVideo

                                onClicked: {
                                    if (selectionGuard.running)
                                        return;
                                    kindRow.loadAudioPreset();
                                }
                            }

                            Item { Layout.fillWidth: true }
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: kindRow.isInternalAudio && audioConfigController.loading
                            text: audioConfigController.processingStage.length > 0
                                  ? audioConfigController.processingStage
                                  : qsTr("Processing audio…")
                            font.pixelSize: 12
                            color: Theme.textHint
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: linkGroupService
        function onGroupsChanged() {
            mediaFileModel.reload();
        }
    }

    Connections {
        target: mediaFileModel
        function onMediaCountChanged() {
            root._refreshAllKindRowCaches();
            root.restoreLastSelection();
        }
    }

    Component.onCompleted: root.restoreLastSelection()
}
