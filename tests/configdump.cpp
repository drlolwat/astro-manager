#include "profilegenerator.h"

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

using namespace AstroSpatial;

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    const QString profileId = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                       : QStringLiteral("reference-7.1");
    const QString target = argc > 2 ? QString::fromLocal8Bit(argv[2])
                                    : QStringLiteral("alsa_output.usb-Astro_Gaming_Astro_A50-00.stereo-chat");
    const QString generated = ProfileGenerator::generate(
        ProfileGenerator::profile(profileId),
        QStringLiteral("/usr/share/libmysofa/default.sofa"), target, true);
    if (argc > 3) {
        QFile output(QString::fromLocal8Bit(argv[3]));
        if (!output.open(QIODevice::WriteOnly | QIODevice::Text))
            return 2;
        output.write(generated.toUtf8());
    } else {
        QTextStream(stdout) << generated;
    }
    return 0;
}
