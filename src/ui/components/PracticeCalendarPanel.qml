pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property int sidePanelWidth: 300

    signal openPracticeRequested(int songId, string title, int baseBpm, int practiceAssetId)

    readonly property var today: new Date()

    property var viewDate: new Date(today.getFullYear(), today.getMonth(), 1)
    property var monthMarkers: ({})

    implicitWidth: sidePanelWidth
    Layout.preferredWidth: sidePanelWidth
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

    function datesEqual(a, b) {
        const da = toJsDate(a)
        const db = toJsDate(b)
        if (!da || !db)
            return false
        return da.getFullYear() === db.getFullYear()
                && da.getMonth() === db.getMonth()
                && da.getDate() === db.getDate()
    }

    function setSelectedDate(date) {
        const js = toJsDate(date)
        if (!js)
            return
        practiceTracker.selectedDate = js
    }

    function syncViewToSelected() {
        const sel = toJsDate(practiceTracker.selectedDate)
        if (sel)
            viewDate = new Date(sel.getFullYear(), sel.getMonth(), 1)
    }

    function shiftMonth(delta) {
        viewDate = new Date(viewDate.getFullYear(), viewDate.getMonth() + delta, 1)
        refreshMonthMarkers()
    }

    function refreshMonthMarkers() {
        const summary = reminderController.monthCalendarSummary(
                    viewDate.getFullYear(), viewDate.getMonth() + 1)
        const map = {}
        for (let i = 0; i < summary.length; ++i) {
            const row = summary[i]
            map[row.day] = row
        }
        monthMarkers = map
    }

    function markerForDay(day) {
        return monthMarkers[day] ?? null
    }

    function openDayMenu(date) {
        const js = toJsDate(date)
        if (!js)
            return
        dayContextMenu.targetDate = js
        dayContextMenu.itemsModel = reminderController.dayPracticeDetails(js)
        dayContextMenu.popup()
    }

    Component.onCompleted: {
        if (!toJsDate(practiceTracker.selectedDate))
            setSelectedDate(today)
        else
            syncViewToSelected()
        refreshMonthMarkers()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Label {
            text: qsTr("Calendar")
            font.pixelSize: 14
            font.weight: Font.DemiBold
            color: Theme.accentLight
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Select a day. Dots show practice progress. Right-click a day for details.")
            font.pixelSize: 12
            color: Theme.textSecondary
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Row {
                spacing: 4
                Rectangle { width: 8; height: 8; radius: 4; color: Theme.calendarPracticedDot; anchors.verticalCenter: parent.verticalCenter }
                Label { text: qsTr("Practiced"); font.pixelSize: 10; color: Theme.textHint }
            }

            Row {
                spacing: 4
                Rectangle { width: 8; height: 8; radius: 4; color: Theme.calendarCompleteDot; anchors.verticalCenter: parent.verticalCenter }
                Label { text: qsTr("Done"); font.pixelSize: 10; color: Theme.textHint }
            }

            Row {
                spacing: 4
                Rectangle { width: 8; height: 8; radius: 4; color: Theme.calendarPartialDot; anchors.verticalCenter: parent.verticalCenter }
                Label { text: qsTr("Partial"); font.pixelSize: 10; color: Theme.textHint }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            ToolButton {
                text: "‹"
                onClicked: root.shiftMonth(-1)
            }

            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: Qt.formatDate(root.viewDate, "MMMM yyyy")
                font.pixelSize: 13
                font.weight: Font.DemiBold
                color: Theme.textPrimary
            }

            ToolButton {
                text: "›"
                onClicked: root.shiftMonth(1)
            }
        }

        DayOfWeekRow {
            id: dayOfWeekRow
            Layout.fillWidth: true
            locale: monthGrid.locale
        }

        MonthGrid {
            id: monthGrid
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            month: root.viewDate ? root.viewDate.getMonth() : new Date().getMonth()
            year: root.viewDate ? root.viewDate.getFullYear() : new Date().getFullYear()
            locale: Qt.locale()

            onClicked: (date) => root.setSelectedDate(date)

            delegate: Item {
                id: delegateItem

                required property var model

                readonly property bool inCurrentMonth: delegateItem.model.month === monthGrid.month
                readonly property bool isSelected: root.datesEqual(delegateItem.model.date, practiceTracker.selectedDate)
                readonly property var dayMarker: root.markerForDay(delegateItem.model.day)

                implicitWidth: 36
                implicitHeight: 36

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton)
                            root.openDayMenu(delegateItem.model.date)
                        else
                            root.setSelectedDate(delegateItem.model.date)
                    }
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: 28
                    height: 28
                    radius: 14
                    color: delegateItem.isSelected ? Theme.calendarSelected
                                                   : (delegateItem.model.today ? Theme.calendarTodayBackground : "transparent")
                    border.color: delegateItem.isSelected ? Theme.calendarSelectedBorder : "transparent"
                }

                Label {
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: delegateItem.dayMarker ? -3 : 0
                    text: delegateItem.model.day
                    font.pixelSize: 12
                    font.weight: delegateItem.isSelected ? Font.Bold : Font.Normal
                    color: !delegateItem.inCurrentMonth ? Theme.textOutOfMonth
                                                        : (delegateItem.isSelected ? Theme.textOnAccent : Theme.textPrimary)
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 2
                    spacing: 2
                    visible: delegateItem.inCurrentMonth && delegateItem.dayMarker

                    Rectangle {
                        visible: delegateItem.dayMarker && delegateItem.dayMarker.practiced
                        width: 5
                        height: 5
                        radius: 2.5
                        color: Theme.calendarPracticedDot
                    }

                    Rectangle {
                        visible: delegateItem.dayMarker
                                 && (delegateItem.dayMarker.status === "complete"
                                     || delegateItem.dayMarker.status === "manual")
                        width: 5
                        height: 5
                        radius: 2.5
                        color: Theme.calendarCompleteDot
                    }

                    Rectangle {
                        visible: delegateItem.dayMarker
                                 && (delegateItem.dayMarker.status === "partial"
                                     || delegateItem.dayMarker.status === "mixed")
                        width: 5
                        height: 5
                        radius: 2.5
                        color: Theme.calendarPartialDot
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: {
                const sel = root.toJsDate(practiceTracker.selectedDate)
                return sel
                        ? qsTr("Selected: %1").arg(Qt.formatDate(sel, Qt.locale().dateFormat(Locale.ShortFormat)))
                        : qsTr("No date selected")
            }
            font.pixelSize: 12
            color: Theme.textHint
        }

        Item { Layout.fillHeight: true }
    }

    Menu {
        id: dayContextMenu

        property var targetDate: null
        property var itemsModel: []

        title: targetDate
               ? qsTr("Practice on %1").arg(Qt.formatDate(targetDate, Qt.locale().dateFormat(Locale.ShortFormat)))
               : ""

        Instantiator {
            model: dayContextMenu.itemsModel

            MenuItem {
                required property var modelData

                text: {
                    const title = modelData.songTitle ?? qsTr("Exercise")
                    if (modelData.completionStatus === "partial")
                        return qsTr("%1 — %2").arg(title).arg(modelData.completionDetail)
                    if (modelData.completionStatus === "pending")
                        return qsTr("%1 — not practiced").arg(title)
                    if (modelData.completionStatus === "practiced")
                        return qsTr("%1 — practiced").arg(title)
                    if (modelData.isCompleted)
                        return qsTr("%1 — done").arg(title)
                    return title
                }

                onTriggered: {
                    if (modelData.completionStatus === "partial") {
                        partialConfirmDialog.reminderId = modelData.reminderId
                        partialConfirmDialog.targetDate = dayContextMenu.targetDate
                        partialConfirmDialog.songTitle = modelData.songTitle
                        partialConfirmDialog.detail = modelData.completionDetail
                        partialConfirmDialog.songId = modelData.songId
                        partialConfirmDialog.baseBpm = modelData.baseBpm
                        partialConfirmDialog.practiceAssetId = modelData.practiceAssetId
                        partialConfirmDialog.open()
                        return
                    }

                    root.openPracticeRequested(
                                modelData.songId, modelData.songTitle, modelData.baseBpm,
                                modelData.practiceAssetId)
                }
            }

            onObjectAdded: (index, object) => dayContextMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => dayContextMenu.removeItem(object)
        }

        MenuSeparator {
            visible: dayContextMenu.itemsModel.length === 0
        }

        MenuItem {
            visible: dayContextMenu.itemsModel.length === 0
            enabled: false
            text: qsTr("No practice logged on this day")
        }
    }

    Dialog {
        id: partialConfirmDialog
        implicitWidth: parent

        property int reminderId: 0
        property var targetDate: null
        property string songTitle: ""
        property string detail: ""
        property int songId: 0
        property int baseBpm: 0
        property int practiceAssetId: 0

        anchors.centerIn: parent
        modal: true
        title: qsTr("Mark as done?")

        contentItem: Label {
            wrapMode: Text.WordWrap
            text: qsTr("%1\n\n%2\n\nMark this exercise as completed for the day?")
                    .arg(partialConfirmDialog.songTitle)
                    .arg(partialConfirmDialog.detail)
            color: Theme.textPrimary
        }

        footer: DialogButtonBox {
            Button {
                text: qsTr("Open exercise")
                DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
                onClicked: {
                    root.openPracticeRequested(
                                partialConfirmDialog.songId, partialConfirmDialog.songTitle,
                                partialConfirmDialog.baseBpm, partialConfirmDialog.practiceAssetId)
                    partialConfirmDialog.close()
                }
            }
            Button {
                text: qsTr("Mark as done")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: {
                    if (partialConfirmDialog.reminderId > 0 && partialConfirmDialog.targetDate)
                        reminderController.acceptPartialCompletion(
                                    partialConfirmDialog.reminderId, partialConfirmDialog.targetDate)
                    partialConfirmDialog.close()
                }
            }
            Button {
                text: qsTr("Cancel")
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                onClicked: partialConfirmDialog.close()
            }
        }
    }

    Connections {
        target: practiceTracker
        function onSelectedDateChanged() {
            root.syncViewToSelected()
        }
        function onJournalSaved() {
            root.refreshMonthMarkers()
        }
    }

    Connections {
        target: reminderController
        function onCalendarDataChanged() {
            root.refreshMonthMarkers()
        }
        function onFilterDateChanged() {
            root.refreshMonthMarkers()
        }
    }
}
