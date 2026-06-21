#ifndef IFILERELATIONREPOSITORY_H
#define IFILERELATIONREPOSITORY_H

#include <optional>
#include <utility>

#include <QList>

#include "MediaFile.h"

class IFileRelationRepository {
  public:
    virtual ~IFileRelationRepository() = default;

    IFileRelationRepository(const IFileRelationRepository &) = delete;
    IFileRelationRepository &operator=(const IFileRelationRepository &) = delete;
    IFileRelationRepository(IFileRelationRepository &&) = delete;
    IFileRelationRepository &operator=(IFileRelationRepository &&) = delete;

    virtual bool linkToPrimary(qlonglong primaryMediaId, qlonglong secondaryMediaId) = 0;
    virtual bool unlink(qlonglong secondaryMediaId) = 0;
    virtual bool deleteRelationsForPrimary(qlonglong primaryMediaId) = 0;
    virtual QList<MediaFile> getLinkedMedia(qlonglong primaryMediaId) = 0;
    virtual bool isSecondaryMedia(qlonglong mediaId) = 0;
    virtual std::optional<qlonglong> getPrimaryMediaId(qlonglong mediaId) = 0;
    /** @brief Returns every file relation as primary to secondary media id pairs. */
    virtual QList<std::pair<qlonglong, qlonglong>> getAllRelationPairs() = 0;
    virtual bool isMediaInAnyGroup(qlonglong mediaId) = 0;

  protected:
    IFileRelationRepository() = default;
};

#endif // IFILERELATIONREPOSITORY_H
