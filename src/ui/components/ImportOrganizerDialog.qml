pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root

    property var organizer: null

    title: qsTr("Organize import")
    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(Overlay.overlay.width - 48, 1100)
    height: Math.min(Overlay.overlay.height - 48, 720)
    closePolicy: Dialog.CloseOnEscape
    standardButtons: Dialog.NoButton

    onRejected: {
        if (organizer)
            organizer.cancel()
    }

    Connections {
        target: root.organizer
        enabled: root.organizer !== null

        function onActiveChanged() {
            if (root.organizer.active)
                root.open()
            else
                root.close()
        }
    }

    contentItem: ColumnLayout {
        spacing: 12
        width: root.availableWidth
        height: root.availableHeight

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Create folders on the right, then move selected files with >. Only mapped files will be imported.")
            color: Theme.textSecondary
        }

        Label {
            Layout.fillWidth: true
            visible: (organizer?.statusMessage ?? "").length > 0
            text: organizer?.statusMessage ?? ""
            color: Theme.textHint
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            // Source column
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                spacing: 6

                Label {
                    text: qsTr("Source")
                    font.weight: Font.DemiBold
                    color: Theme.textHeading
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    RadioButton {
                        text: qsTr("All")
                        checked: (organizer?.filterMode ?? 0) === 0
                        onClicked: organizer.filterMode = 0
                    }
                    RadioButton {
                        text: qsTr("Duplicates")
                        checked: (organizer?.filterMode ?? 0) === 1
                        onClicked: organizer.filterMode = 1
                    }
                    RadioButton {
                        text: qsTr("Errors")
                        checked: (organizer?.filterMode ?? 0) === 2
                        onClicked: organizer.filterMode = 2
                    }
                }

                DarkTextField {
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search…")
                    text: organizer?.searchText ?? ""
                    onTextEdited: organizer.searchText = text
                }

                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        Layout.fillWidth: true
                        flat: true
                        text: qsTr("Expand")
                        onClicked: organizer.expandAll(true)
                    }
                    Button {
                        Layout.fillWidth: true
                        flat: true
                        text: qsTr("Collapse")
                        onClicked: organizer.collapseAll(true)
                    }
                }

                Frame {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    padding: 0

                    ListView {
                        id: sourceList
                        anchors.fill: parent
                        clip: true
                        model: organizer?.sourceModel ?? null
                        currentIndex: -1

                        delegate: ItemDelegate {
                            id: sourceDelegate
                            required property int index
                            required property var model

                            width: ListView.view.width
                            highlighted: (organizer?.selectedSourceNodeId ?? -1) === model.nodeId

                            contentItem: RowLayout {
                                spacing: 6

                                Item { Layout.preferredWidth: Math.max(0, sourceDelegate.model.depth) * 14 }

                                Button {
                                    visible: sourceDelegate.model.isFolder
                                    flat: true
                                    text: sourceDelegate.model.expanded ? "▾" : "▸"
                                    Layout.preferredWidth: 28
                                    onClicked: organizer.toggleExpanded(true, sourceDelegate.model.nodeId)
                                }

                                Item {
                                    visible: !sourceDelegate.model.isFolder
                                    Layout.preferredWidth: 28
                                }

                                CheckBox {
                                    visible: !sourceDelegate.model.isFolder
                                    checked: sourceDelegate.model.checked
                                    enabled: sourceDelegate.model.status !== 2 && sourceDelegate.model.status !== 3
                                    onToggled: organizer.setChecked(sourceDelegate.model.nodeId, checked)
                                }

                                Label {
                                    Layout.fillWidth: true
                                    elide: Text.ElideMiddle
                                    text: sourceDelegate.model.isFolder
                                          ? sourceDelegate.model.name
                                          : (sourceDelegate.model.statusLabel.length > 0
                                             ? sourceDelegate.model.name + " (" + sourceDelegate.model.statusLabel + ")"
                                             : sourceDelegate.model.name)
                                    color: sourceDelegate.model.status === 1
                                           ? Theme.textHint
                                           : Theme.textPrimary
                                }
                            }

                            onClicked: organizer.selectedSourceNodeId = model.nodeId
                        }
                    }
                }
            }

            // Middle buttons
            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                spacing: 10

                Button {
                    text: qsTr("New folder")
                    onClicked: newFolderDialog.open()
                }

                Button {
                    text: ">"
                    enabled: !(organizer?.scanning ?? true)
                             && (organizer?.selectedSourceNodeId ?? -1) >= 0
                    onClicked: organizer.mapSelection(true)
                }

                Button {
                    text: qsTr("Contents >")
                    enabled: !(organizer?.scanning ?? true)
                             && (organizer?.selectedSourceNodeId ?? -1) >= 0
                    onClicked: organizer.mapSelectionContentsOnly()
                }

                Button {
                    text: "<"
                    enabled: !(organizer?.scanning ?? true)
                             && (organizer?.selectedTargetNodeId ?? 0) > 0
                    onClicked: organizer.unmapSelection()
                }
            }

            // Target column
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                spacing: 6

                Label {
                    text: qsTr("Target structure")
                    font.weight: Font.DemiBold
                    color: Theme.textHeading
                }

                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    text: organizer?.managedRootName ?? ""
                    color: Theme.textHint
                    font.pixelSize: 12
                }

                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        Layout.fillWidth: true
                        flat: true
                        text: qsTr("Expand")
                        onClicked: organizer.expandAll(false)
                    }
                    Button {
                        Layout.fillWidth: true
                        flat: true
                        text: qsTr("Collapse")
                        onClicked: organizer.collapseAll(false)
                    }
                }

                Frame {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    padding: 0

                    ListView {
                        id: targetList
                        anchors.fill: parent
                        clip: true
                        model: organizer?.targetModel ?? null

                        delegate: ItemDelegate {
                            id: targetDelegate
                            required property int index
                            required property var model

                            width: ListView.view.width
                            highlighted: (organizer?.selectedTargetNodeId ?? -1) === model.nodeId

                            contentItem: RowLayout {
                                spacing: 6

                                Item { Layout.preferredWidth: Math.max(0, targetDelegate.model.depth) * 14 }

                                Button {
                                    visible: targetDelegate.model.isFolder
                                    flat: true
                                    text: targetDelegate.model.expanded ? "▾" : "▸"
                                    Layout.preferredWidth: 28
                                    onClicked: organizer.toggleExpanded(false, targetDelegate.model.nodeId)
                                }

                                Item {
                                    visible: !targetDelegate.model.isFolder
                                    Layout.preferredWidth: 28
                                }

                                Label {
                                    Layout.fillWidth: true
                                    elide: Text.ElideMiddle
                                    text: targetDelegate.model.name
                                    color: Theme.textPrimary
                                }
                            }

                            onClicked: organizer.selectedTargetNodeId = model.nodeId
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            BusyIndicator {
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                running: organizer?.scanning ?? false
                visible: running
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("%1 planned, %2 still in source")
                        .arg(organizer?.plannedCount ?? 0)
                        .arg(organizer?.sourceCount ?? 0)
                color: Theme.textSecondary
            }

            Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }

            Button {
                text: qsTr("Import")
                enabled: organizer?.canCommit ?? false
                highlighted: true
                onClicked: organizer.commit()
            }
        }
    }

    Dialog {
        id: newFolderDialog
        title: qsTr("New folder")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: 360

        onAccepted: {
            const name = folderNameField.text.trim()
            if (name.length > 0)
                organizer.addFolder(name)
            folderNameField.text = ""
        }

        contentItem: ColumnLayout {
            spacing: 8
            width: newFolderDialog.availableWidth

            DarkTextField {
                id: folderNameField
                Layout.fillWidth: true
                placeholderText: qsTr("Folder name")
                onAccepted: newFolderDialog.accept()
            }
        }
    }

}
