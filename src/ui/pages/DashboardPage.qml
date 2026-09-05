pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../components"
import "../js/MediaKind.js" as MediaKind

Page {
    id: root

    readonly property bool useWideLayout: width > 900

    required property bool sessionLocked

    readonly property var mediaKindOptions: [
        { kind: MediaKind.GuitarPro, label: qsTr("GP") },
        { kind: MediaKind.Audio, label: qsTr("Audio") },
        { kind: MediaKind.Video, label: qsTr("Video") },
        { kind: MediaKind.Document, label: qsTr("Document") },
        { kind: MediaKind.Image, label: qsTr("Image") }
    ]

    signal songSelected(int songId, string title, int baseBpm, int practiceAssetId, int mediaId, int sourceReminderId)
    signal reminderEditRequested(int songId, string title, int baseBpm, int reminderId, int practiceAssetId)

    background: Rectangle { color: Theme.windowBackground }

    header: TopBar {
        title: qsTr("Repertoire")
        showBack: false
        sessionLocked: root.sessionLocked
        sessionLabel: qsTr("Timer running")
        onStopSessionRequested: {
            practiceTracker.stopAndSaveWithAssetId(practiceTracker.assetId);
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 35
        spacing: 12

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Exercises on the left, reminders in the center, calendar on the right — tap a card to open media and training.")
            font.pixelSize: 13
            color: Theme.textSecondary
        }

        Label {
            Layout.fillWidth: true
            visible: importService.statusMessage.length > 0
            wrapMode: Text.WordWrap
            text: importService.statusMessage
            font.pixelSize: 12
            color: Theme.textHint
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            DarkTextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search by title, artist, or tuning…")
                text: songModel.searchText
                onTextEdited: songModel.searchText = text

                Keys.onEscapePressed: {
                    text = ""
                    songModel.searchText = ""
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: root.mediaKindOptions

                delegate: Button {
                    id: kindChip
                    required property var modelData

                    readonly property bool selected: songModel.mediaKindFilter === modelData.kind

                    text: modelData.label
                    flat: true
                    padding: 6
                    leftPadding: 10
                    rightPadding: 10
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    palette.buttonText: selected ? Theme.textOnAccent : Theme.textSecondary

                    background: Rectangle {
                        radius: 4
                        color: kindChip.selected
                               ? Theme.highlight
                               : (kindChip.down ? Theme.cardBackgroundPressed : Theme.panelBackgroundNested)
                        border.color: kindChip.selected ? Theme.highlight : Theme.border
                        border.width: 1
                    }

                    onClicked: {
                        songModel.mediaKindFilter =
                                songModel.mediaKindFilter === modelData.kind ? "" : modelData.kind
                    }
                }
            }

            Item { Layout.fillWidth: true }

            ComboBox {
                id: bpmFilterCombo
                Layout.preferredWidth: 140
                model: {
                    const options = [{ value: 0, label: qsTr("All tempos") }]
                    const known = songModel.knownBpms
                    for (let i = 0; i < known.length; ++i)
                        options.push({ value: known[i], label: qsTr("%1 BPM").arg(known[i]) })
                    return options
                }
                textRole: "label"
                currentIndex: {
                    const known = songModel.knownBpms
                    if (songModel.bpmFilter <= 0)
                        return 0
                    for (let i = 0; i < known.length; ++i) {
                        if (known[i] === songModel.bpmFilter)
                            return i + 1
                    }
                    return 0
                }
                onActivated: index => {
                    songModel.bpmFilter = index <= 0 ? 0 : Number(songModel.knownBpms[index - 1])
                }
            }

            ComboBox {
                id: tuningFilterCombo
                Layout.preferredWidth: 180
                model: {
                    const options = [{ id: 0, name: qsTr("All tunings") }]
                    const known = songModel.knownTunings
                    for (let i = 0; i < known.length; ++i)
                        options.push(known[i])
                    return options
                }
                textRole: "name"
                currentIndex: {
                    const known = songModel.knownTunings
                    if (songModel.tuningIdFilter <= 0)
                        return 0
                    for (let i = 0; i < known.length; ++i) {
                        if (Number(known[i].id) === songModel.tuningIdFilter)
                            return i + 1
                    }
                    return 0
                }
                onActivated: index => {
                    if (index <= 0) {
                        songModel.tuningIdFilter = 0
                        return
                    }
                    songModel.tuningIdFilter = Number(songModel.knownTunings[index - 1].id)
                }
            }

            CheckBox {
                text: qsTr("Favorites")
                checked: songModel.favoritesOnly
                onToggled: songModel.favoritesOnly = checked
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            CheckBox {
                text: qsTr("Hide groups")
                checked: songModel.hideContainers
                onToggled: songModel.hideContainers = checked
            }

            CheckBox {
                text: qsTr("Show groups only")
                checked: songModel.containersOnly
                onToggled: songModel.containersOnly = checked
            }
        }

        Loader {
            id: contentLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: root.useWideLayout ? wideLayout : narrowLayout
        }

        Label {
            visible: songModel.catalogReady && songModel.totalCount === 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("No exercises yet. Import media via Import → File… in the menu bar.")
            font.pixelSize: 13
            color: Theme.textMuted
        }
    }

    Component {
        id: wideLayout

        RowLayout {
            anchors.fill: parent
            spacing: 16

            DashboardExerciseList {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 280

                onSongActivated: (songId, title, baseBpm, mediaId) =>
                    root.songSelected(songId, title, baseBpm, 0, mediaId, 0)
            }

            DashboardReminderPanel {
                onOpenSessionRequested: (songId, title, baseBpm, practiceAssetId, reminderId) =>
                    root.songSelected(songId, title, baseBpm, practiceAssetId, 0, reminderId)
                onEditReminderRequested: (songId, title, baseBpm, reminderId, practiceAssetId) =>
                    root.reminderEditRequested(songId, title, baseBpm, reminderId, practiceAssetId)
            }

            PracticeCalendarPanel {
                onOpenPracticeRequested: (songId, title, baseBpm, practiceAssetId, reminderId) =>
                    root.songSelected(songId, title, baseBpm, practiceAssetId, 0, reminderId || 0)
            }
        }
    }

    Component {
        id: narrowLayout

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            DashboardExerciseList {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: Math.max(200, root.height * 0.45)

                onSongActivated: (songId, title, baseBpm, mediaId) =>
                    root.songSelected(songId, title, baseBpm, 0, mediaId, 0)
            }

            DashboardReminderPanel {
                Layout.fillWidth: true
                panelWidth: parent.width

                onOpenSessionRequested: (songId, title, baseBpm, practiceAssetId, reminderId) =>
                    root.songSelected(songId, title, baseBpm, practiceAssetId, 0, reminderId)
                onEditReminderRequested: (songId, title, baseBpm, reminderId, practiceAssetId) =>
                    root.reminderEditRequested(songId, title, baseBpm, reminderId, practiceAssetId)
            }

            PracticeCalendarPanel {
                Layout.fillWidth: true
                sidePanelWidth: parent.width

                onOpenPracticeRequested: (songId, title, baseBpm, practiceAssetId, reminderId) =>
                    root.songSelected(songId, title, baseBpm, practiceAssetId, 0, reminderId || 0)
            }
        }
    }
}
