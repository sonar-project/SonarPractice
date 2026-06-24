/**
 * @file AudioConfigController.cpp
 * @brief Bridges AudioPlaybackEngine to QML and coordinates preset/region state.
 */

#include "AudioConfigController.h"

#include <utility>

#include "AudioConfigPreset.h"
#include "AudioConstants.h"
#include "IPathResolver.h"
#include "MediaFile.h"
#include "interfaces/IAudioConfigPresetRepository.h"
#include "interfaces/IMediaFileRepository.h"

AudioConfigController::AudioConfigController(const Dependencies &dependencies, QObject *parent)
    : QObject(parent), m_dependencies(dependencies) {
    connectEngine();
}

void AudioConfigController::connectEngine() {
    connect(&m_engine, &AudioPlaybackEngine::playingChanged, this,
            &AudioConfigController::playingChanged);

    connect(&m_engine, &AudioPlaybackEngine::loadingChanged, this, [this]() {
        if (!m_engine.isLoading() && m_cancelPending) {
            m_cancelPending = false;
            setStatusMessage(tr("Processing cancelled."));
        }
        if (!m_engine.isLoading() && m_pendingPlayAfterLoad && m_engine.durationMs() > 0 &&
            !m_engine.hasPlaybackData()) {
            m_pendingPlayAfterLoad = false;
            m_engine.play();
        }
        emit loadingChanged();
    });

    connect(&m_engine, &AudioPlaybackEngine::processingStageChanged, this,
            &AudioConfigController::processingStageChanged);
    connect(&m_engine, &AudioPlaybackEngine::processingProgressChanged, this,
            &AudioConfigController::processingProgressChanged);

    connect(&m_engine, &AudioPlaybackEngine::positionMsChanged, this,
            &AudioConfigController::positionMsChanged);

    connect(&m_engine, &AudioPlaybackEngine::durationMsChanged, this,
            &AudioConfigController::durationMsChanged);

    connect(&m_engine, &AudioPlaybackEngine::peaksChanged, this, [this]() {
        if (m_loadingTargetMediaFileId > 0 && m_mediaFileId != m_loadingTargetMediaFileId) {
            return;
        }
        syncPeaksFromEngine();
        if (!m_engine.peaks().isEmpty() && m_engine.durationMs() > 0) {
            m_engineLoadedMediaFileId =
                m_loadingTargetMediaFileId > 0 ? m_loadingTargetMediaFileId : m_mediaFileId;
            syncRegionToEngine();
            if (!m_engine.hasPlaybackData() && m_engine.lastError().isEmpty()) {
                setStatusMessage(tr("Waveform ready."));
            }
        }
    });

    connect(&m_engine, &AudioPlaybackEngine::errorChanged, this, [this]() {
        if (!m_engine.lastError().isEmpty()) {
            setStatusMessage(m_engine.lastError());
        }
    });

    connect(&m_engine, &AudioPlaybackEngine::playbackReadyChanged, this, [this]() {
        if (m_loadingTargetMediaFileId > 0 && m_mediaFileId != m_loadingTargetMediaFileId) {
            return;
        }
        const qint64 newDurationMs = m_engine.durationMs();
        const bool initialLoad = m_lastPlaybackDurationMs <= 0 || m_regionEndMs <= 0;
        if (initialLoad) {
            qint64 regionEnd = m_regionEndMs;
            if (regionEnd <= 0) {
                regionEnd = newDurationMs;
            }
            applyRegionMs(m_regionStartMs, regionEnd, false);
        } else if (m_lastPlaybackDurationMs != newDurationMs) {
            scaleRegionToNewDuration(m_lastPlaybackDurationMs, newDurationMs);
        }
        m_lastPlaybackDurationMs = newDurationMs;
        m_engineLoadedMediaFileId =
            m_loadingTargetMediaFileId > 0 ? m_loadingTargetMediaFileId : m_mediaFileId;
        m_loadingTargetMediaFileId = 0;
        syncRegionToEngine();
        emit durationMsChanged();
        emit positionMsChanged();

        if (m_pendingPlayAfterLoad) {
            m_pendingPlayAfterLoad = false;
            m_engine.play();
        } else if (m_pendingPresetLoad) {
            m_pendingPresetLoad = false;
            loadSelectedPreset();
        }

        persistCurrentSettings();

        if (m_engine.lastError().isEmpty() && initialLoad && !m_engine.isPlaying()) {
            setStatusMessage(tr("Audio ready."));
        }
    });
}

