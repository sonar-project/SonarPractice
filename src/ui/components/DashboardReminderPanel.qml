pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property int panelWidth: 300
    property bool completedExpanded: false

    readonly property int openBlockCount: reminderController.dayReminders.openSongBlocks.length
    readonly property int completedCount: reminderController.dayReminders.completedExerciseCount
    readonly property bool hasReminders: root.openBlockCount > 0 || root.completedCount > 0

    signal openSessionRequested(int songId, string title, int baseBpm, int practiceAssetId, int reminderId)
    signal editReminderRequested(int songId, string title, int baseBpm, int reminderId, int practiceAssetId)

    implicitWidth: panelWidth
    Layout.preferredWidth: panelWidth
    Layout.minimumWidth: panelWidth
    Layout.fillHeight: true
    radius: 10
    color: Theme.panelBackground
    border.color: Theme.border

    function toJsDate(value) {
        if (!value)
            return null
        if (value instanceof Date)
            return value
        if (value.year !== undefined)
            return new Date(value.year, value.month - 1, value.day)
        return null
    }

    function selectedDateLabel() {
        const sel = toJsDate(practiceTracker.selectedDate)
        return sel
                ? Qt.formatDate(sel, Qt.locale().dateFormat(Locale.ShortFormat))
                : qsTr("today")
    }

    function completionBorderColor(status, isCompleted) {
        if (isCompleted || status === "done" || status === "manual")
            return Theme.calendarCompleteDot
        if (status === "partial")
            return Theme.calendarPartialDot
        if (status === "practiced" || status === "pending")
            return Theme.borderSubtle
        return Theme.borderActive
    }

    function completionLabel(status, detail, isCompleted) {
        if (isCompleted || status === "done")
            return qsTr("Completed")
        if (status === "manual")
            return qsTr("Marked as done")
        if (status === "partial")
            return detail.length > 0 ? detail : qsTr("Partially met")
        if (status === "pending")
            return qsTr("Not practiced yet")
        return ""
    }

    function promptPartialCompletion(reminderId, songTitle, detail) {
        partialConfirmDialog.reminderId = reminderId
        partialConfirmDialog.songTitle = songTitle
        partialConfirmDialog.detail = detail
        partialConfirmDialog.open()
    }

    function syncCompletedSection() {
        root.completedExpanded = root.openBlockCount === 0 && root.completedCount > 0
    }

    Component {
        id: exerciseCard

        Rectangle {
            id: card
            required property var modelData

            readonly property string titleText: {
                const label = (card.modelData.exerciseLabel || "").trim()
                return label.length > 0 ? label : (card.modelData.scheduleLabel || "")
            }
            readonly property string statusText: root.completionLabel(
                                                     card.modelData.completionStatus || "",
                                                     card.modelData.completionDetail || "",
                                                     !!card.modelData.isCompleted)
            readonly property bool showSchedule: (card.modelData.scheduleLabel || "") !== card.titleText

            width: parent ? parent.width : implicitWidth
            implicitHeight: cardColumn.implicitHeight + 16
            radius: 8
            color: Theme.cardBackground
            border.color: root.completionBorderColor(
                              card.modelData.completionStatus || "", !!card.modelData.isCompleted)
            border.width: card.modelData.isCompleted ? 2 : 1

            ColumnLayout {
                id: cardColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Label {
                        Layout.fillWidth: true
                        text: card.titleText
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                    }

                    Label {
                        visible: card.statusText.length > 0
                        text: card.modelData.isCompleted
                              ? "✓"
                              : (card.modelData.completionStatus === "partial" ? "!" : "")
                        font.pixelSize: 14
                        font.weight: Font.Bold
                        color: root.completionBorderColor(
                                   card.modelData.completionStatus || "", !!card.modelData.isCompleted)
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: card.statusText.length > 0
                    text: card.statusText
                    font.pixelSize: 10
                    color: card.modelData.isCompleted ? Theme.success : Theme.warning
                    wrapMode: Text.WordWrap
                }

                Label {
                    Layout.fillWidth: true
                    visible: card.showSchedule
                    text: card.modelData.scheduleLabel || ""
                    font.pixelSize: 10
                    color: Theme.textSecondary
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Button {
                        text: qsTr("Open exercise")
                        flat: true
                        font.pixelSize: 10
                        onClicked: root.openSessionRequested(
                                       card.modelData.songId, card.modelData.songTitle,
                                       card.modelData.baseBpm, card.modelData.practiceAssetId,
                                       card.modelData.reminderId)
                    }

                    Button {
                        visible: card.modelData.completionStatus === "partial"
                        text: qsTr("Mark as done")
                        flat: true
                        font.pixelSize: 10
                        onClicked: root.promptPartialCompletion(
                                       card.modelData.reminderId, card.modelData.songTitle,
                                       card.modelData.completionDetail || "")
                    }

                    Button {
                        text: qsTr("Edit")
                        flat: true
                        font.pixelSize: 10
                        onClicked: root.editReminderRequested(
                                       card.modelData.songId, card.modelData.songTitle,
                                       card.modelData.baseBpm, card.modelData.reminderId,
                                       card.modelData.practiceAssetId)
                    }

                    Button {
                        text: qsTr("Delete")
                        flat: true
                        font.pixelSize: 10
                        onClicked: reminderController.deleteReminder(card.modelData.reminderId)
                    }
                }
            }
        }
    }

    Component {
        id: songBlock

        Column {
            id: songColumn
            required property int index
            required property var modelData

            width: parent ? parent.width : implicitWidth
            spacing: 8

            Label {
                width: parent ? parent.width : implicitWidth
                text: songColumn.modelData.songTitle || ""
                font.pixelSize: 13
                font.weight: Font.DemiBold
                color: Theme.accentLight
                elide: Text.ElideRight
            }

            Repeater {
                model: songColumn.modelData.exercises
                delegate: exerciseCard
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Label {
            text: qsTr("Reminders")
            font.pixelSize: 14
            font.weight: Font.DemiBold
            color: Theme.accentLight
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("On %1").arg(root.selectedDateLabel())
            font.pixelSize: 12
            color: Theme.textSecondary
        }

        Label {
            Layout.fillWidth: true
            visible: !root.hasReminders
            wrapMode: Text.WordWrap
            text: qsTr("No reminders on this day.")
            font.pixelSize: 12
            color: Theme.textMuted
        }

        Label {
            Layout.fillWidth: true
            visible: root.openBlockCount === 0 && root.completedCount > 0
            wrapMode: Text.WordWrap
            text: qsTr("All exercises for this day are done.")
            font.pixelSize: 12
            color: Theme.textSecondary
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.hasReminders
            clip: true

            Column {
                width: root.width - 24
                spacing: 12

                Repeater {
                    model: reminderController.dayReminders.openSongBlocks
                    delegate: songBlock
                }

                RowLayout {
                    width: parent.width
                    visible: root.completedCount > 0
                    spacing: 4

                    ToolButton {
                        text: root.completedExpanded ? "▼" : "▶"
                        onClicked: root.completedExpanded = !root.completedExpanded
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Done (%1)").arg(root.completedCount)
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        color: Theme.textHint
                    }
                }

                Column {
                    width: parent.width
                    spacing: 12
                    visible: root.completedExpanded
                    height: visible ? implicitHeight : 0

                    Repeater {
                        model: reminderController.dayReminders.completedSongBlocks
                        delegate: songBlock
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
            visible: !root.hasReminders
        }
    }

    Dialog {
        id: partialConfirmDialog
        implicitWidth: Math.max(240, root.panelWidth - 32)

        property int reminderId: 0
        property string songTitle: ""
        property string detail: ""

        anchors.centerIn: Overlay.overlay
        modal: true
        title: qsTr("Mark as done?")
        standardButtons: Dialog.Ok | Dialog.Cancel

        contentItem: Label {
            width: partialConfirmDialog.availableWidth
            wrapMode: Text.WordWrap
            text: qsTr("%1\n\n%2\n\nThe reminder condition was only partially met. Mark as completed for this day anyway?")
                    .arg(partialConfirmDialog.songTitle)
                    .arg(partialConfirmDialog.detail)
            color: Theme.textPrimary
        }

        onAccepted: {
            const sel = root.toJsDate(practiceTracker.selectedDate)
            if (partialConfirmDialog.reminderId > 0 && sel)
                reminderController.acceptPartialCompletion(partialConfirmDialog.reminderId, sel)
        }
    }

    Connections {
        target: practiceTracker
        function onSelectedDateChanged() {
            reminderController.reloadDayReminders()
            root.syncCompletedSection()
        }
    }

    Component.onCompleted: {
        reminderController.reloadDayReminders()
        root.syncCompletedSection()
    }
}
