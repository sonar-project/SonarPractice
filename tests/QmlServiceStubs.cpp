#include "QmlServiceStubs.h"

#include "PracticeConstants.h"

#include <QQmlContext>

StubDayReminderModel::StubDayReminderModel(QObject *parent)
    : QAbstractListModel(parent) {}

int StubDayReminderModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return 0;
}

QVariant StubDayReminderModel::data(const QModelIndex &index, int role) const {
    Q_UNUSED(index)
    Q_UNUSED(role)
    return {};
}

StubImportService::StubImportService(QObject *parent)
    : QObject(parent) {}

bool StubImportService::busy() const { return false; }

QString StubImportService::statusMessage() const { return {}; }

int StubImportService::progressCurrent() const { return 0; }

int StubImportService::progressTotal() const { return 0; }

QString StubImportService::progressFileName() const { return {}; }

int StubImportService::lastImportedCount() const { return 0; }

int StubImportService::lastSkippedCount() const { return 0; }

int StubImportService::lastFailedCount() const { return 0; }

void StubImportService::importPaths(const QStringList &paths) { Q_UNUSED(paths); }

void StubImportService::clearStatusMessage() {}

StubPracticeTracker::StubPracticeTracker(QObject *parent)
    : QObject(parent)
    , m_journalModel(this) {}

bool StubPracticeTracker::timerRunning() const { return false; }

QString StubPracticeTracker::elapsedDisplay() const { return QStringLiteral("00:00"); }

int StubPracticeTracker::startBar() const { return PracticeConstants::kDefaultStartBar; }

int StubPracticeTracker::endBar() const { return PracticeConstants::kDefaultEndBar; }

int StubPracticeTracker::targetBpm() const { return PracticeConstants::kDefaultTargetBpm; }

QObject *StubPracticeTracker::journalModel() { return &m_journalModel; }

QDate StubPracticeTracker::selectedDate() const { return m_selectedDate; }

qlonglong StubPracticeTracker::songId() const { return m_songId; }

void StubPracticeTracker::setSelectedDate(const QDate &date) {
    if (m_selectedDate == date) {
        return;
    }
    m_selectedDate = date;
    emit selectedDateChanged();
}

void StubPracticeTracker::setSongId(qlonglong songId) {
    if (m_songId == songId) {
        return;
    }
    m_songId = songId;
    emit songIdChanged();
}

bool StubPracticeTracker::startTimer() { return false; }

bool StubPracticeTracker::stopAndSave() { return false; }

void StubPracticeTracker::cancelTimer() {}

void StubPracticeTracker::reloadJournal() {}

void StubPracticeTracker::reloadJournalNote() {}

void StubPracticeTracker::saveJournalNote() {}

void StubPracticeTracker::loadTrainingDefaults(int fallbackBpm) { Q_UNUSED(fallbackBpm); }

StubPracticeSession::StubPracticeSession(QObject *parent)
    : QObject(parent) {}

void StubPracticeSession::startPractice(qlonglong mediaId) { Q_UNUSED(mediaId); }

void StubPracticeSession::openAsset(qlonglong mediaId) { Q_UNUSED(mediaId); }

StubSongModel::StubSongModel(QObject *parent)
    : QObject(parent) {}

int StubSongModel::count() const { return 0; }

bool StubSongModel::catalogReady() const { return false; }

void StubSongModel::reload() {}

void StubSongModel::setGroupExpanded(qlonglong groupId, bool expanded) {
    Q_UNUSED(groupId);
    Q_UNUSED(expanded);
}

bool StubSongModel::isGroupExpanded(qlonglong groupId) const {
    Q_UNUSED(groupId);
    return false;
}

const QString &StubSongModel::searchText() const { return m_searchText; }

void StubSongModel::setSearchText(const QString &text) {
    if (m_searchText == text) {
        return;
    }
    m_searchText = text;
    emit searchTextChanged();
}

void StubSongModel::setHideContainers(bool hide) {
    if (m_hideContainers == hide) {
        return;
    }
    m_hideContainers = hide;
    if (hide) {
        m_containersOnly = false;
        emit containersOnlyChanged();
        if (m_expandAllGroups) {
            m_expandAllGroups = false;
            emit expandAllGroupsChanged();
        }
    }
    emit hideContainersChanged();
}

