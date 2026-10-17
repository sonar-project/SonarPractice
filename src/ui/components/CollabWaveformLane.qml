import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var peaks: []
    property int durationMs: 0
    property int positionMs: 0
    property var markers: []
    property int selectedIndex: 0
    property bool markersLocked: true
    /** Signed audio offset of the selected bar against the score, in source ms. */
    property int shiftMs: 0
    property bool alignEnabled: true

    property real viewStart: 0
    property real viewSpan: 1

    property int dragIndex: -1
    property int dragAudioMs: 0
    property bool sliding: false
    property real slidePressX: 0
    property int slideDeltaMs: 0
    property int pendingFocusMs: -1

    readonly property int hitPixels: 10
    readonly property color anchorColor: "#e53935"

    signal markerSelected(int index)
    signal markerMoved(int index, int audioMs)
    signal audioSlid(int deltaMs)
    signal hearRequested()

    implicitHeight: 120

    function canvasColor(c) {
        return Qt.rgba(c.r, c.g, c.b, c.a)
    }

    function markerAt(index) {
        if (!root.markers || index < 0 || index >= root.markers.length)
            return null
        return root.markers[index]
    }

    function markerAudioMs(index) {
        if (index === root.dragIndex)
            return root.dragAudioMs
        const marker = root.markerAt(index)
        return marker ? marker.audioMs + root.slideDeltaMs : 0
    }

    function viewOriginMs() {
        if (root.durationMs <= 0)
            return 0
        const span = root.viewSpan * root.durationMs
        let start = root.viewStart * root.durationMs + root.slideDeltaMs
        if (start < 0)
            start = 0
        if (span > 0 && start + span > root.durationMs)
            start = Math.max(0, root.durationMs - span)
        return start
    }

    function xForMs(ms) {
        if (root.durationMs <= 0 || root.viewSpan <= 0)
            return 0
        const start = root.viewOriginMs()
        const span = root.viewSpan * root.durationMs
        return ((ms - start) / span) * width
    }

    function msForX(x) {
        if (width <= 0 || root.durationMs <= 0)
            return 0
        const start = root.viewOriginMs()
        const span = root.viewSpan * root.durationMs
        const ms = start + (x / width) * span
        return Math.max(0, Math.min(root.durationMs, Math.round(ms)))
    }

    function formatShift(ms) {
        const n = Math.round(ms)
        if (n > 0)
            return "+" + n + " ms"
        return n + " ms"
    }

    function showAnchor(ms) {
        if (root.durationMs <= 0) {
            root.pendingFocusMs = Math.max(0, ms)
            return
        }
        root.pendingFocusMs = -1
        let spanMs = root.viewSpan * root.durationMs
        if (spanMs > root.durationMs * 0.85)
            spanMs = Math.min(root.durationMs, 20000)
        spanMs = Math.max(2000, spanMs)
        let startMs = ms - spanMs / 2
        if (startMs < 0)
            startMs = 0
        if (startMs + spanMs > root.durationMs)
            startMs = Math.max(0, root.durationMs - spanMs)
        root.viewStart = startMs / root.durationMs
        root.viewSpan = spanMs / root.durationMs
    }

    function hitMarker(x) {
        if (!root.markers)
            return -1
        let best = -1
        let bestDistance = root.hitPixels + 1
        for (let i = 0; i < root.markers.length; ++i) {
            const pinX = root.xForMs(root.markerAudioMs(i))
            const distance = Math.abs(pinX - x)
            if (distance <= root.hitPixels && distance < bestDistance) {
                best = i
                bestDistance = distance
            }
        }
        return best
    }

    function zoomAroundSelection(factor) {
        if (root.durationMs <= 0 || factor <= 0)
            return
        const marker = root.markerAt(root.selectedIndex)
        const center = marker ? root.markerAudioMs(root.selectedIndex) : root.positionMs
        let spanMs = (root.viewSpan * root.durationMs) / factor
        spanMs = Math.max(2000, Math.min(root.durationMs, spanMs))
        let startMs = center - spanMs / 2
        if (startMs < 0)
            startMs = 0
        if (startMs + spanMs > root.durationMs)
            startMs = Math.max(0, root.durationMs - spanMs)
        root.viewStart = startMs / root.durationMs
        root.viewSpan = spanMs / root.durationMs
    }

    onDurationMsChanged: {
        if (root.pendingFocusMs >= 0) {
            root.showAnchor(root.pendingFocusMs)
            waveformCanvas.requestPaint()
            return
        }
        root.viewStart = 0
        root.viewSpan = 1
        waveformCanvas.requestPaint()
    }

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: Theme.panelBackgroundNested
        border.color: Theme.borderSubtle
        clip: true
    }

    Canvas {
        id: waveformCanvas
        anchors.fill: parent
        anchors.margins: 4

        onPaint: {
            const context = getContext("2d")
            context.reset()
            context.clearRect(0, 0, width, height)
            if (width <= 0 || height <= 0)
                return

            const peaks = root.peaks || []
            if (peaks.length === 0 || root.durationMs <= 0) {
                context.fillStyle = root.canvasColor(Theme.textSecondary)
                context.font = "12px sans-serif"
                context.fillText(qsTr("Waveform appears when the backing track is ready"), 8, height / 2)
                return
            }

            const originMs = root.viewOriginMs()
            const spanMs = Math.max(1, root.viewSpan * root.durationMs)
            const startFrac = originMs / root.durationMs
            const endFrac = Math.min(1, (originMs + spanMs) / root.durationMs)
            const startBucket = Math.max(0, Math.floor(startFrac * peaks.length))
            const endBucket = Math.min(peaks.length,
                                        Math.max(startBucket + 1, Math.ceil(endFrac * peaks.length)))
            const count = Math.max(1, endBucket - startBucket)
            const bucketWidth = width / count
            const centerY = height / 2
            context.fillStyle = root.canvasColor(Theme.accent)
            for (let bucket = startBucket; bucket < endBucket; ++bucket) {
                const peak = Number(peaks[bucket])
                const barHeight = Math.max(2, peak * (height - 16))
                const x = (bucket - startBucket) * bucketWidth
                context.fillRect(x, centerY - barHeight / 2, Math.max(1, bucketWidth - 1), barHeight)
            }

            const playX = root.xForMs(root.positionMs)
            if (playX >= 0 && playX <= width) {
                context.strokeStyle = root.canvasColor(Theme.warning)
                context.lineWidth = 1
                context.beginPath()
                context.moveTo(playX, 0)
                context.lineTo(playX, height)
                context.stroke()
            }

            context.font = "11px sans-serif"
            context.textBaseline = "top"
            const list = root.markers || []
            for (let i = 0; i < list.length; ++i) {
                const ms = root.markerAudioMs(i)
                const x = root.xForMs(ms)
                if (x < -20 || x > width + 20)
                    continue
                const selected = i === root.selectedIndex
                context.strokeStyle = selected ? root.anchorColor : root.canvasColor(Theme.textPrimary)
                context.lineWidth = selected ? 3 : 2
                context.beginPath()
                context.moveTo(x, 0)
                context.lineTo(x, height)
                context.stroke()
                const marker = list[i]
                context.fillStyle = selected ? root.anchorColor : root.canvasColor(Theme.textPrimary)
                const caption = selected
                        ? qsTr("Bar %1  %2").arg(marker.bar).arg(root.formatShift(root.shiftMs + root.slideDeltaMs))
                        : qsTr("Bar %1").arg(marker.bar)
                context.fillText(caption, Math.min(width - 96, x + 4), 22)
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: {
            if (root.sliding || (root.alignEnabled && root.markers && root.markers.length > 0
                                 && root.hitMarker(mouseX) < 0))
                return Qt.SizeHorCursor
            if (root.markersLocked)
                return Qt.ArrowCursor
            return root.hitMarker(mouseX) >= 0 || root.dragIndex >= 0
                    ? Qt.SizeHorCursor : Qt.ArrowCursor
        }
        onPressed: (mouse) => {
            const hit = root.hitMarker(mouse.x)
            if (hit >= 0) {
                root.markerSelected(hit)
                if (root.markersLocked)
                    return
                root.dragIndex = hit
                root.dragAudioMs = root.markerAudioMs(hit)
                return
            }
            if (!root.alignEnabled || !root.markers || root.markers.length === 0)
                return
            root.sliding = true
            root.slidePressX = mouse.x
            root.slideDeltaMs = 0
        }
        onPositionChanged: (mouse) => {
            if (root.dragIndex >= 0 && !root.markersLocked) {
                root.dragAudioMs = root.msForX(mouse.x)
                waveformCanvas.requestPaint()
                return
            }
            if (!root.sliding || width <= 0)
                return
            const span = root.viewSpan * root.durationMs
            let delta = -Math.round(((mouse.x - root.slidePressX) / width) * span)
            const marker = root.markerAt(root.selectedIndex)
            if (marker)
                delta = Math.max(-marker.audioMs, delta)
            root.slideDeltaMs = delta
        }
        onReleased: (mouse) => {
            if (root.dragIndex >= 0) {
                const index = root.dragIndex
                const ms = root.msForX(mouse.x)
                root.dragIndex = -1
                root.markerMoved(index, ms)
                return
            }
            if (!root.sliding)
                return
            const delta = root.slideDeltaMs
            root.sliding = false
            if (delta !== 0)
                root.audioSlid(delta)
            root.slideDeltaMs = 0
        }
        onCanceled: {
            root.dragIndex = -1
            root.sliding = false
            root.slideDeltaMs = 0
            waveformCanvas.requestPaint()
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 6
        radius: 6
        color: Theme.toolbarBackground
        border.color: Theme.border
        z: 2
        width: cornerRow.implicitWidth + 8
        height: cornerRow.implicitHeight + 4

        RowLayout {
            id: cornerRow
            anchors.centerIn: parent
            spacing: 2

            ToolButton {
                text: root.markersLocked ? qsTr("Unlock") : qsTr("Lock")
                ToolTip.visible: hovered
                ToolTip.text: root.markersLocked
                             ? qsTr("Unlock markers to drag them")
                             : qsTr("Lock markers")
                onClicked: root.markersLocked = !root.markersLocked
            }
            ToolButton {
                text: qsTr("−")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Zoom out")
                onClicked: root.zoomAroundSelection(0.5)
            }
            ToolButton {
                text: qsTr("+")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Zoom in")
                onClicked: root.zoomAroundSelection(2)
            }
            ToolButton {
                text: qsTr("Hear")
                enabled: root.markerAt(root.selectedIndex) !== null
                onClicked: root.hearRequested()
            }
        }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 6
        radius: 6
        color: Theme.toolbarBackground
        border.color: Theme.border
        z: 2
        width: shiftRow.implicitWidth + 10
        height: shiftRow.implicitHeight + 4
        visible: root.markers && root.markers.length > 0

        RowLayout {
            id: shiftRow
            anchors.centerIn: parent
            spacing: 2

            Label {
                text: root.formatShift(root.shiftMs + root.slideDeltaMs)
                color: root.anchorColor
                font.bold: true
                font.pixelSize: 13
                Layout.rightMargin: 4
            }
            ToolButton {
                text: qsTr("−250")
                enabled: root.alignEnabled
                onClicked: root.audioSlid(-250)
            }
            ToolButton {
                text: qsTr("−10")
                enabled: root.alignEnabled
                onClicked: root.audioSlid(-10)
            }
            ToolButton {
                text: qsTr("+10")
                enabled: root.alignEnabled
                onClicked: root.audioSlid(10)
            }
            ToolButton {
                text: qsTr("+250")
                enabled: root.alignEnabled
                onClicked: root.audioSlid(250)
            }
        }
    }

    onPeaksChanged: waveformCanvas.requestPaint()
    onPositionMsChanged: waveformCanvas.requestPaint()
    onMarkersChanged: waveformCanvas.requestPaint()
    onSelectedIndexChanged: waveformCanvas.requestPaint()
    onViewStartChanged: waveformCanvas.requestPaint()
    onViewSpanChanged: waveformCanvas.requestPaint()
    onDragAudioMsChanged: waveformCanvas.requestPaint()
    onSlideDeltaMsChanged: waveformCanvas.requestPaint()
    onShiftMsChanged: waveformCanvas.requestPaint()
    onWidthChanged: waveformCanvas.requestPaint()
    onHeightChanged: waveformCanvas.requestPaint()
}
