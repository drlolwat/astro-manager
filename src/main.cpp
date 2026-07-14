#include "appcontroller.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QIcon>
#include <QJsonDocument>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSystemTrayIcon>
#include <QWindow>

using AstroSpatial::AppController;

class ApplicationBridge : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.drlolwat.AstroManager1")

public:
    ApplicationBridge(QWindow *window, AppController *controller, QObject *parent = nullptr)
        : QObject(parent)
        , m_window(window)
        , m_controller(controller)
    {
        connect(m_controller, &AppController::stateChanged, this, &ApplicationBridge::StateChanged);
    }

public slots:
    void Show()
    {
        if (!m_window)
            return;
        m_window->show();
        m_window->raise();
        m_window->requestActivate();
    }

    void SetMode(int mode)
    {
        if (m_controller)
            m_controller->activateMode(mode);
    }

    void SetOutput(int endpoint)
    {
        if (m_controller)
            m_controller->selectOutputEndpoint(endpoint);
    }

    QVariantMap State() const
    {
        if (!m_controller)
            return {};
        QVariantMap device = m_controller->hardwareManager()->state();
        device.insert(QStringLiteral("busy"), m_controller->hardwareManager()->busy());
        device.insert(QStringLiteral("error"), m_controller->hardwareManager()->errorMessage());
        return {
            {QStringLiteral("audio"), QVariantMap{
                {QStringLiteral("connected"), m_controller->connected()},
                {QStringLiteral("mode"), m_controller->mode()},
                {QStringLiteral("outputEndpoint"), m_controller->outputEndpoint()},
                {QStringLiteral("profileId"), m_controller->profileId()},
                {QStringLiteral("eqPresetId"), m_controller->eqPresetId()},
                {QStringLiteral("status"), m_controller->statusTitle()},
                {QStringLiteral("detail"), m_controller->statusDetail()},
            }},
            {QStringLiteral("device"), device},
            {QStringLiteral("profiles"), m_controller->savedProfiles()},
        };
    }

    QString StateJson() const
    {
        return QString::fromUtf8(QJsonDocument::fromVariant(State()).toJson(QJsonDocument::Indented));
    }

    void RefreshHardware() { if (m_controller) m_controller->hardwareManager()->refresh(); }
    void SaveToHeadset() { if (m_controller) m_controller->hardwareManager()->saveToHeadset(); }
    void RevertToSaved() { if (m_controller) m_controller->hardwareManager()->revertToSaved(); }
    void ApplyProfile(const QString &id) { if (m_controller) m_controller->applyUnifiedProfile(id); }
    void CreateProfile(const QString &name) { if (m_controller) m_controller->createUnifiedProfile(name); }
    void DeleteProfile(const QString &id) { if (m_controller) m_controller->deleteUnifiedProfile(id); }
    void SetActiveEqPreset(int preset) { if (m_controller) m_controller->hardwareManager()->setActiveEqPreset(preset); }
    void SetEqPresetName(int preset, const QString &name) { if (m_controller) m_controller->hardwareManager()->setEqPresetName(preset, name); }
    void SetEqGain(int preset, int band, int gain) { if (m_controller) m_controller->hardwareManager()->setEqGain(preset, band, gain); }
    void SetEqBand(int preset, int band, int frequency, double bandwidth) { if (m_controller) m_controller->hardwareManager()->setEqBand(preset, band, frequency, bandwidth); }
    void SetSlider(int slider, int value) { if (m_controller) m_controller->hardwareManager()->setSlider(slider, value); }
    void SetNoiseGate(int mode) { if (m_controller) m_controller->hardwareManager()->setNoiseGate(mode); }
    void SetMicrophoneEq(int preset) { if (m_controller) m_controller->hardwareManager()->setMicrophoneEq(preset); }
    void SetAlertVolume(int value) { if (m_controller) m_controller->hardwareManager()->setAlertVolume(value); }
    void SetDefaultBalance(int value) { if (m_controller) m_controller->hardwareManager()->setDefaultBalance(value); }
    void AuditionHardwareEq(bool enabled) { if (m_controller) m_controller->hardwareManager()->auditionHardwareEq(enabled); }

signals:
    void StateChanged();