void StubSongModel::setContainersOnly(bool only) {
    if (m_containersOnly == only) {
        return;
    }
    m_containersOnly = only;
    if (only) {
        m_hideContainers = false;
        emit hideContainersChanged();
        if (m_expandAllGroups) {
            m_expandAllGroups = false;
            emit expandAllGroupsChanged();
        }
    }
    emit containersOnlyChanged();
}

void StubSongModel::setExpandAllGroups(bool expand) {
    if (m_expandAllGroups == expand) {
        return;
    }
    m_expandAllGroups = expand;
    emit expandAllGroupsChanged();
}

StubMediaFileModel::StubMediaFileModel(QObject *parent)
    : QObject(parent) {}

int StubMediaFileModel::count() const { return 0; }

QVariantList StubMediaFileModel::filesForKind(const QString &mediaKind) const {
    Q_UNUSED(mediaKind);
    return {};
}

void StubMediaFileModel::reload() {}

StubLibraryLinkModel::StubLibraryLinkModel(QObject *parent)
    : QObject(parent) {}

int StubLibraryLinkModel::count() const { return 0; }

const QString &StubLibraryLinkModel::searchText() const { return m_searchText; }

const QString &StubLibraryLinkModel::folderFilter() const { return m_folderFilter; }

void StubLibraryLinkModel::setSearchText(const QString &text) {
    if (m_searchText == text) {
        return;
    }
    m_searchText = text;
    emit searchTextChanged();
}

void StubLibraryLinkModel::setFolderFilter(const QString &path) {
    if (m_folderFilter == path) {
        return;
    }
    m_folderFilter = path;
    emit folderFilterChanged();
}

void StubLibraryLinkModel::setHideContainers(bool hide) {
    if (m_hideContainers == hide) {
        return;
    }
    m_hideContainers = hide;
    if (hide) {
        m_containersOnly = false;
        emit containersOnlyChanged();
    }
    emit hideContainersChanged();
}

void StubLibraryLinkModel::setContainersOnly(bool only) {
    if (m_containersOnly == only) {
        return;
    }
    m_containersOnly = only;
    if (only) {
        m_hideContainers = false;
        emit hideContainersChanged();
    }
    emit containersOnlyChanged();
}

QString StubLibraryLinkModel::defaultLinkTitle() const {
    QString trimmed = m_searchText.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }
    if (trimmed.contains(QStringLiteral("&&")) || trimmed.contains(QStringLiteral("||"))) {
        return {};
    }
    return trimmed;
}

QString StubLibraryLinkModel::titleForSong(qlonglong songId) const {
    Q_UNUSED(songId);
    return {};
}

void StubLibraryLinkModel::updateSongsLinkState(qlonglong groupId, const QVariantList &songIds) {
    Q_UNUSED(groupId);
    Q_UNUSED(songIds);
}

void StubLibraryLinkModel::clearSongsLinkState(const QVariantList &songIds) { Q_UNUSED(songIds); }

bool StubLibraryLinkModel::isSongLinked(qlonglong songId) const {
    Q_UNUSED(songId);
    return false;
}

QStringList StubLibraryLinkModel::distinctFolderPaths() const { return {}; }

QVariantList StubLibraryLinkModel::visibleUnlinkedSongIds() const { return {}; }

QVariantList StubLibraryLinkModel::orderSongIdsForLinking(const QVariantList &songIds) const {
    return songIds;
}

void StubLibraryLinkModel::reload() { emit modelReset(); }

StubLinkGroupService::StubLinkGroupService(QObject *parent)
    : QObject(parent) {}

qlonglong StubLinkGroupService::createGroupFromSongs(const QString &title,
                                                     qlonglong primarySongId,
                                                     const QVariantList &secondarySongIds) {
    Q_UNUSED(title);
    Q_UNUSED(primarySongId);
    Q_UNUSED(secondarySongIds);
    return 0;
}

QVariantList StubLinkGroupService::dissolveGroupForSong(qlonglong primarySongId) {
    Q_UNUSED(primarySongId);
    return {};
}

bool StubLinkGroupService::updateGroupTitle(qlonglong primarySongId, const QString &title) {
    Q_UNUSED(primarySongId);
    Q_UNUSED(title);
    return false;
}

QVariantMap StubLinkGroupService::groupInfoForSong(qlonglong songId) const {
    Q_UNUSED(songId);
    return {};
}

QVariantList StubLinkGroupService::allGroups() const { return {}; }

