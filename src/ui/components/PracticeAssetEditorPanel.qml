// src/ui/components/PracticeAssetEditorPanel.qml
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../js/MediaKind.js" as MediaKind

/**
 * Editor for assembling a composite practice asset.
 *
 * Inputs:
 *   - songId          : int  – current song
 *   - initialAssetId  : int  – 0 = new, >0 = load existing
 *
 * Output signal:
 *   - assetCommitted(int assetId)
 */
GroupBox {
    id: root

    required property int songId
    property int initialAssetId: 0

    signal assetCommitted(int assetId)

    // Selected media_file_id per slot
    property int selectedGuitarProId: 0
    property int selectedAudioId:     0
    property int selectedVideoId:     0
    property int selectedImageId:     0

    title: qsTr("Assemble practice material")

    // On open: load existing asset when initialAssetId is set
    Component.onCompleted: root.loadInitialAsset()

    onInitialAssetIdChanged: root.loadInitialAsset()

    function loadInitialAsset() {
        if (root.initialAssetId <= 0) return
        const a = reminderController.practiceAssetPayload(root.initialAssetId)
        root.selectedGuitarProId = a.guitarProId ?? 0
        root.selectedAudioId     = a.audioId     ?? 0
        root.selectedVideoId     = a.videoId     ?? 0
        root.selectedImageId     = a.imageId     ?? 0
    }

    function commit() {
        // Upsert via controller; upsertCompositeAsset returns the shared asset id
        const assetId = practiceAssetController.upsertCompositeAsset(
            root.songId,
            root.selectedGuitarProId,
            root.selectedAudioId,
            root.selectedVideoId,
            root.selectedImageId
        )
        if (assetId > 0)
            root.assetCommitted(assetId)
    }

    ColumnLayout {
        width: parent.width
        spacing: 10

        // ── GuitarPro ────────────────────────────────────────────────
        Label { text: qsTr("Guitar Pro") }
        ComboBox {
            Layout.fillWidth: true
            model: mediaFileModel.filesForKind(MediaKind.GuitarPro)
            textRole: "displayName"
            onActivated: root.selectedGuitarProId =
                currentIndex >= 0 ? model[currentIndex].mediaId : 0
        }

        // ── Audio ────────────────────────────────────────────────────
        Label { text: qsTr("Audio") }
        ComboBox {
            Layout.fillWidth: true
            model: mediaFileModel.filesForKind(MediaKind.Audio)
            textRole: "displayName"
            onActivated: root.selectedAudioId =
                currentIndex >= 0 ? model[currentIndex].mediaId : 0
        }

        // ── Video ────────────────────────────────────────────────────
        Label { text: qsTr("Video") }
        ComboBox {
            Layout.fillWidth: true
            model: mediaFileModel.filesForKind(MediaKind.Video)
            textRole: "displayName"
            onActivated: root.selectedVideoId =
                currentIndex >= 0 ? model[currentIndex].mediaId : 0
        }

        // ── Image / document ──────────────────────────────────────────
        Label { text: qsTr("Image / document") }
        ComboBox {
            Layout.fillWidth: true
            model: [
                ...mediaFileModel.filesForKind(MediaKind.Image),
                ...mediaFileModel.filesForKind(MediaKind.Document)
            ]
            textRole: "displayName"
            onActivated: root.selectedImageId =
                currentIndex >= 0 ? model[currentIndex].mediaId : 0
        }

        Button {
            Layout.fillWidth: true
            text: qsTr("Save asset")
            highlighted: true
            onClicked: root.commit()
        }
    }
}
