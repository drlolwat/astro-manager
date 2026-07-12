#include "systemdmanager.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

namespace AstroSpatial {
namespace {

constexpr auto UnitName = "astro-spatial-dsp.service";

QDBusInterface managerInterface()
{
    return QDBusInterface(QStringLiteral("org.freedesktop.systemd1"),
                          QStringLiteral("/org/freedesktop/systemd1"),
                          QStringLiteral("org.freedesktop.systemd1.Manager"),
                          QDBusConnection::sessionBus());
}

QString systemdEscape(QString path)
{
    path.replace(QLatin1Char('%'), QStringLiteral("%%"));
    path.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return path;
}

} // namespace

SystemdManager::SystemdManager(QObject *parent)
    : QObject(parent)
{
}

QString SystemdManager::unitPath() const
{
    return QDir::homePath() + QStringLiteral("/.config/systemd/user/") + QString::fromLatin1(UnitName);
}

bool SystemdManager::installUserUnit(const QString &configPath, QString *error)
{
    const QFileInfo info(unitPath());
    if (!QDir().mkpath(info.absolutePath())) {
        if (error) *error = QStringLiteral("Could not create %1").arg(info.absolutePath());
        return false;
    }

    const QString unit = QStringLiteral(R"([Unit]
Description=Astro Spatial PipeWire DSP
After=pipewire.service wireplumber.service
Wants=pipewire.service wireplumber.service

[Service]
Type=simple
ExecStart=/usr/bin/pipewire -c "%1"
Restart=on-failure
RestartSec=1

[Install]
WantedBy=default.target
)").arg(systemdEscape(configPath));

    QSaveFile file(unitPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)
        || file.write(unit.toUtf8()) != unit.toUtf8().size()
        || !file.commit()) {
        if (error) *error = QStringLiteral("Could not install the systemd user unit: %1").arg(file.errorString());
        return false;
    }
    return daemonReload(error);
}

bool SystemdManager::daemonReload(QString *error)
{
    auto manager = managerInterface();
    const QDBusMessage reply = manager.call(QStringLiteral("Reload"));
    if (reply.type() == QDBusMessage::ErrorMessage) {
        if (error) *error = reply.errorMessage();
        return false;
    }
    return true;
}

bool SystemdManager::callUnitMethod(const QString &method, QString *error)
{
    auto manager = managerInterface();
    const QDBusMessage reply = manager.call(method, QString::fromLatin1(UnitName), QStringLiteral("replace"));
    if (reply.type() == QDBusMessage::ErrorMessage) {
        if (error) *error = reply.errorMessage();
        return false;
    }
    return true;
}

bool SystemdManager::start(QString *error)
{
    return callUnitMethod(QStringLiteral("StartUnit"), error);
}

bool SystemdManager::stop(QString *error)
{
    return callUnitMethod(QStringLiteral("StopUnit"), error);
}

bool SystemdManager::restart(QString *error)
{
    return callUnitMethod(QStringLiteral("RestartUnit"), error);
}

} // namespace AstroSpatial