qlonglong AudioConfigController::mediaFileId() const { return m_mediaFileId; }

void AudioConfigController::setMediaFileId(qlonglong mediaFileId) {
    const bool sameId = m_mediaFileId == mediaFileId;
    if (sameId) {
        const bool engineMatches = m_engineLoadedMediaFileId == mediaFileId &&
                                   m_engine.durationMs() > 0 && !m_engine.peaks().isEmpty();
        if (mediaFileId > 0 && !engineMatches && !m_engine.isLoading()) {
            reloadMedia();
        }
        return;
    }

    persistCurrentSettings();
    m_mediaFileId = mediaFileId;
    emit mediaFileIdChanged();
    reloadMedia();
}

const QString &AudioConfigController::displayName() const { return m_displayName; }

const QString &AudioConfigController::songTitle() const { return m_songTitle; }

void AudioConfigController::setSongTitle(const QString &songTitle) {
    if (m_songTitle == songTitle) {
        return;
    }
    m_songTitle = songTitle;
    emit songTitleChanged();
}

bool AudioConfigController::playing() const { return m_engine.isPlaying(); }

bool AudioConfigController::loading() const { return m_engine.isLoading(); }

const QString &AudioConfigController::processingStage() const { return m_engine.processingStage(); }

int AudioConfigController::processingProgress() const { return m_engine.processingProgress(); }

qint64 AudioConfigController::positionMs() const { return m_engine.positionMs(); }

qint64 AudioConfigController::durationMs() const { return m_engine.durationMs(); }

int AudioConfigController::tempoPercent() const { return m_tempoPercent; }

void AudioConfigController::setTempoPercent(int tempoPercent) {
    const int bounded =
        qBound(AudioConstants::kMinTempoPercent, tempoPercent, AudioConstants::kMaxTempoPercent);
    if (m_tempoPercent == bounded) {
        return;
    }
    m_tempoPercent = bounded;
    m_engine.setTempoPercent(m_tempoPercent);
    emit tempoPercentChanged();
    persistCurrentSettings();
}

const QString &AudioConfigController::eqPresetId() const { return m_eqPresetId; }

void AudioConfigController::setEqPresetId(const QString &presetId) {
    if (m_eqPresetId == presetId) {
        return;
    }
    m_eqPresetId = presetId;
    m_engine.setEqPresetId(m_eqPresetId);
    emit eqPresetIdChanged();
    persistCurrentSettings();
}

qint64 AudioConfigController::regionStartMs() const { return m_regionStartMs; }

void AudioConfigController::setRegionStartMs(qint64 regionStartMs) {
    applyRegionMs(regionStartMs, m_regionEndMs, true);
}

qint64 AudioConfigController::regionEndMs() const { return m_regionEndMs; }

void AudioConfigController::setRegionEndMs(qint64 regionEndMs) {
    applyRegionMs(m_regionStartMs, regionEndMs, true);
}

bool AudioConfigController::canUndoRegion() const { return !m_regionUndoStack.empty(); }

bool AudioConfigController::presetApplied() const { return m_presetApplied; }

bool AudioConfigController::loopEnabled() const { return m_loopEnabled; }

void AudioConfigController::setLoopEnabled(bool enabled) {
    if (m_loopEnabled == enabled) {
        return;
    }
    m_loopEnabled = enabled;
    m_engine.setLoopEnabled(m_loopEnabled);
    emit loopEnabledChanged();
    persistCurrentSettings();
}

