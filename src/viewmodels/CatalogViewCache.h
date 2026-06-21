#ifndef CATALOGVIEWCACHE_H
#define CATALOGVIEWCACHE_H

#include "CatalogSnapshot.h"

#include <QHash>
#include <QList>
#include <QVariantList>

#include <memory>

struct SongListRowData {
    qlonglong id{};
    QString title{};
    int baseBpm{};
    qlonglong artistId{};
    QString artistName{};
    qlonglong tuningId{};
    QString tuningName{};
    QVariantList assetSummary{};
    bool isLinkedGroup{false};
    QString linkGroupTitle{};
    int linkedMediaCount{};
    QString displayTitle{};
    qlonglong linkGroupId{};
    bool isContainerMember{false};
    qlonglong hubSongId{};
    QString searchHaystack{};
};

struct LibraryLinkRowData {
    qlonglong songId{};
    QString title{};
    qlonglong mediaId{};
    QString mediaKind{};
    QString importRoot{};
    QString sourceRelativePath{};
    QString folderPath{};
    bool isPrimary{false};
    bool isLinked{false};
    QString linkGroupTitle{};
    QString artistName{};
    QString searchHaystack{};
};

/**
 * @brief Pre-built catalog rows for view models, loaded on a worker thread.
 */
class CatalogViewCache {
  public:
    [[nodiscard]] static CatalogViewCache load(const CatalogSnapshot::Dependencies &deps);
    [[nodiscard]] static CatalogViewCache fromSnapshot(CatalogSnapshot snapshot);

    [[nodiscard]] const CatalogSnapshot &snapshot() const;
    [[nodiscard]] std::shared_ptr<const CatalogSnapshot> sharedSnapshot() const;
    [[nodiscard]] const QList<SongListRowData> &songAllRows() const;
    [[nodiscard]] const QList<SongListRowData> &songSecondaryRows() const;
    [[nodiscard]] const QList<LibraryLinkRowData> &libraryRows() const;
    [[nodiscard]] int folderCountFor(const QString &folderPath, bool includeSubfolders) const;
    [[nodiscard]] const QHash<QString, int> &folderCountDirect() const;
    [[nodiscard]] const QHash<QString, int> &folderCountRecursive() const;

  private:
    CatalogViewCache(std::shared_ptr<const CatalogSnapshot> snapshot,
                     QList<SongListRowData> songAllRows, QList<SongListRowData> songSecondaryRows,
                     QList<LibraryLinkRowData> libraryRows, QHash<QString, int> folderCountDirect,
                     QHash<QString, int> folderCountRecursive);

    std::shared_ptr<const CatalogSnapshot> m_snapshot{};
    QList<SongListRowData> m_songAllRows{};
    QList<SongListRowData> m_songSecondaryRows{};
    QList<LibraryLinkRowData> m_libraryRows{};
    QHash<QString, int> m_folderCountDirect{};
    QHash<QString, int> m_folderCountRecursive{};
};

using CatalogViewCachePtr = std::shared_ptr<CatalogViewCache>;

Q_DECLARE_METATYPE(CatalogViewCachePtr)

#endif // CATALOGVIEWCACHE_H
