import QtQuick
import QtQuick.Controls
import QtTest

TestCase {
    name: "TrainingPanel"
    when: windowShown

    Loader {
        id: panelLoader
        source: "../../src/ui/components/TrainingPanel.qml"
    }

    property var panel: panelLoader.item

    function test_initial_timer_display() {
        compare(panelLoader.status, Loader.Ready)
        compare(practiceTracker.timerRunning, false)
        compare(practiceTracker.elapsedDisplay, "00:00")
    }

    function test_start_timer_updates_state() {
        compare(panelLoader.status, Loader.Ready)
        practiceTracker.cancelTimer()
        verify(practiceTracker.startTimer())
        compare(practiceTracker.timerRunning, true)
        compare(practiceTracker.elapsedDisplay, "00:01")
    }

    function test_stop_and_save_adds_journal_row() {
        compare(panelLoader.status, Loader.Ready)
        practiceTracker.cancelTimer()
        practiceTracker.journalModel.clear()
        practiceTracker.startBar = 1
        practiceTracker.endBar = 8
        verify(practiceTracker.startTimer())
        verify(practiceTracker.stopAndSave())
        compare(practiceTracker.timerRunning, false)
        compare(practiceTracker.journalModel.rowCount, 1)
    }

    function test_invalid_bar_range_fails_save() {
        compare(panelLoader.status, Loader.Ready)
        practiceTracker.cancelTimer()
        practiceTracker.startBar = 10
        practiceTracker.endBar = 2
        verify(practiceTracker.startTimer())
        verify(!practiceTracker.stopAndSave())
    }
}