QVariantList AudioConfigController::peaks() const { return m_peaks; }

void AudioConfigController::syncPeaksFromEngine() {
    QVariantList next;
    const QVector<float> peakVector = m_engine.peaks();
    next.reserve(peakVector.size());
    for (float peak : peakVector) {
        next.append(peak);
    }
    if (m_peaks == next) {
        return;
    }
    m_peaks = std::move(next);
    emit peaksChanged();
}

const QStringList &AudioConfigController::presetNames() const { return m_presetNames; }

int AudioConfigController::selectedPresetIndex() const { return m_selectedPresetIndex; }

void AudioConfigController::setSelectedPresetIndex(int index) {
    if (m_selectedPresetIndex == index) {
        return;
    }
    m_selectedPresetIndex = index;
    emit selectedPresetIndexChanged();
}

const QString &AudioConfigController::presetNameInput() const { return m_presetNameInput; }

void AudioConfigController::setPresetNameInput(const QString &name) {
    if (m_presetNameInput == name) {
        return;
    }
    m_presetNameInput = name;
    emit presetNameInputChanged();
}

const QString &AudioConfigController::statusMessage() const { return m_statusMessage; }

void AudioConfigController::setPresetApplied(bool applied) {
    if (m_presetApplied == applied) {
        return;
    }
    m_presetApplied = applied;
    emit presetAppliedChanged();
}

void AudioConfigController::reloadMedia() {
    clearRegionUndoStack();
    m_lastPlaybackDurationMs = 0;
    m_engineLoadedMediaFileId = 0;
    setPresetApplied(false);
    if (!m_peaks.isEmpty()) {
        m_peaks.clear();
        emit peaksChanged();
    }
    m_engine.stop();
    m_displayName.clear();
    emit displayNameChanged();

    if (m_mediaFileId <= 0) {
        setStatusMessage(tr("No audio file selected."));
        return;
    }

    if (m_engine.isLoading() && m_mediaFileId == m_loadingTargetMediaFileId) {
        return;
    }

    m_engine.cancelProcessing();

    const std::optional<MediaFile> mediaFile = m_dependencies.mediaRepo.getMediaFile(m_mediaFileId);
    if (!mediaFile.has_value()) {
        setStatusMessage(tr("Media file not found."));
        return;
    }

    m_displayName = mediaFile->filePath.section(QLatin1Char('/'), -1);
    emit displayNameChanged();

    const QString resolvedPath = m_dependencies.pathResolver.resolve(*mediaFile);
    if (!restoreSettingsForMedia(m_mediaFileId)) {
        m_tempoPercent = AudioConstants::kDefaultTempoPercent;
        m_eqPresetId = QStringLiteral("flat");
        m_loopEnabled = false;
        m_regionStartMs = AudioConstants::kDefaultRegionStartMs;
        m_regionEndMs = 0;
        emit tempoPercentChanged();
        emit eqPresetIdChanged();
        emit loopEnabledChanged();
        emit regionChanged();
    }

    m_engine.setTempoPercent(m_tempoPercent);
    m_engine.setEqPresetId(m_eqPresetId);
    m_engine.setLoopEnabled(m_loopEnabled);
    syncRegionToEngine();

    m_loadingTargetMediaFileId = m_mediaFileId;
    m_engine.loadFile(resolvedPath);
    if (!m_engine.lastError().isEmpty()) {
        setStatusMessage(m_engine.lastError());
        m_pendingPlayAfterLoad = false;
        return;
    }

    refreshPresetList();
}

void AudioConfigController::preparePresetsForMedia(qlonglong mediaFileId) {
    if (mediaFileId <= 0) {
        return;
    }

    if (m_mediaFileId == mediaFileId) {
        refreshPresetList();
        return;
    }

    persistCurrentSettings();
    m_mediaFileId = mediaFileId;
    emit mediaFileIdChanged();
    setSelectedPresetIndex(-1);
    refreshPresetList();
    reloadMedia();
}

