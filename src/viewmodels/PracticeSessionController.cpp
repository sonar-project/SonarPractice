/**
 * @file PracticeSessionController.cpp
 * @brief External file launch for practice and asset-open flows.
 */

#include "PracticeSessionController.h"

#include "ApplicationErrorLog.h"
#include "ILauncher.h"
#include "IPathResolver.h"
#include "interfaces/IMediaFileRepository.h"

PracticeSessionController::PracticeSessionController(const Dependencies &dependencies,
                                                     QObject *parent)
    : QObject(parent), m_dependencies(dependencies) {}

bool PracticeSessionController::openAsset(qlonglong mediaFileId) {
    return launchMediaFile(mediaFileId, QStringLiteral("openAsset"));
}

bool PracticeSessionController::startPractice(qlonglong mediaFileId) {
    const std::optional<MediaFile> mediaFile = m_dependencies.mediaRepo.getMediaFile(mediaFileId);
    if (!mediaFile.has_value()) {
        return reportFailure(QStringLiteral("startPractice"), tr("Media file not found"));
    }

    if (!mediaFile->canBePracticed) {
        return reportFailure(QStringLiteral("startPractice"),
                             tr("This media file cannot be used for practice"));
    }

    return launchMediaFile(mediaFileId, QStringLiteral("startPractice"));
}

bool PracticeSessionController::launchMediaFile(qlonglong mediaFileId, const QString &context) {
    const std::optional<MediaFile> mediaFile = m_dependencies.mediaRepo.getMediaFile(mediaFileId);
    if (!mediaFile.has_value()) {
        return reportFailure(context, tr("Media file not found"));
    }

    const QString target = m_dependencies.pathResolver.resolve(*mediaFile);
    if (!m_dependencies.launcher.open(target, mediaFile->sourceType)) {
        return reportFailure(context, tr("Could not open: %1").arg(target));
    }

    return true;
}

bool PracticeSessionController::reportFailure(const QString &context, const QString &message) {
    m_dependencies.errorLog.logError(
        QStringLiteral("PracticeSession.%1").arg(context), message);
    emit launchFailed(message);
    return false;
}
