#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDBusInterface>
#include <QDBusReply>
#include <QJsonDocument>
#include <QTextStream>

namespace {

QDBusInterface manager()
{
    return QDBusInterface(QStringLiteral("io.github.drlolwat.AstroManager"),
                          QStringLiteral("/Application"),
                          QStringLiteral("io.github.drlolwat.AstroManager1"),
                          QDBusConnection::sessionBus());
}

int callNoReply(const QString &method, const QVariant &argument = {})
{
    auto interface = manager();
    const QDBusMessage reply = argument.isValid() ? interface.call(method, argument)
                                                  : interface.call(method);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        QTextStream(stderr) << reply.errorMessage() << Qt::endl;
        return 1;
    }
    return 0;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("astro-managerctl"));
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Control Astro Manager over the session bus"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("command"),
        QStringLiteral("status, show, refresh, save, revert, mode, output, profile, capture, or delete-profile"));
    parser.addPositionalArgument(QStringLiteral("value"), QStringLiteral("Value for mode/output/profile"),
                                 QStringLiteral("[value]"));
    parser.process(app);
    const auto args = parser.positionalArguments();
    if (args.isEmpty())
        parser.showHelp(1);

    const QString command = args[0].toLower();
    if (command == QLatin1String("status")) {
        auto interface = manager();
        const QDBusReply<QString> reply = interface.call(QStringLiteral("StateJson"));
        if (!reply.isValid()) {
            QTextStream(stderr) << reply.error().message() << Qt::endl;
            return 1;
        }
        QTextStream(stdout) << reply.value();
        return 0;
    }
    if (command == QLatin1String("show"))
        return callNoReply(QStringLiteral("Show"));
    if (command == QLatin1String("refresh"))
        return callNoReply(QStringLiteral("RefreshHardware"));
    if (command == QLatin1String("save"))
        return callNoReply(QStringLiteral("SaveToHeadset"));
    if (command == QLatin1String("revert"))
        return callNoReply(QStringLiteral("RevertToSaved"));
    if (args.size() < 2) {
        QTextStream(stderr) << "This command requires a value." << Qt::endl;
        return 2;
    }
    if (command == QLatin1String("mode")) {
        const QString value = args[1].toLower();
        const int mode = (value == QLatin1String("5.1")) ? 1
            : (value == QLatin1String("7.1")) ? 2 : 0;
        return callNoReply(QStringLiteral("SetMode"), mode);
    }
    if (command == QLatin1String("output"))
        return callNoReply(QStringLiteral("SetOutput"), args[1].toLower() == QLatin1String("game") ? 1 : 0);
    if (command == QLatin1String("profile"))
        return callNoReply(QStringLiteral("ApplyProfile"), args[1]);
    if (command == QLatin1String("capture"))
        return callNoReply(QStringLiteral("CreateProfile"), args[1]);
    if (command == QLatin1String("delete-profile"))
        return callNoReply(QStringLiteral("DeleteProfile"), args[1]);

    QTextStream(stderr) << "Unknown command: " << command << Qt::endl;
    return 2;
}
