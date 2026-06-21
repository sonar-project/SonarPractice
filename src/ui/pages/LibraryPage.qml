import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../components"
import "../js/MediaKind.js" as MediaKind

Page {
    id: root

    required property bool sessionLocked

    focus: true

    signal backRequested()
    signal songSelected(int songId, string title, int baseBpm)

    background: Rectangle { color: Theme.windowBackground }

    property var selectedSongIds: []
    property string linkTitle: ""
    property int primarySongId: 0
    property bool editingExistingGroup: false
    property int editingGroupId: 0
    property var selectedUnlinkedIds: []
    property int selectedUnlinkedCount: 0
    property bool linkBusy: false
    property string linkStatusText: ""

    readonly property bool nothingSelected: selectedSongIds.length === 0 && !editingExistingGroup

    readonly property bool allVisibleUnlinkedSelected: {
        if (editingExistingGroup || sessionLocked)
            return false

        const visibleIds = libraryLinkModel.visibleUnlinkedSongIds()
        if (visibleIds.length === 0 || selectedSongIds.length !== visibleIds.length)
            return false

        for (let i = 0; i < visibleIds.length; ++i) {
            if (selectedSongIds.indexOf(visibleIds[i]) < 0)
                return false
        }
        return true
    }

    function linkTitleForPrimary(songId) {
        const fromGroupTitle = linkTitle.trim()
        if (fromGroupTitle.length > 0)
            return fromGroupTitle
        return libraryLinkModel.titleForSong(songId)
    }

    function refreshUnlinkedSelection() {
        const ids = []
        for (let i = 0; i < selectedSongIds.length; ++i) {
            const songId = selectedSongIds[i]
            if (!libraryLinkModel.isSongLinked(songId))
                ids.push(songId)
        }
        selectedUnlinkedIds = ids
        selectedUnlinkedCount = ids.length
    }

    function clearSelection() {
        selectedSongIds = []
        linkTitle = ""
        primarySongId = 0
        editingExistingGroup = false
        editingGroupId = 0
        refreshUnlinkedSelection()
    }

    function toggleSelection(songId) {
        if (libraryLinkModel.isSongLinked(songId))
            return

        const index = selectedSongIds.indexOf(songId)
        if (index >= 0) {
            selectedSongIds.splice(index, 1)
        } else {
            selectedSongIds.push(songId)
        }
        selectedSongIds = selectedSongIds.slice()
        updateLinkTitle()
        refreshUnlinkedSelection()
    }

    function handleFileRowTap(songId, isLinked) {
        if (isLinked) {
            const info = linkGroupService.groupInfoForSong(songId)
            if (editingExistingGroup && editingGroupId === info.groupId) {
                clearSelection()
                fileList.forceActiveFocus()
                return
            }
            loadGroupForEditing(songId)
            return
        }
        if (editingExistingGroup)
            clearSelection()
        toggleSelection(songId)
    }

    function selectAllVisible() {
        if (sessionLocked)
            return

        const visibleIds = libraryLinkModel.visibleUnlinkedSongIds()
        if (visibleIds.length === 0)
            return

        editingExistingGroup = false
        editingGroupId = 0
        selectedSongIds = libraryLinkModel.orderSongIdsForLinking(visibleIds)
        primarySongId = selectedSongIds[0]
        linkTitle = linkTitleForPrimary(primarySongId)
        refreshUnlinkedSelection()
        fileList.forceActiveFocus()
    }

    function finishLinkBusy() {
        linkBusy = false
        linkStatusText = ""
    }

    function beginLinkBusy(message, fileCount) {
        linkStatusText = message
        linkBusy = true
        libraryBusyOverlay.fileCount = fileCount
    }

    function linkAllVisible() {
        if (linkBusy || sessionLocked)
            return

        const visibleIds = libraryLinkModel.visibleUnlinkedSongIds()
        if (visibleIds.length < 2)
            return

        beginLinkBusy(qsTr("Linking visible files…"), visibleIds.length)

        Qt.callLater(() => {
            selectAllVisible()
            if (selectedSongIds.length < 2) {
                finishLinkBusy()
                return
            }
            createLinkGroup(finishLinkBusy)
        })
    }

    function setPrimary(songId) {
        const index = selectedSongIds.indexOf(songId)
        if (index <= 0)
            return
        selectedSongIds.splice(index, 1)
        selectedSongIds.unshift(songId)
        selectedSongIds = selectedSongIds.slice()
        primarySongId = songId
        refreshUnlinkedSelection()
    }

    function updateLinkTitle() {
        if (selectedSongIds.length === 0) {
            if (!editingExistingGroup) {
                linkTitle = ""
                primarySongId = 0
            }
            return
        }
        primarySongId = selectedSongIds[0]
        if (linkTitle.trim().length === 0)
            linkTitle = linkTitleForPrimary(primarySongId)
        refreshUnlinkedSelection()
    }

    function loadGroupForEditing(songId) {
        const info = linkGroupService.groupInfoForSong(songId)
        if (!info.primarySongId)
            return

        editingExistingGroup = true
        editingGroupId = info.groupId
        primarySongId = info.primarySongId
        linkTitle = info.title

        const members = info.memberSongIds || []
        selectedSongIds = []
        for (let i = 0; i < members.length; ++i)
            selectedSongIds.push(members[i])
        selectedSongIds = selectedSongIds.slice()
        refreshUnlinkedSelection()
    }

    function createLinkGroup(onFinished) {
        if (selectedSongIds.length < 2 || linkTitle.trim().length === 0) {
            if (onFinished)
                onFinished()
            return
        }

        const memberSongIds = selectedSongIds.slice()
        const secondary = selectedSongIds.slice(1)
        const groupId = linkGroupService.createGroupFromSongs(
            linkTitle.trim(), primarySongId, secondary)

        if (groupId > 0) {
            libraryLinkModel.updateSongsLinkState(groupId, memberSongIds)
            clearSelection()
            fileList.forceActiveFocus()
            Qt.callLater(() => songModel.reload())
        }

        if (onFinished)
            onFinished()
    }

    function saveGroupTitle() {
        if (!editingExistingGroup || linkTitle.trim().length === 0)
            return
        if (linkGroupService.updateGroupTitle(primarySongId, linkTitle.trim())) {
            libraryLinkModel.reload()
            songModel.reload()
        }
    }

    function dissolveCurrentGroup(onFinished) {
        if (linkBusy || sessionLocked) {
            if (onFinished)
                onFinished()
            return
        }

        const targetSongId = primarySongId
        if (targetSongId <= 0) {
            if (onFinished)
                onFinished()
            return
        }

        beginLinkBusy(qsTr("Unlinking…"), 0)

        Qt.callLater(() => {
            const dissolvedSongIds = linkGroupService.dissolveGroupForSong(targetSongId)
            if (dissolvedSongIds.length > 0) {
                libraryBusyOverlay.fileCount = dissolvedSongIds.length
                libraryLinkModel.clearSongsLinkState(dissolvedSongIds)
                clearSelection()
                fileList.forceActiveFocus()
                Qt.callLater(() => songModel.reload())
            }
            finishLinkBusy()
            if (onFinished)
                onFinished()
        })
    }

    function addSelectionToGroup() {
        const groupId = targetGroupCombo.currentGroupId
        const songIds = selectedUnlinkedIds
        if (groupId <= 0 || songIds.length === 0)
            return

        if (linkGroupService.addSongsToGroup(groupId, songIds)) {
            libraryLinkModel.updateSongsLinkState(groupId, songIds)
            clearSelection()
            songModel.reload()
        }
    }

    ListModel {
        id: groupPickerModel

        function refresh() {
            clear()
            const groups = linkGroupService.allGroups()
            for (let i = 0; i < groups.length; ++i) {
                const group = groups[i]
                append({
                    groupId: group.groupId,
                    displayTitle: group.title + " (" + group.memberCount + ")"
                })
            }
        }
    }

    header: TopBar {
        title: qsTr("Library")
        showBack: true
        sessionLocked: root.sessionLocked
        onBackRequested: root.backRequested()
    }

    Shortcut {
        sequences: [StandardKey.SelectAll, "Ctrl+A"]
        context: Qt.WindowShortcut
        enabled: !searchField.activeFocus
                 && !linkTitleField.activeFocus
                 && !folderTree.hasInputFocus
                 && !root.sessionLocked
                 && !root.editingExistingGroup
        onActivated: root.linkAllVisible()
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        ResizableSidePanel {
            id: folderPanel
            title: qsTr("Folder")
            settingsCategory: "LibraryPage"
            widthSettingsKey: "folderPanelWidth"
            collapsedSettingsKey: "folderPanelCollapsed"
            defaultWidth: 280
            minWidth: 200
            maxWidth: 480

            LibraryFolderTree {
                id: folderTree
                anchors.fill: parent
                linkModel: libraryLinkModel
                selectedPath: libraryLinkModel.folderFilter
                onPathSelected: (path) => libraryLinkModel.folderFilter = path
            }
        }

        GroupBox {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: qsTr("Files")

            background: Rectangle {
                radius: 10
                color: Theme.panelBackground
                border.color: Theme.border
            }

            label: Label {
                text: parent.title
                color: Theme.textPrimary
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: qsTr("Select multiple files by clicking (each row on/off). "
                               + "Ctrl+A or “Link visible” creates a group "
                               + "from all matches of the current search.")
                    font.pixelSize: 12
                    color: Theme.textSecondary
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    DarkTextField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Search… e.g. Picking Licks && mp4 || pdf")
                        text: libraryLinkModel.searchText
                        onTextEdited: libraryLinkModel.searchText = text
                    }

                    Button {
                        text: qsTr("Link visible")
                        enabled: libraryLinkModel.visibleUnlinkedCount >= 2
                                 && !root.editingExistingGroup
                                 && !root.sessionLocked
                                 && !root.linkBusy
                        onClicked: root.linkAllVisible()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    CheckBox {
                        text: qsTr("Select all")
                        enabled: !root.sessionLocked
                                 && libraryLinkModel.visibleUnlinkedCount > 0
                                 && !root.editingExistingGroup
                        checked: root.allVisibleUnlinkedSelected
                        onToggled: {
                            if (checked)
                                root.selectAllVisible()
                            else if (root.allVisibleUnlinkedSelected)
                                root.clearSelection()
                        }
                    }

                    CheckBox {
                        text: qsTr("Select none")
                        enabled: !root.sessionLocked
                        checked: root.nothingSelected
                        onToggled: {
                            if (checked)
                                root.clearSelection()
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    CheckBox {
                        text: qsTr("Hide groups")
                        checked: libraryLinkModel.hideContainers
                        onToggled: libraryLinkModel.hideContainers = checked
                    }

                    CheckBox {
                        text: qsTr("Show groups only")
                        checked: libraryLinkModel.containersOnly
                        onToggled: libraryLinkModel.containersOnly = checked
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ListView {
                        id: fileList
                        model: libraryLinkModel
                        spacing: 4
                        focus: true
                        activeFocusOnTab: true

                        Keys.onPressed: (event) => {
                            if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_A) {
                                event.accepted = true
                                root.linkAllVisible()
                            }
                        }

                        delegate: ItemDelegate {
                            id: fileRow
                            required property int songId
                            required property string title
                            required property string sourceRelativePath
                            required property string mediaKind
                            required property bool isLinked
                            required property bool isPrimary
                            required property string linkGroupTitle

                            width: fileList.width
                            highlighted: selectedSongIds.indexOf(fileRow.songId) >= 0

                            TapHandler {
                                acceptedButtons: Qt.LeftButton
                                onTapped: root.handleFileRowTap(fileRow.songId, fileRow.isLinked)
                            }

                            contentItem: RowLayout {
                                spacing: 8

                                CheckBox {
                                    visible: !fileRow.isLinked
                                    enabled: false
                                    checked: selectedSongIds.indexOf(fileRow.songId) >= 0
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Label {
                                        Layout.fillWidth: true
                                        text: fileRow.title
                                        font.weight: Font.DemiBold
                                        color: Theme.textHeading
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: fileRow.sourceRelativePath.length > 0
                                              ? fileRow.sourceRelativePath
                                              : qsTr("No import path")
                                        font.pixelSize: 11
                                        color: Theme.textSecondary
                                        elide: Text.ElideMiddle
                                    }

                                    Label {
                                        visible: fileRow.isLinked
                                        text: fileRow.isPrimary
                                              ? qsTr("Group primary: %1").arg(fileRow.linkGroupTitle)
                                              : qsTr("In group: %1").arg(fileRow.linkGroupTitle)
                                        font.pixelSize: 11
                                        color: Theme.accent
                                    }

                                    Label {
                                        visible: !fileRow.isLinked
                                                 && selectedSongIds.indexOf(fileRow.songId) === 0
                                                 && selectedSongIds.length > 1
                                        text: qsTr("Primary file")
                                        font.pixelSize: 11
                                        color: Theme.link
                                    }
                                }

                                Label {
                                    text: MediaKind.icon(fileRow.mediaKind)
                                    color: MediaKind.accentColor(fileRow.mediaKind)
                                }
                            }
                        }
                    }
                }
            }
        }

        GroupBox {
            Layout.preferredWidth: 300
            Layout.fillHeight: true
            title: editingExistingGroup ? qsTr("Edit group") : qsTr("New group")

            background: Rectangle {
                radius: 10
                color: Theme.panelBackground
                border.color: Theme.border
            }

            label: Label {
                text: parent.title
                color: Theme.textPrimary
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: editingExistingGroup
                          ? qsTr("Edit or dissolve group. Save title with “Save”.")
                          : (selectedSongIds.length < 2
                             ? qsTr("Select at least two unlinked files.")
                             : qsTr("%1 files — first is primary.").arg(selectedSongIds.length))
                    font.pixelSize: 12
                    color: Theme.textSecondary
                }

                DarkTextField {
                    id: linkTitleField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Group title")
                    text: root.linkTitle
                    onTextEdited: root.linkTitle = text
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    Column {
                        width: parent.width
                        spacing: 4
                        Repeater {
                            model: selectedSongIds
                            delegate: RowLayout {
                                width: parent.width
                                spacing: 6
                                Label {
                                    Layout.fillWidth: true
                                    text: (index === 0 ? "★ " : "• ")
                                          + libraryLinkModel.titleForSong(modelData)
                                    font.pixelSize: 12
                                    color: index === 0 ? Theme.link : Theme.textDropHint
                                }
                                Button {
                                    visible: index > 0 && !editingExistingGroup
                                    text: qsTr("Primary")
                                    onClicked: root.setPrimary(modelData)
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        Layout.fillWidth: true
                        visible: !editingExistingGroup
                        text: root.linkBusy
                              ? qsTr("Linking…")
                              : qsTr("Link visible files")
                        enabled: libraryLinkModel.visibleUnlinkedCount >= 2
                                 && !root.sessionLocked
                                 && !root.linkBusy
                        onClicked: root.linkAllVisible()
                    }

                    GroupBox {
                        Layout.fillWidth: true
                        visible: !editingExistingGroup && selectedSongIds.length > 0
                        title: qsTr("Add to group")

                        background: Rectangle {
                            radius: 8
                            color: Theme.cardBackground
                            border.color: Theme.borderSubtle
                        }

                        label: Label {
                            text: parent.title
                            color: Theme.textPrimary
                            font.pixelSize: 12
                        }

                        ColumnLayout {
                            width: parent.width
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: selectedUnlinkedCount > 0
                                      ? qsTr("%1 unlinked file(s) selected.").arg(selectedUnlinkedCount)
                                      : qsTr("Selection contains no unlinked files.")
                                font.pixelSize: 12
                                color: Theme.textSecondary
                            }

                            ComboBox {
                                id: targetGroupCombo
                                Layout.fillWidth: true
                                enabled: groupPickerModel.count > 0
                                model: groupPickerModel
                                textRole: "displayTitle"

                                readonly property int currentGroupId: {
                                    if (currentIndex < 0 || currentIndex >= groupPickerModel.count)
                                        return 0
                                    return groupPickerModel.get(currentIndex).groupId
                                }

                                displayText: groupPickerModel.count > 0
                                             ? (currentIndex >= 0 ? currentText : qsTr("Choose group…"))
                                             : qsTr("No groups available")
                            }

                            Button {
                                Layout.fillWidth: true
                                text: qsTr("Add")
                                enabled: targetGroupCombo.currentGroupId > 0
                                         && selectedUnlinkedCount > 0
                                         && !root.sessionLocked
                                onClicked: root.addSelectionToGroup()
                            }
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        visible: !editingExistingGroup
                        text: qsTr("Create group")
                        enabled: selectedSongIds.length >= 2 && linkTitle.trim().length > 0
                                 && !root.linkBusy
                        onClicked: {
                            beginLinkBusy(qsTr("Creating group…"), selectedSongIds.length)
                            Qt.callLater(() => root.createLinkGroup(root.finishLinkBusy))
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        visible: editingExistingGroup
                        text: qsTr("Save title")
                        enabled: linkTitle.trim().length > 0
                        onClicked: root.saveGroupTitle()
                    }

                    Button {
                        Layout.fillWidth: true
                        text: root.linkBusy
                              ? qsTr("Unlinking…")
                              : qsTr("Unlink")
                        enabled: (editingExistingGroup || (primarySongId > 0
                                                          && libraryLinkModel.isSongLinked(primarySongId)))
                                 && !root.linkBusy
                                 && !root.sessionLocked
                        onClicked: root.dissolveCurrentGroup()
                    }

                    Button {
                        Layout.fillWidth: true
                        visible: selectedSongIds.length > 0 || editingExistingGroup
                        text: qsTr("Reset selection")
                        onClicked: root.clearSelection()
                    }

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Go to exercise")
                        enabled: primarySongId > 0
                        onClicked: root.songSelected(
                            primarySongId,
                            linkTitle.length > 0 ? linkTitle : libraryLinkModel.titleForSong(primarySongId),
                            0)
                    }
                }
            }
        }
    }

    Connections {
        target: linkGroupService
        function onGroupsChanged() {
            groupPickerModel.refresh()
        }
    }

    LibraryBusyOverlay {
        id: libraryBusyOverlay
        active: root.linkBusy
        statusText: root.linkStatusText.length > 0
                      ? root.linkStatusText
                      : qsTr("Please wait…")
    }

    Component.onCompleted: {
        if (!libraryLinkModel.loaded)
            libraryLinkModel.reload()
        fileList.forceActiveFocus()
        groupPickerModel.refresh()
        refreshUnlinkedSelection()
    }
}
