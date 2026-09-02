/**
 * @file GuitarProPreviewController.cpp
 * @brief Guitar Pro ASCII preview + AlphaTab WebEngine player bridge.
 */

#include "GuitarProPreviewController.h"

#include "ApplicationErrorLog.h"
#include "AppSettings.h"
#include "IPathResolver.h"
#include "MediaFile.h"
#include "interfaces/IMediaFileRepository.h"

#ifdef SONARPRACTICE_HAS_WEBENGINE
#include "SoundFontSchemeHandler.h"
#endif

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QUrl>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaObject>
#include <QThread>
#include <utility>

namespace {

    QString jsStringLiteral(const QString &value) {
        QString escaped = value;
        escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
        escaped.replace(QLatin1Char('\''), QStringLiteral("\\'"));
        escaped.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
        escaped.replace(QLatin1Char('\r'), QStringLiteral("\\r"));
        return QLatin1Char('\'') + escaped + QLatin1Char('\'');
    }

    QString jsBool(bool value) {
        return value ? QStringLiteral("true") : QStringLiteral("false");
    }

} // namespace

GuitarProPreviewController::GuitarProPreviewController(const Dependencies &dependencies,
                                                       QObject *parent)
    : QObject(parent), m_dependencies(dependencies) {
#ifdef SONARPRACTICE_HAS_WEBENGINE
    m_usePlayer = true;
#else
    m_usePlayer = false;
#endif
    connect(&m_dependencies.appSettings, &AppSettings::settingsChanged, this, [this]() {
        m_loadedSoundFontKey.clear();
        m_soundFontCustomAbandoned = false;
        if (!m_bridgeReady) {
            return;
        }
        emitPlayerCommand(applyPlayerSettingsJavaScript());
        ensureCustomSoundFontLoaded();
    });
}

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

bool GuitarProPreviewController::playerAvailable() const {
#ifdef SONARPRACTICE_HAS_WEBENGINE
    return true;
#else
    return false;
#endif
}

bool GuitarProPreviewController::usePlayer() const { return m_usePlayer && playerAvailable(); }

void GuitarProPreviewController::setUsePlayer(bool usePlayer) {
    const bool next = usePlayer && playerAvailable();
    if (m_usePlayer == next) {
        return;
    }
    m_usePlayer = next;
    emit usePlayerChanged();
    if (m_usePlayer && !m_scoreBase64.isEmpty()) {
        scheduleScoreLoad();
    }
}

const QString &GuitarProPreviewController::scoreBase64() const { return m_scoreBase64; }

QString GuitarProPreviewController::playerPageUrl() const {
    return QStringLiteral(
        "qrc:/qt/qml/com/sonarp/sonarpractice/src/ui/web/alphatab/player.html");
}

bool GuitarProPreviewController::playing() const { return m_playing; }

bool GuitarProPreviewController::playerReady() const { return m_playerReady; }

int GuitarProPreviewController::tempoPercent() const { return m_tempoPercent; }

void GuitarProPreviewController::setTempoPercent(int tempoPercent) {
    const int clamped = qBound(25, tempoPercent, 200);
    if (m_tempoPercent == clamped) {
        return;
    }
    m_tempoPercent = clamped;
    emit tempoPercentChanged();
    emitPlayerCommand(QStringLiteral("window.sonarAlphaTab && window.sonarAlphaTab.setTempoPercent(%1);")
                          .arg(m_tempoPercent));
}

bool GuitarProPreviewController::loopEnabled() const { return m_loopEnabled; }

void GuitarProPreviewController::setLoopEnabled(bool enabled) {
    if (m_loopEnabled == enabled) {
        return;
    }
    m_loopEnabled = enabled;
    emit loopChanged();
    emitPlayerCommand(applyPlayerSettingsJavaScript());
}

int GuitarProPreviewController::loopStartBar() const { return m_loopStartBar; }

