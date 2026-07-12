#include "profilegenerator.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace AstroSpatial;

class ProfileGeneratorTest : public QObject
{
    Q_OBJECT

private slots:
    void exposesExpectedProfiles();
    void generates51Graph();
    void generates71Graph();
    void generatesBassHeavyEqBeforeLimiter();
    void canDisableEq();
    void escapesPathsAndTargets();
    void rejectsMissingSofa();
    void identityS16ConversionIsExact();
};

void ProfileGeneratorTest::exposesExpectedProfiles()
{
    const auto profiles = ProfileGenerator::profiles();
    QCOMPARE(profiles.size(), 3);
    QCOMPARE(profiles.at(0).channels.size(), 6);
    QCOMPARE(profiles.at(1).channels.size(), 8);
    QCOMPARE(profiles.at(2).id, QStringLiteral("wide-7.1"));
}

void ProfileGeneratorTest::generates51Graph()
{
    const auto text = ProfileGenerator::generate(
        ProfileGenerator::profile(QStringLiteral("reference-5.1")),
        QStringLiteral("/usr/share/libmysofa/default.sofa"),
        QStringLiteral("a50.chat"));
    QVERIFY(text.contains(QStringLiteral("audio.channels = 6")));
    QVERIFY(text.contains(QStringLiteral("audio.position = [ FL FR FC LFE SL SR ]")));
    QVERIFY(text.contains(QStringLiteral("target.object = \"a50.chat\"")));
    QVERIFY(text.contains(QStringLiteral("bq_lowpass")));
    QVERIFY(text.contains(QStringLiteral("name = lfe_delay")));
    QVERIFY(text.contains(QStringLiteral("\"latency\" = 0.011625")));
    QVERIFY(text.contains(QStringLiteral("\"ovs\" = 21")));
    QVERIFY(!text.contains(QStringLiteral("spatial_LFE")));
}

void ProfileGeneratorTest::generates71Graph()
{
    const auto text = ProfileGenerator::generate(
        ProfileGenerator::profile(QStringLiteral("reference-7.1")),
        QStringLiteral("/usr/share/libmysofa/default.sofa"),
        QStringLiteral("a50.game"));
    QVERIFY(text.contains(QStringLiteral("audio.channels = 8")));
    QVERIFY(text.contains(QStringLiteral("audio.position = [ FL FR FC LFE RL RR SL SR ]")));
    QVERIFY(text.contains(QStringLiteral("spatial_RL")));
    QVERIFY(text.contains(QStringLiteral("spatial_SR")));
}

void ProfileGeneratorTest::generatesBassHeavyEqBeforeLimiter()
{
    const auto text = ProfileGenerator::generate(
        ProfileGenerator::profile(QStringLiteral("reference-7.1")),
        QStringLiteral("/usr/share/libmysofa/default.sofa"),
        QStringLiteral("a50.chat"));
    QVERIFY(text.contains(QStringLiteral("label = param_eq name = eqL")));
    QVERIFY(text.contains(QStringLiteral("type = bq_lowshelf freq = 110.0 gain = 7.5")));
    QVERIFY(text.contains(QStringLiteral("type = bq_highshelf freq = 8000.0 gain = -2.0")));
    QVERIFY(text.contains(QStringLiteral("output = \"eqL:Out 1\" input = \"safety_limiter:in_l\"")));
}

void ProfileGeneratorTest::canDisableEq()
{
    const auto text = ProfileGenerator::generate(
        ProfileGenerator::profile(QStringLiteral("reference-5.1")),
        QStringLiteral("/usr/share/libmysofa/default.sofa"),
        QStringLiteral("a50.chat"), true, QStringLiteral("flat"));
    QVERIFY(!text.contains(QStringLiteral("label = param_eq")));
    QVERIFY(text.contains(QStringLiteral("output = \"mixL:Out\" input = \"safety_limiter:in_l\"")));
}

void ProfileGeneratorTest::escapesPathsAndTargets()
{
    const auto text = ProfileGenerator::generate(
        ProfileGenerator::profile(QStringLiteral("reference-5.1")),
        QStringLiteral("/tmp/a \"quoted\".sofa"),
        QStringLiteral("node.\"odd\""), false);
    QVERIFY(text.contains(QStringLiteral("/tmp/a \\\"quoted\\\".sofa")));
    QVERIFY(text.contains(QStringLiteral("node.\\\"odd\\\"")));
    QVERIFY(!text.contains(QStringLiteral("safety_limiter")));
}

void ProfileGeneratorTest::rejectsMissingSofa()
{
    QString error;
    QVERIFY(!ProfileGenerator::validateInputs(
        ProfileGenerator::profile(QStringLiteral("reference-7.1")),
        QStringLiteral("/definitely/missing.sofa"),
        QStringLiteral("target"), &error));
    QVERIFY(error.contains(QStringLiteral("not found")));
}

void ProfileGeneratorTest::identityS16ConversionIsExact()
{
    for (int sample = -32768; sample <= 32767; ++sample) {
        const float normalized = static_cast<float>(sample) / 32768.0f;
        const auto restored = static_cast<qint16>(qRound(normalized * 32768.0f));
        QCOMPARE(restored, static_cast<qint16>(sample));
    }
}

QTEST_MAIN(ProfileGeneratorTest)
#include "profilegenerator_test.moc"
