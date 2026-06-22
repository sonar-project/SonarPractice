import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

import com.sonarp.sonarpractice

Dialog {
    id: root

    title: qsTr("About SonarPractice")
    modal: true
    standardButtons: Dialog.Ok
    anchors.centerIn: parent
    width: Math.min(parent ? parent.width * 0.45 : 420, 420)

    readonly property url githubUrl: "https://github.com/sonar-project/SonarPractice"
    readonly property url iconSource: "qrc:/qt/qml/com/sonarp/sonarpractice/assets/svg/icon.svg"

    contentItem: ColumnLayout {
        spacing: 12
        width: root.availableWidth

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Image {
                source: root.iconSource
                Layout.preferredWidth: 64
                Layout.preferredHeight: 64
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            ColumnLayout {
                spacing: 4
                Layout.fillWidth: true

                Label {
                    text: "SonarPractice"
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    color: Theme.textHeading
                }

                Label {
                    text: qsTr("Repertoire and practice manager")
                    wrapMode: Text.WordWrap
                    color: Theme.textSecondary
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Release") + ": " + (appInfo ? appInfo.version : "")
            color: Theme.textPrimary
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Qt") + ": " + (appInfo ? appInfo.qtVersion : "")
            color: Theme.textSecondary
            font.pixelSize: 12
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Compiler") + ": " + (appInfo ? appInfo.compiler : "")
            color: Theme.textSecondary
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Build date") + ": " + (appInfo ? appInfo.buildDate : "")
            color: Theme.textSecondary
            font.pixelSize: 12
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 4
            height: 1
            color: Theme.borderSubtle
        }

        Label {
            Layout.fillWidth: true
            text: '<a href="' + root.githubUrl + '">github.com/sonar-project/SonarPractice</a>'
            textFormat: Text.RichText
            color: Theme.link
            font.pixelSize: 12
            linkColor: Theme.link

            onLinkActivated: link => Qt.openUrlExternally(link)
        }
    }
}
