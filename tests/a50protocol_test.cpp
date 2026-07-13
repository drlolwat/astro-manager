#include "a50protocol.h"

#include <QtTest>

using namespace AstroSpatial;

class A50ProtocolTest : public QObject
{
    Q_OBJECT

private slots:
    void encodesRequests();
    void decodesResponses();
    void rejectsBadResponses();
    void validatesHardwareRanges();
};

void A50ProtocolTest::encodesRequests()
{
    QCOMPARE(A50Protocol::encodeRequest(A50Command::GetDeviceInfo),
             QByteArray::fromHex("0203"));
    QCOMPARE(A50Protocol::encodeRequest(A50Command::GetSliderValue, QByteArray(1, char(4))),
             QByteArray::fromHex("02680104"));
    QVERIFY(A50Protocol::encodeRequest(A50Command::SetEqPresetName, QByteArray(62, 'x')).isEmpty());
}

void A50ProtocolTest::decodesResponses()
{
    QString error;
    auto decoded = A50Protocol::decodeResponse(QByteArray::fromHex("02020468042a2a"), &error);
    QVERIFY(decoded);
    QCOMPARE(*decoded, QByteArray::fromHex("68042a2a"));
    QVERIFY(error.isEmpty());

    decoded = A50Protocol::decodeResponse(QByteArray::fromHex("02000a0102"), &error);
    QVERIFY(decoded);
    QCOMPARE(*decoded, QByteArray::fromHex("0102"));
}

void A50ProtocolTest::rejectsBadResponses()
{
    QString error;
    QVERIFY(!A50Protocol::decodeResponse(QByteArray::fromHex("02"), &error));
    QVERIFY(error.contains(QStringLiteral("truncated")));
    QVERIFY(!A50Protocol::decodeResponse(QByteArray::fromHex("030200"), &error));
    QVERIFY(!A50Protocol::decodeResponse(QByteArray::fromHex("020100"), &error));
    QVERIFY(!A50Protocol::decodeResponse(QByteArray::fromHex("020900"), &error));
}

void A50ProtocolTest::validatesHardwareRanges()
{
    QVERIFY(A50Protocol::validGain(-7));
    QVERIFY(A50Protocol::validGain(7));
    QVERIFY(!A50Protocol::validGain(8));
    QVERIFY(A50Protocol::validFrequency(80));
    QVERIFY(A50Protocol::validFrequency(15000));
    QVERIFY(!A50Protocol::validFrequency(79));
    QVERIFY(A50Protocol::validBandwidth(1, 0));
    QVERIFY(!A50Protocol::validBandwidth(1, 4096));
    QVERIFY(A50Protocol::validBandwidth(3, 409));
    QVERIFY(A50Protocol::validBandwidth(3, 12288));
}

QTEST_MAIN(A50ProtocolTest)
#include "a50protocol_test.moc"