void AudioConfigController::loadPresetForMedia(qlonglong mediaFileId, int presetIndex) {
    if (mediaFileId <= 0) {
        setStatusMessage(tr("No audio file selected."));
        return;
    }

    preparePresetsForMedia(mediaFileId);

    if (presetIndex < 0 || presetIndex >= m_presetNames.size()) {
        setStatusMessage(tr("No preset selected."));
        return;
    }

    setSelectedPresetIndex(presetIndex);

    const bool needsLoad = !m_engine.hasPlaybackData() || m_engineLoadedMediaFileId != mediaFileId;
    if (needsLoad) {
        m_pendingPresetLoad = true;
        m_pendingPlayAfterLoad = false;
        reloadMedia();
        return;
    }

    loadSelectedPreset();
}

void AudioConfigController::playMediaFile(qlonglong mediaFileId) {
    if (mediaFileId <= 0) {
        return;
    }

    if (m_mediaFileId == mediaFileId && m_engine.isPlaying()) {
        m_engine.pause();
        return;
    }

    if (m_mediaFileId != mediaFileId) {
        persistCurrentSettings();
        m_mediaFileId = mediaFileId;
        emit mediaFileIdChanged();
    }

    const bool engineReadyForMedia = m_engineLoadedMediaFileId == mediaFileId;

    if (engineReadyForMedia) {
        if (m_engine.isLoading()) {
            m_pendingPlayAfterLoad = true;
            m_engine.play();
            return;
        }
        m_engine.play();
        return;
    }

    m_pendingPlayAfterLoad = true;
    m_pendingPresetLoad = false;
    reloadMedia();
}

void AudioConfigController::togglePlayback() {
    if (m_engine.isPlaying()) {
        m_engine.pause();
        return;
    }

    if (m_engine.isLoading()) {
        m_engine.play();
        return;
    }

    m_engine.play();
}

void AudioConfigController::toggleLoop() {
    if (m_loopEnabled) {
        setLoopEnabled(false);
        return;
    }
    setLoopEnabled(true);
}

void AudioConfigController::stopPlayback() { m_engine.stop(); }

void AudioConfigController::commitTempo() { m_engine.commitTempoPercent(); }

void AudioConfigController::cancelProcessing() {
    if (!m_engine.isLoading()) {
        return;
    }
    m_pendingPlayAfterLoad = false;
    m_pendingPresetLoad = false;
    m_cancelPending = true;
    setStatusMessage(tr("Cancelling processing…"));
    m_engine.cancelProcessing();
}

void AudioConfigController::setRegionFromPosition(bool isStartMarker) {
    const qint64 position = m_engine.positionMs();
    if (isStartMarker) {
        setRegionStartMs(position);
    } else {
        setRegionEndMs(position);
    }
}

void AudioConfigController::undoRegion() {
    if (m_regionUndoStack.empty()) {
        return;
    }

    const RegionSnapshot snapshot = m_regionUndoStack.back();
    m_regionUndoStack.pop_back();
    applyRegionMs(snapshot.startMs, snapshot.endMs, false);
    emit canUndoRegionChanged();
    setStatusMessage(tr("Undo marker."));
}

void AudioConfigController::pushRegionUndoSnapshot() {
    m_regionUndoStack.push_back({m_regionStartMs, m_regionEndMs});
    if (static_cast<int>(m_regionUndoStack.size()) > AudioConstants::kMaxRegionUndoSteps) {
        m_regionUndoStack.erase(m_regionUndoStack.begin());
    }
    emit canUndoRegionChanged();
}

void AudioConfigController::clearRegionUndoStack() {
    if (m_regionUndoStack.empty()) {
        return;
    }
    m_regionUndoStack.clear();
    emit canUndoRegionChanged();
}

