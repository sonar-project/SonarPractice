/**
 * @file GuitarProPreviewController.cpp
 * @brief Async Guitar Pro ASCII tab preview for the Practice Hub.
 */

#include "GuitarProPreviewController.h"

#include "ApplicationErrorLog.h"
#include "IPathResolver.h"
#include "MediaFile.h"
#include "interfaces/IMediaFileRepository.h"

#include <QMetaObject>
#include <QThread>

GuitarProPreviewController::GuitarProPreviewController(const Dependencies &dependencies,
                                                       QObject *parent)
    : QObject(parent), m_dependencies(dependencies) {}

bool GuitarProPreviewController::loading() const { return m_loading; }

bool GuitarProPreviewController::loaded() const { return m_loaded; }

bool GuitarProPreviewController::visible() const { return m_visible; }

void GuitarProPreviewController::setVisible(bool visible) {
    if (m_visible == visible) {
        return;
    }
    m_visible = visible;
    emit visibleChanged();
}

const QString &GuitarProPreviewController::errorMessage() const { return m_errorMessage; }

const QString &GuitarProPreviewController::title() const { return m_title; }

const QString &GuitarProPreviewController::artist() const { return m_artist; }

const QString &GuitarProPreviewController::tuning() const { return m_tuning; }

const QString &GuitarProPreviewController::tabText() const { return m_tabText; }

const QStringList &GuitarProPreviewController::trackNames() const { return m_trackNames; }

int GuitarProPreviewController::selectedTrackIndex() const { return m_selectedTrackIndex; }

qlonglong GuitarProPreviewController::mediaFileId() const { return m_mediaFileId; }

void GuitarProPreviewController::setSelectedTrackIndex(int index) {
    if (index < 0 || index >= static_cast<int>(m_preview.tracks.size())) {
        return;
    }
    if (m_selectedTrackIndex == index) {
        return;
    }
    m_selectedTrackIndex = index;
    emit selectedTrackIndexChanged();
    applySelectedTrack();
}

void GuitarProPreviewController::clear() {
    ++m_loadGeneration;
    setLoading(false);
    resetPreviewState();
    setVisible(false);
}

