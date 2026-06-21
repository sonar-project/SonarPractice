pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property int sidePanelWidth: 300

    readonly property var today: new Date()

    property var viewDate: new Date(today.getFullYear(), today.getMonth(), 1)

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
    }

    // Start Point
    Component.onCompleted: {
        if (!toJsDate(practiceTracker.selectedDate))
            setSelectedDate(today)
        else
            syncViewToSelected()
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
            text: qsTr("Select the practice date for journal and reminders.")
            font.pixelSize: 12
            color: Theme.textSecondary
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

                implicitWidth: 36
                implicitHeight: 32

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.setSelectedDate(delegateItem.model.date)
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
                    text: delegateItem.model.day
                    font.pixelSize: 12
                    font.weight: delegateItem.isSelected ? Font.Bold : Font.Normal
                    color: !delegateItem.inCurrentMonth ? Theme.textOutOfMonth
                                                        : (delegateItem.isSelected ? Theme.textOnAccent : Theme.textPrimary)
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

    Connections {
        target: practiceTracker
        function onSelectedDateChanged() {
            root.syncViewToSelected()
        }
    }
}
