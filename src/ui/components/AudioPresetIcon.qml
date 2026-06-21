import QtQuick

import "../js/MediaKind.js" as MediaKind

Item {
    id: root

    property real size: 16
    property color color: MediaKind.accentColor(MediaKind.Audio)

    implicitWidth: size
    implicitHeight: size

    readonly property var sliderPositions: [0.28, 0.58, 0.42]

    Repeater {
        model: 3

        Item {
            required property int index

            readonly property real trackWidth: Math.max(2, root.size * 0.13)
            readonly property real thumbHeight: Math.max(2, root.size * 0.24)
            readonly property real pos: root.sliderPositions[index]

            x: (root.width / 4) * (index + 0.5) - trackWidth / 2
            width: trackWidth
            height: root.height

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.trackWidth
                height: parent.height
                radius: width / 2
                color: root.color
                opacity: 0.3
            }

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                y: parent.pos * (parent.height - parent.thumbHeight)
                width: parent.trackWidth * 2.4
                height: parent.thumbHeight
                radius: height / 2
                color: root.color
            }
        }
    }
}
