pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property int panelWidth: 300
    property bool periodicExpanded: false

    readonly property bool hasReminders: reminderController.dayReminderCount > 0
    readonly property int dailyCount: reminderController.dailyReminderCount
    readonly property int periodicCount: reminderController.periodicReminderCount

    signal openSessionRequested(int songId, string title, int baseBpm, int practiceAssetId)
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
            visible: !reminderController.showAllReminders
            wrapMode: Text.WordWrap
            text: qsTr("On %1").arg(root.selectedDateLabel())
            font.pixelSize: 12
            color: Theme.textSecondary
        }

        CheckBox {
            Layout.fillWidth: true
            text: qsTr("Show all reminders")
            checked: reminderController.showAllReminders
            onToggled: reminderController.showAllReminders = checked
        }

        Label {
            Layout.fillWidth: true
            visible: !root.hasReminders
            wrapMode: Text.WordWrap
            text: qsTr("No reminders on this day.")
            font.pixelSize: 12
            color: Theme.textMuted
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.hasReminders
            clip: true

            Column {
                width: root.width - 24
                spacing: 8

                Label {
                    width: parent.width
                    visible: root.dailyCount > 0
                    text: qsTr("Daily")
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    color: Theme.textPrimary
                }

                Repeater {
                    model: reminderController.dayReminders

                    delegate: Rectangle {
                        id: dailyDelegate

                        required property int index
                        required property int reminderId
                        required property int songId
                        required property string reminderTitle
                        required property string songTitle
                        required property string scheduleLabel
                        required property bool isDaily
                        required property int baseBpm
                        required property int practiceAssetId
                        required property string completionStatus
                        required property string completionDetail
                        required property bool isCompleted
                        required property bool hasJournalEntry

                        width: parent.width
                        visible: dailyDelegate.isDaily
                        height: visible ? dailyCardColumn.implicitHeight + 16 : 0
                        radius: 8
                        color: Theme.cardBackground
                        border.color: root.completionBorderColor(
                                          dailyDelegate.completionStatus, dailyDelegate.isCompleted)
                        border.width: dailyDelegate.isCompleted ? 2 : 1

                        ColumnLayout {
                            id: dailyCardColumn
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Label {
                                    Layout.fillWidth: true
                                    text: dailyDelegate.reminderTitle.length > 0
                                             ? dailyDelegate.reminderTitle
                                             : dailyDelegate.songTitle
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                    color: Theme.textPrimary
                                    elide: Text.ElideRight
                                }

                                Label {
                                    visible: !reminderController.showAllReminders
                                             && dailyCardColumn.completionLabel.length > 0
                                    text: dailyDelegate.isCompleted ? "✓" : (dailyDelegate.completionStatus === "partial" ? "!" : "")
                                    font.pixelSize: 14
                                    font.weight: Font.Bold
                                    color: root.completionBorderColor(
                                               dailyDelegate.completionStatus, dailyDelegate.isCompleted)
                                }
                            }

                            property string completionLabel: root.completionLabel(
                                dailyDelegate.completionStatus, dailyDelegate.completionDetail,
                                dailyDelegate.isCompleted)

                            Label {
                                Layout.fillWidth: true
                                visible: !reminderController.showAllReminders
                                         && dailyCardColumn.completionLabel.length > 0
                                text: dailyCardColumn.completionLabel
                                font.pixelSize: 10
                                color: dailyDelegate.isCompleted ? Theme.success : Theme.warning
                                wrapMode: Text.WordWrap
                            }

                            Label {
                                Layout.fillWidth: true
                                text: dailyDelegate.songTitle
                                font.pixelSize: 11
                                color: Theme.textTertiary
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: dailyDelegate.scheduleLabel
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
                                                   dailyDelegate.songId, dailyDelegate.songTitle, dailyDelegate.baseBpm, dailyDelegate.practiceAssetId)
                                }

                                Button {
                                    visible: !reminderController.showAllReminders
                                             && dailyDelegate.completionStatus === "partial"
                                    text: qsTr("Mark as done")
                                    flat: true
                                    font.pixelSize: 10
                                    onClicked: root.promptPartialCompletion(
                                                   dailyDelegate.reminderId, dailyDelegate.songTitle,
                                                   dailyDelegate.completionDetail)
                                }

                                Button {
                                    text: qsTr("Edit")
                                    flat: true
                                    font.pixelSize: 10
                                    onClicked: root.editReminderRequested(
                                                   dailyDelegate.songId, dailyDelegate.songTitle, dailyDelegate.baseBpm, dailyDelegate.reminderId, dailyDelegate.practiceAssetId)
                                }

                                Button {
                                    text: qsTr("Delete")
                                    flat: true
                                    font.pixelSize: 10
                                    onClicked: reminderController.deleteReminder(dailyDelegate.reminderId)
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    width: parent.width
                    visible: root.periodicCount > 0
                    spacing: 4

                    ToolButton {
                        text: root.periodicExpanded ? "▼" : "▶"
                        onClicked: root.periodicExpanded = !root.periodicExpanded
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("More (%1)").arg(root.periodicCount)
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        color: Theme.textHint
                    }
                }

                Repeater {
                    model: reminderController.dayReminders

                    delegate: Rectangle {
                        id: periodicDelegate

                        required property int index
                        required property int reminderId
                        required property int songId
                        required property string reminderTitle
                        required property string songTitle
                        required property string scheduleLabel
                        required property bool isDaily
                        required property int baseBpm
                        required property int practiceAssetId
                        required property string completionStatus
                        required property string completionDetail
                        required property bool isCompleted
                        required property bool hasJournalEntry

                        width: parent.width
                        visible: !periodicDelegate.isDaily && root.periodicExpanded
                        height: visible ? periodicCardColumn.implicitHeight + 16 : 0
                        radius: 8
                        color: Theme.cardBackground
                        border.color: root.completionBorderColor(
                                          periodicDelegate.completionStatus, periodicDelegate.isCompleted)
                        border.width: periodicDelegate.isCompleted ? 2 : 1

                        ColumnLayout {
                            id: periodicCardColumn
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Label {
                                    Layout.fillWidth: true
                                    text: periodicDelegate.reminderTitle.length > 0
                                              ? periodicDelegate.reminderTitle
                                              : periodicDelegate.songTitle
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                    color: Theme.textPrimary
                                    elide: Text.ElideRight
                                }

                                Label {
                                    visible: periodicCardColumn.completionLabel.length > 0
                                    text: periodicDelegate.isCompleted ? "✓" : (periodicDelegate.completionStatus === "partial" ? "!" : "")
                                    font.pixelSize: 14
                                    font.weight: Font.Bold
                                    color: root.completionBorderColor(
                                               periodicDelegate.completionStatus, periodicDelegate.isCompleted)
                                }
                            }

                            property string completionLabel: root.completionLabel(
                                periodicDelegate.completionStatus, periodicDelegate.completionDetail,
                                periodicDelegate.isCompleted)

                            Label {
                                Layout.fillWidth: true
                                visible: periodicCardColumn.completionLabel.length > 0
                                text: periodicCardColumn.completionLabel
                                font.pixelSize: 10
                                color: periodicDelegate.isCompleted ? Theme.success : Theme.warning
                                wrapMode: Text.WordWrap
                            }

                            Label {
                                Layout.fillWidth: true
                                text: periodicDelegate.songTitle
                                font.pixelSize: 11
                                color: Theme.textTertiary
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: periodicDelegate.scheduleLabel
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
                                                   periodicDelegate.songId, periodicDelegate.songTitle, periodicDelegate.baseBpm, periodicDelegate.practiceAssetId)
                                }

                                Button {
                                    visible: periodicDelegate.completionStatus === "partial"
                                    text: qsTr("Mark as done")
                                    flat: true
                                    font.pixelSize: 10
                                    onClicked: root.promptPartialCompletion(
                                                   periodicDelegate.reminderId, periodicDelegate.songTitle,
                                                   periodicDelegate.completionDetail)
                                }

                                Button {
                                    text: qsTr("Edit")
                                    flat: true
                                    font.pixelSize: 10
                                    onClicked: root.editReminderRequested(
                                                   periodicDelegate.songId, periodicDelegate.songTitle, periodicDelegate.baseBpm, periodicDelegate.reminderId, periodicDelegate.practiceAssetId)
                                }

                                Button {
                                    text: qsTr("Delete")
                                    flat: true
                                    font.pixelSize: 10
                                    onClicked: reminderController.deleteReminder(periodicDelegate.reminderId)
                                }
                            }
                        }
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
        implicitWidth: parent

        property int reminderId: 0
        property string songTitle: ""
        property string detail: ""

        anchors.centerIn: Overlay.overlay
        modal: true
        title: qsTr("Mark as done?")
        standardButtons: Dialog.Ok | Dialog.Cancel

        contentItem: Label {
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
            root.periodicExpanded = false
            reminderController.reloadDayReminders()
        }
    }

    Component.onCompleted: reminderController.reloadDayReminders()
}
