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

    signal backRequested()

    background: Rectangle {
        color: Theme.toolbarBackground
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
                text: root.title
                font.pixelSize: 18
                font.weight: Font.DemiBold
                color: Theme.textToolbar
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                visible: root.subtitle.length > 0
                text: root.subtitle
                font.pixelSize: 12
                color: Theme.textHint
                elide: Text.ElideMiddle
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

        Rectangle {
            visible: root.sessionLocked
            radius: 4
            color: Theme.danger
            implicitWidth: sessionLabel.implicitWidth + 16
            implicitHeight: 24

            Label {
                id: sessionLabel
                anchors.centerIn: parent
                text: root.sessionLabel
                font.pixelSize: 11
                font.weight: Font.Bold
                color: Theme.textOnAccent
            }
        }
    }
}
