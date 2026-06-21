/**
 * @file MediaFileListModel.cpp
 * @brief Loads own and link-group media files for the song list model.
 */

#include "MediaFileListModel.h"

#include "MediaFileMapping.h"
#include "interfaces/IFileRelationRepository.h"
#include "interfaces/ILinkGroupRepository.h"
#include "interfaces/IMediaFileRepository.h"

#include <QFileInfo>
#include <QUrl>

namespace {

    /** Must match the string constants in MediaKind.js. */
    QString kindToString(MediaKind kind) {
        switch (kind) {
        case MediaKind::GuitarPro:
            return QStringLiteral("guitarpro");
        case MediaKind::Audio:
            return QStringLiteral("audio");
        case MediaKind::Video:
            return QStringLiteral("video");
        case MediaKind::Image:
            return QStringLiteral("image");
        case MediaKind::Document:
            return QStringLiteral("document");
        case MediaKind::Unknown:
            return QStringLiteral("unknown");
        }
        return QStringLiteral("unknown");
    }

    QSet<QString> kindsInList(const QList<MediaFile> &files) {
        QSet<QString> result;
        result.reserve(files.size());
        for (const MediaFile &f : files)
            result.insert(kindToString(f.mediaKind));
        return result;
    }

    QList<MediaFile> filesOfKind(const QList<MediaFile> &files, MediaKind kind) {
        QList<MediaFile> result;
        for (const MediaFile &f : files) {
            if (f.mediaKind == kind)
                result.append(f);
        }
        return result;
    }

    MediaKind kindFromString(const QString &s) {
        if (s == QLatin1String("guitarpro"))
            return MediaKind::GuitarPro;
        if (s == QLatin1String("audio"))
            return MediaKind::Audio;
        if (s == QLatin1String("video"))
            return MediaKind::Video;
        if (s == QLatin1String("image"))
            return MediaKind::Image;
        if (s == QLatin1String("document"))
            return MediaKind::Document;
        return MediaKind::Unknown;
    }

} // namespace

MediaFileListModel::MediaFileListModel(IMediaFileRepository &mediaFileRepo,
                                       ILinkGroupRepository &linkGroupRepo,
                                       IFileRelationRepository &fileRelationRepo, QObject *parent)
    : QAbstractListModel(parent), m_mediaFileRepo(mediaFileRepo), m_linkGroupRepo(linkGroupRepo),
      m_fileRelationRepo(fileRelationRepo) {}

int MediaFileListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_files.size());
}

int MediaFileListModel::mediaCount() const { return static_cast<int>(m_files.size()); }

QVariant MediaFileListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_files.size()) {
        return {};
    }

    const MediaFile &file = m_files.at(index.row());

    switch (role) {
    case MediaIdRole:
        return file.id;
    case SongIdRole:
        return file.songId;
    case FilePathRole:
        return file.filePath;
    case FileTypeRole:
        return file.fileType;
    case MediaKindRole:
        return MediaFileMapping::mediaKindToString(file.mediaKind);
    case SourceTypeRole:
        return MediaFileMapping::sourceTypeToString(file.sourceType);
    case IsManagedRole:
        return file.isManaged;
    case CanBePracticedRole:
        return file.canBePracticed;
    case DisplayNameRole:
        return displayNameForFile(file);
    default:
        return {};
    }
}

QHash<int, QByteArray> MediaFileListModel::roleNames() const {
    return {
        {MediaIdRole, "mediaId"},         {SongIdRole, "songId"},
        {FilePathRole, "filePath"},       {FileTypeRole, "fileType"},
        {MediaKindRole, "mediaKind"},     {SourceTypeRole, "sourceType"},
        {IsManagedRole, "isManaged"},     {CanBePracticedRole, "canBePracticed"},
        {DisplayNameRole, "displayName"},
    };
}

qlonglong MediaFileListModel::songId() const { return m_songId; }

void MediaFileListModel::setSongId(qlonglong songId) {
    if (m_songId == songId) {
        return;
    }

    m_songId = songId;
    emit songIdChanged();
    reload();
}

qlonglong MediaFileListModel::resolveHubSongId(qlonglong songId) const {
    const QList<MediaFile> files = m_mediaFileRepo.getMediaFilesBySongId(songId);
    if (files.isEmpty()) {
        return songId;
    }

    const std::optional<qlonglong> primaryMediaId =
        m_fileRelationRepo.getPrimaryMediaId(files.first().id);
    if (!primaryMediaId.has_value()) {
        return songId;
    }

    const std::optional<MediaFile> primaryMedia = m_mediaFileRepo.getMediaFile(*primaryMediaId);
    if (!primaryMedia.has_value()) {
        return songId;
    }

    return primaryMedia->songId;
}

QString MediaFileListModel::displayNameForFile(const MediaFile &file) {
    if (file.sourceType == MediaSourceType::Url) {
        const QUrl url(file.filePath);
        if (url.isValid()) {
            return url.host() + url.path();
        }
    }

    if (!file.sourceRelativePath.isEmpty()) {
        return QFileInfo(file.sourceRelativePath).fileName();
    }

    return QFileInfo(file.filePath).fileName();
}

QVariantList MediaFileListModel::filesForKind(const QString &mediaKind) const {
    const MediaKind kind = kindFromString(mediaKind);

    QVariantList result;
    for (const MediaFile &f : m_files) {
        if (f.mediaKind != kind)
            continue;

        QVariantMap entry;
        entry[QStringLiteral("mediaId")] = f.id;
        entry[QStringLiteral("songId")] = f.songId;
        entry[QStringLiteral("filePath")] = f.filePath;
        entry[QStringLiteral("fileType")] = f.fileType;
        entry[QStringLiteral("mediaKind")] = mediaKind;
        entry[QStringLiteral("sourceType")] = (f.sourceType == MediaSourceType::Local)
                                                  ? QStringLiteral("LOCAL")
                                                  : QStringLiteral("URL");
        entry[QStringLiteral("isManaged")] = f.isManaged;
        entry[QStringLiteral("canBePracticed")] = f.canBePracticed;
        entry[QStringLiteral("displayName")] = displayNameForFile(f);
        result.append(entry);
    }
    return result;
}

void MediaFileListModel::emitChangedKinds(const QList<MediaFile> &oldFiles) {
    // Union of kinds present in either the old or new file list
    const QSet<QString> allKinds = kindsInList(oldFiles) | kindsInList(m_files);

    for (const QString &kindStr : allKinds) {
        const MediaKind kind = kindFromString(kindStr);
        if (filesOfKind(oldFiles, kind) != filesOfKind(m_files, kind))
            emit filesForKindChanged(kindStr);
    }
}

void MediaFileListModel::reload() {
    const QList<MediaFile> oldFiles = m_files;
    const qsizetype oldCount = m_files.size();

    beginResetModel();

    const qlonglong resolvedId = resolveHubSongId(m_songId);

    // 1. Own media for the hub song (Guitar Pro file of the primary song id)
    QList<MediaFile> allFiles = m_mediaFileRepo.getMediaFilesBySongId(resolvedId);

    // 2. Linked media via the group's primary media id (file_relations → video/audio/…)
    const auto group = m_linkGroupRepo.getGroupByPrimarySong(resolvedId);
    if (group.has_value()) {
        const QList<MediaFile> linked = m_fileRelationRepo.getLinkedMedia(group->primaryMediaId);
        allFiles.append(linked);
    }

    m_files = allFiles;
    endResetModel();

    if (m_files.size() != oldCount)
        emit mediaCountChanged();

    emitChangedKinds(oldFiles);
}