#ifndef QMLSERVICESTUBS_H
#define QMLSERVICESTUBS_H

#include <QAbstractListModel>
#include <QDate>
#include <QObject>
#include <QStringList>

class QQmlContext;

class StubImportService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage CONSTANT)
    Q_PROPERTY(int progressCurrent READ progressCurrent CONSTANT)
    Q_PROPERTY(int progressTotal READ progressTotal CONSTANT)
    Q_PROPERTY(QString progressFileName READ progressFileName CONSTANT)
    Q_PROPERTY(int lastImportedCount READ lastImportedCount CONSTANT)
    Q_PROPERTY(int lastSkippedCount READ lastSkippedCount CONSTANT)
    Q_PROPERTY(int lastFailedCount READ lastFailedCount CONSTANT)

public:
    explicit StubImportService(QObject *parent = nullptr);

    [[nodiscard]] bool busy() const;
    [[nodiscard]] QString statusMessage() const;
    [[nodiscard]] int progressCurrent() const;
    [[nodiscard]] int progressTotal() const;
    [[nodiscard]] QString progressFileName() const;
    [[nodiscard]] int lastImportedCount() const;
    [[nodiscard]] int lastSkippedCount() const;
    [[nodiscard]] int lastFailedCount() const;

public slots:
    void importPaths(const QStringList &paths);
    void clearStatusMessage();
    void cancelImport() {}

signals:
    void busyChanged();
    void importFinished();
};

class StubDayReminderModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit StubDayReminderModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};

class StubPracticeTracker : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool timerRunning READ timerRunning CONSTANT)
    Q_PROPERTY(QString elapsedDisplay READ elapsedDisplay CONSTANT)
    Q_PROPERTY(int startBar READ startBar CONSTANT)
    Q_PROPERTY(int endBar READ endBar CONSTANT)
    Q_PROPERTY(int targetBpm READ targetBpm CONSTANT)
    Q_PROPERTY(QObject *journalModel READ journalModel CONSTANT)
    Q_PROPERTY(QDate selectedDate READ selectedDate WRITE setSelectedDate NOTIFY selectedDateChanged)
    Q_PROPERTY(qlonglong songId READ songId WRITE setSongId NOTIFY songIdChanged)

public:
    explicit StubPracticeTracker(QObject *parent = nullptr);

    [[nodiscard]] bool timerRunning() const;
    [[nodiscard]] QString elapsedDisplay() const;
    [[nodiscard]] int startBar() const;
    [[nodiscard]] int endBar() const;
    [[nodiscard]] int targetBpm() const;
    [[nodiscard]] QObject *journalModel();
    [[nodiscard]] QDate selectedDate() const;
    [[nodiscard]] qlonglong songId() const;

    void setSelectedDate(const QDate &date);
    void setSongId(qlonglong songId);

public slots:
    bool startTimer();
    bool stopAndSave();
    void cancelTimer();
    void reloadJournal();
    void reloadJournalNote();
    void saveJournalNote();
    void loadTrainingDefaults(int fallbackBpm);

signals:
    void selectedDateChanged();
    void songIdChanged();

private:
    QObject m_journalModel;
    QDate m_selectedDate{QDate::currentDate()};
    qlonglong m_songId{0};
};

class StubPracticeSession : public QObject {
    Q_OBJECT

public:
    explicit StubPracticeSession(QObject *parent = nullptr);

public slots:
    void startPractice(qlonglong mediaId);
    void openAsset(qlonglong mediaId);
};

class StubSongModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int totalCount READ count NOTIFY countChanged)
    Q_PROPERTY(bool catalogReady READ catalogReady NOTIFY catalogReadyChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(bool hideContainers READ hideContainers WRITE setHideContainers NOTIFY
                   hideContainersChanged)
    Q_PROPERTY(bool containersOnly READ containersOnly WRITE setContainersOnly NOTIFY
                   containersOnlyChanged)
    Q_PROPERTY(bool expandAllGroups READ expandAllGroups WRITE setExpandAllGroups NOTIFY
                   expandAllGroupsChanged)

public:
    explicit StubSongModel(QObject *parent = nullptr);

    [[nodiscard]] int count() const;
    [[nodiscard]] bool catalogReady() const;
    [[nodiscard]] const QString &searchText() const;
    [[nodiscard]] bool hideContainers() const { return m_hideContainers; }
    [[nodiscard]] bool containersOnly() const { return m_containersOnly; }
    [[nodiscard]] bool expandAllGroups() const { return m_expandAllGroups; }

    void setSearchText(const QString &text);
    void setHideContainers(bool hide);
    void setContainersOnly(bool only);
    void setExpandAllGroups(bool expand);

    Q_INVOKABLE void setGroupExpanded(qlonglong groupId, bool expanded);
    [[nodiscard]] Q_INVOKABLE bool isGroupExpanded(qlonglong groupId) const;

public slots:
    void reload();

signals:
    void countChanged();
    void catalogReadyChanged();
    void searchTextChanged();
    void hideContainersChanged();
    void containersOnlyChanged();
    void expandAllGroupsChanged();

private:
    QString m_searchText;
    bool m_hideContainers{false};
    bool m_containersOnly{false};
    bool m_expandAllGroups{false};
};

class StubMediaFileModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int count READ count CONSTANT)

public:
    explicit StubMediaFileModel(QObject *parent = nullptr);

    [[nodiscard]] int count() const;

    [[nodiscard]] Q_INVOKABLE QVariantList filesForKind(const QString &mediaKind) const;

public slots:
    void reload();
};

class StubLibraryLinkModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int count READ count CONSTANT)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(
        QString folderFilter READ folderFilter WRITE setFolderFilter NOTIFY folderFilterChanged)
    Q_PROPERTY(bool hideContainers READ hideContainers WRITE setHideContainers NOTIFY
                   hideContainersChanged)
    Q_PROPERTY(bool containersOnly READ containersOnly WRITE setContainersOnly NOTIFY
                   containersOnlyChanged)
    Q_PROPERTY(int visibleUnlinkedCount READ visibleUnlinkedCount NOTIFY visibleUnlinkedCountChanged)

public:
    explicit StubLibraryLinkModel(QObject *parent = nullptr);

    [[nodiscard]] int count() const;
    [[nodiscard]] const QString &searchText() const;
    [[nodiscard]] const QString &folderFilter() const;

    void setSearchText(const QString &text);
    void setFolderFilter(const QString &path);
    void setHideContainers(bool hide);
    void setContainersOnly(bool only);

    [[nodiscard]] int visibleUnlinkedCount() const { return 0; }
    [[nodiscard]] bool hideContainers() const { return m_hideContainers; }
    [[nodiscard]] bool containersOnly() const { return m_containersOnly; }

    [[nodiscard]] Q_INVOKABLE QString defaultLinkTitle() const;
    [[nodiscard]] Q_INVOKABLE QString titleForSong(qlonglong songId) const;
    [[nodiscard]] Q_INVOKABLE bool isSongLinked(qlonglong songId) const;
    [[nodiscard]] Q_INVOKABLE QStringList distinctFolderPaths() const;
    [[nodiscard]] Q_INVOKABLE QVariantList visibleUnlinkedSongIds() const;
    [[nodiscard]] Q_INVOKABLE QVariantList orderSongIdsForLinking(const QVariantList &songIds) const;

    Q_INVOKABLE void updateSongsLinkState(qlonglong groupId, const QVariantList &songIds);
    Q_INVOKABLE void clearSongsLinkState(const QVariantList &songIds);

public slots:
    void reload();

signals:
    void searchTextChanged();
    void folderFilterChanged();
    void hideContainersChanged();
    void containersOnlyChanged();
    void visibleUnlinkedCountChanged();
    void modelReset();

private:
    QString m_searchText;
    QString m_folderFilter;
    bool m_hideContainers{false};
    bool m_containersOnly{false};
};

class StubLinkGroupService : public QObject {
    Q_OBJECT

public:
    explicit StubLinkGroupService(QObject *parent = nullptr);

    [[nodiscard]] Q_INVOKABLE QVariantMap groupInfoForSong(qlonglong songId) const;
    [[nodiscard]] Q_INVOKABLE QVariantList allGroups() const;
    [[nodiscard]] Q_INVOKABLE bool addSongsToGroup(qlonglong groupId, const QVariantList &songIds);

public slots:
    qlonglong createGroupFromSongs(const QString &title,
                                   qlonglong primarySongId,
                                   const QVariantList &secondarySongIds);
    QVariantList dissolveGroupForSong(qlonglong primarySongId);
    bool updateGroupTitle(qlonglong primarySongId, const QString &title);

signals:
    void groupsChanged();
};

class StubAudioConfigController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool playing READ playing CONSTANT)
    Q_PROPERTY(bool loading READ loading CONSTANT)
    Q_PROPERTY(qint64 positionMs READ positionMs CONSTANT)
    Q_PROPERTY(qint64 durationMs READ durationMs CONSTANT)
    Q_PROPERTY(int tempoPercent READ tempoPercent CONSTANT)
    Q_PROPERTY(QString eqPresetId READ eqPresetId CONSTANT)
    Q_PROPERTY(QVariantList peaks READ peaks CONSTANT)
    Q_PROPERTY(QStringList presetNames READ presetNames CONSTANT)
    Q_PROPERTY(QString statusMessage READ statusMessage CONSTANT)
    Q_PROPERTY(qlonglong mediaFileId READ mediaFileId WRITE setMediaFileId NOTIFY mediaFileIdChanged)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(qint64 regionStartMs READ regionStartMs CONSTANT)
    Q_PROPERTY(qint64 regionEndMs READ regionEndMs CONSTANT)
    Q_PROPERTY(bool loopEnabled READ loopEnabled CONSTANT)
    Q_PROPERTY(bool canUndoRegion READ canUndoRegion CONSTANT)
    Q_PROPERTY(bool presetApplied READ presetApplied CONSTANT)
    Q_PROPERTY(QString presetNameInput READ presetNameInput WRITE setPresetNameInput NOTIFY
                   presetNameInputChanged)
    Q_PROPERTY(int selectedPresetIndex READ selectedPresetIndex WRITE setSelectedPresetIndex NOTIFY
                   selectedPresetIndexChanged)

