// src/ui/components/GuitarProTabPreview.qml

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    id: root

    /** Guitar-Pro-style bottom mixer strip (vertical faders). */
    property bool mixerVisible: false
    /** Linked backing-track media id (0 = none). Plays with the score once the buffer is ready. */
    property int backingMediaId: 0
    /** MIDI assigned as audio is not a backing track. The score starts without it. */
    readonly property int activeBackingMediaId:
        (root.backingMediaId > 0 && !audioConfigController.isMidiBacking(root.backingMediaId))
        ? root.backingMediaId : 0
    /** User asked to play before the backing buffer finished rendering. */
    property bool armLinkedStart: false
    /** Practice asset that owns the sync markers. 0 hides the marker controls. */
    property int practiceAssetId: 0
    property bool audioLeadIn: true
    property var syncMarkers: []
    property int selectedMarker: 0
    /** Audio is playing up to the first marker; the tab has not started yet. */
    property bool waitingForJoin: false
    property int joinAtMs: 0
    /** Count-in clicks are running. Audio and the score wait until the bar ends. */
    property bool countInActive: false
    property bool suppressSegmentSeek: false
    property bool deferTransportForCountIn: false
    property bool skipCountIn: false
    property int lastSegmentIndex: -1

    Timer {
        id: markerPreviewTimer
        interval: 700
        onTriggered: audioConfigController.pausePlayback()
    }

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

        Item {
            id: modeHeader
            Layout.fillWidth: true
            implicitHeight: Math.max(modeHeaderRow.implicitHeight,
                                     sessionTimerLabel.visible ? sessionTimerLabel.implicitHeight : 0)

            readonly property bool showSessionTimer: practiceTracker.timerRunning

            RowLayout {
                id: modeHeaderRow
                anchors.fill: parent
                spacing: 8

                Item {
                    Layout.fillWidth: true
                    implicitHeight: scoreTitleLabel.implicitHeight

                    Label {
                        id: scoreTitleLabel
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: {
                            if (!modeHeader.showSessionTimer || modeHeader.width <= 0)
                                return parent.width
                            const untilTimer = modeHeader.width / 2
                                    - sessionTimerLabel.implicitWidth / 2 - 12
                            return Math.max(0, Math.min(parent.width, untilTimer))
                        }
                        text: {
                            const parts = []
                            if (guitarProPreviewController.title.length > 0)
                                parts.push(guitarProPreviewController.title)
                            if (guitarProPreviewController.artist.length > 0)
                                parts.push(guitarProPreviewController.artist)
                            return parts.join(" — ")
                        }
                        elide: Text.ElideRight
                        color: Theme.textPrimary
                        font.bold: true
                    }
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

            Label {
                id: sessionTimerLabel
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                z: 1
                visible: modeHeader.showSessionTimer
                text: practiceTracker.elapsedDisplay
                font.pixelSize: 20
                font.weight: Font.Bold
                font.family: "monospace"
                color: Theme.accentTimer
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
                    delayed: true
                    value: {
                        const names = guitarProPreviewController.trackNames
                        const count = names ? names.length : 0
                        const index = guitarProPreviewController.selectedTrackIndex
                        if (count <= 0)
                            return -1
                        if (index < 0 || index >= count)
                            return 0
                        return index
                    }
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
                text: (root.waitingForJoin || root.countInActive || guitarProPreviewController.playing)
                      ? qsTr("Pause")
                      : (root.armLinkedStart ? qsTr("Preparing…") : qsTr("Play"))
                highlighted: true
                enabled: root.waitingForJoin
                         || root.countInActive
                         || guitarProPreviewController.playing
                         || guitarProPreviewController.soundFontReady
                onClicked: root.requestLinkedPlay()
            }

            Button {
                text: qsTr("Stop")
                enabled: root.countInActive
                         || guitarProPreviewController.playing
                         || guitarProPreviewController.soundFontReady
                onClicked: {
                    root.waitingForJoin = false
                    root.armLinkedStart = false
                    root.countInActive = false
                    root.deferTransportForCountIn = false
                    root.skipCountIn = false
                    markerPreviewTimer.stop()
                    guitarProPreviewController.setSyncSpeedRatio(1)
                    guitarProPreviewController.stop()
                    if (root.activeBackingMediaId > 0)
                        audioConfigController.stopPlayback()
                }
            }

            CheckBox {
                visible: root.activeBackingMediaId > 0
                text: qsTr("Backing")
                checked: !audioConfigController.backingMuted
                onToggled: root.setBackingAudible(checked)
            }

            Label {
                visible: !guitarProPreviewController.soundFontReady
                         && !guitarProPreviewController.playing
                text: qsTr("Loading soundfont…")
                color: Theme.textSecondary
            }

            Label {
                visible: root.activeBackingMediaId > 0 && audioConfigController.loading
                text: audioConfigController.processingStage.length > 0
                      ? audioConfigController.processingStage
                      : qsTr("Preparing audio…")
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

            CheckBox {
                id: countInOneTimeCheck
                text: qsTr("One time")
                visible: guitarProPreviewController.countInEnabled
                enabled: guitarProPreviewController.countInEnabled
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Count in once. Loops continue without another count-in.")
                onToggled: guitarProPreviewController.countInOneTime = checked

                Binding on checked {
                    value: guitarProPreviewController.countInOneTime
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

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: root.activeBackingMediaId > 0 && root.practiceAssetId > 0

            CheckBox {
                id: leadInCheck
                text: qsTr("Lead-in")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Play audio from the start. The tab begins at bar 1 so the first marker meets its audio time. Bars before the marker stay.")
                onToggled: {
                    root.audioLeadIn = checked
                    practiceAssetController.setAudioLeadIn(root.practiceAssetId, checked)
                }

                Binding on checked {
                    value: root.audioLeadIn
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }

            Button {
                text: qsTr("Mark here")
                enabled: !guitarProPreviewController.playing && !root.waitingForJoin
                         && !root.armLinkedStart
                onClicked: root.placeMarkerHere()
            }

            Label {
                color: Theme.textSecondary
                text: {
                    const marker = root.selectedSyncMarker()
                    if (!marker)
                        return qsTr("Click a bar. The red line is that bar in the audio.")
                    const shift = Math.round(marker.audioMs + root.signedScoreMs(marker.tick, 0))
                    const shown = shift > 0 ? "+" + shift : String(shift)
                    return qsTr("Bar %1 · %2 ms").arg(marker.bar).arg(shown)
                }
            }

            Button {
                text: qsTr("−10")
                enabled: root.selectedSyncMarker() !== null
                onClicked: root.nudgeSelectedMarker(-10)
            }
            Button {
                text: qsTr("+10")
                enabled: root.selectedSyncMarker() !== null
                onClicked: root.nudgeSelectedMarker(10)
            }
            Button {
                text: qsTr("−1 bar")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Move the whole tab one bar earlier against the audio")
                enabled: root.joinMarker() !== null && !guitarProPreviewController.playing
                         && !root.waitingForJoin
                onClicked: root.shiftAlignmentByBars(-1)
            }
            Button {
                text: qsTr("+1 bar")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Move the whole tab one bar later. The bells play alone for one more bar.")
                enabled: root.joinMarker() !== null && !guitarProPreviewController.playing
                         && !root.waitingForJoin
                onClicked: root.shiftAlignmentByBars(1)
            }
            Button {
                text: qsTr("Hear")
                enabled: root.selectedSyncMarker() !== null && !guitarProPreviewController.playing
                onClicked: root.previewSelectedMarker()
            }
            Button {
                text: qsTr("Remove")
                enabled: root.selectedSyncMarker() !== null && !guitarProPreviewController.playing
                onClicked: root.removeSelectedMarker()
            }
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

        CollabWaveformLane {
            id: collabLane
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            visible: root.activeBackingMediaId > 0 && root.practiceAssetId > 0
            peaks: audioConfigController.peaks
            durationMs: audioConfigController.durationMs
            positionMs: audioConfigController.positionMs
            markers: root.syncMarkers
            selectedIndex: root.selectedMarker
            alignEnabled: !guitarProPreviewController.playing && !root.waitingForJoin
                          && !root.countInActive
            shiftMs: {
                const marker = root.selectedSyncMarker()
                if (!marker)
                    return 0
                return Math.round(marker.audioMs + root.signedScoreMs(marker.tick, 0))
            }
            onMarkerSelected: (index) => root.selectedMarker = index
            onMarkerMoved: (index, audioMs) => root.moveMarkerTo(index, audioMs)
            onAudioSlid: (deltaMs) => root.shiftAudioByMs(deltaMs)
            onHearRequested: root.previewSelectedMarker()
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

            Row {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                ColumnLayout {
                    visible: root.activeBackingMediaId > 0
                    width: 56
                    height: parent.height
                    spacing: 4

                    Button {
                        text: qsTr("M")
                        checkable: true
                        checked: audioConfigController.backingMuted
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 28
                        font.bold: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Mute backing")
                        onToggled: root.setBackingAudible(!checked)

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
                        text: qsTr("%1%").arg(Math.round(backingFader.value * 100))
                        color: Theme.textSecondary
                        font.pixelSize: 10
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Slider {
                        id: backingFader
                        orientation: Qt.Vertical
                        from: 0
                        to: 1
                        stepSize: 0.01
                        Layout.fillHeight: true
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 28
                        onMoved: audioConfigController.backingVolume = value

                        Binding on value {
                            when: !backingFader.pressed
                            value: audioConfigController.backingVolume
                            restoreMode: Binding.RestoreBindingOrValue
                        }
                    }

                    Label {
                        text: qsTr("Backing")
                        color: Theme.textPrimary
                        font.pixelSize: 10
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                    }
                }

                ColumnLayout {
                    width: 56
                    height: parent.height
                    spacing: 4

                    Button {
                        text: qsTr("M")
                        checkable: true
                        checked: guitarProPreviewController.metronomeMuted
                        flat: true
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 28
                        font.bold: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Mute metronome")
                        onToggled: guitarProPreviewController.metronomeMuted = checked

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
                        text: qsTr("%1%").arg(Math.round(metronomeFader.value * 100))
                        color: Theme.textSecondary
                        font.pixelSize: 10
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Slider {
                        id: metronomeFader
                        orientation: Qt.Vertical
                        from: 0
                        to: 2
                        stepSize: 0.01
                        Layout.fillHeight: true
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 28
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Click level. Above 100% it sits over the backing track.")
                        onMoved: guitarProPreviewController.metronomeVolume = value

                        Binding on value {
                            when: !metronomeFader.pressed
                            value: guitarProPreviewController.metronomeVolume
                            restoreMode: Binding.RestoreBindingOrValue
                        }
                    }

                    Label {
                        text: qsTr("Click")
                        color: Theme.textPrimary
                        font.pixelSize: 10
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                    }
                }

            ListView {
                id: mixerList
                width: parent.width - ((root.activeBackingMediaId > 0 ? 62 : 0) + 62)
                height: parent.height
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

    function prepareLinkedBacking() {
        if (root.activeBackingMediaId <= 0)
            return
        audioConfigController.tempoPercent = guitarProPreviewController.tempoPercent
        audioConfigController.prepareBacking(root.activeBackingMediaId)
    }

    function linkedBackingActive() {
        return root.activeBackingMediaId > 0 && !audioConfigController.backingMuted
    }

    function backingBufferReady() {
        return root.activeBackingMediaId > 0
                && audioConfigController.mediaFileId === root.activeBackingMediaId
                && audioConfigController.playbackReady
                && !audioConfigController.loading
    }

    function resumeLinkedLoop(bar, tick) {
        root.waitingForJoin = false
        markerPreviewTimer.stop()
        // The previous pass may still be on the last segment's tempo. Put the
        // score back on the loop-start tempo before either engine starts.
        root.suppressSegmentSeek = true
        root.lastSegmentIndex = -1
        if ((root.syncMarkers || []).length >= 2 && tick >= 0)
            root.applySegmentSpeed(tick)
        else
            guitarProPreviewController.setSyncSpeedRatio(1)
        root.suppressSegmentSeek = false
        if (root.linkedBackingActive()) {
            const ms = tick >= 0 ? root.audioMsForTick(tick) : root.audioMsForBar(bar)
            audioConfigController.playFromMs(Math.max(0, ms))
        }
        guitarProPreviewController.play()
    }

    function startLinkedTransport() {
        root.skipCountIn = false
        root.armLinkedStart = false
        root.lastSegmentIndex = -1
        markerPreviewTimer.stop()
        guitarProPreviewController.setSyncSpeedRatio(1)
        const marker = root.joinMarker()
        if (!marker) {
            root.waitingForJoin = false
            audioConfigController.startBacking(root.activeBackingMediaId)
            guitarProPreviewController.play()
            return
        }
        if (root.audioLeadIn) {
            const startMs = root.scoreStartAudioMs(marker)
            root.joinAtMs = startMs
            guitarProPreviewController.seekToBar(1)
            if (startMs > 40) {
                root.waitingForJoin = true
                audioConfigController.playFromMs(0)
                return
            }
            root.waitingForJoin = false
            audioConfigController.playFromMs(0)
            guitarProPreviewController.play()
            root.applySegmentSpeed(0)
            return
        }
        root.joinAtMs = marker.audioMs
        root.waitingForJoin = false
        audioConfigController.playFromMs(Math.max(0, marker.audioMs))
        guitarProPreviewController.playFromBar(marker.bar)
        root.applySegmentSpeed(marker.tick)
    }

    function beginLinkedStart() {
        root.armLinkedStart = false
        if (guitarProPreviewController.countInEnabled && !root.skipCountIn) {
            root.countInActive = true
            root.deferTransportForCountIn = true
            root.lastSegmentIndex = -1
            markerPreviewTimer.stop()
            guitarProPreviewController.setSyncSpeedRatio(1)
            const marker = root.joinMarker()
            if (marker)
                guitarProPreviewController.seekToBar(root.audioLeadIn ? 1 : marker.bar)
            guitarProPreviewController.beginCountIn()
            return
        }
        root.skipCountIn = false
        root.startLinkedTransport()
    }

    function setBackingAudible(audible) {
        audioConfigController.backingMuted = !audible
        if (!audible || root.activeBackingMediaId <= 0)
            return
        if (!guitarProPreviewController.playing && !root.armLinkedStart && !root.waitingForJoin)
            return
        root.waitingForJoin = false
        if (guitarProPreviewController.playing)
            guitarProPreviewController.stop()
        audioConfigController.stopPlayback()
        root.armLinkedStart = true
        if (root.backingBufferReady())
            root.beginLinkedStart()
        else
            root.prepareLinkedBacking()
    }

    function requestLinkedPlay() {
        if (root.countInActive) {
            root.countInActive = false
            root.deferTransportForCountIn = false
            root.skipCountIn = false
            guitarProPreviewController.pause()
            if (root.activeBackingMediaId > 0)
                audioConfigController.pausePlayback()
            return
        }
        if (root.waitingForJoin) {
            root.waitingForJoin = false
            audioConfigController.pausePlayback()
            return
        }
        if (root.armLinkedStart && !guitarProPreviewController.playing) {
            root.armLinkedStart = false
            return
        }
        if (guitarProPreviewController.playing) {
            root.armLinkedStart = false
            root.waitingForJoin = false
            guitarProPreviewController.pause()
            if (root.activeBackingMediaId > 0)
                audioConfigController.pausePlayback()
            return
        }
        if (!root.linkedBackingActive()) {
            if (guitarProPreviewController.countInEnabled) {
                root.countInActive = true
                root.deferTransportForCountIn = false
                guitarProPreviewController.beginCountIn()
                return
            }
            guitarProPreviewController.play()
            return
        }
        if (root.backingBufferReady()) {
            root.beginLinkedStart()
            return
        }
        root.armLinkedStart = true
        root.prepareLinkedBacking()
    }

    function sortedMarkers() {
        const list = root.syncMarkers || []
        return list.slice().sort(function (a, b) {
            return (a.tick - b.tick) || (a.bar - b.bar) || (a.beat - b.beat)
        })
    }

    function joinMarker() {
        const list = root.sortedMarkers()
        return list.length > 0 ? list[0] : null
    }

    function selectedSyncMarker() {
        if (root.selectedMarker < 0 || root.selectedMarker >= root.syncMarkers.length)
            return null
        return root.syncMarkers[root.selectedMarker]
    }

    function signedScoreMs(tickA, tickB) {
        const bpm = Math.max(1, guitarProPreviewController.scoreBpm
                              || guitarProPreviewController.playbackBpm || 120)
        const ticks = (tickB || 0) - (tickA || 0)
        return (ticks / 960) * (60000 / bpm)
    }

    function scoreMsBetween(tickA, tickB) {
        return Math.max(0, root.signedScoreMs(tickA, tickB))
    }

    function scoreStartAudioMs(marker) {
        if (!marker)
            return 0
        return marker.audioMs + root.signedScoreMs(marker.tick, 0)
    }

    function audioMsForTick(tick) {
        const pts = root.sortedMarkers()
        if (pts.length === 0)
            return 0
        const t = Math.max(0, tick || 0)
        if (t <= pts[0].tick)
            return Math.max(0, pts[0].audioMs + root.signedScoreMs(pts[0].tick, t))
        if (pts.length === 1)
            return Math.max(0, pts[0].audioMs + root.scoreMsBetween(pts[0].tick, t))
        let i = 0
        while (i + 1 < pts.length && t >= pts[i + 1].tick)
            i++
        if (i >= pts.length - 1) {
            const a = pts[pts.length - 2]
            const b = pts[pts.length - 1]
            const score = root.scoreMsBetween(a.tick, b.tick)
            if (score <= 0)
                return b.audioMs
            return Math.max(0, b.audioMs + root.scoreMsBetween(b.tick, t) * ((b.audioMs - a.audioMs) / score))
        }
        const a = pts[i]
        const b = pts[i + 1]
        const score = root.scoreMsBetween(a.tick, b.tick)
        if (score <= 0)
            return a.audioMs
        return Math.max(0, a.audioMs + (b.audioMs - a.audioMs) * (root.scoreMsBetween(a.tick, t) / score))
    }

    function audioMsForBar(bar) {
        const pts = root.sortedMarkers()
        if (pts.length === 0)
            return 0
        for (let i = 0; i < pts.length; ++i) {
            if (pts[i].bar === bar && pts[i].beat === 1)
                return pts[i].audioMs
        }
        for (let i = 0; i < pts.length; ++i) {
            if (pts[i].bar === bar)
                return pts[i].audioMs
        }
        let lower = null
        let upper = null
        for (let i = 0; i < pts.length; ++i) {
            if (pts[i].bar <= bar)
                lower = pts[i]
            if (pts[i].bar >= bar && upper === null)
                upper = pts[i]
        }
        if (!lower)
            return upper.audioMs
        if (!upper || upper === lower)
            return lower.audioMs + root.scoreMsBetween(lower.tick, lower.tick)
        const span = Math.max(1, upper.bar - lower.bar)
        return Math.round(lower.audioMs + (upper.audioMs - lower.audioMs) * ((bar - lower.bar) / span))
    }

    function applySegmentSpeed(tick) {
        const pts = root.sortedMarkers()
        if (pts.length < 2) {
            if (root.lastSegmentIndex !== -1)
                guitarProPreviewController.setSyncSpeedRatio(1)
            root.lastSegmentIndex = -1
            return
        }
        let i = 0
        while (i + 1 < pts.length && tick >= pts[i + 1].tick)
            i++
        if (i >= pts.length - 1)
            i = pts.length - 2
        if (i === root.lastSegmentIndex)
            return
        const entered = i
        root.lastSegmentIndex = i
        const a = pts[i]
        const b = pts[i + 1]
        const score = root.scoreMsBetween(a.tick, b.tick)
        const audioSpan = b.audioMs - a.audioMs
        const ratio = score > 0 && audioSpan > 0 ? score / audioSpan : 1
        guitarProPreviewController.setSyncSpeedRatio(Math.max(0.5, Math.min(2, ratio)))
        if (!root.suppressSegmentSeek && entered > 0 && guitarProPreviewController.playing)
            audioConfigController.seekPlayback(pts[entered].audioMs)
    }

    function reloadSyncMarkers() {
        if (root.practiceAssetId <= 0) {
            root.syncMarkers = []
            root.audioLeadIn = true
            root.selectedMarker = 0
            return
        }
        root.syncMarkers = practiceAssetController.syncMarkers(root.practiceAssetId)
        root.audioLeadIn = practiceAssetController.audioLeadIn(root.practiceAssetId)
        if (root.selectedMarker >= root.syncMarkers.length)
            root.selectedMarker = 0
    }

    function placeMarkerHere() {
        if (root.practiceAssetId <= 0 || guitarProPreviewController.playing || root.waitingForJoin)
            return
        const bar = Math.max(1, guitarProPreviewController.playheadBar)
        const beat = Math.max(1, guitarProPreviewController.playheadBeat)
        const tick = Math.max(0, guitarProPreviewController.playheadTick)
        const ms = Math.max(0, audioConfigController.positionMs)
        practiceAssetController.saveSyncMarker(root.practiceAssetId, bar, beat, tick, ms)
        root.reloadSyncMarkers()
        for (let i = 0; i < root.syncMarkers.length; ++i) {
            if (root.syncMarkers[i].bar === bar && root.syncMarkers[i].beat === beat) {
                root.selectedMarker = i
                break
            }
        }
    }

    function moveMarkerTo(index, audioMs) {
        if (index < 0 || index >= root.syncMarkers.length || root.practiceAssetId <= 0)
            return
        const marker = root.syncMarkers[index]
        const ms = Math.max(0, Math.round(audioMs))
        root.selectedMarker = index
        practiceAssetController.saveSyncMarker(root.practiceAssetId, marker.bar, marker.beat,
                                                marker.tick, ms)
        root.reloadSyncMarkers()
        for (let i = 0; i < root.syncMarkers.length; ++i) {
            if (root.syncMarkers[i].bar === marker.bar && root.syncMarkers[i].beat === marker.beat) {
                root.selectedMarker = i
                break
            }
        }
    }

    function anchorAtBar(bar, beat, tick) {
        if (root.practiceAssetId <= 0 || bar < 1)
            return
        if (guitarProPreviewController.playing || root.waitingForJoin || root.countInActive)
            return
        const list = root.syncMarkers || []
        for (let i = 0; i < list.length; ++i) {
            if (list[i].bar === bar && list[i].beat === beat) {
                root.selectedMarker = i
                collabLane.showAnchor(list[i].audioMs)
                return
            }
        }
        const current = root.selectedSyncMarker() || root.joinMarker()
        const shift = current ? Math.round(current.audioMs + root.signedScoreMs(current.tick, 0)) : 0
        const audioMs = Math.max(0, Math.round(root.scoreMsBetween(0, tick) + shift))
        if (list.length === 1)
            practiceAssetController.deleteSyncMarker(root.practiceAssetId, list[0].bar, list[0].beat)
        practiceAssetController.saveSyncMarker(root.practiceAssetId, bar, beat, tick, audioMs)
        root.reloadSyncMarkers()
        for (let i = 0; i < root.syncMarkers.length; ++i) {
            if (root.syncMarkers[i].bar === bar && root.syncMarkers[i].beat === beat) {
                root.selectedMarker = i
                break
            }
        }
        collabLane.showAnchor(audioMs)
    }

    function shiftAudioByMs(delta) {
        if (!delta || root.practiceAssetId <= 0)
            return
        const list = (root.syncMarkers || []).slice()
        if (list.length === 0)
            return
        for (let i = 0; i < list.length; ++i) {
            const marker = list[i]
            practiceAssetController.saveSyncMarker(root.practiceAssetId, marker.bar, marker.beat,
                                                    marker.tick,
                                                    Math.max(0, marker.audioMs + delta))
        }
        root.reloadSyncMarkers()
    }

    function shiftAlignmentByBars(direction) {
        const marker = root.joinMarker()
        if (!marker || root.practiceAssetId <= 0 || direction === 0)
            return
        guitarProPreviewController.reportBarSourceMs(marker.bar, direction)
    }

    function applyBarShift(sourceMs, direction) {
        if (sourceMs <= 0 || direction === 0 || root.practiceAssetId <= 0)
            return
        const delta = direction * sourceMs
        const list = (root.syncMarkers || []).slice()
        for (let i = 0; i < list.length; ++i) {
            const marker = list[i]
            practiceAssetController.saveSyncMarker(root.practiceAssetId, marker.bar, marker.beat,
                                                    marker.tick, Math.max(0, marker.audioMs + delta))
        }
        root.reloadSyncMarkers()
        root.previewSelectedMarker()
    }

    function nudgeSelectedMarker(delta) {
        const marker = root.selectedSyncMarker()
        if (!marker || root.practiceAssetId <= 0)
            return
        const ms = Math.max(0, marker.audioMs + delta)
        practiceAssetController.saveSyncMarker(root.practiceAssetId, marker.bar, marker.beat,
                                                marker.tick, ms)
        root.reloadSyncMarkers()
    }

    function removeSelectedMarker() {
        const marker = root.selectedSyncMarker()
        if (!marker || root.practiceAssetId <= 0)
            return
        practiceAssetController.deleteSyncMarker(root.practiceAssetId, marker.bar, marker.beat)
        root.reloadSyncMarkers()
    }

    function previewSelectedMarker() {
        const marker = root.selectedSyncMarker()
        if (!marker || guitarProPreviewController.playing || root.waitingForJoin)
            return
        const start = Math.max(0, marker.audioMs - 250)
        audioConfigController.playFromMs(start)
        const tempo = Math.max(25, audioConfigController.tempoPercent)
        markerPreviewTimer.interval = Math.max(200, Math.round(500 * 100 / tempo))
        markerPreviewTimer.restart()
    }

    function onLinkedAudioPosition() {
        if (!root.waitingForJoin)
            return
        const marker = root.joinMarker()
        if (!marker)
            return
        if (audioConfigController.positionMs + 30 >= root.joinAtMs) {
            root.waitingForJoin = false
            guitarProPreviewController.seekToBar(1)
            guitarProPreviewController.play()
            root.applySegmentSpeed(0)
        }
    }

    onBackingMediaIdChanged: root.prepareLinkedBacking()
    onPracticeAssetIdChanged: root.reloadSyncMarkers()
    Component.onCompleted: {
        root.prepareLinkedBacking()
        root.reloadSyncMarkers()
    }
    Component.onDestruction: guitarProPreviewController.setSyncSpeedRatio(1)

    Connections {
        target: guitarProPreviewController
        function onPlayingChanged() {
            if (root.waitingForJoin || root.countInActive || root.activeBackingMediaId <= 0
                    || guitarProPreviewController.playing)
                return
            audioConfigController.pausePlayback()
        }
        function onTempoPercentChanged() {
            if (root.activeBackingMediaId <= 0)
                return
            const waiting = root.armLinkedStart || root.waitingForJoin
                    || guitarProPreviewController.playing || root.countInActive
            root.waitingForJoin = false
            root.countInActive = false
            root.deferTransportForCountIn = false
            root.skipCountIn = false
            if (guitarProPreviewController.playing || waiting)
                guitarProPreviewController.stop()
            audioConfigController.stopPlayback()
            guitarProPreviewController.setSyncSpeedRatio(1)
            root.lastSegmentIndex = -1
            root.armLinkedStart = waiting
            root.prepareLinkedBacking()
        }
        function onBarSourceMsReady(sourceMs, direction) {
            root.applyBarShift(sourceMs, direction)
        }
        function onBarAnchorClicked(bar, beat, tick) {
            root.anchorAtBar(bar, beat, tick)
        }
        function onCountInStarted() {
            root.countInActive = true
            if (root.activeBackingMediaId > 0)
                audioConfigController.pausePlayback()
        }
        function onCountInCancelled() {
            root.countInActive = false
            root.deferTransportForCountIn = false
        }
        function onCountInFinished(loopRestart, bar, tick) {
            const defer = root.deferTransportForCountIn
            const active = root.countInActive
            root.countInActive = false
            root.deferTransportForCountIn = false
            if (loopRestart) {
                root.resumeLinkedLoop(bar, tick)
                return
            }
            if (!active)
                return
            if (defer) {
                if (root.backingBufferReady())
                    root.startLinkedTransport()
                else {
                    root.skipCountIn = true
                    root.armLinkedStart = true
                    root.prepareLinkedBacking()
                }
                return
            }
            guitarProPreviewController.play()
        }
        function onPlayheadChanged() {
            if (!guitarProPreviewController.playing || root.waitingForJoin || root.countInActive
                    || root.activeBackingMediaId <= 0)
                return
            root.applySegmentSpeed(guitarProPreviewController.playheadTick)
        }
        function onLoopWrapped(bar, tick, seamless) {
            if (seamless) {
                if (root.linkedBackingActive()) {
                    const ms = tick >= 0 ? root.audioMsForTick(tick) : root.audioMsForBar(bar)
                    audioConfigController.playFromMs(Math.max(0, ms))
                }
                return
            }
            root.resumeLinkedLoop(bar, tick)
        }
    }

    Connections {
        target: audioConfigController
        function onPlaybackReadyChanged() {
            if (root.armLinkedStart && root.backingBufferReady())
                root.beginLinkedStart()
        }
        function onPositionMsChanged() {
            root.onLinkedAudioPosition()
        }
    }
}
