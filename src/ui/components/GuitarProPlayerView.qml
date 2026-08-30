// src/ui/components/GuitarProPlayerView.qml
// Only registered in the QML module when SONARPRACTICE_HAS_WEBENGINE is enabled.

pragma ComponentBehavior: Bound

import QtQuick
import QtWebEngine

Item {
    id: root

    function themeJavaScript() {
        return "window.sonarAlphaTab && window.sonarAlphaTab.setTheme("
                + (Theme.isDark ? "true" : "false") + ");"
    }

    function runScript(javaScript) {
        webView.runJavaScript(themeJavaScript() + (javaScript || ""))
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

        onLoadingChanged: function (request) {
            if (request.status === WebEngineView.LoadSucceededStatus) {
                const js = guitarProPreviewController.loadScoreJavaScript()
                root.runScript(js && js.length > 0 ? js : "")
            }
        }
    }

    Connections {
        target: guitarProPreviewController
        function onRunPlayerJavaScript(javaScript) {
            root.runScript(javaScript)
        }
    }

    Connections {
        target: Theme
        function onIsDarkChanged() {
            root.runScript("")
        }
    }
}
