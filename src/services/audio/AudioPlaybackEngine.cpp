#include "AudioPlaybackEngine.h"
#include "AudioProcessorWorker.h"

#include "AudioConstants.h"

#include <QAudioBuffer>
#include <QAudioDecoder>
#include <QEventLoop>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMediaDevices>
#include <QMetaType>
#include <QThread>
#include <QUrl>

#include <cstring>

namespace {

    Q_LOGGING_CATEGORY(lcAudio, "sonarp.audio")

    constexpr double kPercentToRatioDivisor = 100.0;
    constexpr qint64 kSinkBufferDurationUs = 200'000;

    float sampleFromBuffer(const QAudioBuffer &buffer, const SampleRequest &request) {
        const QAudioFormat format = buffer.format();
        const auto channelCount = format.channelCount();

        if (channelCount <= 0) {
            qCInfo(lcAudio) << "sampleFromBuffer: channelCount =" << channelCount;
        }

        if (request.channelIndex < 0 || request.channelIndex >= channelCount) {
            return 0.0F;
        }

        const auto frameOffset = request.frameIndex * format.bytesPerFrame();
        const auto *frameData = buffer.constData<char>() + frameOffset;

        if (format.sampleFormat() == QAudioFormat::Float) {
            return reinterpret_cast<const float *>(frameData)[request.channelIndex];
        }

        if (format.sampleFormat() == QAudioFormat::Int16) {
            return static_cast<float>(
                       reinterpret_cast<const qint16 *>(frameData)[request.channelIndex]) /
                   32768.0F;
        }

        if (format.sampleFormat() == QAudioFormat::Int32) {
            return static_cast<float>(
                       reinterpret_cast<const qint32 *>(frameData)[request.channelIndex]) /
                   2147483648.0F;
        }

        if (format.sampleFormat() == QAudioFormat::UInt8) {
            return (static_cast<float>(
                        reinterpret_cast<const uchar *>(frameData)[request.channelIndex]) -
                    128.0F) /
                   128.0F;
        }

        const int channelByteOffset = request.channelIndex * format.bytesPerSample();

        if ((frameOffset + channelByteOffset < 0) ||
            (frameOffset + channelByteOffset >= buffer.byteCount())) {
            qCWarning(lcAudio) << "Invalid data access within frame";
            return 0.0F;
        }

        return format.normalizedSampleValue(frameData + channelByteOffset);
    }

    QAudioFormat preferredDecodeFormat() {
        QAudioFormat format;
        format.setSampleRate(AudioConstants::kDefaultSampleRateHz);
        format.setChannelCount(AudioConstants::kChannelCount);
        format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
        format.setSampleFormat(QAudioFormat::Float);
        return format;
    }

} // namespace

/**
 * @brief Constructs the engine, initializes worker threads, and sets up audio monitoring.
 * @param parent Optional QObject parent.
 */
AudioPlaybackEngine::AudioPlaybackEngine(QObject *parent)
    : QObject(parent), m_playbackDevice(this) {

    qRegisterMetaType<AudioBuildResult>("AudioBuildResult");
    qRegisterMetaType<AudioPlaybackEngine::BuildParameters>("AudioPlaybackEngine::BuildParameters");

    // --- Worker-Thread Setup ---
    m_workerThread = new QThread(this);
    m_worker = new AudioProcessorWorker();
    m_worker->moveToThread(m_workerThread);

    connect(this, &AudioPlaybackEngine::requestProcess, m_worker, &AudioProcessorWorker::process);

    connect(m_worker, &AudioProcessorWorker::progressUpdated, this,
            [this](const QString &stage, int percent) { setProcessingState(stage, percent); });

    connect(m_worker, &AudioProcessorWorker::resultReady, this,
            &AudioPlaybackEngine::handleRebuildFinished, Qt::QueuedConnection);

    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread->start();

    SampleRequest request;
    request.sampleRateHz = AudioConstants::kDefaultSampleRateHz;
    request.channelCount = AudioConstants::kChannelCount;
    m_sourceChannelCount = request.channelCount;
    updatePlaybackFormat(request);

    // --- Timer ---
    m_positionTimer.setInterval(AudioConstants::kPositionPollIntervalMs);
    connect(&m_positionTimer, &QTimer::timeout, this, [this]() { emit positionMsChanged(); });

    m_tempoDebounceTimer.setSingleShot(true);
    m_tempoDebounceTimer.setInterval(AudioConstants::kTempoDebounceMs);
    connect(&m_tempoDebounceTimer, &QTimer::timeout, this, [this]() { startRebuild(); });
}

