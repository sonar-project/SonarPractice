import QtQuick
import QtTest

TestCase {
    name: "ImportSheet"

    Loader {
        id: sheetLoader
        source: "../../src/ui/pages/ImportSheet.qml"
    }

    SignalSpy {
        id: importSpy
        signalName: "importPaths"
    }

    function test_import_paths_signal() {
        compare(sheetLoader.status, Loader.Ready)
        importSpy.target = sheetLoader.item

        const paths = ["/tmp/test.gp5"]
        sheetLoader.item.importPaths(paths)
        compare(importSpy.count, 1)
        compare(importSpy.signalArguments[0][0].length, 1)
        compare(importSpy.signalArguments[0][0][0], "/tmp/test.gp5")
    }
}
