// src/ui/pages/GuitarProPlayerPage.qml
// Dedicated stack page for the AlphaTab Guitar Pro player (opened from Training).

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../components"

Page {
    id: root
    objectName: "guitarProPlayerPage"

    required property string songTitle
    property int backingMediaId: 0
    property int practiceAssetId: 0

    signal backRequested()
    signal stopSessionRequested()

    background: Rectangle {
        color: Theme.windowBackground
    }

    header: TopBar {
        title: root.songTitle.length > 0 ? root.songTitle : qsTr("Guitar Pro player")
        showBack: true
        sessionLocked: practiceTracker.timerRunning
        showSessionTimer: false
        sessionLabel: qsTr("Timer running")
        onBackRequested: root.backRequested()
        onStopSessionRequested: root.stopSessionRequested()
    }

    StackView.onActivated: guitarProPreviewController.onPlayerSurfaceActivated()

    GuitarProTabPreview {
        anchors.fill: parent
        anchors.margins: 12
        backingMediaId: root.backingMediaId
        practiceAssetId: root.practiceAssetId
    }
}