bool StubLinkGroupService::addSongsToGroup(qlonglong groupId, const QVariantList &songIds) {
    Q_UNUSED(groupId);
    Q_UNUSED(songIds);
    return false;
}

StubReminderController::StubReminderController(QObject *parent)
    : QObject(parent) {}

QString StubReminderController::statusMessage() const { return {}; }

int StubReminderController::dayReminderCount() const { return 0; }

int StubReminderController::dailyReminderCount() const { return 0; }

int StubReminderController::periodicReminderCount() const { return 0; }

bool StubReminderController::showAllReminders() const { return m_showAllReminders; }

QAbstractListModel *StubReminderController::dayReminders() { return &m_dayReminders; }

void StubReminderController::setShowAllReminders(bool showAll) {
    if (m_showAllReminders == showAll) {
        return;
    }
    m_showAllReminders = showAll;
    emit showAllRemindersChanged();
}

void StubReminderController::reloadDayReminders() {}

void StubReminderController::deleteReminder(qlonglong reminderId) { Q_UNUSED(reminderId); }

StubAudioConfigController::StubAudioConfigController(QObject *parent)
    : QObject(parent) {}

qlonglong StubAudioConfigController::mediaFileId() const { return m_mediaFileId; }

void StubAudioConfigController::setMediaFileId(qlonglong mediaFileId) {
    if (m_mediaFileId == mediaFileId) {
        return;
    }
    m_mediaFileId = mediaFileId;
    emit mediaFileIdChanged();
}

QString StubAudioConfigController::displayName() const { return {}; }

qint64 StubAudioConfigController::regionStartMs() const { return 0; }

qint64 StubAudioConfigController::regionEndMs() const { return 0; }

bool StubAudioConfigController::loopEnabled() const { return false; }

bool StubAudioConfigController::canUndoRegion() const { return false; }

bool StubAudioConfigController::presetApplied() const { return false; }

const QString &StubAudioConfigController::presetNameInput() const { return m_presetNameInput; }

void StubAudioConfigController::setPresetNameInput(const QString &name) {
    if (m_presetNameInput == name) {
        return;
    }
    m_presetNameInput = name;
    emit presetNameInputChanged();
}

int StubAudioConfigController::selectedPresetIndex() const { return m_selectedPresetIndex; }

void StubAudioConfigController::setSelectedPresetIndex(int index) {
    if (m_selectedPresetIndex == index) {
        return;
    }
    m_selectedPresetIndex = index;
    emit selectedPresetIndexChanged();
}

bool StubAudioConfigController::playing() const { return false; }

bool StubAudioConfigController::loading() const { return false; }

qint64 StubAudioConfigController::positionMs() const { return 0; }

qint64 StubAudioConfigController::durationMs() const { return 0; }

int StubAudioConfigController::tempoPercent() const { return m_tempoPercent; }

QString StubAudioConfigController::eqPresetId() const { return QStringLiteral("flat"); }

QVariantList StubAudioConfigController::peaks() const { return {}; }

QStringList StubAudioConfigController::presetNames() const { return {}; }

QString StubAudioConfigController::statusMessage() const { return {}; }

QmlServiceStubs::QmlServiceStubs(QObject *parent)
    : QObject(parent)
    , importService(this)
    , practiceTracker(this)
    , practiceSession(this)
    , songModel(this)
    , mediaFileModel(this)
    , libraryLinkModel(this)
    , linkGroupService(this)
    , reminderController(this)
    , audioConfigController(this) {}

void registerQmlServiceStubs(QQmlContext *context, QmlServiceStubs &stubs) {
    if (context == nullptr) {
        return;
    }

    context->setContextProperty(QStringLiteral("songModel"), &stubs.songModel);
    context->setContextProperty(QStringLiteral("mediaFileModel"), &stubs.mediaFileModel);
    context->setContextProperty(QStringLiteral("libraryLinkModel"), &stubs.libraryLinkModel);
    context->setContextProperty(QStringLiteral("linkGroupService"), &stubs.linkGroupService);
    context->setContextProperty(QStringLiteral("practiceSession"), &stubs.practiceSession);
    context->setContextProperty(QStringLiteral("importService"), &stubs.importService);
    context->setContextProperty(QStringLiteral("practiceTracker"), &stubs.practiceTracker);
    context->setContextProperty(QStringLiteral("reminderController"), &stubs.reminderController);
    context->setContextProperty(QStringLiteral("audioConfigController"),
                                &stubs.audioConfigController);
}
