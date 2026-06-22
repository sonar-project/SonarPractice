import QtQuick
import QtQuick.Controls

/**
 * Guitar Pro–style tab staff: horizontal string lines with fret numbers on time axis.
 */
Item {
    id: root

    property var notes: []
    property var stringLabels: []
    property int stringCount: 6
    property int durationMs: 0
    property real regionStartRatio: 0
    property real regionEndRatio: 1
    property real playheadRatio: 0
    property int selectedIndex: -1

    signal noteClicked(int index)

    readonly property int staffLeftMargin: 24

    function canvasColor(c) {
        return Qt.rgba(c.r, c.g, c.b, c.a)
    }

    function stringY(stringIndex) {
        if (stringCount <= 1)
            return plotArea.height / 2
        const spacing = (plotArea.height - 8) / (stringCount - 1)
        return 4 + stringIndex * spacing
    }

    function noteTooltipText(note) {
        const startSec = (note.startMs / 1000).toFixed(2)
        const endSec = (note.endMs / 1000).toFixed(2)
        const tabName = note.tabDisplay !== undefined ? note.tabDisplay : note.label
        return qsTr("%1 — %2 Hz\n%3 s – %4 s")
                .arg(tabName)
                .arg(note.freq.toFixed(1))
                .arg(startSec)
                .arg(endSec)
    }

    implicitHeight: Math.max(88, 12 + Math.max(1, stringCount) * 16)

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: Theme.panelBackgroundNested
        border.color: Theme.borderSubtle
    }

    Item {
        id: plotArea
        anchors.fill: parent
        anchors.margins: 4

        Canvas {
            id: staffCanvas
            anchors.fill: parent

            onPaint: {
                const context = getContext("2d")
                context.reset()
                context.clearRect(0, 0, width, height)

                context.font = "bold 11px sans-serif"
                context.fillStyle = root.canvasColor(Theme.textMuted)
                context.textAlign = "right"
                context.textBaseline = "middle"

                context.strokeStyle = root.canvasColor(Theme.borderSubtle)
                context.lineWidth = 1
                for (let line = 0; line < root.stringCount; ++line) {
                    const y = root.stringY(line)
                    if (line < root.stringLabels.length)
                        context.fillText(root.stringLabels[line], root.staffLeftMargin - 4, y)

                    context.beginPath()
                    context.moveTo(root.staffLeftMargin, y)
                    context.lineTo(width, y)
                    context.stroke()
                }

                const plotWidth = width - root.staffLeftMargin
                const regionStartX = root.staffLeftMargin
                        + Math.min(root.regionStartRatio, root.regionEndRatio) * plotWidth
                const regionEndX = root.staffLeftMargin
                        + Math.max(root.regionStartRatio, root.regionEndRatio) * plotWidth
                context.fillStyle = root.canvasColor(Theme.regionMarkerFill)
                context.globalAlpha = 0.35
                context.fillRect(regionStartX, 0, regionEndX - regionStartX, height)
                context.globalAlpha = 1

                const playheadX = root.staffLeftMargin + root.playheadRatio * plotWidth
                context.strokeStyle = root.canvasColor(Theme.warning)
                context.lineWidth = 1
                context.beginPath()
                context.moveTo(playheadX, 0)
                context.lineTo(playheadX, height)
                context.stroke()
            }
        }

        Repeater {
            model: root.notes

            delegate: Item {
                id: noteItem
                required property var modelData
                required property int index

                readonly property real plotWidth: plotArea.width - root.staffLeftMargin
                readonly property real startRatio: root.durationMs > 0
                        ? modelData.startMs / root.durationMs : 0
                readonly property real endRatio: root.durationMs > 0
                        ? modelData.endMs / root.durationMs : 0
                readonly property real centerRatio: (startRatio + endRatio) / 2
                readonly property real barWidth: Math.max(3, Math.abs(endRatio - startRatio) * plotWidth)
                readonly property bool selected: index === root.selectedIndex
                readonly property int tabString: modelData.tabString !== undefined
                        ? modelData.tabString : 0
                readonly property string fretText: modelData.tabFretText !== undefined
                        ? modelData.tabFretText : "?"

                width: Math.max(fretBadge.width, barWidth)
                height: fretBadge.height
                x: root.staffLeftMargin + centerRatio * plotWidth - width / 2
                y: root.stringY(tabString) - height / 2

                z: selected ? 2 : (noteMouseArea.containsMouse ? 1 : 0)

                Rectangle {
                    id: durationHint
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    width: noteItem.barWidth
                    height: 4
                    radius: 2
                    color: noteItem.selected ? Theme.accent : Theme.highlight
                    opacity: 0.45
                }

                Rectangle {
                    id: fretBadge
                    anchors.centerIn: parent
                    height: 22
                    width: Math.max(22, fretLabel.implicitWidth + 10)
                    radius: 3
                    color: noteItem.selected ? Theme.accent : Theme.toolbarBackground
                    border.width: noteItem.selected ? 2 : 1
                    border.color: noteItem.selected ? Theme.textOnAccent : Theme.borderAccent

                    Text {
                        id: fretLabel
                        anchors.centerIn: parent
                        text: noteItem.fretText
                        color: noteItem.selected ? Theme.textOnAccent : Theme.textHeading
                        font.pixelSize: 13
                        font.bold: true
                    }
                }

                MouseArea {
                    id: noteMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.noteClicked(index)

                    ToolTip.visible: containsMouse
                    ToolTip.delay: 150
                    ToolTip.text: root.noteTooltipText(modelData)
                }
            }
        }
    }

    function repaintStaff() {
        staffCanvas.requestPaint()
    }

    onNotesChanged: Qt.callLater(repaintStaff)
    onRegionStartRatioChanged: repaintStaff()
    onRegionEndRatioChanged: repaintStaff()
    onPlayheadRatioChanged: repaintStaff()
    onStringCountChanged: repaintStaff()
    onStringLabelsChanged: repaintStaff()

    onWidthChanged: repaintStaff()
    onHeightChanged: repaintStaff()
}
