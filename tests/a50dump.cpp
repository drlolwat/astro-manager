#include "a50device.h"

#include <QCoreApplication>
#include <QTextStream>

using namespace AstroSpatial;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);
    A50Device device;
    QString error;
    if (!device.open(&error)) {
        err << error << Qt::endl;
        return 1;
    }
    A50Snapshot snapshot;
    if (!device.readSnapshot(&snapshot, &error)) {
        err << error << Qt::endl;
        return 2;
    }
    out << "A50 Gen 4 " << snapshot.baseFirmware << " / " << snapshot.headsetFirmware << '\n'
        << "battery=" << snapshot.batteryPercent << " charging=" << snapshot.charging
        << " docked=" << snapshot.docked << " on=" << snapshot.headsetOn << '\n'
        << "active-eq=" << snapshot.settings.activeEqPreset
        << " microphone=" << snapshot.settings.microphoneLevel
        << " sidetone=" << snapshot.settings.sidetone
        << " balance=" << snapshot.settings.balance << Qt::endl;
    return 0;
}