void GuitarProPreviewController::setLoopStartBar(int bar) {
    const int clamped = qMax(1, bar);
    if (m_loopStartBar == clamped) {
        return;
    }
    m_loopStartBar = clamped;
    if (m_loopEndBar < m_loopStartBar) {
        m_loopEndBar = m_loopStartBar;
    }
    emit loopChanged();
    if (m_loopEnabled) {
        emitPlayerCommand(applyPlayerSettingsJavaScript());
    }
}

int GuitarProPreviewController::loopEndBar() const { return m_loopEndBar; }

void GuitarProPreviewController::setLoopEndBar(int bar) {
    const int clamped = qMax(m_loopStartBar, bar);
    if (m_loopEndBar == clamped) {
        return;
    }
    m_loopEndBar = clamped;
    emit loopChanged();
    if (m_loopEnabled) {
        emitPlayerCommand(applyPlayerSettingsJavaScript());
    }
}

int GuitarProPreviewController::barCount() const { return m_barCount; }

int GuitarProPreviewController::clampMetronomeDivision(int division) {
    if (division == 8 || division == 16 || division == 32) {
        return division;
    }
    return 4;
}

bool GuitarProPreviewController::metronomeEnabled() const { return m_metronomeEnabled; }

void GuitarProPreviewController::setMetronomeEnabled(bool enabled) {
    if (m_metronomeEnabled == enabled) {
        return;
    }
    m_metronomeEnabled = enabled;
    emit metronomeChanged();
    emitPlayerCommand(QStringLiteral(
                          "window.sonarAlphaTab && window.sonarAlphaTab.setMetronome(%1, %2);")
                          .arg(jsBool(m_metronomeEnabled))
                          .arg(m_metronomeDivision));
}

int GuitarProPreviewController::metronomeDivision() const { return m_metronomeDivision; }

void GuitarProPreviewController::setMetronomeDivision(int division) {
    const int clamped = clampMetronomeDivision(division);
    if (m_metronomeDivision == clamped) {
        return;
    }
    m_metronomeDivision = clamped;
    emit metronomeChanged();
    emitPlayerCommand(QStringLiteral(
                          "window.sonarAlphaTab && window.sonarAlphaTab.setMetronome(%1, %2);")
                          .arg(jsBool(m_metronomeEnabled))
                          .arg(m_metronomeDivision));
}

bool GuitarProPreviewController::countInEnabled() const { return m_countInEnabled; }

void GuitarProPreviewController::setCountInEnabled(bool enabled) {
    if (m_countInEnabled == enabled) {
        return;
    }
    m_countInEnabled = enabled;
    emit countInChanged();
    emitPlayerCommand(QStringLiteral(
                          "window.sonarAlphaTab && window.sonarAlphaTab.setCountIn(%1);")
                          .arg(jsBool(m_countInEnabled)));
}

QVariantList GuitarProPreviewController::mixerTracks() const { return m_mixerTracks; }

int GuitarProPreviewController::clampTransposeSemitones(int semitones) {
    return qBound(-12, semitones, 12);
}

int GuitarProPreviewController::transposeSemitones() const { return m_transposeSemitones; }

void GuitarProPreviewController::setTransposeSemitones(int semitones) {
    const int clamped = clampTransposeSemitones(semitones);
    if (m_transposeSemitones == clamped) {
        return;
    }
    m_transposeSemitones = clamped;
    emit transposeChanged();
    emitPlayerCommand(QStringLiteral(
                          "window.sonarAlphaTab && window.sonarAlphaTab.setTranspose(%1);")
                          .arg(m_transposeSemitones));
}

void GuitarProPreviewController::transposeUp() {
    setTransposeSemitones(m_transposeSemitones + 1);
}

void GuitarProPreviewController::transposeDown() {
    setTransposeSemitones(m_transposeSemitones - 1);
}

void GuitarProPreviewController::resetTranspose() {
    setTransposeSemitones(0);
}

