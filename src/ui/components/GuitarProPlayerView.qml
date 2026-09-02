// src/ui/components/GuitarProPlayerView.qml
// Only registered in the QML module when SONARPRACTICE_HAS_WEBENGINE is enabled.

pragma ComponentBehavior: Bound

import QtQuick
import QtWebEngine

Item {
    id: root

    property var scriptQueue: []

    function themeJavaScript() {
        return "window.sonarAlphaTab && window.sonarAlphaTab.setTheme("
                + (Theme.isDark ? "true" : "false") + ");"
    }

    function runScript(javaScript) {
        webView.runJavaScript(themeJavaScript() + (javaScript || ""))
    }

    function runScriptQueue(scripts) {
        scriptQueue = scripts.slice(0)
        runNextQueuedScript()
    }

    function runNextQueuedScript() {
        if (scriptQueue.length === 0)
            return
        const js = scriptQueue.shift()
        webView.runJavaScript(themeJavaScript() + js, function () {
            runNextQueuedScript()
        })
    }

    WebEngineView {
        id: webView
        anchors.fill: parent
        url: guitarProPreviewController.playerPageUrl

        settings.localContentCanAccessFileUrls: true
        settings.localContentCanAccessRemoteUrls: false
        settings.javascriptEnabled: true
        settings.playbackRequiresUserGesture: false

        onJavaScriptConsoleMessage: function (level, message, lineNumber, sourceId) {
            if (message.indexOf("SONAR|") === 0) {
                guitarProPreviewController.handlePlayerEvent(message.substring(6))
            }
        }
    }

    Connections {
        target: guitarProPreviewController
        function onRunPlayerJavaScript(javaScript) {
            root.runScript(javaScript)
        }
        function onRunPlayerJavaScriptSequence(javaScriptCommands) {
            root.runScriptQueue(javaScriptCommands)
        }
    }

    Connections {
        target: Theme
        function onIsDarkChanged() {
            root.runScript("")
        }
    }
}
