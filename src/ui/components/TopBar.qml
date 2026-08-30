import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ToolBar {
    id: root

    required property string title
    property string subtitle: ""
    property bool showBack: false
    property bool showThemeToggle: true
    property bool sessionLocked: false
    property string sessionLabel: qsTr("Session active")
    /** When true (default while sessionLocked), show a Stop-training control. */
    property bool showStopSession: sessionLocked
    /** Centered elapsed time while a training timer is running. */
    property bool showSessionTimer: sessionLocked

    signal backRequested()
    signal stopSessionRequested()

    background: Rectangle {
        color: Theme.toolbarBackground
    }

    // True horizontal center — visible on Hub and internal player while training.
    Label {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        z: 1
        visible: root.showSessionTimer
        text: practiceTracker.elapsedDisplay
        font.pixelSize: 20
        font.weight: Font.Bold
        font.family: "monospace"
        color: Theme.accentTimer
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 4
        anchors.rightMargin: 8
        spacing: 8

        ToolButton {
            visible: root.showBack
            enabled: !root.sessionLocked
            text: "\u2190"
            font.pixelSize: 18
            palette.buttonText: Theme.textToolbar
            onClicked: root.backRequested()
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
                Layout.fillWidth: true
                // Leave room for the centered timer so long titles don't cover it.
                Layout.rightMargin: root.showSessionTimer ? 72 : 0
                text: root.title
                font.pixelSize: 18
                font.weight: Font.DemiBold
                color: Theme.textToolbar
                elide: Text.ElideRight
            }
        }

        ToolButton {
            visible: root.showThemeToggle
            text: Theme.isDark ? "\u2600" : "\u263E"
            font.pixelSize: 18
            palette.buttonText: Theme.textToolbar
            ToolTip.visible: hovered
            ToolTip.text: Theme.isDark ? qsTr("Light theme") : qsTr("Dark theme")
            onClicked: Theme.toggle()
        }

        Button {
            id: stopSessionButton
            visible: root.showStopSession
            text: qsTr("Stop training")
            flat: true
            padding: 6
            leftPadding: 10
            rightPadding: 10
            font.pixelSize: 11
            font.weight: Font.Bold
            palette.buttonText: Theme.textOnAccent

            background: Rectangle {
                radius: 4
                color: stopSessionButton.down ? Qt.darker(Theme.danger, 1.15) : Theme.danger
            }

            ToolTip.visible: hovered
            ToolTip.text: root.sessionLabel

            onClicked: root.stopSessionRequested()
        }
    }
}
