#include "profilestore.h"

#include "a50protocol.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>

namespace AstroSpatial {
namespace {

QVariantMap readMap(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return {};
    return document.toVariant().toMap();
}

bool writeMap(const QString &path, const QVariantMap &map, QString *error)
{
    QSaveFile file(path);
    const QByteArray bytes = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Indented);
    if (!file.open(QIODevice::WriteOnly)
        || file.write(bytes) != bytes.size() || !file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

} // namespace

ProfileStore::ProfileStore()
{
    QDir().mkpath(directory());
}

QString ProfileStore::directory() const
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/astro-manager/profiles");
}

QString ProfileStore::pathForId(const QString &id) const
{
    return directory() + QLatin1Char('/') + id + QStringLiteral(".json");
}

QVariantList ProfileStore::profiles() const
{
    QVariantList result;
    QDir dir(directory());
    const auto files = dir.entryInfoList({QStringLiteral("*.json")}, QDir::Files, QDir::Time);
    for (const auto &file : files) {
        const auto map = readMap(file.absoluteFilePath());
        QString ignored;
        if (!validate(map, &ignored))
            continue;
        result.append(QVariantMap{
            {QStringLiteral("id"), map.value(QStringLiteral("id"))},
            {QStringLiteral("name"), map.value(QStringLiteral("name"))},
            {QStringLiteral("modified"), map.value(QStringLiteral("modified"))},
        });
    }
    return result;
}

QVariantMap ProfileStore::profile(const QString &id) const
{
    const auto map = readMap(pathForId(id));
    QString ignored;
    return validate(map, &ignored) ? map : QVariantMap{};
}

QVariantMap ProfileStore::hardwareForProfile(const QVariantMap &state)
{
    QVariantMap result;
    static const QStringList scalarKeys = {
        QStringLiteral("activeEqPreset"), QStringLiteral("microphoneEq"),
        QStringLiteral("microphoneLevel"), QStringLiteral("sidetone"),
        QStringLiteral("streamMic"), QStringLiteral("streamChat"),
        QStringLiteral("streamGame"), QStringLiteral("streamAux"),
        QStringLiteral("noiseGate"), QStringLiteral("alertVolume"),
        QStringLiteral("defaultBalance"),
    };
    for (const auto &key : scalarKeys)
        result.insert(key, state.value(key));

    QVariantList presets;
    for (const auto &presetValue : state.value(QStringLiteral("eqPresets")).toList()) {
        const auto preset = presetValue.toMap();
        QVariantList bands;
        for (const auto &bandValue : preset.value(QStringLiteral("bands")).toList()) {
            const auto band = bandValue.toMap();
            bands.append(QVariantMap{
                {QStringLiteral("band"), band.value(QStringLiteral("band"))},
                {QStringLiteral("gain"), band.value(QStringLiteral("gain"))},
                {QStringLiteral("frequency"), band.value(QStringLiteral("frequency"))},
                {QStringLiteral("bandwidth"), band.value(QStringLiteral("bandwidth"))},
            });
        }
        presets.append(QVariantMap{
            {QStringLiteral("slot"), preset.value(QStringLiteral("slot"))},
            {QStringLiteral("name"), preset.value(QStringLiteral("name"))},
            {QStringLiteral("bands"), bands},
        });
    }
    result.insert(QStringLiteral("eqPresets"), presets);
    return result;
}

bool ProfileStore::validate(const QVariantMap &profile, QString *error) const
{
    if (profile.value(QStringLiteral("schemaVersion")).toInt() != 1) {
        if (error) *error = QStringLiteral("Unsupported profile schema version.");
        return false;
    }
    if (profile.value(QStringLiteral("id")).toString().isEmpty()
        || profile.value(QStringLiteral("name")).toString().trimmed().isEmpty()
        || profile.value(QStringLiteral("spatial")).toMap().isEmpty()
        || profile.value(QStringLiteral("hardware")).toMap().isEmpty()) {
        if (error) *error = QStringLiteral("The profile is missing required fields.");
        return false;
    }
    const auto hardware = profile.value(QStringLiteral("hardware")).toMap();
    if (!A50Protocol::validPreset(hardware.value(QStringLiteral("activeEqPreset")).toInt())
        || hardware.value(QStringLiteral("eqPresets")).toList().size() != 3) {
        if (error) *error = QStringLiteral("The profile contains invalid A50 EQ data.");
        return false;
    }
    for (const auto &presetValue : hardware.value(QStringLiteral("eqPresets")).toList()) {
        const auto preset = presetValue.toMap();
        if (!A50Protocol::validPreset(preset.value(QStringLiteral("slot")).toInt())
            || preset.value(QStringLiteral("name")).toString().isEmpty()
            || preset.value(QStringLiteral("bands")).toList().size() != 5) {
            if (error) *error = QStringLiteral("The profile contains an invalid EQ slot.");
            return false;
        }
        for (const auto &bandValue : preset.value(QStringLiteral("bands")).toList()) {
            const auto band = bandValue.toMap();
            const int number = band.value(QStringLiteral("band")).toInt();
            const int wireBandwidth = (number == 1 || number == 5)
                ? 0 : qRound(band.value(QStringLiteral("bandwidth")).toDouble() * 4096.0);
            if (!A50Protocol::validBand(number)
                || !A50Protocol::validGain(band.value(QStringLiteral("gain")).toInt())
                || !A50Protocol::validFrequency(band.value(QStringLiteral("frequency")).toInt())
                || !A50Protocol::validBandwidth(number, wireBandwidth)) {
                if (error) *error = QStringLiteral("The profile contains an invalid EQ band.");
                return false;
            }
        }
    }
    return true;
}

QString ProfileStore::create(const QString &name, const QVariantMap &spatial,
                             const QVariantMap &hardware, QString *error)
{
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        if (error) *error = QStringLiteral("Enter a profile name.");
        return {};
    }
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    const QVariantMap map = {
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), trimmedName},
        {QStringLiteral("created"), now},
        {QStringLiteral("modified"), now},
        {QStringLiteral("spatial"), spatial},
        {QStringLiteral("hardware"), hardwareForProfile(hardware)},
    };
    if (!validate(map, error) || !writeMap(pathForId(id), map, error))
        return {};
    return id;
}

bool ProfileStore::remove(const QString &id, QString *error)
{
    const QString path = pathForId(id);
    if (!QFileInfo::exists(path)) {
        if (error) *error = QStringLiteral("Profile not found.");
        return false;
    }
    if (!QFile::remove(path)) {
        if (error) *error = QStringLiteral("Could not remove the profile.");
        return false;
    }
    return true;
}

bool ProfileStore::importFile(const QString &path, QString *error)
{
    QVariantMap map = readMap(path);
    if (!validate(map, error))
        return false;
    map.insert(QStringLiteral("id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    map.insert(QStringLiteral("modified"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return writeMap(pathForId(map.value(QStringLiteral("id")).toString()), map, error);
}

bool ProfileStore::exportFile(const QString &id, const QString &path, QString *error) const
{
    const auto map = profile(id);
    if (map.isEmpty()) {
        if (error) *error = QStringLiteral("Profile not found.");
        return false;
    }
    return writeMap(path, map, error);
}

} // namespace AstroSpatial