qint64 AudioConfigController::scalePlaybackMs(qint64 millisecond, qint64 oldDurationMs,
                                              qint64 newDurationMs) {
    if (oldDurationMs <= 0 || newDurationMs <= 0) {
        return 0;
    }
    const qint64 scaled = (millisecond * newDurationMs) / oldDurationMs;
    return qBound<qint64>(0, scaled, newDurationMs);
}

void AudioConfigController::scaleRegionToNewDuration(qint64 oldDurationMs, qint64 newDurationMs) {
    if (oldDurationMs <= 0 || newDurationMs <= 0 || oldDurationMs == newDurationMs) {
        return;
    }

    qint64 newStart = scalePlaybackMs(m_regionStartMs, oldDurationMs, newDurationMs);
    qint64 newEnd = scalePlaybackMs(m_regionEndMs, oldDurationMs, newDurationMs);
    if (newEnd <= newStart) {
        newEnd = newDurationMs;
    }
    applyRegionMs(newStart, newEnd, false);
}

void AudioConfigController::applyRegionMs(qint64 regionStartMs, qint64 regionEndMs,
                                          bool recordUndo) {
    const qint64 newStart = qMax<qint64>(0, regionStartMs);
    qint64 newEnd = qMax(newStart, regionEndMs);
    const qint64 duration = m_engine.durationMs();
    if (duration > 0) {
        newEnd = qMin(newEnd, duration);
    }

    if (newStart == m_regionStartMs && newEnd == m_regionEndMs) {
        return;
    }

    if (recordUndo) {
        pushRegionUndoSnapshot();
    }

    m_regionStartMs = newStart;
    m_regionEndMs = newEnd;
    m_engine.setRegionMs(m_regionStartMs, m_regionEndMs);
    emit regionChanged();
    persistCurrentSettings();
}

void AudioConfigController::syncRegionToEngine() {
    qint64 regionEnd = m_regionEndMs;
    const qint64 duration = m_engine.durationMs();
    if (regionEnd <= 0 && duration > 0) {
        regionEnd = duration;
        if (m_regionEndMs <= 0) {
            m_regionEndMs = regionEnd;
            emit regionChanged();
        }
    }

    m_engine.setRegionMs(m_regionStartMs, regionEnd);
}

void AudioConfigController::persistCurrentSettings() { saveSettingsForMedia(m_mediaFileId); }

void AudioConfigController::saveSettingsForMedia(qlonglong mediaFileId) {
    if (mediaFileId <= 0) {
        return;
    }

    CachedMediaSettings settings;
    settings.tempoPercent = m_tempoPercent;
    settings.eqPresetId = m_eqPresetId;
    settings.regionStartMs = m_regionStartMs;
    settings.regionEndMs = m_regionEndMs;
    settings.loopEnabled = m_loopEnabled;
    settings.valid = true;
    m_settingsByMediaId.insert(mediaFileId, settings);
}

bool AudioConfigController::restoreSettingsForMedia(qlonglong mediaFileId) {
    const auto iterator = m_settingsByMediaId.constFind(mediaFileId);
    if (iterator == m_settingsByMediaId.cend() || !iterator->valid) {
        return false;
    }

    const CachedMediaSettings &settings = iterator.value();
    m_tempoPercent = settings.tempoPercent;
    m_eqPresetId = settings.eqPresetId;
    m_loopEnabled = settings.loopEnabled;
    m_regionStartMs = settings.regionStartMs;
    m_regionEndMs = settings.regionEndMs;
    emit tempoPercentChanged();
    emit eqPresetIdChanged();
    emit loopEnabledChanged();
    emit regionChanged();
    return true;
}

