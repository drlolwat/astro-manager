#pragma once

#include <QMetaType>
#include <QString>

namespace AstroSpatial {

Q_NAMESPACE

enum class Mode {
    Direct = 0,
    Surround51 = 1,
    Surround71 = 2,
};
Q_ENUM_NS(Mode)

enum class OutputEndpoint {
    Chat = 0,
    Game = 1,
};
Q_ENUM_NS(OutputEndpoint)

enum class PathStatus {
    Disconnected = 0,
    DirectVerified,
    Converted,
    Spatialized,
    Error,
};
Q_ENUM_NS(PathStatus)

inline QString modeKey(Mode mode)
{
    switch (mode) {
    case Mode::Direct: return QStringLiteral("direct");
    case Mode::Surround51: return QStringLiteral("surround-5.1");
    case Mode::Surround71: return QStringLiteral("surround-7.1");
    }
    return QStringLiteral("direct");
}

inline Mode modeFromKey(const QString &key)
{
    if (key == QLatin1String("surround-5.1")) return Mode::Surround51;
    if (key == QLatin1String("surround-7.1")) return Mode::Surround71;
    return Mode::Direct;
}

inline QString endpointKey(OutputEndpoint endpoint)
{
    return endpoint == OutputEndpoint::Game ? QStringLiteral("game") : QStringLiteral("chat");
}

inline OutputEndpoint endpointFromKey(const QString &key)
{
    return key == QLatin1String("game") ? OutputEndpoint::Game : OutputEndpoint::Chat;
}

} // namespace AstroSpatial

Q_DECLARE_METATYPE(AstroSpatial::Mode)
Q_DECLARE_METATYPE(AstroSpatial::OutputEndpoint)
Q_DECLARE_METATYPE(AstroSpatial::PathStatus)

