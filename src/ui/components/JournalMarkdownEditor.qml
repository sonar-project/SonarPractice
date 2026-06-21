import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../js/MarkdownFormat.js" as MarkdownFormat

GroupBox {
    id: root

    title: qsTr("Journal")
    padding: 12

    // Save only after syncFromTracker() — prevents overwriting with empty data during page rendering.
    property bool ready: false

    function syncFromTracker() {
        editor.syncingFromTracker = true
        editor.text = practiceTracker.journalMarkdown
        editor.syncingFromTracker = false
    }

    function flush() {
        if (!root.ready)
            return
        saveTimer.stop()
        if (editor.text !== practiceTracker.journalMarkdown)
            practiceTracker.journalMarkdown = editor.text
        practiceTracker.saveJournalNote()
    }

    Component.onDestruction: {
        root.ready = false
        root.flush()
    }

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

    Timer {
        id: saveTimer
        interval: 500
        repeat: false
        onTriggered: practiceTracker.saveJournalNote()
    }

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: root.padding
        anchors.bottom: parent.bottom
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Repeater {
                model: [
                    {
                        icon: "qrc:/qt/qml/com/sonarp/sonarpractice/assets/svg/bold.svg",
                        tooltip: qsTr("Bold (Ctrl+B)"),
                        action: function() { MarkdownFormat.toggleBold(editor) }
                    },
                    {
                        icon: "qrc:/qt/qml/com/sonarp/sonarpractice/assets/svg/minus.svg",
                        tooltip: qsTr("List (Ctrl+Shift+L)"),
                        action: function() { MarkdownFormat.insertListItem(editor) }
                    },
                    {
                        icon: "qrc:/qt/qml/com/sonarp/sonarpractice/assets/svg/heading-1.svg",
                        tooltip: qsTr("Heading 1 (Ctrl+1)"),
                        action: function() { MarkdownFormat.setHeading(editor, 1) }
                    },
                    {
                        icon: "qrc:/qt/qml/com/sonarp/sonarpractice/assets/svg/heading-2.svg",
                        tooltip: qsTr("Heading 2 (Ctrl+2)"),
                        action: function() { MarkdownFormat.setHeading(editor, 2) }
                    },
                    {
                        icon: "qrc:/qt/qml/com/sonarp/sonarpractice/assets/svg/heading-3.svg",
                        tooltip: qsTr("Heading 3 (Ctrl+3)"),
                        action: function() { MarkdownFormat.setHeading(editor, 3) }
                    },
                    {
                        icon: "qrc:/qt/qml/com/sonarp/sonarpractice/assets/svg/brackets.svg",
                        tooltip: qsTr("Checkbox (Ctrl+Shift+C)"),
                        action: function() { MarkdownFormat.insertCheckbox(editor) }
                    }
                ]

                delegate: ToolButton {
                    id: formatButton

                    required property var modelData

                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    icon.source: formatButton.modelData.icon
                    icon.width: 18
                    icon.height: 18
                    icon.color: Theme.textPrimary
                    hoverEnabled: true
                    ToolTip.visible: formatButton.hovered
                    ToolTip.text: formatButton.modelData.tooltip
                    ToolTip.delay: 400
                    onClicked: formatButton.modelData.action()

                    background: Rectangle {
                        radius: 6
                        color: formatButton.down ? Theme.toolbarButtonPressed
                              : formatButton.hovered ? Theme.toolbarButtonHover
                              : "transparent"
                        border.color: formatButton.hovered ? Theme.borderSubtle : "transparent"
                    }
                }
            }

            Item { Layout.fillWidth: true }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 120
            clip: true

            TextArea {
                id: editor
                width: parent.width
                wrapMode: TextArea.Wrap
                selectByMouse: true
                placeholderText: qsTr("Notes for the exercise on the selected day…")
                color: Theme.textPrimary
                placeholderTextColor: Theme.textPlaceholder
                font.pixelSize: 13
                topPadding: 8
                bottomPadding: 8
                leftPadding: 10
                rightPadding: 10

                property bool syncingFromTracker: false

                background: Rectangle {
                    radius: 8
                    color: Theme.editorBackground
                    border.color: editor.activeFocus ? Theme.borderAccent : Theme.border
                }

                Keys.onPressed: function(event) {
                    if (!(event.modifiers & Qt.ControlModifier))
                        return

                    if (event.key === Qt.Key_B) {
                        MarkdownFormat.toggleBold(editor)
                        event.accepted = true
                    } else if (event.key === Qt.Key_1) {
                        MarkdownFormat.setHeading(editor, 1)
                        event.accepted = true
                    } else if (event.key === Qt.Key_2) {
                        MarkdownFormat.setHeading(editor, 2)
                        event.accepted = true
                    } else if (event.key === Qt.Key_3) {
                        MarkdownFormat.setHeading(editor, 3)
                        event.accepted = true
                    } else if ((event.modifiers & Qt.ShiftModifier) && event.key === Qt.Key_L) {
                        MarkdownFormat.insertListItem(editor)
                        event.accepted = true
                    } else if ((event.modifiers & Qt.ShiftModifier) && event.key === Qt.Key_C) {
                        MarkdownFormat.insertCheckbox(editor)
                        event.accepted = true
                    }
                }

                onTextChanged: {
                    if (syncingFromTracker || !root.ready)
                        return
                    practiceTracker.journalMarkdown = text
                    saveTimer.restart()
                }

                Connections {
                    target: practiceTracker
                    function onJournalMarkdownChanged() {
                        if (editor.activeFocus || !root.ready)
                            return
                        root.syncFromTracker()
                    }
                    function onSongIdChanged() {
                        if (!root.ready)
                            return
                        root.syncFromTracker()
                    }
                    function onSelectedDateChanged() {
                        if (!root.ready)
                            return
                        root.syncFromTracker()
                    }
                }
            }
        }
    }
}
