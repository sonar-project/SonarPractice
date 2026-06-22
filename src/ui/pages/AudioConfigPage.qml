import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import com.sonarp.sonarpractice
import "../components"

Page {
    id: root
    objectName: "audioConfigPage"

    focus: true;

    required property int mediaFileId
    required property string songTitle

    signal backRequested()

    StackView.onActivated: {
        root.forceActiveFocus()
        audioConfigController.refreshTabTuningList()
    }

    background: Rectangle { color: Theme.windowBackground }

    header: TopBar {
        title: root.songTitle.length > 0 ? root.songTitle : audioConfigController.displayName
        subtitle: {
            if (root.songTitle.length > 0 && audioConfigController.displayName.length > 0
                    && root.songTitle !== audioConfigController.displayName)
                return audioConfigController.displayName
            return ""
        }
        showBack: true
        onBackRequested: root.backRequested()
    }

    function ratioForMs(ms) {
        if (audioConfigController.durationMs <= 0)
            return 0
        return Math.max(0, Math.min(1, ms / audioConfigController.durationMs))
    }

    function syncPitchNoteEditor() {
        const idx = audioConfigController.selectedPitchNoteIndex
        if (idx < 0 || idx >= audioConfigController.pitchNotes.length)
            return
        const note = audioConfigController.pitchNotes[idx]
        pitchNoteStartSpin.value = note.startMs
        pitchNoteEndSpin.value = note.endMs
        pitchNoteStringSpin.value = (note.tabString !== undefined ? note.tabString : 0) + 1
        pitchNoteFretSpin.value = note.tabFret !== undefined ? note.tabFret : 0
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            visible: audioConfigController.hasPitchData

            Label {
                text: qsTr("Tablature")
                color: Theme.textSecondary
                font.pixelSize: 11
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                visible: audioConfigController.hasPitchData
                         && audioConfigController.selectedPitchNoteIndex >= 0

                Label {
                    id: pitchNoteNameLabel
                    text: {
                        const idx = audioConfigController.selectedPitchNoteIndex
                        if (idx < 0 || idx >= audioConfigController.pitchNotes.length)
                            return ""
                        const note = audioConfigController.pitchNotes[idx]
                        return note.tabDisplay !== undefined ? note.tabDisplay : note.label
                    }
                    font.pixelSize: 20
                    font.bold: true
                    color: Theme.accent
                    Layout.minimumWidth: 48
                }

                Label {
                    text: qsTr("Edit:")
                    color: Theme.textSecondary
                }

                Label {
                    text: qsTr("Start ms")
                    color: Theme.textMuted
                    font.pixelSize: 11
                }

                DarkSpinBox {
                    id: pitchNoteStartSpin
                    from: 0
                    to: Math.max(0, audioConfigController.durationMs)
                }

                Label {
                    text: qsTr("End ms")
                    color: Theme.textMuted
                    font.pixelSize: 11
                }

                DarkSpinBox {
                    id: pitchNoteEndSpin
                    from: 0
                    to: Math.max(0, audioConfigController.durationMs)
                }

                Label {
                    text: qsTr("String")
                    color: Theme.textMuted
                    font.pixelSize: 11
                }

                DarkSpinBox {
                    id: pitchNoteStringSpin
                    from: 1
                    to: Math.max(1, audioConfigController.tabStringCount)
                }

                Label {
                    text: qsTr("Fret")
                    color: Theme.textMuted
                    font.pixelSize: 11
                }

                DarkSpinBox {
                    id: pitchNoteFretSpin
                    from: 0
                    to: 24
                }

                Button {
                    text: qsTr("Apply")
                    onClicked: {
                        const idx = audioConfigController.selectedPitchNoteIndex
                        if (idx < 0)
                            return
                        audioConfigController.updatePitchNoteFromTab(
                                    idx,
                                    pitchNoteStartSpin.value,
                                    pitchNoteEndSpin.value,
                                    pitchNoteStringSpin.value - 1,
                                    pitchNoteFretSpin.value)
                    }
                }

                Button {
                    text: qsTr("Delete")
                    onClicked: audioConfigController.removeSelectedPitchNote()
                }
            }
        }

        AudioTimelineViewport {
            id: audioTimeline
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight

            showTablature: audioConfigController.hasPitchData
            tabStringCount: audioConfigController.tabStringCount

            pitchNotes: audioConfigController.pitchNotes
            tabStringLabels: audioConfigController.tabStringLabels
            pitchSelectedIndex: audioConfigController.selectedPitchNoteIndex
            durationMs: audioConfigController.durationMs
            regionStartRatio: root.ratioForMs(audioConfigController.regionStartMs)
            regionEndRatio: root.ratioForMs(audioConfigController.regionEndMs)
            playheadRatio: root.ratioForMs(audioConfigController.positionMs)
            playing: audioConfigController.playing

            waveformPeaks: audioConfigController.peaks
            waveformOpacity: audioConfigController.loading ? 0.45 : 1.0

            onNoteClicked: (index) => {
                audioConfigController.selectedPitchNoteIndex = index
                root.syncPitchNoteEditor()
            }

            onWaveformClicked: (ratio) => {
                const targetMs = Math.round(ratio * audioConfigController.durationMs)
                if (targetMs < audioConfigController.regionEndMs)
                    audioConfigController.regionStartMs = targetMs
                else
                    audioConfigController.regionEndMs = targetMs
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: audioConfigController.playing ? qsTr("Pause") : qsTr("Play")
                enabled: !audioConfigController.loading
                onClicked: audioConfigController.togglePlayback()
            }

            Button {
                text: qsTr("Stop")
                enabled: true
                onClicked: audioConfigController.stopPlayback()
            }

            Label {
                text: qsTr("%1 / %2 s")
                        .arg(Math.floor(audioConfigController.positionMs / 1000))
                        .arg(Math.floor(audioConfigController.durationMs / 1000))
                color: Theme.textSecondary
            }

            Item { Layout.fillWidth: true }

            CheckBox {
                text: qsTr("Loop A–B")
                checked: audioConfigController.loopEnabled
                enabled: true
                onToggled: audioConfigController.loopEnabled = checked
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: qsTr("A = Position")
                enabled: true
                onClicked: audioConfigController.setRegionFromPosition(true)
            }

            Button {
                text: qsTr("B = Position")
                enabled: true
                onClicked: audioConfigController.setRegionFromPosition(false)
            }

            Button {
                text: qsTr("Undo")
                enabled: audioConfigController.canUndoRegion
                onClicked: audioConfigController.undoRegion()
            }
        }

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Tempo")

            background: Rectangle {
                radius: 8
                color: Theme.panelBackground
                border.color: Theme.border
            }

            ColumnLayout {
                width: parent.width
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Slider {
                        id: tempoSlider
                        Layout.fillWidth: true
                        from: 50
                        to: 100
                        stepSize: 1
                        value: audioConfigController.tempoPercent
                        enabled: !audioConfigController.loading

                        onMoved: audioConfigController.tempoPercent = Math.round(value)

                        onPressedChanged: {
                            if (!pressed)
                                audioConfigController.commitTempo()
                        }
                    }

                    ComboBox {
                        id: tempoPresetCombo
                        Layout.preferredWidth: 96
                        enabled: !audioConfigController.loading
                        model: [
                            { label: qsTr("50 %"), value: 50 },
                            { label: qsTr("60 %"), value: 60 },
                            { label: qsTr("90 %"), value: 90 },
                            { label: qsTr("100 %"), value: 100 }
                        ]
                        textRole: "label"

                        function syncIndexFromTempo() {
                            let matchIndex = -1
                            const tempo = audioConfigController.tempoPercent
                            for (let i = 0; i < model.length; ++i) {
                                if (model[i].value === tempo) {
                                    matchIndex = i
                                    break
                                }
                            }
                            if (currentIndex !== matchIndex)
                                currentIndex = matchIndex
                        }

                        Component.onCompleted: syncIndexFromTempo()

                        Connections {
                            target: audioConfigController
                            function onTempoPercentChanged() {
                                tempoPresetCombo.syncIndexFromTempo()
                            }
                        }

                        onActivated: {
                            if (currentIndex < 0 || currentIndex >= model.length)
                                return
                            audioConfigController.tempoPercent = model[currentIndex].value
                            audioConfigController.commitTempo()
                        }
                    }
                }

                Label {
                    text: qsTr("%1 %").arg(Math.round(tempoSlider.value))
                    color: Theme.textSecondary
                }
            }
        }

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("EQ preset")

            background: Rectangle {
                radius: 8
                color: Theme.panelBackground
                border.color: Theme.border
            }

            ComboBox {
                width: parent.width
                enabled: !audioConfigController.loading
                model: [
                    { label: qsTr("Neutral"), value: "flat" },
                    { label: qsTr("Reduce bass"), value: "reduce_low" },
                    { label: qsTr("Reduce treble"), value: "reduce_high" },
                    { label: qsTr("Emphasize mids"), value: "mid_focus" }
                ]
                textRole: "label"
                valueRole: "value"

                Component.onCompleted: {
                    let index = 0
                    for (let i = 0; i < model.length; ++i) {
                        if (model[i].value === audioConfigController.eqPresetId) {
                            index = i
                            break
                        }
                    }
                    currentIndex = index
                }

                onActivated: {
                    if (currentIndex >= 0 && currentIndex < model.length)
                        audioConfigController.eqPresetId = model[currentIndex].value
                }
            }
        }

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Pitch detection")

            background: Rectangle {
                radius: 8
                color: Theme.panelBackground
                border.color: Theme.border
            }

            ColumnLayout {
                width: parent.width
                spacing: 8

                Component.onCompleted: audioConfigController.refreshTabTuningList()

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        id: detectPitchButton
                        text: qsTr("Detect pitch")
                        enabled: !audioConfigController.loading
                                 && !audioConfigController.pitchAnalyzing
                                 && audioConfigController.mediaFileId > 0
                        onClicked: {
                            if (audioConfigController.hasCustomRegion)
                                pitchScopeDialog.open()
                            else
                                audioConfigController.startPitchDetection(false)
                        }
                    }

                    Label {
                        text: qsTr("Instrument")
                        color: Theme.textMuted
                        font.pixelSize: 11
                    }

                    ComboBox {
                        id: tabInstrumentCombo
                        Layout.preferredWidth: 108
                        model: [
                            { label: qsTr("Guitar"), value: 0 },
                            { label: qsTr("Bass"), value: 1 }
                        ]
                        textRole: "label"
                        currentIndex: audioConfigController.tabInstrument

                        onActivated: {
                            if (currentIndex >= 0 && currentIndex < model.length)
                                audioConfigController.tabInstrument = model[currentIndex].value
                        }

                        Connections {
                            target: audioConfigController
                            function onTabLayoutChanged() {
                                tabInstrumentCombo.currentIndex = audioConfigController.tabInstrument
                            }
                        }
                    }

                    Label {
                        text: qsTr("Tuning")
                        color: Theme.textMuted
                        font.pixelSize: 11
                    }

                    ComboBox {
                        id: tabTuningCombo
                        Layout.preferredWidth: 220
                        Layout.minimumWidth: 160
                        Layout.fillWidth: true
                        model: audioConfigController.tabTuningNames
                        currentIndex: audioConfigController.selectedTabTuningIndex
                        displayText: currentIndex >= 0
                                     ? tabTuningCombo.textAt(currentIndex)
                                     : qsTr("Select tuning…")

                        onActivated: audioConfigController.selectedTabTuningIndex = currentIndex

                        Connections {
                            target: audioConfigController
                            function onSelectedTabTuningIndexChanged() {
                                const index = audioConfigController.selectedTabTuningIndex
                                if (tabTuningCombo.currentIndex !== index)
                                    tabTuningCombo.currentIndex = index
                            }
                            function onTabTuningNamesChanged() {
                                if (tabTuningCombo.count === 0) {
                                    tabTuningCombo.currentIndex = -1
                                    return
                                }
                                const index = audioConfigController.selectedTabTuningIndex
                                tabTuningCombo.currentIndex =
                                        index >= 0 && index < tabTuningCombo.count ? index : -1
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: qsTr("Detection mode")
                        color: Theme.textMuted
                        font.pixelSize: 11
                    }

                    ComboBox {
                        id: pitchDetectionModeCombo
                        Layout.preferredWidth: 140
                        model: [
                            { label: qsTr("Rhythmic"), value: 0 },
                            { label: qsTr("Melodic"), value: 1 },
                            { label: qsTr("Hybrid"), value: 2 }
                        ]
                        textRole: "label"
                        currentIndex: audioConfigController.pitchDetectionMode

                        onActivated: {
                            if (currentIndex >= 0 && currentIndex < model.length)
                                audioConfigController.pitchDetectionMode = model[currentIndex].value
                        }

                        Connections {
                            target: audioConfigController
                            function onPitchDetectionModeChanged() {
                                if (pitchDetectionModeCombo.currentIndex
                                        !== audioConfigController.pitchDetectionMode)
                                    pitchDetectionModeCombo.currentIndex =
                                            audioConfigController.pitchDetectionMode
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("Rhythmic: pick strokes · Melodic: note changes · Hybrid: timing + melody")
                        color: Theme.textHint
                        font.pixelSize: 10
                    }
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    visible: tabTuningCombo.count === 0
                    text: qsTr("No tunings in the library yet. Import songs to populate tuning names.")
                    color: Theme.textHint
                    font.pixelSize: 11
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    visible: audioConfigController.pitchAnalyzing

                    BusyIndicator {
                        running: audioConfigController.pitchAnalyzing
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("Analyzing pitch in the background…")
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }
                }
            }
        }

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Saved configurations")

            background: Rectangle {
                radius: 8
                color: Theme.panelBackground
                border.color: Theme.border
            }

            ColumnLayout {
                width: parent.width
                spacing: 8

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    visible: audioConfigController.presetNames.length > 0
                             && !audioConfigController.presetApplied
                    text: qsTr("No preset is active on open. Select a saved preset "
                               + "and tap Load to apply tempo, EQ, and markers.")
                    font.pixelSize: 12
                    color: Theme.textHint
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TextField {
                        id: presetNameField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Preset name")
                        text: audioConfigController.presetNameInput
                        onTextEdited: audioConfigController.presetNameInput = text
                    }

                    Button {
                        text: qsTr("Save")
                        enabled: true
                        onClicked: audioConfigController.savePreset()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    ComboBox {
                        id: presetCombo
                        Layout.fillWidth: true
                        model: audioConfigController.presetNames
                        enabled: count > 0
                        currentIndex: audioConfigController.selectedPresetIndex

                        onActivated: audioConfigController.selectedPresetIndex = currentIndex

                        Connections {
                            target: audioConfigController
                            function onSelectedPresetIndexChanged() {
                                if (presetCombo.currentIndex !== audioConfigController.selectedPresetIndex)
                                    presetCombo.currentIndex = audioConfigController.selectedPresetIndex
                            }
                            function onPresetNamesChanged() {
                                if (presetCombo.count === 0) {
                                    presetCombo.currentIndex = -1
                                    return
                                }
                                const index = audioConfigController.selectedPresetIndex
                                presetCombo.currentIndex =
                                        index >= 0 && index < presetCombo.count ? index : 0
                            }
                        }
                    }

                    Button {
                        text: qsTr("Load")
                        enabled: presetCombo.count > 0
                        onClicked: audioConfigController.loadSelectedPreset()
                    }

                    Button {
                        text: qsTr("Delete")
                        enabled: presetCombo.count > 0
                        onClicked: audioConfigController.deleteSelectedPreset()
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: audioConfigController.statusMessage.length > 0
                    && !audioConfigController.loading
            wrapMode: Text.WordWrap
            text: audioConfigController.statusMessage
            color: Theme.textSecondary
            font.pixelSize: 12
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Keys: Alt+p Play/Pause · Alt+a Start marker · Alt+b End marker · Alt+l Loop · Ctrl+Z Undo")
            color: Theme.textMuted
            font.pixelSize: 10
        }
    }

    Connections {
        target: audioConfigController
        function onSelectedPitchNoteIndexChanged() {
            root.syncPitchNoteEditor()
        }
        function onPitchNotesChanged() {
            if (audioConfigController.selectedPitchNoteIndex >= 0)
                root.syncPitchNoteEditor()
        }
        function onHasPitchDataChanged() {
            if (audioConfigController.hasPitchData)
                audioTimeline.resetView()
        }
        function onTransientNoticeRequested(message) {
            if (!root.visible || message.length === 0)
                return
            presetNoticePopup.show(message)
        }
    }

    TransientNoticePopup {
        id: presetNoticePopup
    }

    Dialog {
        id: pitchScopeDialog
        modal: true
        title: qsTr("Analyze pitch")
        standardButtons: Dialog.NoButton
        anchors.centerIn: parent
        width: Math.min(Math.max(root.width - 48, 280), 420)

        background: Rectangle {
            radius: 8
            color: Theme.panelBackground
            border.color: Theme.border
        }

        contentItem: ColumnLayout {
            spacing: 12
            width: parent.width

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("A loop region is set. Analyze the region only or the entire file?")
                color: Theme.textPrimary
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    Layout.fillWidth: true
                    text: qsTr("A–B region")
                    onClicked: {
                        pitchScopeDialog.close()
                        audioConfigController.startPitchDetection(true)
                    }
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("Full file")
                    onClicked: {
                        pitchScopeDialog.close()
                        audioConfigController.startPitchDetection(false)
                    }
                }

                Button {
                    text: qsTr("Cancel")
                    onClicked: pitchScopeDialog.close()
                }
            }
        }
    }

    Shortcut {
        sequence: "Alt+p"
        enabled: root.visible
        onActivated: audioConfigController.togglePlayback()
    }

    Shortcut {
        sequence: "Alt+a"
        enabled: root.visible
        onActivated: audioConfigController.setRegionFromPosition(true)
    }

    Shortcut {
        sequence: "Alt+b"
        enabled: root.visible
        onActivated: audioConfigController.setRegionFromPosition(false)
    }

    Shortcut {
        sequence: "Alt+l"
        enabled: root.visible
        onActivated: audioConfigController.toggleLoop()
    }

    Shortcut {
        sequences: ["Ctrl+Z", "Meta+Z"]
        enabled: root.visible && audioConfigController.canUndoRegion
        onActivated: audioConfigController.undoRegion()
    }

    Component.onCompleted: {
        audioConfigController.songTitle = root.songTitle
        audioConfigController.mediaFileId = root.mediaFileId
    }

    onSongTitleChanged: audioConfigController.songTitle = root.songTitle
    onMediaFileIdChanged: audioConfigController.mediaFileId = root.mediaFileId
}
