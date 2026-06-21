import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

Item {
    id: root

    default property alias content: panelBox.contentData

    property string title: ""
    property string settingsCategory: "ResizableSidePanel"
    property string widthSettingsKey: "width"
    property string collapsedSettingsKey: "collapsed"
    property int minWidth: 180
    property int maxWidth: 520
    property int defaultWidth: 280
    readonly property int collapsedWidth: 36

    property int userWidth: defaultWidth
    property bool collapsed: false

    readonly property int effectiveWidth: collapsed ? collapsedWidth : userWidth

    Layout.preferredWidth: effectiveWidth
    Layout.minimumWidth: effectiveWidth
    Layout.maximumWidth: effectiveWidth
    Layout.fillHeight: true

    Settings {
        id: prefs
        location: appSettings.settingsFileLocation
        category: root.settingsCategory

        Component.onCompleted: {
            root.userWidth = prefs.value(root.widthSettingsKey, root.defaultWidth)
            root.collapsed = prefs.value(root.collapsedSettingsKey, false)
        }
    }

    function saveWidth() {
        prefs.setValue(widthSettingsKey, userWidth)
    }

    function saveCollapsed() {
        prefs.setValue(collapsedSettingsKey, collapsed)
    }

    function toggleCollapsed() {
        collapsed = !collapsed
        saveCollapsed()
    }

    GroupBox {
        id: panelBox
        anchors.fill: parent
        visible: !root.collapsed

        background: Rectangle {
            radius: 10
            color: Theme.panelBackground
            border.color: Theme.border
        }

        label: RowLayout {
            spacing: 4

            Label {
                text: root.title
                color: Theme.textPrimary
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            ToolButton {
                text: "«"
                display: AbstractButton.TextOnly
                font.pixelSize: 14
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Collapse panel")
                onClicked: root.toggleCollapsed()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.collapsed
        radius: 10
        color: Theme.panelBackground
        border.color: Theme.border

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 8

            ToolButton {
                Layout.alignment: Qt.AlignHCenter
                text: "›"
                display: AbstractButton.TextOnly
                font.pixelSize: 14
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Expand panel")
                onClicked: root.toggleCollapsed()
            }

            Item { Layout.fillHeight: true }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: root.title
                rotation: -90
                transformOrigin: Item.Center
                color: Theme.textSecondary
                font.pixelSize: 11
            }

            Item { Layout.fillHeight: true }
        }
    }

    Rectangle {
        id: resizeGrip
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 6
        visible: !root.collapsed
        z: 10
        color: resizeHover.hovered || resizeDrag.active ? Theme.borderFocus : "transparent"

        HoverHandler { id: resizeHover }

        DragHandler {
            id: resizeDrag
            cursorShape: Qt.SizeHorCursor
            property real widthAtStart: 0

            onActiveChanged: {
                if (active) {
                    widthAtStart = root.userWidth
                } else {
                    root.saveWidth()
                }
            }

            onTranslationChanged: {
                root.userWidth = Math.max(root.minWidth,
                                          Math.min(root.maxWidth, widthAtStart + translation.x))
            }
        }
    }
}
