import QtQuick
import QtQuick.Controls

import "../js/MediaKind.js" as MediaKind

Item {
    id: root

    required property string kind
    property real size: 16

    implicitWidth: size
    implicitHeight: size

    Image {
        anchors.fill: parent
        visible: MediaKind.iconSource(root.kind) !== ""
        source: MediaKind.iconSource(root.kind)
        fillMode: Image.PreserveAspectFit
        mipmap: true
    }

    Label {
        anchors.centerIn: parent
        visible: MediaKind.iconSource(root.kind) === ""
        text: MediaKind.icon(root.kind)
        font.pixelSize: Math.max(8, root.size * 0.75)
        color: MediaKind.accentColor(root.kind)
    }
}
