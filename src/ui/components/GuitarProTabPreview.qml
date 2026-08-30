// src/ui/components/GuitarProTabPreview.qml

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    id: root

    title: qsTr("Guitar Pro preview")
    visible: guitarProPreviewController.visible
    padding: 12

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
        width: parent.width
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: {
                    const parts = []
                    if (guitarProPreviewController.title.length > 0)
                        parts.push(guitarProPreviewController.title)
                    if (guitarProPreviewController.artist.length > 0)
                        parts.push(guitarProPreviewController.artist)
                    return parts.join(" — ")
                }
                elide: Text.ElideRight
                Layout.fillWidth: true
                color: Theme.textPrimary
                font.bold: true
            }

            Button {
                text: qsTr("Close")
                flat: true
                onClicked: guitarProPreviewController.clear()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: guitarProPreviewController.loaded && !guitarProPreviewController.loading

            Label {
                text: qsTr("Track")
                color: Theme.textSecondary
            }

            ComboBox {
                id: trackCombo
                Layout.preferredWidth: 280
                model: guitarProPreviewController.trackNames
                onActivated: guitarProPreviewController.selectedTrackIndex = currentIndex

                Binding on currentIndex {
                    value: Math.max(0, guitarProPreviewController.selectedTrackIndex)
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }

            Label {
                text: guitarProPreviewController.tuning.length > 0
                      ? qsTr("Tuning: %1").arg(guitarProPreviewController.tuning)
                      : ""
                color: Theme.textSecondary
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: guitarProPreviewController.loading
            visible: running
        }

        Label {
            Layout.fillWidth: true
            visible: guitarProPreviewController.errorMessage.length > 0
            wrapMode: Text.WordWrap
            text: guitarProPreviewController.errorMessage
            color: Theme.error
        }

        ScrollView {
            id: tabScroll
            Layout.fillWidth: true
            Layout.preferredHeight: 220
            Layout.minimumHeight: 120
            clip: true
            visible: guitarProPreviewController.tabText.length > 0
                     && !guitarProPreviewController.loading

            ScrollBar.horizontal.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            TextArea {
                id: tabText
                readOnly: true
                wrapMode: TextEdit.NoWrap
                text: guitarProPreviewController.tabText
                color: Theme.textPrimary
                selectedTextColor: Theme.textOnAccent
                selectionColor: Theme.accent
                background: Rectangle {
                    color: Theme.editorBackground
                    border.color: Theme.border
                    radius: 4
                }
                font.family: "monospace"
                font.pixelSize: 13
            }
        }
    }
}