public:
    explicit StubAudioConfigController(QObject *parent = nullptr);

    [[nodiscard]] qlonglong mediaFileId() const;
    void setMediaFileId(qlonglong mediaFileId);
    [[nodiscard]] QString displayName() const;
    [[nodiscard]] qint64 regionStartMs() const;
    [[nodiscard]] qint64 regionEndMs() const;
    [[nodiscard]] bool loopEnabled() const;
    [[nodiscard]] bool canUndoRegion() const;
    [[nodiscard]] bool presetApplied() const;
    [[nodiscard]] const QString &presetNameInput() const;
    void setPresetNameInput(const QString &name);
    [[nodiscard]] int selectedPresetIndex() const;
    void setSelectedPresetIndex(int index);

    [[nodiscard]] bool playing() const;
    [[nodiscard]] bool loading() const;
    [[nodiscard]] qint64 positionMs() const;
    [[nodiscard]] qint64 durationMs() const;
    [[nodiscard]] int tempoPercent() const;
    [[nodiscard]] QString eqPresetId() const;
    [[nodiscard]] QVariantList peaks() const;
    [[nodiscard]] QStringList presetNames() const;
    [[nodiscard]] QString statusMessage() const;

signals:
    void mediaFileIdChanged();
    void presetNameInputChanged();
    void selectedPresetIndexChanged();
    void transientNoticeRequested(const QString &message);

public slots:
    void reloadMedia() {}
    void playMediaFile(qlonglong mediaFileId) { Q_UNUSED(mediaFileId) }
    void preparePresetsForMedia(qlonglong mediaFileId) { Q_UNUSED(mediaFileId) }
    void loadPresetForMedia(qlonglong mediaFileId, int presetIndex) {
        Q_UNUSED(mediaFileId)
        Q_UNUSED(presetIndex)
    }
    void togglePlayback() {}
    void stopPlayback() {}
    void commitTempo() {}
    void setRegionFromPosition(bool isStartMarker) { Q_UNUSED(isStartMarker) }
    void undoRegion() {}
    void savePreset() {}
    void loadSelectedPreset() {}
    void deleteSelectedPreset() {}

private:
    qlonglong m_mediaFileId{0};
    QString m_presetNameInput;
    int m_selectedPresetIndex{-1};
    static constexpr const int m_tempoPercent{100};
};

class StubReminderController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString statusMessage READ statusMessage CONSTANT)
    Q_PROPERTY(int dayReminderCount READ dayReminderCount CONSTANT)
    Q_PROPERTY(int dailyReminderCount READ dailyReminderCount CONSTANT)
    Q_PROPERTY(int periodicReminderCount READ periodicReminderCount CONSTANT)
    Q_PROPERTY(bool showAllReminders READ showAllReminders WRITE setShowAllReminders NOTIFY
                   showAllRemindersChanged)
    Q_PROPERTY(QAbstractListModel *dayReminders READ dayReminders CONSTANT)

public:
    explicit StubReminderController(QObject *parent = nullptr);

    [[nodiscard]] QString statusMessage() const;
    [[nodiscard]] int dayReminderCount() const;
    [[nodiscard]] int dailyReminderCount() const;
    [[nodiscard]] int periodicReminderCount() const;
    [[nodiscard]] bool showAllReminders() const;
    [[nodiscard]] QAbstractListModel *dayReminders();

    void setShowAllReminders(bool showAll);

public slots:
    void reloadDayReminders();
    void deleteReminder(qlonglong reminderId);

signals:
    void showAllRemindersChanged();

private:
    StubDayReminderModel m_dayReminders;
    bool m_showAllReminders{false};
};

class QmlServiceStubs : public QObject {
    Q_OBJECT

public:
    explicit QmlServiceStubs(QObject *parent = nullptr);

    StubImportService importService;
    StubPracticeTracker practiceTracker;
    StubPracticeSession practiceSession;
    StubSongModel songModel;
    StubMediaFileModel mediaFileModel;
    StubLibraryLinkModel libraryLinkModel;
    StubLinkGroupService linkGroupService;
    StubReminderController reminderController;
    StubAudioConfigController audioConfigController;
};

void registerQmlServiceStubs(QQmlContext *context, QmlServiceStubs &stubs);

#endif // QMLSERVICESTUBS_H
