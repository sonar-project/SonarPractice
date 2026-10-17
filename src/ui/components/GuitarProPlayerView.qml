// src/ui/components/GuitarProPlayerView.qml
// Only registered in the QML module when SONARPRACTICE_HAS_WEBENGINE is enabled.

pragma ComponentBehavior: Bound

import QtQuick
import QtWebEngine

Item {
    id: root

    property var scriptQueue: []
    property int scriptEpoch: 0

    function themeJavaScript() {
        return "window.sonarAlphaTab && window.sonarAlphaTab.setTheme("
                + (Theme.isDark ? "true" : "false") + ");"
    }

    function runScript(javaScript) {
        webView.runJavaScript(themeJavaScript() + (javaScript || ""))
    }

    function runScriptQueue(scripts) {
        scriptEpoch += 1
        const epoch = scriptEpoch
        scriptQueue = scripts.slice(0)
        runNextQueuedScript(epoch)
    }

    function runNextQueuedScript(epoch) {
        if (epoch !== root.scriptEpoch)
            return
        if (scriptQueue.length === 0)
            return
        const js = scriptQueue.shift()
        // SoundFont byte chunks are large; skip theme prepend to keep transfer fast.
        const full = (js && js.indexOf("SoundFontBytes") !== -1)
                     ? js
                     : (themeJavaScript() + js)
        webView.runJavaScript(full, function () {
            if (epoch !== root.scriptEpoch)
                return
            runNextQueuedScript(epoch)
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
