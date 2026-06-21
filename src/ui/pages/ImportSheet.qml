import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import com.sonarp.sonarpractice

Drawer {
    id: root

    signal pickFilesRequested()

    edge: Qt.BottomEdge
    height: parent.height * 0.55
    modal: true
    interactive: true

    signal importPaths(var paths)

    background: Rectangle {
        color: Theme.toolbarBackground
        topLeftRadius: 16
        topRightRadius: 16
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        Label {
            text: qsTr("Import media")
            font.pixelSize: 20
            font.weight: Font.Bold
            color: Theme.textHeading
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("No wizard — drop files or folders here. "
                       + "ImportService will group them into exercises later.")
            font.pixelSize: 13
            color: Theme.textSecondary
        }

        DropArea {
            id: dropArea
            Layout.fillWidth: true
            Layout.fillHeight: true

            onEntered: (drag) => drag.accept(Qt.CopyAction)
            onDropped: (drop) => {
                var paths = []
                if (drop.hasUrls) {
                    for (var i = 0; i < drop.urls.length; ++i) {
                        paths.push(Qt.resolvedUrl(drop.urls[i]).toString().replace("file://", ""))
                    }
                }
                if (paths.length > 0) {
                    root.importPaths(paths)
                    root.close()
                }
            }

            Rectangle {
                anchors.fill: parent
                radius: 12
                color: dropArea.containsDrag ? Theme.dropZoneDragBackground : Theme.dropZoneBackground
                border.color: dropArea.containsDrag ? Theme.borderAccent : Theme.borderSubtle
                border.width: 2

                Label {
                    anchors.centerIn: parent
                    text: qsTr("Drop files here\n(GP, PDF, audio, images, URLs)")
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 14
                    color: Theme.textDropHint
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Button {
                text: qsTr("Cancel")
                onClicked: root.close()
            }

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Import files")
                highlighted: true
                onClicked: {
                    root.pickFilesRequested()
                    root.close()
                }
            }
        }
    }
}
