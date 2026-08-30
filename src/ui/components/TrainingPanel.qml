import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../components"

GroupBox {
    id: root

    property int practiceAssetId: 0
    property bool practiceMaterialReady: false
    /** Linked Guitar Pro media on the active practice asset (0 if none). */
    property int guitarProMediaId: 0
    property int songBaseBpm: 60

    /** Emitted when the internal player page should be shown (after timer start or reopen). */
    signal internalPlayerRequested()

    title: qsTr("Training")
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

    property bool openPlayerWhenReady: false

    /** Upper bar limit from the linked GP file; 999 when unknown / no GP. */
    readonly property int barLimit: {
        if (root.guitarProMediaId > 0 && guitarProPreviewController.barCount > 0)
            return guitarProPreviewController.barCount
        return 999
    }

    function clampBarsToLimit() {
        const limit = root.barLimit
        if (practiceTracker.startBar > limit)
            practiceTracker.startBar = limit
        if (practiceTracker.endBar > limit)
            practiceTracker.endBar = limit
        if (practiceTracker.endBar < practiceTracker.startBar)
            practiceTracker.endBar = practiceTracker.startBar
    }

    onBarLimitChanged: root.clampBarsToLimit()

    function tempoPercentForTargetBpm() {
        const base = root.songBaseBpm > 0 ? root.songBaseBpm : 100
        return Math.max(25, Math.min(200, Math.round(practiceTracker.targetBpm * 100 / base)))
    }

    function ensureScoreLoaded() {
        if (root.guitarProMediaId <= 0)
            return false
        if (guitarProPreviewController.mediaFileId === root.guitarProMediaId
                && (guitarProPreviewController.loaded || guitarProPreviewController.loading))
            return true
        guitarProPreviewController.usePlayer = true
        guitarProPreviewController.load(root.guitarProMediaId)
        return true
    }

    function applyInternalPlayerSession() {
        if (root.guitarProMediaId <= 0)
            return false

        const gp = guitarProPreviewController
        gp.usePlayer = true
        if (gp.loaded && trackCombo.count > 0)
            gp.selectedTrackIndex = Math.max(0, trackCombo.currentIndex)

        // Bars first, then enable loop so one JS apply covers the full range.
        gp.loopStartBar = practiceTracker.startBar
        gp.loopEndBar = Math.max(practiceTracker.startBar, practiceTracker.endBar)
        gp.loopEnabled = true
        gp.tempoPercent = root.tempoPercentForTargetBpm()
        return true
    }

    function requestInternalPlayer() {
        if (!root.ensureScoreLoaded())
            return

        if (guitarProPreviewController.loaded) {
            root.applyInternalPlayerSession()
            root.internalPlayerRequested()
            return
        }

        // Wait until parse finishes so track/loop are applied after defaults.
        root.openPlayerWhenReady = true
    }

    function syncInternalPlayerLoad() {
        if (root.guitarProMediaId <= 0)
            return
        root.ensureScoreLoaded()
    }

    onGuitarProMediaIdChanged: {
        if (root.guitarProMediaId <= 0) {
            useExternalCheck.checked = false
            root.openPlayerWhenReady = false
            return
        }
        // Preload so Track combo has names for the default internal-player flow.
        root.ensureScoreLoaded()
    }

    Connections {
        target: guitarProPreviewController
        function onLoadedChanged() {
            if (!root.openPlayerWhenReady || !guitarProPreviewController.loaded)
                return
            root.openPlayerWhenReady = false
            root.applyInternalPlayerSession()
            root.internalPlayerRequested()
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: 12

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: practiceTracker.elapsedDisplay
            font.pixelSize: 36
            font.weight: Font.Bold
            font.family: "monospace"
            color: Theme.accentTimer
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label { text: qsTr("Bar from"); color: Theme.textTertiary }
            DarkSpinBox {
                id: startBarSpin
                from: 1
                to: root.barLimit
                implicitWidth: 70
                value: practiceTracker.startBar
                onValueModified: {
                    const v = Math.min(value, root.barLimit)
                    practiceTracker.startBar = v
                    // endBar must never be below startBar (also enforced in the controller).
                    if (practiceTracker.endBar < v)
                        practiceTracker.endBar = v
                }

                Binding on value {
                    when: !startBarSpin.activeFocus
                    value: practiceTracker.startBar
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }

            Label { text: qsTr("Bar to"); color: Theme.textTertiary }
            DarkSpinBox {
                id: endBarSpin
                from: practiceTracker.startBar
                to: root.barLimit
                implicitWidth: 70
                value: practiceTracker.endBar
                onValueModified: {
                    practiceTracker.endBar = Math.max(practiceTracker.startBar,
                                                      Math.min(value, root.barLimit))
                }

                Binding on value {
                    when: !endBarSpin.activeFocus
                    value: practiceTracker.endBar
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }

            Label {
                text: qsTr("Tempo: %1 BPM").arg(practiceTracker.targetBpm)
                color: Theme.textTertiary
            }
            DarkSpinBox {
                id: bpmSpin
                from: 20
                to: 320
                implicitWidth: 70
                stepSize: 5
                value: practiceTracker.targetBpm
                onValueModified: practiceTracker.targetBpm = value

                Binding on value {
                    when: !bpmSpin.activeFocus
                    value: practiceTracker.targetBpm
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            visible: guitarProPreviewController.playerAvailable

            CheckBox {
                id: useExternalCheck
                text: qsTr("Use external")
                enabled: root.guitarProMediaId > 0 && !practiceTracker.timerRunning
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Skip the internal player and use Guitar Pro / TuxGuitar via Open externally.")
            }

            Label {
                text: qsTr("Track")
                color: Theme.textTertiary
                visible: root.guitarProMediaId > 0
            }

            ComboBox {
                id: trackCombo
                Layout.preferredWidth: 220
                Layout.fillWidth: true
                visible: root.guitarProMediaId > 0
                enabled: root.guitarProMediaId > 0 && guitarProPreviewController.loaded
                         && !practiceTracker.timerRunning
                model: guitarProPreviewController.trackNames
                onActivated: guitarProPreviewController.selectedTrackIndex = currentIndex

                Binding on currentIndex {
                    value: Math.max(0, guitarProPreviewController.selectedTrackIndex)
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }

            BusyIndicator {
                Layout.preferredWidth: 22
                Layout.preferredHeight: 22
                running: root.guitarProMediaId > 0 && guitarProPreviewController.loading
                visible: running
            }
        }

        Label {
            Layout.fillWidth: true
            visible: guitarProPreviewController.playerAvailable && root.guitarProMediaId <= 0
            wrapMode: Text.WordWrap
            text: qsTr("Select a Guitar Pro file above (and mark it active) to use the internal player.")
            color: Theme.textSecondary
            font.pixelSize: 12
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                Layout.fillWidth: true
                text: practiceTracker.timerRunning ? qsTr("Stop and save") : qsTr("Start timer")
                highlighted: true
                enabled: practiceTracker.timerRunning || root.practiceMaterialReady
                onClicked: {
                    if (practiceTracker.timerRunning) {
                        practiceTracker.stopAndSaveWithAssetId(root.practiceAssetId);
                        return
                    }

                    practiceTracker.startTimer()
                    // Internal player is the default; "Use external" opts out.
                    if (!useExternalCheck.checked && root.guitarProMediaId > 0
                            && guitarProPreviewController.playerAvailable)
                        root.requestInternalPlayer()
                }
            }

            Button {
                visible: practiceTracker.timerRunning && !useExternalCheck.checked
                         && root.guitarProMediaId > 0 && guitarProPreviewController.playerAvailable
                text: qsTr("Open player")
                onClicked: root.requestInternalPlayer()
            }

            Button {
                visible: practiceTracker.timerRunning
                text: qsTr("Cancel")
                onClicked: {
                    root.openPlayerWhenReady = false
                    practiceTracker.cancelTimer()
                }
            }
        }
    }
}
