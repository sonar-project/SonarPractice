pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property var linkModel
    property string selectedPath: ""
    property string folderSearchText: ""
    property string customPathText: ""

    signal pathSelected(string path)

    readonly property var treeController: treeLogic

    implicitHeight: 200

    function rebuildTree() {
        treeLogic.rebuild(linkModel.distinctFolderPaths())
    }

    readonly property bool hasInputFocus: folderSearchField.activeFocus || customPathField.activeFocus

    function selectPath(path) {
        selectedPath = path
        customPathText = path
        pathSelected(path)
    }

    function applyCustomPath() {
        const trimmed = customPathField.text.trim()
        customPathText = trimmed
        selectPath(trimmed)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        DarkTextField {
            id: folderSearchField
            Layout.fillWidth: true
            placeholderText: qsTr("Search folders…")
            text: root.folderSearchText
            onTextEdited: {
                root.folderSearchText = text
                treeLogic.refreshFlatList()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            DarkTextField {
                id: customPathField
                Layout.fillWidth: true
                placeholderText: qsTr("Custom path…")
                text: root.customPathText
                onTextEdited: root.customPathText = text
                onAccepted: root.applyCustomPath()
            }

            Button {
                text: qsTr("OK")
                onClicked: root.applyCustomPath()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Button {
                Layout.fillWidth: true
                text: qsTr("Expand all")
                flat: true
                font.pixelSize: 11
                onClicked: treeLogic.expandAll()
            }

            Button {
                Layout.fillWidth: true
                text: qsTr("Collapse all")
                flat: true
                font.pixelSize: 11
                onClicked: treeLogic.collapseAll()
            }
        }

        ListView {
            id: folderTreeList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: treeLogic.flatModel

            delegate: ItemDelegate {
                id: folderRow
                width: folderTreeList.width
                height: 28
                highlighted: root.selectedPath === folderRow.path

                required property int depth
                required property string path
                required property string name
                required property bool hasChildren
                required property bool expanded
                required property int fileCount
                required property bool visibleInSearch

                onClicked: root.selectPath(folderRow.path)

                contentItem: RowLayout {
                    spacing: 4

                    Item { implicitWidth: Math.max(0, folderRow.depth) * 14; implicitHeight: 1 }

                    Label {
                        // implicitWidth: 14
                        horizontalAlignment: Text.AlignHCenter
                        text: folderRow.hasChildren ? (folderRow.expanded ? "▼" : "▶") : ""
                        font.pixelSize: 9
                        color: Theme.textSecondary

                        TapHandler {
                            enabled: folderRow.hasChildren
                            onTapped: treeLogic.toggleExpanded(folderRow.path)
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: folderRow.name
                        elide: Text.ElideRight
                        color: Theme.textPrimary
                        font.pixelSize: 12
                    }

                    Label {
                        text: folderRow.fileCount > 0 ? String(folderRow.fileCount) : ""
                        font.pixelSize: 10
                        color: Theme.textMuted
                    }
                }

                ToolTip.visible: folderRow.hovered && folderRow.path.length > 0
                ToolTip.text: folderRow.path
                ToolTip.delay: 400

                background: Rectangle {
                    color: folderRow.highlighted ? Theme.panelBackgroundNested
                                                 : (folderRow.hovered ? Theme.toolbarButtonHover : "transparent")
                    radius: 4
                }
            }
        }
    }

    QtObject {
        id: treeLogic

        property ListModel flatModel: ListModel {}

        property var nodesByPath: ({})
        property var rootChildren: []
        property var expandedPaths: ({})

        function rebuild(paths) {
            nodesByPath = {}
            rootChildren = []

            let hasRootFiles = false
            for (let i = 0; i < paths.length; ++i) {
                if (paths[i] === "/") {
                    hasRootFiles = true
                } else {
                    insertPath(paths[i])
                }
            }

            if (hasRootFiles) {
                const rootNode = {
                    path: "/",
                    name: qsTr("Root directory"),
                    children: []
                }
                nodesByPath["/"] = rootNode
                rootChildren.push(rootNode)
            }

            sortChildren(rootChildren)
            for (const path in nodesByPath) {
                sortChildren(nodesByPath[path].children)
            }

            expandedPaths[""] = true
            refreshFlatList()
        }

        function insertPath(fullPath) {
            if (!fullPath || fullPath === "/")
                return

            const parts = fullPath.split("/").filter(function (part) { return part.length > 0 })
            let currentPath = ""
            let parentChildren = rootChildren

            for (let i = 0; i < parts.length; ++i) {
                currentPath = currentPath ? currentPath + "/" + parts[i] : parts[i]

                if (!nodesByPath[currentPath]) {
                    const node = {
                        path: currentPath,
                        name: parts[i],
                        children: []
                    }
                    nodesByPath[currentPath] = node
                    parentChildren.push(node)
                }

                parentChildren = nodesByPath[currentPath].children
            }
        }

        function sortChildren(children) {
            children.sort(function (a, b) {
                return a.name.localeCompare(b.name, Qt.locale(), { sensitivity: "base" })
            })
        }

        function nodeMatchesSearch(node) {
            const query = root.folderSearchText.trim().toLowerCase()
            if (query.length === 0)
                return true

            if (node.name.toLowerCase().indexOf(query) >= 0)
                return true
            if (node.path.toLowerCase().indexOf(query) >= 0)
                return true

            for (let i = 0; i < node.children.length; ++i) {
                if (subtreeMatchesSearch(node.children[i], query))
                    return true
            }
            return false
        }

        function subtreeMatchesSearch(node, query) {
            if (node.name.toLowerCase().indexOf(query) >= 0)
                return true
            if (node.path.toLowerCase().indexOf(query) >= 0)
                return true

            for (let i = 0; i < node.children.length; ++i) {
                if (subtreeMatchesSearch(node.children[i], query))
                    return true
            }
            return false
        }

        function refreshFlatList() {
            flatModel.clear()

            const query = root.folderSearchText.trim()
            const searching = query.length > 0

            flatModel.append({
                depth: 0,
                path: "",
                name: qsTr("All folders"),
                hasChildren: rootChildren.length > 0,
                expanded: true,
                fileCount: root.linkModel.fileCountForFolder("", true),
                visibleInSearch: true
            })

            appendVisibleNodes(rootChildren, 0, searching)
        }

        function appendVisibleNodes(children, depth, searching) {
            for (let i = 0; i < children.length; ++i) {
                const node = children[i]
                if (searching && !nodeMatchesSearch(node))
                    continue

                const hasChildren = node.children.length > 0
                const expanded = searching || !!expandedPaths[node.path]

                flatModel.append({
                    depth: depth + 1,
                    path: node.path,
                    name: node.name,
                    hasChildren: hasChildren,
                    expanded: expanded,
                    fileCount: root.linkModel.fileCountForFolder(node.path, true),
                    visibleInSearch: true
                })

                if (hasChildren && expanded)
                    appendVisibleNodes(node.children, depth + 1, searching)
            }
        }

        function toggleExpanded(path) {
            if (expandedPaths[path]) {
                delete expandedPaths[path]
            } else {
                expandedPaths[path] = true
            }
            refreshFlatList()
        }

        function expandAll() {
            for (const path in nodesByPath)
                expandedPaths[path] = true
            refreshFlatList()
        }

        function collapseAll() {
            expandedPaths = { "": true }
            refreshFlatList()
        }
    }

    Connections {
        target: root.linkModel
        function onModelReset() {
            root.rebuildTree()
        }
    }

    Component.onCompleted: rebuildTree()
}
