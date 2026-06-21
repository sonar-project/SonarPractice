import QtQuick
import QtTest

TestCase {
    name: "ConfigPageBindings"

    function init() {
        appSettings.storageStrategy = "link"
        appSettings.saveConfiguration()
        appSettings.reload()
    }

    function test_storage_strategy_default_is_link() {
        compare(appSettings.storageStrategy, "link")
    }

    function test_extension_defaults_available() {
        compare(appSettings.defaultExtensionCategoriesForUi().length, 5)
    }

    function test_save_updates_storage_strategy() {
        appSettings.storageStrategy = "copy"
        appSettings.saveConfiguration()
        compare(appSettings.storageStrategy, "copy")
    }
}
