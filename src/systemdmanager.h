#pragma once

#include <QObject>
#include <QString>

namespace AstroSpatial {

class SystemdManager : public QObject
{
    Q_OBJECT

public:
    explicit SystemdManager(QObject *parent = nullptr);

    bool installUserUnit(const QString &configPath, QString *error = nullptr);
    bool daemonReload(QString *error = nullptr);
    bool start(QString *error = nullptr);
    bool stop(QString *error = nullptr);
    bool restart(QString *error = nullptr);
    QString unitPath() const;

private:
    bool callUnitMethod(const QString &method, QString *error);
};

} // namespace AstroSpatial

