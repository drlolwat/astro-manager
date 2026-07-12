#include "appcontroller.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QIcon>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSystemTrayIcon>
#include <QWindow>

using AstroSpatial::AppController;

class ApplicationBridge : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.drlolwat.AstroSpatial.Application")

public:
    ApplicationBridge(QWindow *window, AppController *controller, QObject *parent = nullptr)
        : QObject(parent)
        , m_window(window)
        , m_controller(controller)
    {
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

private:
    QWindow *m_window = nullptr;
    AppController *m_controller = nullptr;
};

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Astro Spatial"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setOrganizationName(QStringLiteral("AstroSpatial"));
    QApplication::setDesktopFileName(QStringLiteral("io.github.drlolwat.AstroSpatial"));
    QApplication::setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Binaural surround controller for the Astro A50"));
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

    constexpr auto serviceName = "io.github.drlolwat.AstroSpatial";
    auto bus = QDBusConnection::sessionBus();
    if (!bus.registerService(QString::fromLatin1(serviceName))) {
        QDBusInterface existing(QString::fromLatin1(serviceName), QStringLiteral("/Application"),
                                QStringLiteral("io.github.drlolwat.AstroSpatial.Application"), bus);
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
    engine.loadFromModule(QStringLiteral("AstroSpatial"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty())
        return 1;

    auto *window = qobject_cast<QWindow *>(engine.rootObjects().constFirst());
    if (!window)
        return 1;
    if (parser.isSet(backgroundOption))
        window->hide();

    ApplicationBridge bridge(window, &controller);
    bus.registerObject(QStringLiteral("/Application"), &bridge,
                       QDBusConnection::ExportAllSlots);

    QSystemTrayIcon tray(QIcon::fromTheme(QStringLiteral("audio-headphones")), &application);
    tray.setToolTip(QStringLiteral("Astro Spatial"));
    QMenu trayMenu;
    auto *showAction = trayMenu.addAction(QStringLiteral("Open Astro Spatial"));
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
