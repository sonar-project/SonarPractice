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

    StackView.onActivated: root.forceActiveFocus()

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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        AudioWaveformView {
            id: waveform
            Layout.fillWidth: true
            Layout.preferredHeight: 140
            peaks: audioConfigController.peaks
            opacity: audioConfigController.loading ? 0.45 : 1.0

            Behavior on opacity {
                NumberAnimation { duration: 180 }
            }

            Connections {
                target: audioConfigController
                function onPeaksChanged() {
                    waveform.peaks = audioConfigController.peaks
                }
            }
            regionStartRatio: root.ratioForMs(audioConfigController.regionStartMs)
            regionEndRatio: root.ratioForMs(audioConfigController.regionEndMs)
            playheadRatio: root.ratioForMs(audioConfigController.positionMs)

            onClickedRatio: (ratio) => {
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
        function onTransientNoticeRequested(message) {
            if (!root.visible || message.length === 0)
                return
            presetNoticePopup.show(message)
        }
    }

    TransientNoticePopup {
        id: presetNoticePopup
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
