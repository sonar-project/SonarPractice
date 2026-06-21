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

                        width: parent.width
                        visible: dailyDelegate.isDaily
                        height: visible ? dailyCardColumn.implicitHeight + 16 : 0
                        radius: 8
                        color: Theme.cardBackground
                        border.color: Theme.borderActive

                        ColumnLayout {
                            id: dailyCardColumn
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

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

                        width: parent.width
                        visible: !periodicDelegate.isDaily && root.periodicExpanded
                        height: visible ? periodicCardColumn.implicitHeight + 16 : 0
                        radius: 8
                        color: Theme.cardBackground
                        border.color: Theme.borderSubtle

                        ColumnLayout {
                            id: periodicCardColumn
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

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

    Connections {
        target: practiceTracker
        function onSelectedDateChanged() {
            root.periodicExpanded = false
            reminderController.reloadDayReminders()
        }
    }

    Component.onCompleted: reminderController.reloadDayReminders()
}
