#include "pipewiremonitor.h"

#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

using namespace AstroSpatial;

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    PipeWireMonitor monitor;
    QString error;
    if (!monitor.start(&error)) {
        QTextStream(stderr) << error << '\n';
        return 2;
    }
    QTimer::singleShot(1000, &application, [&] {
        QTextStream(stdout)
            << "chat=" << monitor.chatAvailable()
            << " game=" << monitor.gameAvailable()
            << " metadata=" << monitor.metadataAvailable()
            << " default=" << monitor.defaultNodeName() << '\n';
        application.exit(monitor.chatAvailable() && monitor.gameAvailable()
                             && monitor.metadataAvailable() ? 0 : 1);
    });
    return application.exec();
}