void AudioConfigController::savePreset() {
    const QString trimmedName = m_presetNameInput.trimmed();
    if (trimmedName.isEmpty()) {
        setStatusMessage(tr("Please enter a name for the preset."));
        return;
    }
    if (m_mediaFileId <= 0) {
        setStatusMessage(tr("No media file active."));
        return;
    }

    AudioConfigPreset preset;
    preset.mediaFileId = m_mediaFileId;
    preset.name = trimmedName;
    preset.tempoPercent = m_tempoPercent;
    preset.eqPresetId = m_eqPresetId;
    preset.regionStartMs = m_regionStartMs;
    preset.regionEndMs = m_regionEndMs;
    preset.loopEnabled = m_loopEnabled;

    const std::optional<qlonglong> presetId = m_dependencies.presetRepo.createPreset(preset);
    if (!presetId.has_value()) {
        setStatusMessage(tr("Could not save preset."));
        return;
    }

    refreshPresetList();
    for (int index = 0; std::cmp_less(index, m_presetIds.size()); ++index) {
        if (m_presetIds[static_cast<size_t>(index)] == *presetId) {
            setSelectedPresetIndex(index);
            break;
        }
    }
    setStatusMessage(tr("Preset saved."));
}

void AudioConfigController::loadSelectedPreset() {
    if (m_selectedPresetIndex < 0 && !m_presetIds.empty()) {
        setSelectedPresetIndex(0);
    }

    if (m_selectedPresetIndex < 0 ||
        std::cmp_greater_equal(m_selectedPresetIndex, m_presetIds.size())) {
        setStatusMessage(tr("No preset selected."));
        return;
    }

    const std::optional<AudioConfigPreset> preset = m_dependencies.presetRepo.getPreset(
        m_presetIds[static_cast<size_t>(m_selectedPresetIndex)]);
    if (!preset.has_value()) {
        setStatusMessage(tr("Preset not found."));
        return;
    }

    setTempoPercent(preset->tempoPercent);
    setEqPresetId(preset->eqPresetId);
    clearRegionUndoStack();
    applyRegionMs(preset->regionStartMs, preset->regionEndMs, false);
    setLoopEnabled(preset->loopEnabled);
    m_engine.commitTempoPercent();
    persistCurrentSettings();
    const QString notice = tr("Preset \"%1\" loaded.").arg(preset->name);
    setStatusMessage(notice);
    emit transientNoticeRequested(notice);
    setPresetApplied(true);
}

void AudioConfigController::deleteSelectedPreset() {
    if (m_selectedPresetIndex < 0 ||
        std::cmp_greater_equal(m_selectedPresetIndex, m_presetIds.size())) {
        setStatusMessage(tr("No preset selected."));
        return;
    }

    const qlonglong presetId = m_presetIds[static_cast<size_t>(m_selectedPresetIndex)];
    if (!m_dependencies.presetRepo.deletePreset(presetId)) {
        setStatusMessage(tr("Could not delete preset."));
        return;
    }
    refreshPresetList();
    setSelectedPresetIndex(-1);
    setStatusMessage(tr("Preset deleted."));
}

void AudioConfigController::refreshPresetList() {
    m_presetNames.clear();
    m_presetIds.clear();

    if (m_mediaFileId <= 0) {
        emit presetNamesChanged();
        return;
    }

    const std::vector<AudioConfigPreset> presets =
        m_dependencies.presetRepo.listPresetsForMedia(m_mediaFileId);
    m_presetIds.reserve(presets.size());
    for (const AudioConfigPreset &preset : presets) {
        m_presetIds.push_back(preset.id);
        m_presetNames.append(preset.name);
    }
    syncPresetSelectionAfterRefresh();
    emit presetNamesChanged();
}

void AudioConfigController::syncPresetSelectionAfterRefresh() {
    if (m_presetIds.empty()) {
        setSelectedPresetIndex(-1);
        return;
    }

    if (m_selectedPresetIndex < 0 ||
        std::cmp_greater_equal(m_selectedPresetIndex, m_presetIds.size())) {
        setSelectedPresetIndex(0);
    }
}

void AudioConfigController::setStatusMessage(const QString &message) {
    if (m_statusMessage == message) {
        return;
    }
    m_statusMessage = message;
    emit statusMessageChanged();
}
