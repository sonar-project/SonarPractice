// src/ui/components/GuitarProTabPreview.qml

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    id: root

    title: qsTr("Guitar Pro preview")
    visible: guitarProPreviewController.visible
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

    readonly property bool showPlayer: guitarProPreviewController.usePlayer

    ColumnLayout {
        width: parent.width
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

            Button {
                text: qsTr("Close")
                flat: true
                onClicked: guitarProPreviewController.clear()
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
                text: qsTr("Loop")
                checked: guitarProPreviewController.loopEnabled
                onToggled: guitarProPreviewController.loopEnabled = checked
            }

            Label {
                text: qsTr("Bars")
                color: Theme.textSecondary
                visible: guitarProPreviewController.loopEnabled
            }

            SpinBox {
                from: 1
                to: Math.max(1, guitarProPreviewController.barCount)
                value: guitarProPreviewController.loopStartBar
                visible: guitarProPreviewController.loopEnabled
                editable: true
                onValueModified: guitarProPreviewController.loopStartBar = value
            }

            Label {
                text: "–"
                visible: guitarProPreviewController.loopEnabled
                color: Theme.textSecondary
            }

            SpinBox {
                from: 1
                to: Math.max(1, guitarProPreviewController.barCount)
                value: guitarProPreviewController.loopEndBar
                visible: guitarProPreviewController.loopEnabled
                editable: true
                onValueModified: guitarProPreviewController.loopEndBar = value
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
            Layout.preferredHeight: 360
            Layout.minimumHeight: 220
            active: root.showPlayer && guitarProPreviewController.playerAvailable
                    && guitarProPreviewController.visible
            source: active ? "GuitarProPlayerView.qml" : ""
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 220
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
