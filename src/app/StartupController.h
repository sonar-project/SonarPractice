#ifndef STARTUPCONTROLLER_H
#define STARTUPCONTROLLER_H

#include "AppSettings.h"
#include "ApplicationBootstrap.h"

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <qqmlintegration.h>

class QQmlContext;

/**
 * \brief Manages the startup process of the application.
 *
 * Sits between QML and ApplicationBootstrap, exposing startup state as
 * QML-bindable properties and driving the two-phase initialization sequence:
 *
 * - needsConfiguration() == true  → QML shows setup wizard;
 *   call initializeDatabase() when done.
 * - needsConfiguration() == false → call beginAsyncInitialization() on launch.
 *
 */
class StartupController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("StartupController uncreateable")

    Q_PROPERTY(bool needsConfiguration READ needsConfiguration CONSTANT)
    Q_PROPERTY(bool shellReady READ shellReady NOTIFY shellReadyChanged)
    Q_PROPERTY(bool catalogReady READ catalogReady NOTIFY catalogReadyChanged)
    Q_PROPERTY(bool initializing READ initializing NOTIFY initializingChanged)
    Q_PROPERTY(QString initStatusText READ initStatusText NOTIFY initStatusTextChanged)

  public:
    explicit StartupController(AppSettings &appSettings, ApplicationBootstrap &bootstrap,
                               QObject *parent = nullptr);

    /** \return True when no database exists yet and setup is required. */
    [[nodiscard]] bool needsConfiguration() const;

    [[nodiscard]] bool shellReady() const;
    [[nodiscard]] bool catalogReady() const;
    [[nodiscard]] bool initializing() const;

    [[nodiscard]] const QString &initStatusText() const;

    /**
     * \brief Sets the QQmlContext used for registerContextProperties().
     *
     * Must be called before the shell becomes ready so that context properties
     * are available to QML on first load.
     */
    void setQmlContext(QQmlContext *context);

  signals:
    void shellReadyChanged();
    void catalogReadyChanged();
    void readyChanged();
    void initializingChanged();
    void initStatusTextChanged();

  public slots:
    /**
     * \brief Wires ApplicationBootstrap signals to this controller.
     *
     * Called automatically in the constructor. Exposed as a slot so it can
     * be called again if the bootstrap instance is replaced (e.g. in tests).
     */
    void connectBootstrapSignals();

    /**
     * \brief Recomputes and emits the initializing / initStatusText properties.
     */
    void updateInitializingState();

    /**
     * \brief Full synchronous startup for the first-run / setup-wizard path.
     *
     * Calls initializeShell() + beginCatalogLoad() on the bootstrap, registers
     * context properties and emits the relevant ready signals.
     *
     * \return True on success, false if the shell could not be initialized.
     */
    [[nodiscard]] Q_INVOKABLE bool initializeDatabase();

    /**
     * \brief Starts the normal async startup sequence.
     *
     * No-op when needsConfiguration() is true. Otherwise:
     *   1. ensureDefaults + managed storage directory
     *   2. initializeShell()
     *   3. scheduleLoadShellData()
     *   4. beginCatalogLoad()  ← catalog loads on a worker thread
     */
    Q_INVOKABLE void beginAsyncInitialization();

    /**
     * \brief Registers all bootstrap-owned objects as QML context properties.
     *
     * Safe to call multiple times – guards against null context and unready shell.
     */
    Q_INVOKABLE void registerContextProperties();

    [[nodiscard]] Q_INVOKABLE QString browseStorageDirectory(const QString &startDir);
    [[nodiscard]] Q_INVOKABLE QString browseImportFolder(const QString &startDir);
    [[nodiscard]] Q_INVOKABLE bool createStorageDirectory(const QString &path);

  private:
    void ensureManagedStorageDirectoryIfNeeded(const AppSettings &appSettings);

    AppSettings &m_appSettings;
    ApplicationBootstrap &m_bootstrap;
    QQmlContext *m_qmlContext{nullptr};
    bool m_initializing{false};
    QString m_initStatusText;
};

#endif // STARTUPCONTROLLER_H
