#pragma once

#include <QVariantList>
#include <QVariantMap>

namespace AstroSpatial {

class ProfileStore
{
public:
    ProfileStore();

    QVariantList profiles() const;
    QVariantMap profile(const QString &id) const;
    QString create(const QString &name, const QVariantMap &spatial,
                   const QVariantMap &hardware, QString *error);
    bool remove(const QString &id, QString *error);
    bool importFile(const QString &path, QString *error);
    bool exportFile(const QString &id, const QString &path, QString *error) const;

    static QVariantMap hardwareForProfile(const QVariantMap &state);

private:
    bool validate(const QVariantMap &profile, QString *error) const;
    QString directory() const;
    QString pathForId(const QString &id) const;
};

} // namespace AstroSpatial
