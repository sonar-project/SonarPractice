#ifndef PRACTICESESSIONCONTROLLER_H
#define PRACTICESESSIONCONTROLLER_H

#include <QObject>
#include <QtQml/qqmlregistration.h>

class ApplicationErrorLog;
class IMediaFileRepository;
class IPathResolver;
class ILauncher;

/**
 * @brief Opens media files externally for the Practice Hub flow.
 *
 * openAsset() launches any file; startPractice() requires canBePracticed.
 * UI navigation lock while the timer runs is handled in QML via practiceTracker.timerRunning.
 */
class PracticeSessionController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Instances are provided via the 'practiceSession' context property.")

  public:
    struct Dependencies {
        IMediaFileRepository &mediaRepo;
        IPathResolver &pathResolver;
        ILauncher &launcher;
        ApplicationErrorLog &errorLog;
    };

    explicit PracticeSessionController(const Dependencies &dependencies, QObject *parent = nullptr);

  public slots:
    /** Opens a file externally (path resolved via IPathResolver). */
    bool openAsset(qlonglong mediaFileId);
    /** Validates practice media and launches externally. */
    bool startPractice(qlonglong mediaFileId);

  signals:
    void launchFailed(const QString &message);

  private:
    bool launchMediaFile(qlonglong mediaFileId, const QString &context);
    bool reportFailure(const QString &context, const QString &message);

    Dependencies m_dependencies;
};

#endif // PRACTICESESSIONCONTROLLER_H
