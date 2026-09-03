.pragma library

// Mirrors src/core/database/MediaFile.h and MediaFileMapping.h
var GuitarPro = "guitarpro"
var Audio = "audio"
var Video = "video"
var Image = "image"
var Document = "document"
var Unknown = "unknown"

var Url = "URL"

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