void GuitarProPreviewController::load(qlonglong mediaFileId) {
    if (mediaFileId <= 0) {
        setErrorMessage(tr("Invalid media file"));
        setVisible(true);
        return;
    }

    const std::optional<MediaFile> mediaFile = m_dependencies.mediaRepo.getMediaFile(mediaFileId);
    if (!mediaFile.has_value()) {
        setErrorMessage(tr("Media file not found"));
        m_dependencies.errorLog.logError(QStringLiteral("GuitarProPreview.load"), m_errorMessage);
        setVisible(true);
        return;
    }

    if (mediaFile->mediaKind != MediaKind::GuitarPro) {
        setErrorMessage(tr("Only Guitar Pro files can be previewed"));
        setVisible(true);
        return;
    }

    const QString path = m_dependencies.pathResolver.resolve(*mediaFile);
    const int generation = ++m_loadGeneration;

    setVisible(true);
    setLoading(true);
    setErrorMessage({});
    if (m_mediaFileId != mediaFileId) {
        m_mediaFileId = mediaFileId;
        emit mediaFileIdChanged();
    }

    QThread *thread = QThread::create([this, path, mediaFileId, generation]() {
        LoadResult result;
        result.mediaFileId = mediaFileId;
        result.generation = generation;

        const std::optional<AsciiTabRenderer::SongPreview> preview =
            AsciiTabRenderer::loadFromFile(path);
        if (!preview.has_value()) {
            result.ok = false;
            result.errorMessage = tr("Could not parse Guitar Pro file");
        } else if (preview->tracks.empty()) {
            result.ok = false;
            result.errorMessage = tr("Guitar Pro file has no tracks");
            result.preview = *preview;
        } else {
            result.ok = true;
            result.preview = *preview;
        }

        QMetaObject::invokeMethod(
            this, [this, result = std::move(result)]() { applyLoadResult(result); },
            Qt::QueuedConnection);
    });

    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void GuitarProPreviewController::applyLoadResult(const LoadResult &result) {
    if (result.generation != m_loadGeneration.load()) {
        return;
    }

    setLoading(false);

    if (!result.ok) {
        resetPreviewState();
        m_mediaFileId = result.mediaFileId;
        emit mediaFileIdChanged();
        setErrorMessage(result.errorMessage);
        m_dependencies.errorLog.logError(QStringLiteral("GuitarProPreview.load"),
                                         result.errorMessage);
        return;
    }

    m_preview = result.preview;
    m_mediaFileId = result.mediaFileId;
    emit mediaFileIdChanged();

    if (m_title != m_preview.title) {
        m_title = m_preview.title;
        emit titleChanged();
    }
    if (m_artist != m_preview.artist) {
        m_artist = m_preview.artist;
        emit artistChanged();
    }

    QStringList names;
    names.reserve(static_cast<int>(m_preview.tracks.size()));
    int defaultIndex = 0;
    for (int i = 0; i < static_cast<int>(m_preview.tracks.size()); ++i) {
        const AsciiTabRenderer::TrackPreview &track =
            m_preview.tracks.at(static_cast<std::size_t>(i));
        QString label = track.name.isEmpty() ? tr("Track %1").arg(i + 1) : track.name;
        if (track.percussion) {
            label += tr(" (percussion)");
        }
        names.append(label);
    }
    for (int i = 0; i < static_cast<int>(m_preview.tracks.size()); ++i) {
        if (!m_preview.tracks.at(static_cast<std::size_t>(i)).percussion) {
            defaultIndex = i;
            break;
        }
    }

    if (m_trackNames != names) {
        m_trackNames = names;
        emit trackNamesChanged();
    }

    m_selectedTrackIndex = defaultIndex;
    emit selectedTrackIndexChanged();
    applySelectedTrack();

    if (!m_loaded) {
        m_loaded = true;
        emit loadedChanged();
    }
    setErrorMessage({});
}

void GuitarProPreviewController::applySelectedTrack() {
    if (m_selectedTrackIndex < 0 ||
        m_selectedTrackIndex >= static_cast<int>(m_preview.tracks.size())) {
        if (!m_tuning.isEmpty()) {
            m_tuning.clear();
            emit tuningChanged();
        }
        if (!m_tabText.isEmpty()) {
            m_tabText.clear();
            emit tabTextChanged();
        }
        return;
    }

    const AsciiTabRenderer::TrackPreview &track =
        m_preview.tracks.at(static_cast<std::size_t>(m_selectedTrackIndex));
    if (m_tuning != track.tuningDisplay) {
        m_tuning = track.tuningDisplay;
        emit tuningChanged();
    }
    if (m_tabText != track.tabText) {
        m_tabText = track.tabText;
        emit tabTextChanged();
    }
}

void GuitarProPreviewController::setLoading(bool loading) {
    if (m_loading == loading) {
        return;
    }
    m_loading = loading;
    emit loadingChanged();
}

void GuitarProPreviewController::setErrorMessage(const QString &message) {
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = message;
    emit errorMessageChanged();
}

void GuitarProPreviewController::resetPreviewState() {
    const bool hadLoaded = m_loaded;
    m_preview = {};
    m_mediaFileId = 0;
    emit mediaFileIdChanged();

    if (!m_title.isEmpty()) {
        m_title.clear();
        emit titleChanged();
    }
    if (!m_artist.isEmpty()) {
        m_artist.clear();
        emit artistChanged();
    }
    if (!m_tuning.isEmpty()) {
        m_tuning.clear();
        emit tuningChanged();
    }
    if (!m_tabText.isEmpty()) {
        m_tabText.clear();
        emit tabTextChanged();
    }
    if (!m_trackNames.isEmpty()) {
        m_trackNames.clear();
        emit trackNamesChanged();
    }
    if (m_selectedTrackIndex != -1) {
        m_selectedTrackIndex = -1;
        emit selectedTrackIndexChanged();
    }
    if (hadLoaded) {
        m_loaded = false;
        emit loadedChanged();
    }
    setErrorMessage({});
}