void GuitarProPreviewController::setTrackVolume(int index, double volume) {
    if (index < 0 || index >= m_mixerTracks.size()) {
        return;
    }
    const double clamped = qBound(0.0, volume, 1.0);
    QVariantMap entry = m_mixerTracks.at(index).toMap();
    if (qFuzzyCompare(entry.value(QStringLiteral("volume")).toDouble() + 1.0, clamped + 1.0)
        && entry.contains(QStringLiteral("volume"))) {
        return;
    }
    entry.insert(QStringLiteral("volume"), clamped);
    m_mixerTracks[index] = entry;
    emit mixerTracksChanged();
    emitPlayerCommand(QStringLiteral(
                          "window.sonarAlphaTab && window.sonarAlphaTab.setMixer(%1);")
                          .arg(mixerTracksJavaScriptArray()));
}

void GuitarProPreviewController::setTrackMuted(int index, bool muted) {
    if (index < 0 || index >= m_mixerTracks.size()) {
        return;
    }
    QVariantMap entry = m_mixerTracks.at(index).toMap();
    if (entry.value(QStringLiteral("muted")).toBool() == muted) {
        return;
    }
    entry.insert(QStringLiteral("muted"), muted);
    m_mixerTracks[index] = entry;
    emit mixerTracksChanged();
    emitPlayerCommand(QStringLiteral(
                          "window.sonarAlphaTab && window.sonarAlphaTab.setMixer(%1);")
                          .arg(mixerTracksJavaScriptArray()));
}

void GuitarProPreviewController::setSelectedTrackIndex(int index) {
    if (index < 0 || index >= static_cast<int>(m_preview.tracks.size())) {
        // Allow AlphaTab-only track list after scoreLoaded overwrote names.
        if (index < 0 || index >= m_trackNames.size()) {
            return;
        }
    }
    if (m_selectedTrackIndex == index) {
        return;
    }
    m_selectedTrackIndex = index;
    emit selectedTrackIndexChanged();
    applySelectedTrack();
    emitPlayerCommand(QStringLiteral(
                          "window.sonarAlphaTab && window.sonarAlphaTab.applySettings({ trackIndex: %1 });")
                          .arg(m_selectedTrackIndex));
}

void GuitarProPreviewController::clear() {
    ++m_loadGeneration;
    setLoading(false);
    setPlaying(false);
    setPlayerReady(false);
    resetPreviewState();
    setVisible(false);
}

void GuitarProPreviewController::play() {
    emitPlayerCommand(QStringLiteral("window.sonarAlphaTab && window.sonarAlphaTab.play();"));
}

void GuitarProPreviewController::pause() {
    emitPlayerCommand(QStringLiteral("window.sonarAlphaTab && window.sonarAlphaTab.pause();"));
}

void GuitarProPreviewController::stop() {
    emitPlayerCommand(QStringLiteral("window.sonarAlphaTab && window.sonarAlphaTab.stop();"));
    setPlaying(false);
}

void GuitarProPreviewController::playPause() {
    emitPlayerCommand(QStringLiteral("window.sonarAlphaTab && window.sonarAlphaTab.playPause();"));
}

QString GuitarProPreviewController::loadScoreJavaScript() const {
    if (m_scoreBase64.isEmpty()) {
        return {};
    }
    return QStringLiteral(
               "window.sonarAlphaTab && window.sonarAlphaTab.loadScoreBase64(%1, %2);")
        .arg(jsStringLiteral(m_scoreBase64), playerSettingsObjectJavaScript());
}

QString GuitarProPreviewController::applyPlayerSettingsJavaScript() const {
    return QStringLiteral(
               "if (window.sonarAlphaTab) {"
               " window.sonarAlphaTab.applySettings(%1);"
               "}")
        .arg(playerSettingsObjectJavaScript());
}

QString GuitarProPreviewController::loadSoundFontJavaScript() const {
    return QStringLiteral("window.sonarAlphaTab && window.sonarAlphaTab.loadBuiltInSoundFont();");
}

QString GuitarProPreviewController::currentSoundFontKey() const {
    if (m_dependencies.appSettings.usesBuiltInSoundFont()) {
        return QStringLiteral("__builtin__");
    }
    return m_dependencies.appSettings.effectiveSoundFontPath();
}

QString GuitarProPreviewController::configuredSoundFontUrl() const {
    return QStringLiteral("./soundfont/sonivox.sf3");
}

