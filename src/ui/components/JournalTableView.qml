pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    id: root

    title: qsTr("Practice log")
    padding: 12

    readonly property int tableColumnCount: 6
    readonly property var columnWeights: [1.3, 1, 1, 1, 1, 1.1]

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

    ColumnLayout {
        id: tableLayout

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: root.padding
        anchors.bottom: parent.bottom
        spacing: 8

        function totalWeight() {
            let sum = 0;
            for (let i = 0; i < root.columnWeights.length; ++i)
                sum += root.columnWeights[i];
            return sum;
        }

        function columnWidth(column) {
            const available = Math.max(tableView.width, width - 8);
            const minCol = 56;
            const weight = root.columnWeights[column] ?? 1;
            return Math.max(minCol, Math.floor(available * weight / totalWeight()));
        }

        HorizontalHeaderView {
            id: headerView
            Layout.fillWidth: true
            syncView: tableView
            clip: true

            delegate: Rectangle {
                id: headerDelegate

                required property int column

                implicitWidth: Math.max(1, tableLayout.columnWidth(headerDelegate.column))
                implicitHeight: 36
                color: Theme.tableHeaderBackground
                border.color: Theme.borderSubtle

                Label {
                    anchors.fill: parent
                    anchors.margins: 6
                    text: practiceTracker.journalModel.headerData(headerDelegate.column, Qt.Horizontal, Qt.DisplayRole)
                    ToolTip.visible: headerTipArea.containsMouse && headerLabel.toolTipText.length > 0
                    ToolTip.text: headerLabel.toolTipText
                    ToolTip.delay: 400
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    color: Theme.accentLight
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    id: headerLabel

                    readonly property string toolTipText: practiceTracker.journalModel.headerData(headerDelegate.column, Qt.Horizontal, Qt.ToolTipRole) ?? ""

                    MouseArea {
                        id: headerTipArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }
                }
            }
        }

        TableView {
            id: tableView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 120
            clip: true
            model: practiceTracker.journalModel
            columnSpacing: 0
            rowSpacing: 0

            columnWidthProvider: function (column) {
                return tableLayout.columnWidth(column);
            }

            onWidthChanged: forceLayout()

            delegate: Rectangle {
                id: cellDelegate

                required property int row
                required property int column

                implicitWidth: Math.max(1, tableView.columnWidthProvider(cellDelegate.column))
                implicitHeight: 32
                color: cellDelegate.row % 2 === 0 ? Theme.tableRowEven : Theme.tableRowOdd
                border.color: Theme.border

                TextField {
                    anchors.fill: parent
                    // anchors.margins: 6
                    text: tableView.model.data(tableView.model.index(cellDelegate.row, cellDelegate.column))

                    // color: Theme.textPrimary
                    // font.pixelSize: 12
                    // verticalAlignment: Text.AlignVCenter
                    // elide: Text.ElideRight

                    // Appearance: so it doesn't look like a grey box
                    background: Rectangle { color: "transparent" }
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 12

                    // Important: Editable only if it is column 4 (Streak).
                    readOnly: (cellDelegate.column !== 4)

                    onEditingFinished: {
                        // The data is saved when the user presses Enter or clicks away.
                        tableView.model.setData(tableView.model.index(cellDelegate.row, cellDelegate.column), text, Qt.EditRole)
                    }

                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    enabled: (cellDelegate.column !== 4)

                    onClicked: function (mouse) {
                        practiceTracker.onRowClicked(cellDelegate.row, cellDelegate.column);
                    }

                    onDoubleClicked: function (mouse) {
                        practiceTracker.onRowDoubleClicked(cellDelegate.row, cellDelegate.column);
                    }
                }
            }
        }

        Label {
            visible: practiceTracker.journalModel.rowCount === 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: practiceTracker.songId > 0 ? qsTr("No entries for this exercise on the selected day yet.") : qsTr("No exercise selected.")
            font.pixelSize: 12
            color: Theme.textMuted
        }
    }
}
