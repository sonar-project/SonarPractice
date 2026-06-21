.pragma library

// Mirrors src/core/database/MediaFile.h and MediaFileMapping.h
var GuitarPro = "guitarpro"
var Audio = "audio"
var Video = "video"
var Image = "image"
var Document = "document"
var Unknown = "unknown"

var Local = "LOCAL"
var Url = "URL"

function label(kind) {
    switch (kind) {
    case GuitarPro: return qsTr("Guitar Pro")
    case Audio: return qsTr("Audio")
    case Video: return qsTr("Video")
    case Image: return qsTr("Image")
    case Document: return qsTr("Document")
    default: return qsTr("Unknown")
    }
}

function icon(kind) {
    switch (kind) {
    case GuitarPro: return "\u266B"   // ♫
    case Audio: return "\u266A"       // ♪
    case Video: return "\u25B6"       // ▶
    case Image: return "\u25A3"       // ▣
    case Document: return "\u2261"    // ≡
    default: return "\u2753"          // ?
    }
}

function iconSource(kind) {
    switch (kind) {
    case GuitarPro: return "qrc:/qt/qml/com/sonarp/sonarpractice/assets/png/gp.png"
    case Audio: return "qrc:/qt/qml/com/sonarp/sonarpractice/assets/png/audio.png"
    case Video: return "qrc:/qt/qml/com/sonarp/sonarpractice/assets/png/video.png"
    case Document: return "qrc:/qt/qml/com/sonarp/sonarpractice/assets/png/doc.png"
    case Unknown: return "qrc:/qt/qml/com/sonarp/sonarpractice/assets/png/unlinked.png"
    default: return ""
    }
}

function appIconSource() {
    return "qrc:/icon"
}

function sortedKinds(summary) {
    var order = [GuitarPro, Video, Audio, Document, Image, Unknown]
    var present = {}
    for (var i = 0; i < summary.length; ++i)
        present[summary[i].kind] = true

    var result = []
    for (var j = 0; j < order.length; ++j) {
        if (present[order[j]])
            result.push(order[j])
    }
    return result
}

function accentColor(kind) {
    switch (kind) {
    case GuitarPro: return "#7c4dff"
    case Audio: return "#00bcd4"
    case Video: return "#ab47bc"
    case Image: return "#ffb74d"
    case Document: return "#ef5350"
    default: return "#78909c"
    }
}

function withAlpha(color, alpha) {
    var c = Qt.color(color)
    var a = Math.round(Math.max(0, Math.min(1, alpha)) * 255)
    var r = Math.round(c.r * 255)
    var g = Math.round(c.g * 255)
    var b = Math.round(c.b * 255)
    function hex(n) {
        var s = n.toString(16)
        return s.length === 1 ? "0" + s : s
    }
    return "#" + hex(a) + hex(r) + hex(g) + hex(b)
}

function accentFillColor(kind) {
    switch (kind) {
    case GuitarPro: return "#2e7c4dff"
    case Audio: return "#2e00bcd4"
    case Video: return "#2eab47bc"
    case Image: return "#2effb74d"
    case Document: return "#2eef5350"
    default: return "#2e78909c"
    }
}
