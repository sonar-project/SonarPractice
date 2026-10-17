#ifndef IMPORTORGANIZERCONTROLLER_H
#define IMPORTORGANIZERCONTROLLER_H

#include "ImportOrganizerSession.h"
#include "ImportOrganizerTreeModel.h"
#include "ImportTypes.h"

#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <atomic>

#include <QtQml/qqmlregistration.h>

class AppSettings;
class IMediaFileRepository;
class ImportService;
class IConfigProvider;

/**
 * \brief QML controller for the copy/move Import Organizer (source → target).
 */
class ImportOrganizerController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Instances are provided via the 'importOrganizer' context property.")

    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(bool canCommit READ canCommit NOTIFY planChanged)
    Q_PROPERTY(int plannedCount READ plannedCount NOTIFY planChanged)
    Q_PROPERTY(int sourceCount READ sourceCount NOTIFY planChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString managedRootName READ managedRootName NOTIFY activeChanged)
    Q_PROPERTY(int filterMode READ filterMode WRITE setFilterMode NOTIFY filterChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY filterChanged)
    Q_PROPERTY(ImportOrganizerTreeModel *sourceModel READ sourceModel CONSTANT)
    Q_PROPERTY(ImportOrganizerTreeModel *targetModel READ targetModel CONSTANT)
    Q_PROPERTY(qint64 selectedSourceNodeId READ selectedSourceNodeId WRITE setSelectedSourceNodeId
                   NOTIFY selectionChanged)
    Q_PROPERTY(qint64 selectedTargetNodeId READ selectedTargetNodeId WRITE setSelectedTargetNodeId
                   NOTIFY selectionChanged)

  public:
    struct Dependencies {
        ImportService &importService;
        IMediaFileRepository &mediaFileRepo;
        const IConfigProvider &config;
        const AppSettings &appSettings;
    };

    explicit ImportOrganizerController(Dependencies dependencies, QObject *parent = nullptr);

    [[nodiscard]] bool active() const;
    [[nodiscard]] bool scanning() const;
    [[nodiscard]] bool canCommit() const;
    [[nodiscard]] int plannedCount() const;
    [[nodiscard]] int sourceCount() const;
    [[nodiscard]] QString statusMessage() const;
    [[nodiscard]] QString managedRootName() const;
    [[nodiscard]] int filterMode() const;
    void setFilterMode(int mode);
    [[nodiscard]] QString searchText() const;
    void setSearchText(const QString &text);
    [[nodiscard]] ImportOrganizerTreeModel *sourceModel();
    [[nodiscard]] ImportOrganizerTreeModel *targetModel();
    [[nodiscard]] qint64 selectedSourceNodeId() const;
    void setSelectedSourceNodeId(qint64 nodeId);
    [[nodiscard]] qint64 selectedTargetNodeId() const;
    void setSelectedTargetNodeId(qint64 nodeId);

    Q_INVOKABLE void begin(const QStringList &paths);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void commit();
    Q_INVOKABLE void mapSelection(bool includeFolderWrapper = true);
    Q_INVOKABLE void mapSelectionContentsOnly();
    Q_INVOKABLE void unmapSelection();
    Q_INVOKABLE void addFolder(const QString &folderName);
    Q_INVOKABLE void setChecked(qint64 nodeId, bool checked);
    Q_INVOKABLE void toggleExpanded(bool sourceSide, qint64 nodeId);
    Q_INVOKABLE void expandAll(bool sourceSide);
    Q_INVOKABLE void collapseAll(bool sourceSide);

  signals:
    void activeChanged();
    void scanningChanged();
    void planChanged();
    void statusMessageChanged();
    void filterChanged();
    void selectionChanged();
    void scanFailed(const QString &message);

  private slots:
    void handleScanFinished(bool ok, const QString &message);

  private:
    void refreshModels();
    void setActive(bool active);
    void setScanning(bool scanning);
    void setStatusMessage(const QString &message);
    [[nodiscard]] QStringList existingManagedFolderPaths() const;
    [[nodiscard]] QSet<QString> existingHashes() const;

    Dependencies m_dependencies;
    ImportOrganizerSession m_session;
    ImportOrganizerTreeModel m_sourceModel;
    ImportOrganizerTreeModel m_targetModel;
    std::atomic_bool m_scanCancel{false};
    bool m_active{false};
    bool m_scanning{false};
    QString m_statusMessage{};
    qint64 m_selectedSourceNodeId{ImportOrganizerSession::kInvalidNodeId};
    qint64 m_selectedTargetNodeId{ImportOrganizerSession::kRootNodeId};
};

#endif // IMPORTORGANIZERCONTROLLER_H
