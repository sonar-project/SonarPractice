import QtQuick

Item {
    id: root

    property var peaks: []
    property real regionStartRatio: 0
    property real regionEndRatio: 1
    property real playheadRatio: 0

    readonly property int horizontalPadding: 4
    readonly property int verticalPadding: 8

    function canvasColor(c) {
        return Qt.rgba(c.r, c.g, c.b, c.a)
    }

    implicitHeight: 120

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: Theme.panelBackgroundNested
        border.color: Theme.borderSubtle
    }

    Canvas {
        id: waveformCanvas
        anchors.fill: parent
        anchors.margins: 4

        onPaint: {
            const context = getContext("2d")
            context.reset()
            context.clearRect(0, 0, width, height)

            if (!root.peaks || root.peaks.length === 0)
                return

            const centerY = height / 2
            const bucketWidth = width / root.peaks.length

            context.fillStyle = root.canvasColor(Theme.accent)
            for (let bucket = 0; bucket < root.peaks.length; ++bucket) {
                const peak = Number(root.peaks[bucket])
                const barHeight = Math.max(2, peak * (height - root.verticalPadding * 2))
                const x = bucket * bucketWidth
                context.fillRect(x, centerY - barHeight / 2, Math.max(1, bucketWidth - 1), barHeight)
            }

            const regionStartX = Math.min(root.regionStartRatio, root.regionEndRatio) * width
            const regionEndX = Math.max(root.regionStartRatio, root.regionEndRatio) * width
            context.fillStyle = root.canvasColor(Theme.regionMarkerFill)
            context.globalAlpha = 0.45
            context.fillRect(regionStartX, 0, regionEndX - regionStartX, height)
            context.globalAlpha = 1

            context.lineWidth = 2

            context.strokeStyle = root.canvasColor(Theme.regionMarkerStart)
            context.beginPath()
            context.moveTo(regionStartX, 0)
            context.lineTo(regionStartX, height)
            context.stroke()

            context.strokeStyle = root.canvasColor(Theme.regionMarkerEnd)
            context.beginPath()
            context.moveTo(regionEndX, 0)
            context.lineTo(regionEndX, height)
            context.stroke()

            const playheadX = root.playheadRatio * width
            context.strokeStyle = root.canvasColor(Theme.warning)
            context.lineWidth = 2
            context.beginPath()
            context.moveTo(playheadX, 0)
            context.lineTo(playheadX, height)
            context.stroke()
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: (mouse) => {
            if (width <= 0)
                return
            const ratio = mouse.x / width
            root.clickedRatio(ratio)
        }
    }

    signal clickedRatio(real ratio)

    onPeaksChanged: Qt.callLater(waveformCanvas.requestPaint)
    onRegionStartRatioChanged: waveformCanvas.requestPaint()
    onRegionEndRatioChanged: waveformCanvas.requestPaint()
    onPlayheadRatioChanged: waveformCanvas.requestPaint()
    onWidthChanged: waveformCanvas.requestPaint()
    onHeightChanged: waveformCanvas.requestPaint()
}