QString GuitarProPreviewController::configuredCustomSoundFontUrl() const {
    if (m_dependencies.appSettings.usesBuiltInSoundFont()) {
        return {};
    }

    const QString path = m_dependencies.appSettings.effectiveSoundFontPath();
    if (path.isEmpty()) {
        return {};
    }

#ifdef SONARPRACTICE_HAS_WEBENGINE
    if (m_dependencies.soundFontSchemeHandler != nullptr) {
        m_dependencies.soundFontSchemeHandler->setFilePath(path);
        if (!m_dependencies.soundFontSchemeHandler->hasCachedData()) {
            return {};
        }
        return SoundFontSchemeHandler::loadUrl();
    }
#endif

    return QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded);
}

void GuitarProPreviewController::ensureCustomSoundFontLoaded() {
    if (!usePlayer() || !m_bridgeReady || m_soundFontLoadInFlight) {
        return;
    }
    if (m_dependencies.appSettings.usesBuiltInSoundFont() || m_soundFontCustomAbandoned) {
        return;
    }

    const QString key = currentSoundFontKey();
    if (key.isEmpty() || key == m_loadedSoundFontKey) {
        return;
    }

    emitPlayerCommand(
        QStringLiteral("window.sonarAlphaTab && window.sonarAlphaTab.ensureInitialized();"));
    syncSoundFontToPlayer();
}

void GuitarProPreviewController::scheduleScoreLoad() {
    if (!usePlayer() || m_scoreBase64.isEmpty()) {
        return;
    }

    if (!m_bridgeReady) {
        m_pendingScoreAfterSoundFont = true;
        return;
    }

    if (m_soundFontLoadInFlight) {
        m_pendingScoreAfterSoundFont = true;
        return;
    }

    if (!m_dependencies.appSettings.usesBuiltInSoundFont() && !m_soundFontCustomAbandoned &&
        currentSoundFontKey() != m_loadedSoundFontKey) {
        m_pendingScoreAfterSoundFont = true;
        ensureCustomSoundFontLoaded();
        return;
    }

    m_pendingScoreAfterSoundFont = false;
    emitPlayerCommand(loadScoreJavaScript());
}

void GuitarProPreviewController::syncSoundFontToPlayer() {
    if (!usePlayer() || m_soundFontLoadInFlight) {
        return;
    }

    const QString key = currentSoundFontKey();
    if (key.isEmpty() || key == m_loadedSoundFontKey) {
        return;
    }

    if (!m_dependencies.appSettings.usesBuiltInSoundFont() && m_soundFontCustomAbandoned) {
        return;
    }

    if (key == QLatin1String("__builtin__")) {
        m_soundFontLoadInFlight = true;
        emitPlayerCommand(loadSoundFontJavaScript());
        return;
    }

    emitSoundFontBytesLoad();
}

void GuitarProPreviewController::emitSoundFontBytesLoad() {
    const QString path = m_dependencies.appSettings.effectiveSoundFontPath();
    const QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile() || fileInfo.size() <= 0) {
        const QString display = tr("Could not open SoundFont file: %1").arg(path);
        setErrorMessage(display);
        m_dependencies.errorLog.logError(QStringLiteral("GuitarProPreview.soundFont"), display);
        m_soundFontLoadInFlight = true;
        emitPlayerCommand(loadSoundFontJavaScript());
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        const QString display = tr("Could not open SoundFont file: %1").arg(path);
        setErrorMessage(display);
        m_dependencies.errorLog.logError(QStringLiteral("GuitarProPreview.soundFont"), display);
        m_soundFontLoadInFlight = true;
        emitPlayerCommand(loadSoundFontJavaScript());
        return;
    }

    const QByteArray data = file.readAll();
    if (data.isEmpty()) {
        const QString display = tr("SoundFont file is empty: %1").arg(path);
        setErrorMessage(display);
        m_dependencies.errorLog.logError(QStringLiteral("GuitarProPreview.soundFont"), display);
        m_soundFontLoadInFlight = true;
        emitPlayerCommand(loadSoundFontJavaScript());
        return;
    }

    m_soundFontLoadInFlight = true;

    constexpr int kChunkSize = 384 * 1024;
    QStringList scripts;
    scripts.reserve((data.size() + kChunkSize - 1) / kChunkSize + 2);
    scripts.append(QStringLiteral(
        "window.sonarAlphaTab && window.sonarAlphaTab.beginSoundFontBytesLoad();"));
    for (int offset = 0; offset < data.size(); offset += kChunkSize) {
        const QByteArray chunk = data.mid(offset, kChunkSize);
        scripts.append(QStringLiteral(
                           "window.sonarAlphaTab && "
                           "window.sonarAlphaTab.appendSoundFontBytesChunk(%1);")
                           .arg(jsStringLiteral(QString::fromLatin1(chunk.toBase64()))));
    }
    scripts.append(QStringLiteral(
        "window.sonarAlphaTab && window.sonarAlphaTab.finishSoundFontBytesLoad();"));
    emit runPlayerJavaScriptSequence(scripts);
}