/**
 * @brief Stops playback, cleans up the worker thread, and destroys audio resources.
 */
AudioPlaybackEngine::~AudioPlaybackEngine() {
    stop();
    m_workerThread->quit();
    m_workerThread->wait();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/**
 * @brief Loads an audio file from disk, decodes it to float PCM, and schedules a build.
 *
 * This method handles the end-to-end ingestion process: normalizing the file path,
 * decoding the audio using QAudioDecoder, upmixing mono to stereo if necessary,
 * and initiating the processing pipeline (stretching/EQ) via the worker thread.
 *
 * @param filePath Path to the input file (must exist).
 * @return void Emits loadingChanged(), errorChanged(), and durationMsChanged() based on outcome.
 *
 * @throw std::runtime_error (implicitly handled via errorChanged signal if decoding fails).
 * @see decodeFileToFloat, scheduleRebuild
 */
void AudioPlaybackEngine::loadFile(const QString &filePath) {
    const QString normalizedPath = QFileInfo(filePath).absoluteFilePath();
    if (m_loading && normalizedPath == m_activeLoadPath) {
        return;
    }
    if (m_loading) {
        m_cancelRequested.store(true, std::memory_order_release);
    }
    stop();
    m_activeLoadPath = normalizedPath;
    m_loading = true;
    setProcessingState(tr("Preparing audio file…"), -1);
    emit loadingChanged();

    m_sourceFilePath.clear();
    m_sourceDurationMs = 0;
    m_segmentSourceStartMs = 0;
    m_lastError.clear();
    emit errorChanged();

    const QFileInfo fileInfo(normalizedPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        m_lastError = tr("Audio file not found: %1").arg(filePath);
        m_loading = false;
        m_activeLoadPath.clear();
        emit loadingChanged();
        emit errorChanged();
        return;
    }

    m_sourceFilePath = normalizedPath;
    m_segmentSourceStartMs = 0;
    m_segmentSourceEndMs = 0;
    m_peaks.clear();
    emit peaksChanged();
    emit durationMsChanged();
    teardownActivePlayback();
    m_playbackDevice.setPcmData({});

    BuildParameters params;
    params.sourceFilePath = normalizedPath;
    params.mode = BuildParameters::Mode::MetadataAndPeaks;
    params.tempoPercent = m_pendingTempoPercent;
    params.eqPresetId = m_eqPresetId;

    dispatchProcess(std::move(params));
}

void AudioPlaybackEngine::cancelProcessing() {
    m_cancelRequested.store(true, std::memory_order_release);
    setProcessingState(tr("Preparing cancellation…"), -1);
}

/**
 * @brief Starts audio playback from the current buffer position.
 *
 * Ensures a valid audio sink is available, resets the playback device position if
 * necessary, and starts the QAudioSink.
 *
 * @return void Emits playingChanged() on success or errorChanged() on failure.
 * @see ensureAudioSink, stop
 */
void AudioPlaybackEngine::play() {
    if (m_playbackDevice.size() <= 0) {
        if (m_sourceFilePath.isEmpty()) {
            m_lastError = tr("No playback data — please wait or reload the file.");
            emit errorChanged();
            qCWarning(lcAudio) << "play() skipped: no source file";
            return;
        }
        if (m_loading) {
            m_autoPlayAfterSegmentBuild = true;
            return;
        }
        m_autoPlayAfterSegmentBuild = true;
        startSegmentBuild();
        return;
    }

    if (!ensureAudioSink()) {
        qCWarning(lcAudio) << "play() skipped:" << m_lastError;
        return;
    }

    m_pauseRequested = false;
    syncLoopRegionToEngine();
    m_playbackDevice.resetReadPosition();

    if (m_audioSink->state() == QAudio::SuspendedState) {
        m_audioSink->resume();
    } else {
        m_audioSink->start(&m_playbackDevice);
    }

    if (m_audioSink->error() != QAudio::NoError) {
        m_lastError = tr("Audio output: %1").arg(m_audioSink->error());
        emit errorChanged();
        qCWarning(lcAudio) << "play() sink error:" << m_lastError;
        return;
    }

    qCInfo(lcAudio) << "play() started" << "pcmBytes" << m_playbackDevice.size() << "sinkState"
                    << m_audioSink->state() << "bytesFree" << m_audioSink->bytesFree();

    beginPlaybackFromSink();
}

/**
 * @brief Pauses the current audio playback.
 *
 * Suspends the audio sink and stops the position polling timer.
 *
 * @return void Emits playingChanged().
 * @see play
 */
void AudioPlaybackEngine::pause() {
    m_pauseRequested = true;

    if (m_audioSink != nullptr) {
        m_audioSink->suspend();
    }

    m_playing = false;
    m_positionTimer.stop();

    emit playingChanged();
}

/**
 * @brief Stops playback and resets the playhead to the beginning.
 *
 * Fully stops the audio sink and resets the read position of the playback device.
 *
 * @return void Emits playingChanged() and positionMsChanged().
 * @see play
 */
void AudioPlaybackEngine::stop() {
    m_pauseRequested = true;
    teardownActivePlayback();
    m_playbackDevice.resetReadPosition();
    emit positionMsChanged();
}

/**
 * @brief Sets the requested playback tempo as a percentage.
 * @param tempoPercent The speed multiplier (e.g., 100 for normal speed).
 */
void AudioPlaybackEngine::setTempoPercent(int tempoPercent) {
    m_pendingTempoPercent =
        qBound(AudioConstants::kMinTempoPercent, tempoPercent, AudioConstants::kMaxTempoPercent);
}

/**
 * @brief Triggers the processing pipeline to apply the pending tempo change.
 */
void AudioPlaybackEngine::commitTempoPercent() {
    if (m_pendingTempoPercent == m_appliedTempoPercent) {
        return;
    }
    scheduleRebuild(false);
}

/**
 * @brief Applies an equalizer preset by its ID and rebuilds the audio buffer.
 * @param presetId The unique identifier for the EQ preset.
 */
void AudioPlaybackEngine::setEqPresetId(const QString &presetId) {
    if (m_eqPresetId == presetId) {
        return;
    }
    m_eqPresetId = presetId;
    scheduleRebuild(false);
}

/**
 * @brief Defines the loop region boundaries in milliseconds.
 * @param regionStartMs The start time in milliseconds.
 * @param regionEndMs The end time in milliseconds.
 */
void AudioPlaybackEngine::setRegionMs(qint64 regionStartMs, qint64 regionEndMs) {
    const qint64 newStart = qMax<qint64>(0, regionStartMs);
    qint64 newEnd = qMax(newStart, regionEndMs);
    if (m_sourceDurationMs > 0) {
        newEnd = qMin(newEnd, m_sourceDurationMs);
    }

    const bool regionChanged = newStart != m_regionStartMs || newEnd != m_regionEndMs;
    m_regionStartMs = newStart;
    m_regionEndMs = newEnd;

    if (regionChanged && m_playbackDevice.size() > 0) {
        teardownActivePlayback();
        m_playbackDevice.setPcmData({});
        m_segmentSourceStartMs = 0;
        m_segmentSourceEndMs = 0;
    }

    syncLoopRegionToEngine();
    emit positionMsChanged();
}

/**
 * @brief Enables or disables looping for the defined region.
 * @param enabled True to enable looping.
 */
void AudioPlaybackEngine::setLoopEnabled(bool enabled) {
    m_loopEnabled = enabled;
    syncLoopRegionToEngine();
}

/**
 * @brief Checks if audio is currently loading/playing.
 * @return True if loading/playing, false otherwise.
 */
bool AudioPlaybackEngine::isLoading() const { return m_loading; }

const QString &AudioPlaybackEngine::processingStage() const { return m_processingStage; }

int AudioPlaybackEngine::processingProgress() const { return m_processingProgress; }

void AudioPlaybackEngine::setProcessingState(const QString &stage, int progressPercent) {
    if (m_processingStage != stage) {
        m_processingStage = stage;
        emit processingStageChanged();
    }
    if (m_processingProgress != progressPercent) {
        m_processingProgress = progressPercent;
        emit processingProgressChanged();
    }
}

bool AudioPlaybackEngine::isPlaying() const { return m_playing; }

/**
 * @brief Returns the current playback position in milliseconds.
 * @return Current position in ms.
 */
qint64 AudioPlaybackEngine::positionMs() const {
    return m_segmentSourceStartMs + bytesToMs(m_playbackDevice.playbackPositionBytes());
}

/**
 * @brief Returns the total duration of the current audio file in milliseconds.
 * @return Total duration in ms.
 */
qint64 AudioPlaybackEngine::durationMs() const { return m_sourceDurationMs; }

/**
 * @brief Returns the computed peak values for waveform visualization.
 * @return A vector of floating-point peak values.
 */
const QVector<float> &AudioPlaybackEngine::peaks() const { return m_peaks; }

/**
 * @brief Returns the last error message encountered by the engine.
 * @return A string containing the error description.
 */
const QString &AudioPlaybackEngine::lastError() const { return m_lastError; }

/**
 * @brief Checks if the engine contains valid playback data.
 * @return True if audio data is ready.
 */
bool AudioPlaybackEngine::hasPlaybackData() const {
    return !m_loading && m_playbackDevice.size() > 0;
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void AudioPlaybackEngine::scheduleRebuild(bool immediate) {
    if (m_sourceFilePath.isEmpty()) {
        return;
    }
    if (immediate) {
        m_tempoDebounceTimer.stop();
        startRebuild();
        return;
    }
    m_tempoDebounceTimer.start();
}

void AudioPlaybackEngine::startRebuild() { startSegmentBuild(); }

void AudioPlaybackEngine::startSegmentBuild() {
    if (m_sourceFilePath.isEmpty()) {
        return;
    }

    m_resumeAfterRebuild = m_playing;
    m_resumePositionMs = positionMs();
    teardownActivePlayback();
    m_playbackDevice.setPcmData({});

    m_loading = true;
    setProcessingState(tr("Processing audio…"), -1);
    emit loadingChanged();

    BuildParameters params;
    params.sourceFilePath = m_sourceFilePath;
    params.mode = BuildParameters::Mode::PlaybackSegment;
    params.regionStartMs = m_regionStartMs;
    params.regionEndMs = m_regionEndMs > 0 ? m_regionEndMs : m_sourceDurationMs;
    params.sourceSampleRateHz = m_sourceSampleRateHz;
    params.tempoPercent = m_pendingTempoPercent;
    params.eqPresetId = m_eqPresetId;

    dispatchProcess(std::move(params));
}

void AudioPlaybackEngine::dispatchProcess(BuildParameters params) {
    m_cancelRequested.store(false, std::memory_order_release);
    params.buildGeneration = ++m_buildGeneration;
    params.cancelRequested = &m_cancelRequested;
    emit requestProcess(std::move(params));
}

void AudioPlaybackEngine::handleRebuildFinished(const AudioBuildResult &result) {
    if (result.buildGeneration != m_buildGeneration) {
        return;
    }

    m_loading = false;
    m_processingStage.clear();
    m_processingProgress = -1;
    m_cancelRequested.store(false, std::memory_order_release);
    emit processingStageChanged();
    emit processingProgressChanged();
    emit loadingChanged();

    if (result.cancelled) {
        m_autoPlayAfterSegmentBuild = false;
        m_activeLoadPath.clear();
        return;
    }

    if (!result.success) {
        m_lastError = result.errorMessage;
        m_autoPlayAfterSegmentBuild = false;
        m_activeLoadPath.clear();
        emit errorChanged();
        return;
    }

    m_appliedTempoPercent = result.tempoPercent;

    if (result.metadataOnly) {
        applyMetadataResult(result);
        return;
    }

    m_activeLoadPath.clear();
    m_lastError.clear();
    emit errorChanged();
    applyBuildResult(result);
}

void AudioPlaybackEngine::applyMetadataResult(const AudioBuildResult &result) {
    m_peaks = result.peaks;
    m_sourceDurationMs = result.durationMs;
    m_sourceSampleRateHz = result.sampleRateHz;
    m_sourceChannelCount = result.channelCount;
    if (m_regionEndMs <= 0 && m_sourceDurationMs > 0) {
        m_regionEndMs = m_sourceDurationMs;
    }
    m_activeLoadPath.clear();
    m_lastError.clear();

    SampleRequest request;
    request.sampleRateHz = result.sampleRateHz;
    request.channelCount = result.channelCount;
    updatePlaybackFormat(request);

    emit peaksChanged();
    emit durationMsChanged();
    emit errorChanged();

    if (m_autoPlayAfterSegmentBuild) {
        QTimer::singleShot(0, this, [this]() { startSegmentBuild(); });
    }
}

void AudioPlaybackEngine::applyBuildResult(const AudioBuildResult &result) {
    const bool resumePlayback = m_resumeAfterRebuild;
    qint64 resumeMs = m_resumePositionMs;
    m_resumeAfterRebuild = false;

    if (result.pcmInt16.isEmpty() || result.channelCount <= 0) {
        m_lastError = tr("Processing produced no playback data.");
        m_autoPlayAfterSegmentBuild = false;
        emit errorChanged();
        return;
    }

    destroyAudioSink();

    ++m_bufferGeneration;
    m_playbackDevice.setPcmData(result.pcmInt16);
    m_segmentSourceStartMs = result.segmentSourceStartMs;
    m_segmentSourceEndMs = result.segmentSourceEndMs;

    SampleRequest request;
    request.sampleRateHz = result.sampleRateHz;
    request.channelCount = result.channelCount;
    updatePlaybackFormat(request);

    syncLoopRegionToEngine();
    emit playbackReadyChanged();

    const bool autoPlay = m_autoPlayAfterSegmentBuild;
    m_autoPlayAfterSegmentBuild = false;

    if (resumePlayback) {
        QTimer::singleShot(0, this, [this, resumeMs]() {
            if (m_loading) {
                return;
            }
            restartSinkFromPosition(resumeMs);
        });
    } else if (autoPlay) {
        QTimer::singleShot(0, this, [this]() {
            if (m_loading) {
                return;
            }
            play();
        });
    }
}

void AudioPlaybackEngine::teardownActivePlayback() {
    destroyAudioSink();
    if (m_playing) {
        m_playing = false;
        m_positionTimer.stop();
        emit playingChanged();
    }
}

void AudioPlaybackEngine::destroyAudioSink() {
    if (m_audioSink == nullptr) {
        return;
    }

    disconnect(m_audioSink.get(), nullptr, this, nullptr);
    m_audioSink->stop();
    m_audioSink.reset();
}

/**
 * @brief Initializes or validates the QAudioSink for the current playback format.
 *
 * Checks if the default audio output device supports the current format and
 * provides a fallback to Int16 if the preferred format is unsupported.
 *
 * @return bool True if a valid audio sink was created or already exists, false otherwise.
 * @see destroyAudioSink
 */
bool AudioPlaybackEngine::ensureAudioSink() {
    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (device.isNull()) {
        m_lastError = tr("No audio output device found.");
        emit errorChanged();
        return false;
    }

    QAudioFormat sinkFormat = m_audioFormat;
    if (!device.isFormatSupported(sinkFormat)) {
        QAudioFormat fallback = device.preferredFormat();
        fallback.setSampleRate(sinkFormat.sampleRate());
        fallback.setChannelCount(sinkFormat.channelCount());
        fallback.setSampleFormat(QAudioFormat::Int16);
        fallback.setChannelConfig(
            QAudioFormat::defaultChannelConfigForChannelCount(sinkFormat.channelCount()));
        if (device.isFormatSupported(fallback)) {
            sinkFormat = fallback;
        } else {
            m_lastError = tr("Audio format is not supported by the output device.");
            emit errorChanged();
            return false;
        }
    }

    if (m_audioSink != nullptr && m_audioSink->format() != sinkFormat) {
        destroyAudioSink();
    }

    if (m_audioSink == nullptr) {
        m_audioSink = std::make_unique<QAudioSink>(device, sinkFormat);

        const auto bufferBytes = sinkFormat.bytesForDuration(kSinkBufferDurationUs);
        if (bufferBytes > 0) {
            m_audioSink->setBufferSize(bufferBytes);
        }

        connect(m_audioSink.get(), &QAudioSink::stateChanged, this,
                &AudioPlaybackEngine::handleSinkStateChanged);

        qCInfo(lcAudio) << "audio sink created" << "device" << device.description() << "rate"
                        << sinkFormat.sampleRate() << "channels" << sinkFormat.channelCount()
                        << "bufferBytes" << bufferBytes;
    }

    if (m_audioSink->isNull()) {
        m_lastError = tr("Could not initialize audio output.");
        emit errorChanged();
        return false;
    }
    return true;
}

void AudioPlaybackEngine::restartSinkFromPosition(qint64 sourcePositionMs) {
    m_pauseRequested = false;
    destroyAudioSink();

    if (!ensureAudioSink()) {
        return;
    }

    syncLoopRegionToEngine();
    m_playbackDevice.setPlaybackPositionBytes(sourceMsToPlaybackBytes(sourcePositionMs));
    m_audioSink->start(&m_playbackDevice);

    if (m_audioSink->error() != QAudio::NoError) {
        m_lastError = tr("Audio output: %1").arg(m_audioSink->error());
        emit errorChanged();
        return;
    }
    beginPlaybackFromSink();
}

void AudioPlaybackEngine::beginPlaybackFromSink() {
    m_playing = true;
    m_positionTimer.start();
    emit playingChanged();
}

void AudioPlaybackEngine::handleSinkStateChanged(QAudio::State state) {
    qCDebug(lcAudio) << "sink state" << state << "pauseRequested" << m_pauseRequested;

    if (m_pauseRequested) {
        return;
    }
    if (state == QAudio::ActiveState) {
        if (!m_playing) {
            beginPlaybackFromSink();
        }
        return;
    }
    if (state == QAudio::IdleState || state == QAudio::StoppedState) {
        if (state == QAudio::IdleState && m_playing && m_loopEnabled && !m_pauseRequested &&
            m_playbackDevice.size() > 0) {
            syncLoopRegionToEngine();
            m_playbackDevice.resetReadPosition();
            if (m_audioSink != nullptr) {
                m_audioSink->start(&m_playbackDevice);
            }
            return;
        }
        if (m_playing) {
            m_playing = false;
            m_positionTimer.stop();
            emit playingChanged();
            emit positionMsChanged();
        }
    }
}

void AudioPlaybackEngine::updatePlaybackFormat(SampleRequest request) {
    m_audioFormat.setSampleRate(request.sampleRateHz);
    m_audioFormat.setChannelCount(request.channelCount);
    m_audioFormat.setSampleFormat(QAudioFormat::Int16);
    m_audioFormat.setChannelConfig(
        QAudioFormat::defaultChannelConfigForChannelCount(request.channelCount));
}

void AudioPlaybackEngine::syncLoopRegionToEngine() {
    if (m_playbackDevice.size() <= 0) {
        return;
    }

    const qint64 bufferBytes = m_playbackDevice.size();
    qint64 segmentSourceStart = m_segmentSourceStartMs;
    qint64 segmentSourceEnd = m_segmentSourceEndMs;
    if (segmentSourceEnd <= segmentSourceStart) {
        segmentSourceEnd = m_regionEndMs > 0 ? m_regionEndMs : m_sourceDurationMs;
    }

    const qint64 sourceSpanMs = segmentSourceEnd - segmentSourceStart;
    qint64 loopEndMs = m_regionEndMs > 0 ? m_regionEndMs : m_sourceDurationMs;
    const qint64 loopStartMs = qBound(segmentSourceStart, m_regionStartMs, segmentSourceEnd);
    loopEndMs = qBound(loopStartMs, loopEndMs, segmentSourceEnd);

    qint64 startByte = 0;
    qint64 endByte = bufferBytes;

    if (sourceSpanMs > 0 && (loopStartMs > segmentSourceStart || loopEndMs < segmentSourceEnd)) {
        startByte = ((loopStartMs - segmentSourceStart) * bufferBytes) / sourceSpanMs;
        endByte = loopEndMs >= segmentSourceEnd
                      ? bufferBytes
                      : ((loopEndMs - segmentSourceStart) * bufferBytes) / sourceSpanMs;
    }

    startByte = qBound<qint64>(0, startByte, bufferBytes);
    endByte = qBound<qint64>(0, endByte, bufferBytes);
    if (endByte <= startByte && bufferBytes > 0) {
        startByte = 0;
        endByte = bufferBytes;
    }

    m_playbackDevice.setLoopRegion(startByte, endByte, m_loopEnabled);
}

qint64 AudioPlaybackEngine::sourceMsToPlaybackBytes(qint64 sourceMs) const {
    if (m_playbackDevice.size() <= 0) {
        return 0;
    }

    qint64 segmentSourceStart = m_segmentSourceStartMs;
    qint64 segmentSourceEnd = m_segmentSourceEndMs;
    if (segmentSourceEnd <= segmentSourceStart) {
        segmentSourceEnd = m_regionEndMs > 0 ? m_regionEndMs : m_sourceDurationMs;
    }

    const qint64 sourceSpanMs = segmentSourceEnd - segmentSourceStart;
    if (sourceSpanMs <= 0) {
        return 0;
    }

    const qint64 clampedSourceMs = qBound(segmentSourceStart, sourceMs, segmentSourceEnd);
    return ((clampedSourceMs - segmentSourceStart) * m_playbackDevice.size()) / sourceSpanMs;
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

/**
 * @brief Converts floating point PCM samples to 16-bit signed integer PCM.
 *
 * Clamps the float values to the [-1.0, 1.0] range before converting to
 * the Int16 format required by the audio sink.
 *
 * @param samples A vector of normalized floating point samples.
 * @return QByteArray The resulting raw PCM data in Int16 format.
 *
 * @see decodeFileToFloat
 */
QByteArray AudioPlaybackEngine::floatToInt16(const std::vector<float> &samples) {
    QByteArray pcm;
    const auto byteCount = static_cast<qsizetype>(samples.size()) *
                           static_cast<qsizetype>(AudioConstants::kBytesPerSample);
    pcm.resize(byteCount);
    auto *output = reinterpret_cast<qint16 *>(pcm.data());
    for (std::size_t i = 0; i < samples.size(); ++i) {
        const auto clamped = qBound(-1.0F, samples[i], 1.0F);
        output[i] = static_cast<qint16>(clamped * 32767.0F);
    }
    return pcm;
}

/**
 * @brief Decodes an audio file into a vector of floating point samples.
 *
 * Uses QAudioDecoder to read a file and convert various sample formats
 * (Int16, Int32, Float, etc.) into a normalized float representation.
 *
 * @param filePath Path to the input file (must exist).
 * @param request Reference to a SampleRequest to be populated with detected sample rate and channel
 * count.
 * @param errorMessage String to be populated if the decoding process fails.
 * @return std::vector<float> A vector containing the interleaved audio samples.
 *
 * @see sampleFromBuffer
 */
std::vector<float> AudioPlaybackEngine::decodeFileToFloat(const QString &filePath,
                                                          SampleRequest request,
                                                          QString &errorMessage,
                                                          const std::atomic_bool *cancelRequested) {
    std::vector<float> samples;

    const auto isCancelled = [&]() {
        return cancelRequested != nullptr && cancelRequested->load(std::memory_order_acquire);
    };

    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        errorMessage = tr("Audio file not found: %1").arg(filePath);
        return samples;
    }

    QAudioDecoder decoder;
    const QAudioFormat decodeFormat = preferredDecodeFormat();
    decoder.setAudioFormat(decodeFormat);
    decoder.setSource(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));

    if (!decoder.isSupported()) {
        errorMessage = tr("Audio format is not supported: %1").arg(fileInfo.fileName());
        return samples;
    }

    QEventLoop loop;
    std::size_t globalFrameIndex = 0;
    bool reachedEndFrame = false;

    const auto frameInRange = [&](std::size_t frameIndex) {
        if (frameIndex < static_cast<std::size_t>(request.decodeStartFrame)) {
            return false;
        }
        if (request.decodeEndFrame > 0 &&
            frameIndex >= static_cast<std::size_t>(request.decodeEndFrame)) {
            return false;
        }
        return true;
    };

    const auto appendDecodedBuffer = [&](const QAudioBuffer &buffer) {
        if (isCancelled() || reachedEndFrame) {
            return;
        }

        if (!buffer.isValid()) {
            return;
        }

        const QAudioFormat format = buffer.format();

        request.channelCount = format.channelCount();
        if (request.channelCount == 0) {
            qCritical() << "channelCount is 0 in appendDecodedBuffer()";
        }

        request.sampleRateHz = format.sampleRate();
        const auto frameCount = buffer.frameCount();
        if (frameCount <= 0) {
            return;
        }

        const auto channelCount = static_cast<std::size_t>(request.channelCount);

        for (qsizetype frame = 0; frame < frameCount; ++frame) {
            if ((frame & 0xFFF) == 0 && isCancelled()) {
                return;
            }

            if (request.decodeEndFrame > 0 &&
                globalFrameIndex >= static_cast<std::size_t>(request.decodeEndFrame)) {
                reachedEndFrame = true;
                decoder.stop();
                loop.quit();
                return;
            }

            if (frameInRange(globalFrameIndex)) {
                if (samples.empty()) {
                    samples.reserve(static_cast<std::size_t>(frameCount) * channelCount);
                }
                for (int channel = 0; channel < request.channelCount; ++channel) {
                    SampleRequest sampleReq;
                    sampleReq.frameIndex = frame;
                    sampleReq.channelIndex = channel;
                    samples.push_back(sampleFromBuffer(buffer, sampleReq));
                }
            }

            ++globalFrameIndex;
        }
    };

    const auto drainDecoderBuffers = [&]() {
        while (decoder.bufferAvailable()) {
            if (isCancelled()) {
                decoder.stop();
                errorMessage = tr("Cancelled.");
                samples.clear();
                loop.quit();
                return;
            }
            appendDecodedBuffer(decoder.read());
        }
    };

    QObject::connect(
        &decoder, static_cast<void (QAudioDecoder::*)(QAudioDecoder::Error)>(&QAudioDecoder::error),
        [&](QAudioDecoder::Error decodeError) {
            if (decodeError == QAudioDecoder::NoError) {
                return;
            }
            errorMessage = decoder.errorString();
            loop.quit();
        });
    QObject::connect(&decoder, &QAudioDecoder::bufferReady, [&]() { drainDecoderBuffers(); });
    QObject::connect(&decoder, &QAudioDecoder::bufferAvailableChanged, [&](bool available) {
        if (available) {
            drainDecoderBuffers();
        }
    });
    QObject::connect(&decoder, &QAudioDecoder::finished, &loop, &QEventLoop::quit);

    if (!decoder.signalsBlocked()) {
        decoder.start();
        loop.exec();
        if (isCancelled() && errorMessage.isEmpty()) {
            decoder.stop();
            errorMessage = tr("Cancelled.");
            samples.clear();
        } else {
            drainDecoderBuffers();
        }
    } else {
        return samples;
    }

    if (isCancelled() && errorMessage.isEmpty()) {
        errorMessage = tr("Cancelled.");
        samples.clear();
    }

    if (request.channelCount <= 0) {
        request.channelCount = decodeFormat.channelCount();
        if (request.channelCount <= 0) {
            qCInfo(lcAudio) << "ChannelCount is 0, setting to default value";
        }
    }
    if (request.sampleRateHz <= 0) {
        request.sampleRateHz = decodeFormat.sampleRate();
    }

    if (samples.empty() && errorMessage.isEmpty()) {
        errorMessage = decoder.error() != QAudioDecoder::NoError
                           ? decoder.errorString()
                           : tr("Decoder returned no audio data.");
    }

    qCInfo(lcAudio) << "decoded" << fileInfo.fileName() << "frames"
                    << (request.channelCount > 0
                            ? samples.size() / static_cast<size_t>(request.channelCount)
                            : 0)
                    << "rate" << request.sampleRateHz << "ch" << request.channelCount;

    return samples;
}

/**
 * @brief Calculates the millisecond position based on the number of bytes read.
 *
 * @param bytes The current playback position in bytes.
 * @return qint64 The equivalent position in milliseconds.
 */
qint64 AudioPlaybackEngine::bytesToMs(qint64 bytes) const {
    if (m_audioFormat.channelCount() <= 0 || m_audioFormat.sampleRate() <= 0) {
        return 0;
    }
    const qint64 bytesPerFrame =
        static_cast<qint64>(m_audioFormat.channelCount()) * AudioConstants::kBytesPerSample;
    const qint64 frames = bytes / bytesPerFrame;
    return (frames * 1000LL) / m_audioFormat.sampleRate();
}

/**
 * @brief Calculates the byte offset based on a given millisecond position.
 *
 * @param ms The target position in milliseconds.
 * @return qint64 The equivalent offset in bytes.
 */
qint64 AudioPlaybackEngine::msToBytes(qint64 ms) const {
    if (m_audioFormat.channelCount() <= 0 || m_audioFormat.sampleRate() <= 0) {
        return 0;
    }
    const qint64 frames = (ms * m_audioFormat.sampleRate()) / 1000;
    return frames * m_audioFormat.channelCount() * AudioConstants::kBytesPerSample;
}
