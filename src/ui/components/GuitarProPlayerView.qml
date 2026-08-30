// src/ui/components/GuitarProPlayerView.qml
// Only registered in the QML module when SONARPRACTICE_HAS_WEBENGINE is enabled.

pragma ComponentBehavior: Bound

import QtQuick
import QtWebEngine

Item {
    id: root

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
                if (js && js.length > 0)
                    webView.runJavaScript(js)
            }
        }
    }

    function runScript(javaScript) {
        if (javaScript && javaScript.length > 0)
            webView.runJavaScript(javaScript)
    }

    Connections {
        target: guitarProPreviewController
        function onRunPlayerJavaScript(javaScript) {
            root.runScript(javaScript)
        }
    }
}
