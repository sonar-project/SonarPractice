import QtQuick
import QtTest

TestCase {
    name: "AppSettingsBindings"

    function init() {
        appSettings.storageStrategy = "link"
        appSettings.resetExtensionCategoriesToDefaults()
        appSettings.saveConfiguration()
        appSettings.reload()
    }

    function test_file_dialog_filters_not_empty() {
        compare(appSettings.fileDialogNameFilters.length > 0, true)
    }

    function test_managed_storage_default_repertoire_folder() {
        verify(appSettings.managedStorageRoot.indexOf("SonarPractice-Repertoire") >= 0)
    }

    function test_allowed_extensions_include_gp5() {
        compare(appSettings.allowedExtensions.indexOf("gp5") >= 0, true)
    }
}