private:
    QWindow *m_window = nullptr;
    AppController *m_controller = nullptr;
};

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Astro Manager for Linux"));
    QApplication::setApplicationVersion(QStringLiteral("0.4.1"));
    QApplication::setOrganizationName(QStringLiteral("AstroSpatial")); // Preserve 0.1 settings.
    QApplication::setDesktopFileName(QStringLiteral("io.github.drlolwat.AstroManager"));
    QApplication::setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("A50 Gen 4 device and spatial-audio manager"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption backgroundOption(
        QStringLiteral("background"), QStringLiteral("Start minimized in the system tray."));
    const QCommandLineOption modeOption(
        QStringLiteral("mode"),
        QStringLiteral("Activate direct, 5.1, or 7.1 after startup."),
        QStringLiteral("mode"));
    const QCommandLineOption outputOption(
        QStringLiteral("output"),
        QStringLiteral("Route to the chat or game A50 endpoint."),
        QStringLiteral("endpoint"));
    parser.addOption(backgroundOption);
    parser.addOption(modeOption);
    parser.addOption(outputOption);
    parser.process(application);

    constexpr auto serviceName = "io.github.drlolwat.AstroManager";
    auto bus = QDBusConnection::sessionBus();
    if (!bus.registerService(QString::fromLatin1(serviceName))) {
        QDBusInterface existing(QString::fromLatin1(serviceName), QStringLiteral("/Application"),
                                QStringLiteral("io.github.drlolwat.AstroManager1"), bus);
        if (parser.isSet(outputOption)) {
            const QString output = parser.value(outputOption).toLower();
            existing.call(QStringLiteral("SetOutput"), output == QLatin1String("game") ? 1 : 0);
        }
        if (parser.isSet(modeOption)) {
            const QString mode = parser.value(modeOption).toLower();
            int value = 0;
            if (mode == QLatin1String("5.1") || mode == QLatin1String("surround-5.1"))
                value = 1;
            else if (mode == QLatin1String("7.1") || mode == QLatin1String("surround-7.1"))
                value = 2;
            existing.call(QStringLiteral("SetMode"), value);
        }
        if (!parser.isSet(backgroundOption))
            existing.call(QStringLiteral("Show"));
        return 0;
    }

    AppController controller;
    if (parser.isSet(outputOption)) {
        const QString output = parser.value(outputOption).toLower();
        controller.selectOutputEndpoint(output == QLatin1String("game") ? 1 : 0);
    }
    if (parser.isSet(modeOption)) {
        const QString mode = parser.value(modeOption).toLower();
        int modeValue = 0;
        if (mode == QLatin1String("5.1") || mode == QLatin1String("surround-5.1"))
            modeValue = 1;
        else if (mode == QLatin1String("7.1") || mode == QLatin1String("surround-7.1"))
            modeValue = 2;
        controller.activateMode(modeValue);
    }
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("controller"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("hardware"), controller.hardwareManager());
    engine.loadFromModule(QStringLiteral("AstroManager"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty())
        return 1;

    auto *window = qobject_cast<QWindow *>(engine.rootObjects().constFirst());
    if (!window)
        return 1;
    if (parser.isSet(backgroundOption))
        window->hide();

    ApplicationBridge bridge(window, &controller);
    bus.registerObject(QStringLiteral("/Application"), &bridge,
                       QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);
    bus.registerService(QStringLiteral("io.github.drlolwat.AstroSpatial")); // 0.1 compatibility alias.

    QSystemTrayIcon tray(QIcon::fromTheme(QStringLiteral("audio-headphones")), &application);
    tray.setToolTip(QStringLiteral("Astro Manager for Linux"));
    QMenu trayMenu;
    auto *showAction = trayMenu.addAction(QStringLiteral("Open Astro Manager"));
    QObject::connect(showAction, &QAction::triggered, &bridge, &ApplicationBridge::Show);
    trayMenu.addSeparator();
    auto *directAction = trayMenu.addAction(QStringLiteral("Switch to Direct Stereo"));
    QObject::connect(directAction, &QAction::triggered, &controller,
                     [&controller] { controller.activateMode(0); });
    trayMenu.addSeparator();
    auto *quitAction = trayMenu.addAction(QStringLiteral("Quit Controller"));
    QObject::connect(quitAction, &QAction::triggered, &application, &QApplication::quit);
    tray.setContextMenu(&trayMenu);
    QObject::connect(&tray, &QSystemTrayIcon::activated, &bridge,
                     [&bridge](QSystemTrayIcon::ActivationReason reason) {
                         if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
                             bridge.Show();
                     });
    tray.show();

    return application.exec();
}

#include "main.moc"
