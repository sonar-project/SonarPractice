# src/viewmodels/CMakeLists.txt
qt_add_library(ViewModelsLib STATIC
    CatalogSnapshot.h
    CatalogSnapshot.cpp
    CatalogViewCache.h
    CatalogViewCache.cpp
    SongModel.h
    SongModel.cpp
    MediaFileListModel.h
    MediaFileListModel.cpp
    LibraryLinkModel.h
    LibraryLinkModel.cpp
    LibrarySearchExpression.h
    LibrarySearchExpression.cpp
    PracticeSessionController.h
    PracticeSessionController.cpp
    JournalTableModel.h
    JournalTableModel.cpp
    PracticeTrackerController.h
    PracticeTrackerController.cpp
    ReminderListModel.h
    ReminderListModel.cpp
    DayReminderModel.h
    DayReminderModel.cpp
    ReminderController.h
    ReminderController.cpp
    AudioConfigController.h
    AudioConfigController.cpp
    PracticeAssetController.h
    PracticeAssetController.cpp
    AppInfo.h
)

target_link_libraries(ViewModelsLib PUBLIC DatabaseMangerLib ServicesLib AudioLib Qt6::Core Qt6::Qml)

target_include_directories(ViewModelsLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# qt_extract_metatypes(ViewModelsLib)
