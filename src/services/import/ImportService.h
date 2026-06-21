#ifndef IMPORTSERVICE_H
#define IMPORTSERVICE_H

#include "IImportService.h"

#include "AppSettings.h"
#include "LibraryManager.h"
#include "interfaces/IArtistRepository.h"
#include "interfaces/IMediaFileRepository.h"
#include "interfaces/ISongRepository.h"
#include "interfaces/ITuningRepository.h"
#include "interfaces/iConfigProvider.h"

#include <QObject>
#include <atomic>

#include <QtQml/qqmlregistration.h>

class ImportService : public QObject, public IImportService {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(int progressCurrent READ progressCurrent NOTIFY progressChanged)
    Q_PROPERTY(int progressTotal READ progressTotal NOTIFY progressChanged)
    Q_PROPERTY(QString progressFileName READ progressFileName NOTIFY progressChanged)
    Q_PROPERTY(int lastImportedCount READ lastImportedCount NOTIFY lastSummaryChanged)
    Q_PROPERTY(int lastSkippedCount READ lastSkippedCount NOTIFY lastSummaryChanged)
    Q_PROPERTY(int lastFailedCount READ lastFailedCount NOTIFY lastSummaryChanged)

  public:
    struct Dependencies {
        IArtistRepository &artistRepo;
        ITuningRepository &tuningRepo;
        ISongRepository &songRepo;
        IMediaFileRepository &mediaFileRepo;
        LibraryManager &libraryManager;
        const IConfigProvider &config;
        const AppSettings &appSettings;
        QString databasePath;
    };

    explicit ImportService(Dependencies dependencies, QObject *parent = nullptr);

    [[nodiscard]] bool isBusy() const override;

    ImportResult importFile(const QString &absolutePath,
                            StorageStrategy strategy = StorageStrategy::Link) override;

    void importDirectory(const QString &directoryPath,
                         StorageStrategy strategy = StorageStrategy::Link) override;

    Q_INVOKABLE void importPaths(const QStringList &paths,
                                 StorageStrategy strategy = StorageStrategy::Link);

    [[nodiscard]] const QString &statusMessage() const;
    [[nodiscard]] int progressCurrent() const;
    [[nodiscard]] int progressTotal() const;
    [[nodiscard]] const QString &progressFileName() const;
    [[nodiscard]] int lastImportedCount() const;
    [[nodiscard]] int lastSkippedCount() const;
    [[nodiscard]] int lastFailedCount() const;

  public slots:
    void clearStatusMessage();
    void cancelImport() override;

  private slots:
    void handleBatchProgress(int current, int total, const QString &currentFile,
                             const ImportResult &result);
    void handleBatchFinished(const ImportSummary &summary);
    void handleBatchFailed(const QString &message);

  signals:
    void busyChanged();
    void statusMessageChanged();
    void progressChanged();
    void lastSummaryChanged();
    void importProgress(int current, int total, const QString &currentFile);
    void fileImported(const ImportResult &result);
    void importFinished(const ImportSummary &summary);
    void errorOccurred(const QString &message);

  private:
    void startBatchImport(const QList<ImportFileEntry> &files, StorageStrategy strategy);

    [[nodiscard]] bool isCancelled() const;
    [[nodiscard]] bool tryBeginImport();
    void endImport();
    void setStatusMessage(const QString &message);
    void setProgress(int current, int total, const QString &fileName);
    void emitImportFinished(const ImportSummary &summary);

    Dependencies m_dependencies;
    std::atomic_bool m_busy{false};
    std::atomic_bool m_cancelRequested{false};
    QString m_statusMessage{};
    int m_progressCurrent{};
    int m_progressTotal{};
    QString m_progressFileName{};
    int m_lastImportedCount{};
    int m_lastSkippedCount{};
    int m_lastFailedCount{};
};

#endif // IMPORTSERVICE_H
