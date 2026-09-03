// src/ui/components/GuitarProTabPreview.qml

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    id: root

    /** Guitar-Pro-style bottom mixer strip (vertical faders). */
    property bool mixerVisible: false

    background: Rectangle {
        radius: 10
        color: Theme.panelBackground
        border.color: Theme.border
    }

    readonly property bool showPlayer: guitarProPreviewController.usePlayer
    readonly property bool mixerAvailable: root.showPlayer
            && guitarProPreviewController.loaded
            && guitarProPreviewController.mixerTracks.length > 0
    readonly property url mixerIconSource:
            "qrc:/qt/qml/com/sonarp/sonarpractice/assets/svg/mixer.svg"

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: {
                    const parts = []
                    if (guitarProPreviewController.title.length > 0)
                        parts.push(guitarProPreviewController.title)
                    if (guitarProPreviewController.artist.length > 0)
                        parts.push(guitarProPreviewController.artist)
                    return parts.join(" — ")
                }
                elide: Text.ElideRight
                Layout.fillWidth: true
                color: Theme.textPrimary
                font.bold: true
            }

            Button {
                text: qsTr("ASCII")
                flat: true
                visible: guitarProPreviewController.playerAvailable
                checkable: true
                checked: !guitarProPreviewController.usePlayer
                onClicked: guitarProPreviewController.usePlayer = false
            }

            Button {
                text: qsTr("Player")
                flat: true
                visible: guitarProPreviewController.playerAvailable
                checkable: true
                checked: guitarProPreviewController.usePlayer
                onClicked: guitarProPreviewController.usePlayer = true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: guitarProPreviewController.loaded && !guitarProPreviewController.loading

            Label {
                text: qsTr("Track")
                color: Theme.textSecondary
            }

            ComboBox {
                id: trackCombo
                Layout.preferredWidth: 240
                model: guitarProPreviewController.trackNames
                onActivated: guitarProPreviewController.selectedTrackIndex = currentIndex

                Binding on currentIndex {
                    value: Math.max(0, guitarProPreviewController.selectedTrackIndex)
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }

            Label {
                text: guitarProPreviewController.tuning.length > 0
                      ? qsTr("Tuning: %1").arg(guitarProPreviewController.tuning)
                      : ""
                color: Theme.textSecondary
                elide: Text.ElideRight
                Layout.fillWidth: true
                visible: !root.showPlayer
            }
        }

        // Playback tools (transpose affects audio only — tab stays unchanged).
        Rectangle {
            Layout.fillWidth: true
            visible: root.showPlayer && guitarProPreviewController.loaded
            radius: 8
            color: Theme.panelBackgroundNested
            border.color: Theme.border
            implicitHeight: playerToolsLayout.implicitHeight + 16

            RowLayout {
                id: playerToolsLayout
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                Label {
                    text: qsTr("Transpose")
                    color: Theme.textSecondary
                    font.weight: Font.DemiBold
                }

                Button {
                    text: qsTr("−1")
                    flat: true
                    enabled: guitarProPreviewController.transposeSemitones > -12
                    onClicked: guitarProPreviewController.transposeDown()
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("One semitone lower")
                }

                DarkSpinBox {
                    id: transposeSpin
                    from: -12
                    to: 12
                    editable: true
                    onValueModified: guitarProPreviewController.transposeSemitones = value

                    Binding on value {
                        when: !transposeSpin.activeFocus
                        value: guitarProPreviewController.transposeSemitones
                        restoreMode: Binding.RestoreBindingOrValue
                    }
                }

                Label {
                    text: qsTr("st")
                    color: Theme.textSecondary
                    font.pixelSize: 11
                }

                Button {
                    text: qsTr("+1")
                    flat: true
                    enabled: guitarProPreviewController.transposeSemitones < 12
                    onClicked: guitarProPreviewController.transposeUp()
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("One semitone higher")
                }

                Button {
                    text: qsTr("Reset")
                    flat: true
                    visible: guitarProPreviewController.transposeSemitones !== 0
                    onClicked: guitarProPreviewController.resetTranspose()
                }

                Label {
                    text: guitarProPreviewController.tuning.length > 0
                          ? qsTr("Notated tuning: %1").arg(guitarProPreviewController.tuning)
                          : ""
                    color: Theme.textSecondary
                    font.pixelSize: 11
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    visible: text.length > 0
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: root.showPlayer && guitarProPreviewController.loaded

            Button {
                text: guitarProPreviewController.playing ? qsTr("Pause") : qsTr("Play")
                highlighted: true
                enabled: guitarProPreviewController.playing
                         || guitarProPreviewController.soundFontReady
                onClicked: guitarProPreviewController.playPause()
            }

            Button {
                text: qsTr("Stop")
                enabled: guitarProPreviewController.playing
                         || guitarProPreviewController.soundFontReady
                onClicked: guitarProPreviewController.stop()
            }

            Label {
                visible: !guitarProPreviewController.soundFontReady
                         && !guitarProPreviewController.playing
                text: qsTr("Loading soundfont…")
                color: Theme.textSecondary
            }

            Label {
                text: qsTr("Tempo")
                color: Theme.textSecondary
            }

            DarkSpinBox {
                id: tempoBpmSpin
                from: guitarProPreviewController.minPlaybackBpm
                to: guitarProPreviewController.maxPlaybackBpm
                stepSize: 2
                editable: true
                Layout.preferredWidth: 110
                onValueModified: guitarProPreviewController.playbackBpm = value

                Binding on value {
                    value: guitarProPreviewController.playbackBpm
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }

            Label {
                text: qsTr("BPM")
                color: Theme.textSecondary
            }

            Label {
                text: {
                    const score = guitarProPreviewController.scoreBpm
                    if (score > 0)
                        return qsTr("(%1% of %2)").arg(guitarProPreviewController.tempoPercent).arg(score)
                    return qsTr("(%1%)").arg(guitarProPreviewController.tempoPercent)
                }
                color: Theme.textHint
            }

            CheckBox {
                id: loopCheck
                text: qsTr("Loop")
                onToggled: guitarProPreviewController.loopEnabled = checked

                Binding on checked {
                    value: guitarProPreviewController.loopEnabled
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }

            Label {
                text: qsTr("Bars")
                color: Theme.textSecondary
                visible: guitarProPreviewController.loopEnabled
            }

            SpinBox {
                id: loopStartSpin
                from: 1
                // Keep range wide enough before barCount arrives so values are not clamped to 1.
                to: Math.max(1, guitarProPreviewController.barCount,
                             guitarProPreviewController.loopEndBar,
                             guitarProPreviewController.loopStartBar)
                visible: guitarProPreviewController.loopEnabled
                editable: true
                onValueModified: guitarProPreviewController.loopStartBar = value

                Binding on value {
                    when: !loopStartSpin.activeFocus
                    value: guitarProPreviewController.loopStartBar
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }

            Label {
                text: "–"
                visible: guitarProPreviewController.loopEnabled
                color: Theme.textSecondary
            }

            SpinBox {
                id: loopEndSpin
                from: 1
                to: Math.max(1, guitarProPreviewController.barCount,
                             guitarProPreviewController.loopEndBar,
                             guitarProPreviewController.loopStartBar)
                visible: guitarProPreviewController.loopEnabled
                editable: true
                onValueModified: guitarProPreviewController.loopEndBar = value

                Binding on value {
                    when: !loopEndSpin.activeFocus
                    value: guitarProPreviewController.loopEndBar
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }

            CheckBox {
                id: metronomeCheck
                text: qsTr("Metronome")
                onToggled: guitarProPreviewController.metronomeEnabled = checked

                Binding on checked {
                    value: guitarProPreviewController.metronomeEnabled
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }

            CheckBox {
                id: countInCheck
                text: qsTr("Count-in")
                onToggled: guitarProPreviewController.countInEnabled = checked

                Binding on checked {
                    value: guitarProPreviewController.countInEnabled
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }

            ComboBox {
                id: metronomeDivisionCombo
                Layout.preferredWidth: 88
                visible: guitarProPreviewController.metronomeEnabled
                         || guitarProPreviewController.countInEnabled
                model: [
                    { label: qsTr("1/4"), value: 4 },
                    { label: qsTr("1/8"), value: 8 },
                    { label: qsTr("1/16"), value: 16 },
                    { label: qsTr("1/32"), value: 32 }
                ]
                textRole: "label"
                valueRole: "value"
                onActivated: guitarProPreviewController.metronomeDivision = currentValue

                Binding on currentIndex {
                    value: {
                        const d = guitarProPreviewController.metronomeDivision
                        if (d === 8) return 1
                        if (d === 16) return 2
                        if (d === 32) return 3
                        return 0
                    }
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }

            Item { Layout.fillWidth: true }
        }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: guitarProPreviewController.loading
            visible: running
        }

        Label {
            Layout.fillWidth: true
            visible: guitarProPreviewController.errorMessage.length > 0
            wrapMode: Text.WordWrap
            text: guitarProPreviewController.errorMessage
            color: Theme.error
        }

        Label {
            Layout.fillWidth: true
            visible: !guitarProPreviewController.playerAvailable
            wrapMode: Text.WordWrap
            text: qsTr("Interactive player requires Qt WebEngine. Showing ASCII tablature.")
            color: Theme.textSecondary
            font.pixelSize: 12
        }

        Loader {
            id: playerLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 220
            active: root.showPlayer && guitarProPreviewController.playerAvailable
            source: active ? "GuitarProPlayerView.qml" : ""
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: !root.showPlayer
            Layout.minimumHeight: 120
            clip: true
            visible: !root.showPlayer
                     && guitarProPreviewController.tabText.length > 0
                     && !guitarProPreviewController.loading

            ScrollBar.horizontal.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            TextArea {
                readOnly: true
                wrapMode: TextEdit.NoWrap
                text: guitarProPreviewController.tabText
                color: Theme.textPrimary
                selectedTextColor: Theme.textOnAccent
                selectionColor: Theme.accent
                background: Rectangle {
                    color: Theme.editorBackground
                    border.color: Theme.border
                    radius: 4
                }
                font.family: "monospace"
                font.pixelSize: 13
            }
        }

        // Guitar-Pro-style docked mixer: horizontal strip of vertical faders.
        Rectangle {
            id: mixerDock
            Layout.fillWidth: true
            Layout.preferredHeight: root.mixerVisible && root.mixerAvailable ? 200 : 0
            visible: Layout.preferredHeight > 0
            radius: 8
            color: Theme.panelBackgroundNested
            border.color: Theme.border
            clip: true

            Behavior on Layout.preferredHeight {
                NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
            }

            ListView {
                id: mixerList
                anchors.fill: parent
                anchors.margins: 8
                orientation: ListView.Horizontal
                clip: true
                spacing: 6
                model: guitarProPreviewController.mixerTracks

                ScrollBar.horizontal: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                delegate: Item {
                    id: faderStrip
                    required property var modelData
                    required property int index

                    width: 56
                    height: mixerList.height

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Button {
                            text: qsTr("M")
                            checkable: true
                            checked: !!faderStrip.modelData.muted
                            flat: true
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 32
                            Layout.preferredHeight: 28
                            font.bold: true
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Mute")
                            onToggled: guitarProPreviewController.setTrackMuted(faderStrip.index, checked)

                            background: Rectangle {
                                radius: 4
                                color: parent.checked ? Theme.danger : (parent.hovered
                                        ? Theme.toolbarButtonHover
                                        : "transparent")
                                border.color: parent.checked ? Theme.danger : Theme.border
                            }
                            contentItem: Label {
                                text: parent.text
                                color: parent.checked ? Theme.textOnAccent : Theme.textPrimary
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font: parent.font
                            }
                        }

                        Label {
                            text: qsTr("%1%").arg(Math.round(volumeFader.value * 100))
                            color: Theme.textSecondary
                            font.pixelSize: 10
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 40
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Slider {
                            id: volumeFader
                            orientation: Qt.Vertical
                            from: 0
                            to: 1
                            stepSize: 0.01
                            Layout.fillHeight: true
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 28
                            onMoved: guitarProPreviewController.setTrackVolume(faderStrip.index, value)

                            Binding on value {
                                when: !volumeFader.pressed
                                value: Number(faderStrip.modelData.volume)
                                restoreMode: Binding.RestoreBindingOrValue
                            }
                        }

                        Label {
                            text: faderStrip.modelData.name
                                  || qsTr("Track %1").arg(faderStrip.index + 1)
                            elide: Text.ElideRight
                            color: Theme.textPrimary
                            font.pixelSize: 10
                            Layout.fillWidth: true
                            Layout.preferredHeight: 28
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                        }
                    }
                }
            }
        }

        // Bottom bar: mixer show/hide (Guitar Pro-style).
        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            visible: root.mixerAvailable

            ToolButton {
                id: mixerToggle
                checkable: true
                checked: root.mixerVisible
                icon.source: root.mixerIconSource
                icon.width: 18
                icon.height: 18
                icon.color: checked ? Theme.accent : Theme.textPrimary
                ToolTip.visible: hovered
                ToolTip.text: checked ? qsTr("Hide mixer") : qsTr("Show mixer")
                onToggled: root.mixerVisible = checked

                Binding on checked {
                    value: root.mixerVisible
                    restoreMode: Binding.RestoreBindingOrValue
                }

                background: Rectangle {
                    radius: 6
                    color: mixerToggle.checked
                           ? Theme.accentFill
                           : (mixerToggle.hovered ? Theme.toolbarButtonHover : "transparent")
                    border.color: mixerToggle.checked ? Theme.borderAccent : Theme.border
                }
            }

            Item { Layout.fillWidth: true }
        }
    }

    onMixerAvailableChanged: {
        if (!root.mixerAvailable)
            root.mixerVisible = false
    }
}
