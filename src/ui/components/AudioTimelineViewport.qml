import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Shared horizontal zoom and scroll for tablature and waveform.
 */
Item {
    id: root

    property bool showTablature: true
    property int tabStringCount: 6
    property real zoomLevel: 1.0

    property var pitchNotes: []
    property var tabStringLabels: []
    property int pitchSelectedIndex: -1
    property int durationMs: 0
    property real regionStartRatio: 0
    property real regionEndRatio: 1
    property real playheadRatio: 0
    property bool playing: false
    property bool followPlayhead: true

    property var waveformPeaks: []
    property real waveformOpacity: 1.0

    property bool _autoScrolling: false
    property bool _userPausedFollow: false

    readonly property int tabHeight: showTablature
            ? Math.max(88, 12 + Math.max(1, tabStringCount) * 16) : 0
    readonly property int waveformHeight: 140
    readonly property real contentWidth: Math.max(timelineFlickable.width,
                                                  timelineFlickable.width * zoomLevel)

    implicitHeight: zoomRow.implicitHeight + 8 + timelineFlickable.height + 10

    signal noteClicked(int index)
    signal waveformClicked(real ratio)

    function resetView() {
        root.zoomLevel = 1.0
        root._userPausedFollow = false
        setContentX(0)
    }

    function setContentX(x) {
        _autoScrolling = true
        timelineFlickable.contentX = x
        _autoScrolling = false
    }

    function scrollToPlayhead() {
        if (durationMs <= 0 || contentWidth <= timelineFlickable.width)
            return
        const playheadX = playheadRatio * contentWidth
        const target = playheadX - timelineFlickable.width / 2
        const maxX = Math.max(0, contentWidth - timelineFlickable.width)
        setContentX(Math.max(0, Math.min(maxX, target)))
    }

    function followPlayheadIfNeeded() {
        if (!playing || !followPlayhead || _userPausedFollow)
            return
        if (durationMs <= 0 || contentWidth <= timelineFlickable.width)
            return
        if (timelineFlickable.moving || timelineFlickable.dragging)
            return

        const playheadX = playheadRatio * contentWidth
        const viewLeft = timelineFlickable.contentX
        const viewRight = viewLeft + timelineFlickable.width
        const margin = timelineFlickable.width * 0.12
        const anchor = timelineFlickable.width * 0.35
        const maxX = Math.max(0, contentWidth - timelineFlickable.width)

        if (playheadX > viewRight - margin)
            setContentX(Math.min(maxX, playheadX - anchor))
        else if (playheadX < viewLeft + margin)
            setContentX(Math.max(0, playheadX - anchor))
    }

    function ratioAtContentX(localX) {
        if (contentWidth <= 0)
            return 0
        return Math.max(0, Math.min(1, (timelineFlickable.contentX + localX) / contentWidth))
    }

    RowLayout {
        id: zoomRow
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 8

        Label {
            text: qsTr("Timeline zoom")
            color: Theme.textMuted
            font.pixelSize: 11
        }

        Slider {
            id: zoomSlider
            Layout.fillWidth: true
            from: 100
            to: 1600
            stepSize: 25
            value: Math.round(root.zoomLevel * 100)

            onMoved: root.zoomLevel = value / 100.0
        }

        Label {
            Layout.preferredWidth: 44
            horizontalAlignment: Text.AlignRight
            text: qsTr("%1×").arg(root.zoomLevel.toFixed(1))
            color: Theme.textSecondary
            font.pixelSize: 11
        }

        Button {
            text: qsTr("Fit")
            onClicked: root.resetView()
        }

        Button {
            text: qsTr("Playhead")
            enabled: root.durationMs > 0
            onClicked: {
                root._userPausedFollow = false
                root.scrollToPlayhead()
            }
        }

        CheckBox {
            text: qsTr("Follow")
            checked: root.followPlayhead
            enabled: root.durationMs > 0
            font.pixelSize: 11

            onToggled: {
                root.followPlayhead = checked
                if (checked) {
                    root._userPausedFollow = false
                    if (root.playing)
                        root.scrollToPlayhead()
                }
            }
        }
    }

    Flickable {
        id: timelineFlickable
        anchors.top: zoomRow.bottom
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.tabHeight + root.waveformHeight
        contentWidth: root.contentWidth
        contentHeight: height
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        onContentXChanged: {
            if (!root._autoScrolling && root.playing && root.followPlayhead)
                root._userPausedFollow = true
        }

        onDraggingChanged: {
            if (dragging && root.playing && root.followPlayhead)
                root._userPausedFollow = true
        }

        ScrollBar.horizontal: ScrollBar {
            id: timelineScrollBar
            policy: timelineFlickable.contentWidth > timelineFlickable.width
                    ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        }

        WheelHandler {
            target: timelineFlickable
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onWheel: (event) => {
                if (event.modifiers & Qt.ControlModifier) {
                    const factor = event.angleDelta.y > 0 ? 1.12 : 0.89
                    const nextZoom = Math.max(1.0, Math.min(16.0, root.zoomLevel * factor))
                    const anchorX = timelineFlickable.contentX + event.x
                    const anchorRatio = contentWidth > 0 ? anchorX / contentWidth : 0
                    root.zoomLevel = nextZoom
                    root.setContentX(Math.max(
                                0,
                                Math.min(
                                    Math.max(0, root.contentWidth - timelineFlickable.width),
                                    anchorRatio * root.contentWidth - event.x)))
                    event.accepted = true
                }
            }
        }

        Column {
            width: root.contentWidth
            spacing: 0

            PitchNotesView {
                visible: root.showTablature
                width: parent.width
                height: root.tabHeight
                notes: root.pitchNotes
                stringLabels: root.tabStringLabels
                stringCount: root.tabStringCount
                durationMs: root.durationMs
                regionStartRatio: root.regionStartRatio
                regionEndRatio: root.regionEndRatio
                playheadRatio: root.playheadRatio
                selectedIndex: root.pitchSelectedIndex
                onNoteClicked: (index) => root.noteClicked(index)
            }

            AudioWaveformView {
                id: waveform
                width: parent.width
                height: root.waveformHeight
                peaks: root.waveformPeaks
                opacity: root.waveformOpacity
                regionStartRatio: root.regionStartRatio
                regionEndRatio: root.regionEndRatio
                playheadRatio: root.playheadRatio

                onClickedRatio: (ratio) => root.waveformClicked(ratio)
            }
        }
    }

    onZoomLevelChanged: {
        if (zoomSlider.value !== Math.round(zoomLevel * 100))
            zoomSlider.value = Math.round(zoomLevel * 100)
    }

    onDurationMsChanged: resetView()

    onPlayheadRatioChanged: followPlayheadIfNeeded()

    onPlayingChanged: {
        if (playing && followPlayhead && !_userPausedFollow)
            scrollToPlayhead()
    }
}
