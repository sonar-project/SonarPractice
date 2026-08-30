#ifndef GUITARPROPREVIEWCONTROLLER_H
#define GUITARPROPREVIEWCONTROLLER_H

#include "AsciiTabRenderer.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

#include <atomic>
#include <vector>

class ApplicationErrorLog;
class IMediaFileRepository;
class IPathResolver;

/**
 * @brief QML view model for read-only Guitar Pro ASCII tablature preview.
 *
 * Parses on a worker thread; track switching reuses the cached SongPreview.
 */
class GuitarProPreviewController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Instances are provided via the 'guitarProPreviewController' context property.")

    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY artistChanged)
    Q_PROPERTY(QString tuning READ tuning NOTIFY tuningChanged)
    Q_PROPERTY(QString tabText READ tabText NOTIFY tabTextChanged)
    Q_PROPERTY(QStringList trackNames READ trackNames NOTIFY trackNamesChanged)
    Q_PROPERTY(int selectedTrackIndex READ selectedTrackIndex WRITE setSelectedTrackIndex NOTIFY
                   selectedTrackIndexChanged)
    Q_PROPERTY(qlonglong mediaFileId READ mediaFileId NOTIFY mediaFileIdChanged)

  public:
    struct Dependencies {
        IMediaFileRepository &mediaRepo;
        IPathResolver &pathResolver;
        ApplicationErrorLog &errorLog;
    };

    explicit GuitarProPreviewController(const Dependencies &dependencies,
                                        QObject *parent = nullptr);

    [[nodiscard]] bool loading() const;
    [[nodiscard]] bool loaded() const;
    [[nodiscard]] bool visible() const;
    void setVisible(bool visible);

    [[nodiscard]] const QString &errorMessage() const;
    [[nodiscard]] const QString &title() const;
    [[nodiscard]] const QString &artist() const;
    [[nodiscard]] const QString &tuning() const;
    [[nodiscard]] const QString &tabText() const;
    [[nodiscard]] const QStringList &trackNames() const;
    [[nodiscard]] int selectedTrackIndex() const;
    void setSelectedTrackIndex(int index);
    [[nodiscard]] qlonglong mediaFileId() const;

  public slots:
    /** Loads and shows preview for @p mediaFileId (Guitar Pro only). */
    Q_INVOKABLE void load(qlonglong mediaFileId);
    /** Hides the panel and clears the current preview. */
    Q_INVOKABLE void clear();

  signals:
    void loadingChanged();
    void loadedChanged();
    void visibleChanged();
    void errorMessageChanged();
    void titleChanged();
    void artistChanged();
    void tuningChanged();
    void tabTextChanged();
    void trackNamesChanged();
    void selectedTrackIndexChanged();
    void mediaFileIdChanged();

  private:
    struct LoadResult {
        qlonglong mediaFileId{};
        int generation{};
        bool ok{false};
        QString errorMessage{};
        AsciiTabRenderer::SongPreview preview{};
    };

    void applyLoadResult(const LoadResult &result);
    void applySelectedTrack();
    void setLoading(bool loading);
    void setErrorMessage(const QString &message);
    void resetPreviewState();

    Dependencies m_dependencies;
    std::atomic<int> m_loadGeneration{0};

    bool m_loading{false};
    bool m_loaded{false};
    bool m_visible{false};
    qlonglong m_mediaFileId{};
    QString m_errorMessage{};
    QString m_title{};
    QString m_artist{};
    QString m_tuning{};
    QString m_tabText{};
    QStringList m_trackNames{};
    int m_selectedTrackIndex{-1};
    AsciiTabRenderer::SongPreview m_preview{};
};

#endif // GUITARPROPREVIEWCONTROLLER_H
