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

    signal backRequested()
    signal stopSessionRequested()

    background: Rectangle {
        color: Theme.windowBackground
    }

    header: TopBar {
        title: root.songTitle.length > 0 ? root.songTitle : qsTr("Guitar Pro player")
        subtitle: {
            const parts = []
            if (guitarProPreviewController.title.length > 0
                    && guitarProPreviewController.title !== root.songTitle)
                parts.push(guitarProPreviewController.title)
            if (guitarProPreviewController.artist.length > 0)
                parts.push(guitarProPreviewController.artist)
            return parts.join(" — ")
        }
        showBack: true
        sessionLocked: practiceTracker.timerRunning
        sessionLabel: qsTr("Timer running")
        onBackRequested: root.backRequested()
        onStopSessionRequested: root.stopSessionRequested()
    }

    StackView.onActivated: guitarProPreviewController.onPlayerSurfaceActivated()

    GuitarProTabPreview {
        anchors.fill: parent
        anchors.margins: 12
        pageMode: true
    }
}
