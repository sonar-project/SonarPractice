# QML files and their relationship to one another

Documentation on how QML files are loaded and how they relate to one another.

Dependency tree

1. src/ui/Main.qml
    - Loads `com.sonarp.sonarpractice:Main` using `QQmlApplicationEngine::loadFromModule`.

2. src/ui/Main.qml

    - The application's main QML file.
    - Loaded components:
        - ConfigPage
        - DashboardPage
        - PracticeHubPage
        - AudioConfigPage
        - LibraryPage

3. DashboardPage.qml

    - Located in `src/ui/pages/`.
    - Loaded components:
        - DashboardExerciseList
        - DashboardReminderPanel
        - PracticeCalendarPanel
        - SongReminderPanel

4. SongReminderPanel.qml

    - Located in `src/ui/components/`.
    - Custom component used by DashboardReminderPanel.


## Hierarchical tree view

Main.cpp \
└── Main.qml \
├── ConfigPage.qml \
├── DashboardPage.qml \
│ ├── DashboardExerciseList.qml \
│ ├── DashboardReminderPanel.qml \
│ │ └── SongReminderPanel.qml \
│ └── PracticeCalendarPanel.qml \
├── PracticeHubPage.qml \
├── AudioConfigPage.qml \
└── LibraryPage.qml

## Explanation

Main.qml: Serves as the entry point for the QML application and defines the basic structure and navigation.

DashboardPage.qml: Contains components for the dashboard view, including exercise lists and reminder panels.

SongReminderPanel.qml: A custom component within `DashboardReminderPanel.qml` that manages reminders.

This tree provides a clear overview of the structure and interdependencies of your QML files.