QString GuitarProPreviewController::playerSettingsObjectJavaScript() const {
    return QStringLiteral(
               "{"
               "trackIndex:%1,"
               "tempoPercent:%2,"
               "loopEnabled:%3,"
               "loopStartBar:%4,"
               "loopEndBar:%5,"
               "mixer:%6,"
               "metronomeEnabled:%7,"
               "metronomeDivision:%8,"
               "countInEnabled:%9,"
               "transposeSemitones:%10,"
               "soundFontUseCustom:%11"
               "}")
        .arg(m_selectedTrackIndex)
        .arg(m_tempoPercent)
        .arg(jsBool(m_loopEnabled))
        .arg(m_loopStartBar)
        .arg(m_loopEndBar)
        .arg(mixerTracksJavaScriptArray())
        .arg(jsBool(m_metronomeEnabled))
        .arg(m_metronomeDivision)
        .arg(jsBool(m_countInEnabled))
        .arg(m_transposeSemitones, 0, 10)
        .arg(jsBool(!m_dependencies.appSettings.usesBuiltInSoundFont()
                    && !m_dependencies.appSettings.effectiveSoundFontPath().isEmpty()));
}

void GuitarProPreviewController::handlePlayerEvent(const QString &json) {
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        return;
    }
    const QJsonObject root = doc.object();
    const QString event = root.value(QStringLiteral("event")).toString();
    const QJsonObject payload = root.value(QStringLiteral("payload")).toObject();

    if (event == QLatin1String("bridgeReady")) {
        m_bridgeReady = true;
        m_loadedSoundFontKey.clear();
        m_soundFontCustomAbandoned = false;
        m_soundFontLoadInFlight = false;
        ensureCustomSoundFontLoaded();
        scheduleScoreLoad();
        return;
    }

    if (event == QLatin1String("scoreLoaded")) {
        const QJsonArray names = payload.value(QStringLiteral("trackNames")).toArray();
        if (!names.isEmpty()) {
            QStringList trackNames;
            trackNames.reserve(names.size());
            for (const QJsonValue &v : names) {
                trackNames.append(v.toString());
            }
            if (m_trackNames != trackNames) {
                m_trackNames = trackNames;
                emit trackNamesChanged();
            }
            rebuildMixerTracks(trackNames);
        }
        const QString title = payload.value(QStringLiteral("title")).toString();
        if (!title.isEmpty() && m_title != title) {
            m_title = title;
            emit titleChanged();
        }
        const QString artist = payload.value(QStringLiteral("artist")).toString();
        if (!artist.isEmpty() && m_artist != artist) {
            m_artist = artist;
            emit artistChanged();
        }
        setBarCount(payload.value(QStringLiteral("barCount")).toInt());
        if (m_loopEndBar < 1) {
            m_loopEndBar = 1;
        }
        if (m_barCount > 0 && m_loopEndBar > m_barCount) {
            m_loopEndBar = m_barCount;
            emit loopChanged();
        }
        emitPlayerCommand(applyPlayerSettingsJavaScript());
        return;
    }

    if (event == QLatin1String("playerReady")) {
        setPlayerReady(true);
        emitPlayerCommand(applyPlayerSettingsJavaScript());
        return;
    }

    if (event == QLatin1String("soundFontLoaded")) {
        m_soundFontLoadInFlight = false;
        if (payload.value(QStringLiteral("builtIn")).toBool()) {
            m_loadedSoundFontKey = QStringLiteral("__builtin__");
        } else {
            m_loadedSoundFontKey = currentSoundFontKey();
            m_soundFontCustomAbandoned = false;
        }
        setErrorMessage({});
        if (m_pendingScoreAfterSoundFont && !m_scoreBase64.isEmpty()) {
            m_pendingScoreAfterSoundFont = false;
            emitPlayerCommand(loadScoreJavaScript());
        }
        return;
    }

    if (event == QLatin1String("soundFontLoadFailed")) {
        m_soundFontLoadInFlight = false;
        const QString message = payload.value(QStringLiteral("message")).toString();
        const QString display =
            message.isEmpty() ? tr("SoundFont could not be loaded") : message;
        setErrorMessage(display);
        m_dependencies.errorLog.logError(QStringLiteral("GuitarProPreview.soundFont"), display);
        if (!m_dependencies.appSettings.usesBuiltInSoundFont()) {
            m_loadedSoundFontKey = QStringLiteral("__builtin__");
            m_soundFontCustomAbandoned = true;
        }
        return;
    }

    if (event == QLatin1String("playerState")) {
        setPlaying(payload.value(QStringLiteral("playing")).toBool());
        return;
    }

    if (event == QLatin1String("error")) {
        const QString message = payload.value(QStringLiteral("message")).toString();
        if (!message.isEmpty()) {
            setErrorMessage(message);
            m_dependencies.errorLog.logError(QStringLiteral("GuitarProPreview.player"), message);
        }
    }
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
    setPlayerReady(false);
    setPlaying(false);
    m_soundFontLoadInFlight = false;
    setErrorMessage({});
    if (m_mediaFileId != mediaFileId) {
        m_mediaFileId = mediaFileId;
        emit mediaFileIdChanged();
    }

    QThread *thread = QThread::create([this, path, mediaFileId, generation]() {
        LoadResult result;
        result.mediaFileId = mediaFileId;
        result.generation = generation;

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            result.ok = false;
            result.errorMessage = tr("Could not read Guitar Pro file");
        } else {
            result.scoreBase64 = QString::fromLatin1(file.readAll().toBase64());
            file.close();

            const std::optional<AsciiTabRenderer::SongPreview> preview =
                AsciiTabRenderer::loadFromFile(path);
            if (!preview.has_value()) {
                // Player may still load the raw bytes via AlphaTab.
                result.ok = !result.scoreBase64.isEmpty();
                if (!result.ok) {
                    result.errorMessage = tr("Could not parse Guitar Pro file");
                } else {
                    result.errorMessage = tr("ASCII preview unavailable; using interactive player");
                }
            } else if (preview->tracks.empty()) {
                result.ok = !result.scoreBase64.isEmpty();
                result.preview = *preview;
                if (!result.ok) {
                    result.errorMessage = tr("Guitar Pro file has no tracks");
                }
            } else {
                result.ok = true;
                result.preview = *preview;
            }
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

    if (m_preview.barCount > 0) {
        setBarCount(m_preview.barCount);
    }

    if (m_scoreBase64 != result.scoreBase64) {
        m_scoreBase64 = result.scoreBase64;
        emit scoreBase64Changed();
    }

    if (m_title != m_preview.title && !m_preview.title.isEmpty()) {
        m_title = m_preview.title;
        emit titleChanged();
    }
    if (m_artist != m_preview.artist && !m_preview.artist.isEmpty()) {
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

    if (!names.isEmpty() && m_trackNames != names) {
        m_trackNames = names;
        emit trackNamesChanged();
    }
    rebuildMixerTracks(names.isEmpty() ? m_trackNames : names);

    m_selectedTrackIndex = names.isEmpty() ? 0 : defaultIndex;
    emit selectedTrackIndexChanged();
    applySelectedTrack();

    if (!m_loaded) {
        m_loaded = true;
        emit loadedChanged();
    }

    if (!result.errorMessage.isEmpty() && m_preview.tracks.empty()) {
        setErrorMessage(result.errorMessage);
    } else {
        setErrorMessage({});
    }

    if (usePlayer() && !m_scoreBase64.isEmpty()) {
        scheduleScoreLoad();
    }
}

void GuitarProPreviewController::applySelectedTrack() {
    if (m_selectedTrackIndex < 0 ||
        m_selectedTrackIndex >= static_cast<int>(m_preview.tracks.size())) {
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

void GuitarProPreviewController::setPlaying(bool playing) {
    if (m_playing == playing) {
        return;
    }
    m_playing = playing;
    emit playingChanged();
}

void GuitarProPreviewController::setPlayerReady(bool ready) {
    if (m_playerReady == ready) {
        return;
    }
    m_playerReady = ready;
    emit playerReadyChanged();
}

void GuitarProPreviewController::setBarCount(int count) {
    const int clamped = qMax(0, count);
    if (m_barCount == clamped) {
        return;
    }
    m_barCount = clamped;
    emit barCountChanged();
}

void GuitarProPreviewController::emitPlayerCommand(const QString &javaScript) {
    if (javaScript.isEmpty() || !usePlayer()) {
        return;
    }
    emit runPlayerJavaScript(javaScript);
}

void GuitarProPreviewController::resetPreviewState() {
    const bool hadLoaded = m_loaded;
    m_preview = {};
    m_mediaFileId = 0;
    emit mediaFileIdChanged();

    if (!m_scoreBase64.isEmpty()) {
        m_scoreBase64.clear();
        emit scoreBase64Changed();
    }
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
    setBarCount(0);
    m_loopEnabled = false;
    m_loopStartBar = 1;
    m_loopEndBar = 1;
    emit loopChanged();
    if (m_metronomeEnabled) {
        m_metronomeEnabled = false;
        emit metronomeChanged();
    }
    if (m_metronomeDivision != 4) {
        m_metronomeDivision = 4;
        emit metronomeChanged();
    }
    if (m_countInEnabled) {
        m_countInEnabled = false;
        emit countInChanged();
    }
    resetMixerTracks();
    if (m_transposeSemitones != 0) {
        m_transposeSemitones = 0;
        emit transposeChanged();
    }
    m_loadedSoundFontKey.clear();
    m_soundFontLoadInFlight = false;
    m_soundFontCustomAbandoned = false;
    if (hadLoaded) {
        m_loaded = false;
        emit loadedChanged();
    }
    setErrorMessage({});
}

void GuitarProPreviewController::rebuildMixerTracks(const QStringList &names) {
    QVariantList next;
    next.reserve(names.size());
    for (int i = 0; i < names.size(); ++i) {
        QVariantMap entry;
        entry.insert(QStringLiteral("name"), names.at(i));
        entry.insert(QStringLiteral("volume"), 1.0);
        entry.insert(QStringLiteral("muted"), false);
        next.append(entry);
    }
    if (m_mixerTracks == next) {
        return;
    }
    m_mixerTracks = std::move(next);
    emit mixerTracksChanged();
}

void GuitarProPreviewController::resetMixerTracks() {
    if (m_mixerTracks.isEmpty()) {
        return;
    }
    m_mixerTracks.clear();
    emit mixerTracksChanged();
}

QString GuitarProPreviewController::mixerTracksJavaScriptArray() const {
    QStringList parts;
    parts.reserve(m_mixerTracks.size());
    for (const QVariant &item : m_mixerTracks) {
        const QVariantMap entry = item.toMap();
        const double volume = qBound(0.0, entry.value(QStringLiteral("volume")).toDouble(), 1.0);
        const bool muted = entry.value(QStringLiteral("muted")).toBool();
        parts.append(QStringLiteral("{volume:%1,muted:%2}")
                         .arg(volume, 0, 'f', 4)
                         .arg(jsBool(muted)));
    }
    return QLatin1Char('[') + parts.join(QLatin1Char(',')) + QLatin1Char(']');
}
