import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../components"

GroupBox {
    id: root

    property int practiceAssetId: 0
    property bool practiceMaterialReady: false

    title: qsTr("Training")
    padding: 12

    background: Rectangle {
        radius: 10
        color: Theme.panelBackground
        border.color: Theme.border
    }

    label: Label {
        text: root.title
        font.pixelSize: 14
        font.weight: Font.DemiBold
        color: Theme.textPrimary
    }

    ColumnLayout {
        width: parent.width
        spacing: 12

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: practiceTracker.elapsedDisplay
            font.pixelSize: 36
            font.weight: Font.Bold
            font.family: "monospace"
            color: Theme.accentTimer
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label { text: qsTr("Bar from"); color: Theme.textTertiary }
            DarkSpinBox {
                id: startBarSpin
                from: 1
                to: 999
                value: practiceTracker.startBar
                onValueModified: practiceTracker.startBar = value

                Binding on value {
                    when: !startBarSpin.activeFocus
                    value: practiceTracker.startBar
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }

            Label { text: qsTr("Bar to"); color: Theme.textTertiary }
            DarkSpinBox {
                id: endBarSpin
                from: 1
                to: 999
                value: practiceTracker.endBar
                onValueModified: practiceTracker.endBar = value

                Binding on value {
                    when: !endBarSpin.activeFocus
                    value: practiceTracker.endBar
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }

            Label {
                text: qsTr("Tempo: %1 BPM").arg(practiceTracker.targetBpm)
                color: Theme.textTertiary
            }
            DarkSpinBox {
                id: bpmSpin
                from: 20
                to: 320
                stepSize: 5
                value: practiceTracker.targetBpm
                onValueModified: practiceTracker.targetBpm = value

                Binding on value {
                    when: !bpmSpin.activeFocus
                    value: practiceTracker.targetBpm
                    restoreMode: Binding.RestoreBindingOrValue
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                Layout.fillWidth: true
                text: practiceTracker.timerRunning ? qsTr("Stop and save") : qsTr("Start timer")
                highlighted: true
                enabled: practiceTracker.timerRunning || root.practiceMaterialReady
                onClicked: {
                    if (practiceTracker.timerRunning)
                        practiceTracker.stopAndSaveWithAssetId(root.practiceAssetId);
                    else
                        practiceTracker.startTimer()
                }
            }

            Button {
                visible: practiceTracker.timerRunning
                text: qsTr("Cancel")
                onClicked: practiceTracker.cancelTimer()
            }
        }
    }
}
