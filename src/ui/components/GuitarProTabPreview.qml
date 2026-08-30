// src/ui/components/GuitarProTabPreview.qml

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    id: root

    /** When true, fills a dedicated stack page (always visible; Back closes the page). */
    property bool pageMode: false

    background: Rectangle {
        radius: 10
        color: Theme.panelBackground
        border.color: Theme.border
    }

    readonly property bool showPlayer: guitarProPreviewController.usePlayer

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

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: root.showPlayer && guitarProPreviewController.loaded

            Button {
                text: guitarProPreviewController.playing ? qsTr("Pause") : qsTr("Play")
                highlighted: true
                onClicked: guitarProPreviewController.playPause()
            }

            Button {
                text: qsTr("Stop")
                onClicked: guitarProPreviewController.stop()
            }

            Label {
                text: qsTr("Tempo")
                color: Theme.textSecondary
            }

            Slider {
                from: 50
                to: 150
                stepSize: 5
                value: guitarProPreviewController.tempoPercent
                Layout.preferredWidth: 140
                onMoved: guitarProPreviewController.tempoPercent = Math.round(value)
            }

            Label {
                text: qsTr("%1%").arg(guitarProPreviewController.tempoPercent)
                color: Theme.textSecondary
                Layout.preferredWidth: 40
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
            Layout.fillHeight: root.pageMode
            Layout.preferredHeight: root.pageMode ? 0 : 360
            Layout.minimumHeight: 220
            active: root.showPlayer && guitarProPreviewController.playerAvailable
                    && (root.pageMode || guitarProPreviewController.visible)
            source: active ? "GuitarProPlayerView.qml" : ""
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: root.pageMode && !root.showPlayer
            Layout.preferredHeight: root.pageMode ? 0 : 220
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
    }
}
