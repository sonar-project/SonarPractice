pragma Singleton

import QtCore
import QtQuick

Settings {
    id: root

    location: appSettings.settingsFileLocation
    category: "Appearance"

    property bool isDark: true

    function toggle() {
        isDark = !isDark
    }

    readonly property color windowBackground: isDark ? "#16161f" : "#f3f4f8"
    readonly property color windowBackgroundDeep: isDark ? "#0d0d14" : "#e8e9ed"
    readonly property color toolbarBackground: isDark ? "#1e1e2e" : "#ffffff"
    readonly property color panelBackground: isDark ? "#1a1a28" : "#ffffff"
    readonly property color panelBackgroundNested: isDark ? "#252536" : "#eef0f4"
    readonly property color cardBackground: isDark ? "#252536" : "#ffffff"
    readonly property color cardBackgroundPressed: isDark ? "#2a2a3d" : "#e4e6ec"
    readonly property color assetTileBackground: isDark ? "#2a2a3d" : "#eef0f4"
    readonly property color dropZoneBackground: isDark ? "#252536" : "#f5f6fa"
    readonly property color dropZoneDragBackground: isDark ? "#2a2a3d" : "#ebe8ff"
    readonly property color editorBackground: isDark ? "#16161f" : "#ffffff"
    readonly property color tableRowEven: isDark ? "#16161f" : "#f8f9fb"
    readonly property color tableRowOdd: isDark ? "#1e1e2e" : "#ffffff"
    readonly property color tableHeaderBackground: isDark ? "#252536" : "#eef0f4"
    readonly property color inputBackground: isDark ? "#252536" : "#ffffff"
    readonly property color inputBackgroundDisabled: isDark ? "#2a2a3a" : "#f0f1f4"
    readonly property color calendarTodayBackground: isDark ? "#3d3d5c" : "#e0e2ea"

    readonly property color border: isDark ? "#2a2a3a" : "#d8dbe3"
    readonly property color borderSubtle: isDark ? "#3d3d5c" : "#c5c9d4"
    readonly property color borderFocus: isDark ? "#5c6bc0" : "#5c6bc0"
    readonly property color borderAccent: isDark ? "#7c4dff" : "#651fff"
    readonly property color borderAccentMuted: isDark ? "#5c4dff" : "#651fff"
    readonly property color borderActive: isDark ? "#5c6bc0" : "#5c6bc0"

    readonly property color textPrimary: isDark ? "#eceff1" : "#1a1a2e"
    readonly property color textHeading: isDark ? "#f5f5f5" : "#12121f"
    readonly property color textTitle: isDark ? "#ffffff" : "#12121f"
    readonly property color textToolbar: isDark ? "#e0e0e0" : "#424242"
    readonly property color textSecondary: isDark ? "#78909c" : "#5f6b7a"
    readonly property color textTertiary: isDark ? "#9e9e9e" : "#757575"
    readonly property color textMuted: isDark ? "#607d8b" : "#8a939e"
    readonly property color textHint: isDark ? "#90a4ae" : "#6b7280"
    readonly property color textPlaceholder: isDark ? "#607d8b" : "#9aa3ad"
    readonly property color textDropHint: isDark ? "#b0bec5" : "#6b7280"
    readonly property color textOutOfMonth: isDark ? "#546e7a" : "#b0b8c4"
    readonly property color textOnAccent: "#ffffff"

    readonly property color accent: isDark ? "#7c4dff" : "#651fff"
    readonly property color accentLight: isDark ? "#b39ddb" : "#7e57c2"
    readonly property color accentFill: "#267c4dff"
    readonly property color accentTimer: isDark ? "#7c4dff" : "#651fff"
    readonly property color highlight: "#5c6bc0"
    readonly property color link: isDark ? "#00bcd4" : "#00838f"
    readonly property color warning: isDark ? "#ffd54f" : "#f9a825"
    readonly property color success: isDark ? "#a5d6a7" : "#2e7d32"
    readonly property color error: isDark ? "#ef9a9a" : "#c62828"
    readonly property color danger: "#c62828"
    readonly property color regionMarkerStart: isDark ? "#66bb6a" : "#2e7d32"
    readonly property color regionMarkerEnd: isDark ? "#ef5350" : "#c62828"
    readonly property color regionMarkerFill: isDark ? "#3366bb6a" : "#332e7d32"

    readonly property color overlay: isDark ? "#99000000" : "#66000000"
    readonly property color toolbarButtonHover: isDark ? "#2a2a3a" : "#e8eaf0"
    readonly property color toolbarButtonPressed: isDark ? "#3d3d5c" : "#d8dbe3"
    readonly property color calendarSelected: isDark ? "#7c4dff" : "#651fff"
    readonly property color calendarSelectedBorder: isDark ? "#b39ddb" : "#9575cd"
    readonly property color calendarPracticedDot: isDark ? "#4fc3f7" : "#0288d1"
    readonly property color calendarCompleteDot: isDark ? "#81c784" : "#2e7d32"
    readonly property color calendarPartialDot: isDark ? "#ffd54f" : "#f9a825"
}
