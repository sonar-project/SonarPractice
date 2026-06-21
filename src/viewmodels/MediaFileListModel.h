#ifndef MEDIAFILELISTMODEL_H
#define MEDIAFILELISTMODEL_H

#include "MediaFile.h"

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

class IMediaFileRepository;
class ILinkGroupRepository;
class IFileRelationRepository;

/**
 * @brief Media files for one song, including link-group siblings.
 *
 * Emits filesForKindChanged() per kind so QML can patch caches instead of
 * rebinding on every reload().
 */
class MediaFileListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Instances are provided via the 'mediaFileModel' context property.")
    Q_PROPERTY(qlonglong songId READ songId WRITE setSongId NOTIFY songIdChanged)
    Q_PROPERTY(int count READ mediaCount NOTIFY mediaCountChanged)

  public:
    enum Roles : uint16_t {
        MediaIdRole = Qt::UserRole + 1,
        SongIdRole,
        FilePathRole,
        FileTypeRole,
        MediaKindRole,
        SourceTypeRole,
        IsManagedRole,
        CanBePracticedRole,
        DisplayNameRole
    };
    Q_ENUM(Roles)

    explicit MediaFileListModel(IMediaFileRepository &mediaFileRepo,
                                ILinkGroupRepository &linkGroupRepo,
                                IFileRelationRepository &fileRelationRepo,
                                QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] qlonglong songId() const;
    [[nodiscard]] int mediaCount() const;

    /** Snapshot for one kind; prefer caching in QML and listen to filesForKindChanged(). */
    Q_INVOKABLE QVariantList filesForKind(const QString &mediaKind) const;

    [[nodiscard]] static QString displayNameForFile(const MediaFile &file);
    [[nodiscard]] static QString mediaKindToString(MediaKind kind);

  public slots:
    void setSongId(qlonglong songId);
    void reload();

  signals:
    void songIdChanged();
    void mediaCountChanged();

    /** Emitted from reload() when the file list for @p mediaKind changed. */
    void filesForKindChanged(const QString &mediaKind);

  private:
    /** Maps a linked member song id to the link-group primary song id. */
    [[nodiscard]] qlonglong resolveHubSongId(qlonglong songId) const;

    void emitChangedKinds(const QList<MediaFile> &oldFiles);

    IMediaFileRepository &m_mediaFileRepo;
    ILinkGroupRepository &m_linkGroupRepo;
    IFileRelationRepository &m_fileRelationRepo;
    qlonglong m_songId{};
    QList<MediaFile> m_files{};
};

#endif // MEDIAFILELISTMODEL_H
