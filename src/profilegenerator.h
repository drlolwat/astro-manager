#pragma once

#include "types.h"

#include <QList>
#include <QString>

namespace AstroSpatial {

struct ChannelPosition {
    QString channel;
    int azimuth = 0;
    int elevation = 0;
};

struct ProfileSpec {
    QString id;
    QString displayName;
    Mode mode = Mode::Surround71;
    QList<ChannelPosition> channels;
};

struct EqualizerBand {
    QString type;
    double frequency = 0.0;
    double gain = 0.0;
    double q = 0.707;
};

struct EqualizerPreset {
    QString id;
    QString displayName;
    QList<EqualizerBand> bands;
};

class ProfileGenerator
{
public:
    static QList<ProfileSpec> profiles();
    static ProfileSpec profile(const QString &id);
    static QString defaultProfileId(Mode mode);
    static QList<EqualizerPreset> equalizerPresets();
    static EqualizerPreset equalizerPreset(const QString &id);

    static QString generate(const ProfileSpec &profile,
                            const QString &sofaPath,
                            const QString &targetNode,
                            bool includeLimiter = true,
                            const QString &equalizerPresetId = QStringLiteral("bass-heavy"));

    static bool validateInputs(const ProfileSpec &profile,
                               const QString &sofaPath,
                               const QString &targetNode,
                               QString *error = nullptr);
};

} // namespace AstroSpatial
