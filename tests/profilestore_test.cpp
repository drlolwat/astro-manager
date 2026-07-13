#include "profilestore.h"

#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

using namespace AstroSpatial;

class ProfileStoreTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void roundTripsUnifiedProfiles();
};

void ProfileStoreTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

static QVariantMap hardwareState()
{
    QVariantList presets;
    for (int slot = 1; slot <= 3; ++slot) {
        QVariantList bands;
        for (int band = 1; band <= 5; ++band) {
            bands.append(QVariantMap{
                {QStringLiteral("band"), band},
                {QStringLiteral("gain"), slot - 2},
                {QStringLiteral("frequency"), 100 * band},
                {QStringLiteral("bandwidth"), (band == 1 || band == 5) ? 0.0 : 1.0},
            });
        }
        presets.append(QVariantMap{
            {QStringLiteral("slot"), slot},
            {QStringLiteral("name"), QStringLiteral("Preset %1").arg(slot)},
            {QStringLiteral("bands"), bands},
        });
    }
    return {
        {QStringLiteral("activeEqPreset"), 1},
        {QStringLiteral("eqPresets"), presets},
        {QStringLiteral("microphoneEq"), 0},
        {QStringLiteral("microphoneLevel"), 100},
        {QStringLiteral("sidetone"), 50},
        {QStringLiteral("streamMic"), 100},
        {QStringLiteral("streamChat"), 60},
        {QStringLiteral("streamGame"), 70},
        {QStringLiteral("streamAux"), 40},
        {QStringLiteral("noiseGate"), 2},
        {QStringLiteral("alertVolume"), 80},
        {QStringLiteral("defaultBalance"), 128},
    };
}

void ProfileStoreTest::roundTripsUnifiedProfiles()
{
    ProfileStore store;
    QString error;
    for (const auto &existing : store.profiles())
        store.remove(existing.toMap().value(QStringLiteral("id")).toString(), &error);
    const QVariantMap spatial = {
        {QStringLiteral("mode"), QStringLiteral("surround-7.1")},
        {QStringLiteral("outputEndpoint"), QStringLiteral("chat")},
        {QStringLiteral("hrtfProfile"), QStringLiteral("reference-7.1")},
        {QStringLiteral("softwareEq"), QStringLiteral("bass-heavy")},
    };
    const QString id = store.create(QStringLiteral("Test Rig"), spatial, hardwareState(), &error);
    QVERIFY2(!id.isEmpty(), qPrintable(error));
    QCOMPARE(store.profile(id).value(QStringLiteral("name")).toString(), QStringLiteral("Test Rig"));

    QTemporaryDir temporary;
    const QString exported = temporary.filePath(QStringLiteral("profile.json"));
    QVERIFY2(store.exportFile(id, exported, &error), qPrintable(error));
    QVERIFY2(store.importFile(exported, &error), qPrintable(error));
    QCOMPARE(store.profiles().size(), 2);
    QVERIFY2(store.remove(id, &error), qPrintable(error));
}

QTEST_MAIN(ProfileStoreTest)
#include "profilestore_test.moc"
