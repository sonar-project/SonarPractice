pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../components"

Page {
    id: root

    property bool firstRun: true

    signal configurationSaved()
    signal cancelled()

    background: Rectangle { color: Theme.windowBackground }

    header: TopBar {
        title: root.firstRun ? qsTr("Setup") : qsTr("Settings")
        showBack: !root.firstRun
        onBackRequested: root.cancelled()
    }

    function setExtensionText(categoryKey, value) {
        for (let i = 0; i < extensionModel.count; ++i) {
            if (extensionModel.get(i).key === categoryKey) {
                extensionModel.setProperty(i, "extensions", value)
                return
            }
        }
    }

    function loadExtensionFields() {
        const source = appSettings.extensionCategoriesForUi().length > 0
                ? appSettings.extensionCategoriesForUi()
                : appSettings.defaultExtensionCategoriesForUi()
        extensionModel.clear()
        for (let i = 0; i < source.length; ++i) {
            extensionModel.append({
                key: source[i].key,
                label: source[i].label,
                extensions: source[i].extensions
            })
        }
    }

    function storagePathForUi() {
        return appSettings.hasConfiguredManagedStorageRoot
                ? appSettings.configuredManagedStorageRoot
                : appSettings.defaultManagedStoragePath
    }

    property string storageFeedbackText: ""
    property bool storageFeedbackIsError: false

    function showStorageFeedback(message, isError) {
        storageFeedbackText = message
        storageFeedbackIsError = isError
    }

    function clearStorageFeedback() {
        storageFeedbackText = ""
        storageFeedbackIsError = false
    }

    function saveConfiguration() {
        const categories = []
        for (let i = 0; i < extensionModel.count; ++i) {
            categories.push({
                key: extensionModel.get(i).key,
                extensions: extensionModel.get(i).extensions
            })
        }

        appSettings.storageStrategy = strategyGroup.checkedButton.strategyValue
        // appSettings.storageStrategy = strategyGroup.checkedButton // TODO: test without strategyValue (missing Q_PROPERTY of this)

        if (copyStrategy.checked || moveStrategy.checked) {
            const storagePath = storagePathField.text.trim()
            if (storagePath.length === 0) {
                showStorageFeedback(qsTr("Please specify a target folder."), true)
                return
            }

            if (!startupController.createStorageDirectory(storagePath)) {
                showStorageFeedback(qsTr("Could not create folder: %1").arg(storagePath), true)
                return
            }

            appSettings.managedStorageRoot = storagePath
        }

        appSettings.applyExtensionCategoriesFromUi(categories)
        persistSelectedLanguage()
        appSettings.saveConfiguration()

        if (root.firstRun) {
            if (!startupController.initializeDatabase())
                return
        } else {
            appSettings.reload()
        }

        root.configurationSaved()
    }

    ButtonGroup {
        id: strategyGroup
    }

    Component.onCompleted: {
        selectStrategy(appSettings.storageStrategy)
        storagePathField.text = storagePathForUi()
        loadExtensionFields()
        reloadLanguageModel()
        languageCombo.syncIndexFromLanguage()
    }

    function openStorageFolderDialog() {
        const selected = startupController.browseStorageDirectory(storagePathField.text)
        if (selected.length > 0)
            storagePathField.text = selected
    }

    function createStorageFolder() {
        const path = storagePathField.text.trim().length > 0
                ? storagePathField.text.trim()
                : appSettings.defaultManagedStoragePath
        if (startupController.createStorageDirectory(path)) {
            storagePathField.text = path
            showStorageFeedback(qsTr("Folder ready: %1").arg(path), false)
        } else {
            showStorageFeedback(qsTr("Could not create folder: %1").arg(path), true)
        }
    }

    function selectStrategy(value) {
        if (value === "copy")
            copyStrategy.checked = true
        else if (value === "move")
            moveStrategy.checked = true
        else
            linkStrategy.checked = true
    }

    function reloadLanguageModel() {
        languageListModel.clear()
        const languages = translationManager.availableUiLanguages()
        for (let i = 0; i < languages.length; ++i) {
            const entry = languages[i]
            languageListModel.append({
                code: entry.code ?? "",
                label: entry.label ?? "",
                fileName: entry.fileName ?? "",
                isSource: entry.isSource ?? false
            })
        }
    }

    function persistSelectedLanguage() {
        if (languageCombo.currentIndex < 0)
            return
        const code = languageCombo.selectedLanguageCode()
        if (!code || code.length === 0)
            return
        appSettings.uiLanguage = code
        translationManager.applySavedLanguage()
    }

    ListModel {
        id: languageListModel
    }

    ListModel {
        id: extensionModel
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: root.firstRun ? qsTr("Welcome to SonarPractice") : qsTr("Settings")
            font.pixelSize: 22
            font.weight: Font.Bold
            color: Theme.textHeading
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Configure how imported files are handled and which formats are allowed.")
            font.pixelSize: 13
            color: Theme.textSecondary
        }

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Language")

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: qsTr("English uses the strings from the source code. Other languages load a SonarPractice_*.ts translation file. The system language is selected by default when a matching translation exists.")
                    font.pixelSize: 12
                    color: Theme.textSecondary
                }

                ComboBox {
                    id: languageCombo
                    Layout.fillWidth: true
                    Layout.preferredWidth: 320
                    model: languageListModel
                    textRole: "label"

                    property bool acceptLanguageChanges: false

                    function selectedLanguageCode() {
                        if (currentIndex < 0 || currentIndex >= languageListModel.count)
                            return ""
                        return languageListModel.get(currentIndex).code
                    }

                    function displayLanguageCode() {
                        const stored = translationManager.storedLanguage
                        return stored.length > 0 ? stored : translationManager.uiLanguage
                    }

                    function syncIndexFromLanguage() {
                        acceptLanguageChanges = false
                        let matchIndex = -1
                        const current = displayLanguageCode()
                        for (let i = 0; i < languageListModel.count; ++i) {
                            if (languageListModel.get(i).code === current) {
                                matchIndex = i
                                break
                            }
                        }
                        if (currentIndex !== matchIndex)
                            currentIndex = matchIndex
                        Qt.callLater(() => { acceptLanguageChanges = true })
                    }

                    Connections {
                        target: translationManager
                        function onStoredLanguageChanged() {
                            languageCombo.syncIndexFromLanguage()
                        }
                        function onUiLanguageChanged() {
                            if (translationManager.storedLanguage.length === 0)
                                languageCombo.syncIndexFromLanguage()
                        }
                    }

                    onActivated: function(index) {
                        if (!acceptLanguageChanges || index < 0)
                            return
                        const code = languageListModel.get(index).code
                        if (!code || code.length === 0)
                            return
                        appSettings.uiLanguage = code
                        translationManager.applySavedLanguage()
                    }
                }
            }
        }

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("File management")

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                RadioButton {
                    id: linkStrategy
                    text: qsTr("No management (files stay at original location)")
                    property string strategyValue: "link"
                    ButtonGroup.group: strategyGroup
                    checked: true
                }

                RadioButton {
                    id: copyStrategy
                    text: qsTr("Copy files")
                    property string strategyValue: "copy"
                    ButtonGroup.group: strategyGroup
                }

                RadioButton {
                    id: moveStrategy
                    text: qsTr("Move files")
                    property string strategyValue: "move"
                    ButtonGroup.group: strategyGroup
                }

                Label {
                    Layout.fillWidth: true
                    visible: copyStrategy.checked || moveStrategy.checked
                    wrapMode: Text.WordWrap
                    text: qsTr("Target folder for copy or move:")
                    font.pixelSize: 12
                    color: Theme.textSecondary
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: copyStrategy.checked || moveStrategy.checked

                    DarkTextField {
                        id: storagePathField
                        Layout.fillWidth: true
                        placeholderText: appSettings.defaultManagedStoragePath
                        onTextEdited: root.clearStorageFeedback()
                    }

                    Button {
                        text: qsTr("Choose folder")
                        onClicked: root.openStorageFolderDialog()
                    }

                    Button {
                        text: qsTr("Create folder")
                        onClicked: root.createStorageFolder()
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: copyStrategy.checked || moveStrategy.checked
                    wrapMode: Text.WordWrap
                    text: root.storageFeedbackText
                    font.pixelSize: 12
                    color: root.storageFeedbackIsError ? Theme.error : Theme.success
                }
            }
        }

        GroupBox {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: qsTr("Supported file formats")

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                Repeater {
                    model: extensionModel

                    RowLayout {
                        Layout.fillWidth: true
                        required property string key
                        required property string label
                        required property string extensions

                        Label {
                            Layout.preferredWidth: 120
                            text: label
                            color: Theme.accentLight
                        }

                        DarkTextField {
                            Layout.fillWidth: true
                            text: extensions
                            placeholderText: qsTr("Separate extensions with ;")
                            onTextEdited: root.setExtensionText(key, text)
                        }
                    }
                }

                Button {
                    text: qsTr("Restore defaults")
                    flat: true
                    onClicked: {
                        extensionModel.clear()
                        const defaults = appSettings.defaultExtensionCategoriesForUi()
                        for (let i = 0; i < defaults.length; ++i) {
                            extensionModel.append({
                                key: defaults[i].key,
                                label: defaults[i].label,
                                extensions: defaults[i].extensions
                            })
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Item { Layout.fillWidth: true }

            Button {
                visible: !root.firstRun
                text: qsTr("Cancel")
                onClicked: root.cancelled()
            }

            Button {
                text: root.firstRun ? qsTr("Save and start") : qsTr("Save")
                highlighted: true
                onClicked: root.saveConfiguration()
            }
        }
    }
}
