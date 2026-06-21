#ifndef MEDIASTREAMPROBE_H
#define MEDIASTREAMPROBE_H

#include "MediaFile.h"

#include <QCoreApplication>
#include <QString>

struct MediaStreamProbeResult {
    bool hasVideo{false};
    bool hasAudio{false};
    bool probed{false};
    QString error{};
};

[[nodiscard]] MediaKind classifyFromProbe(const MediaStreamProbeResult &result,
                                          MediaKind extensionDefault);

/**
 * @brief Probes container files for audio/video streams (FFmpeg) or infers from extension.
 */
class MediaStreamProbe {
    Q_DECLARE_TR_FUNCTIONS(MediaStreamProbe)

  public:
    [[nodiscard]] static bool isContainerExtension(const QString &extension);

    [[nodiscard]] static MediaStreamProbeResult probeFile(const QString &absolutePath);

    [[nodiscard]] static MediaStreamProbeResult inferFromExtension(MediaKind extensionKind);

    [[nodiscard]] static MediaStreamProbeResult
    probeOrInfer(const QString &absolutePath, MediaKind extensionKind, const QString &extension);
};

#endif // MEDIASTREAMPROBE_H
