#pragma once

#include <QByteArray>
#include <QString>

#include <array>
#include <optional>

namespace AstroSpatial {

enum class A50Command : quint8 {
    GetDeviceInfo = 0x03,
    GetHeadsetStatus = 0x54,
    GetBaseFirmwareMinor = 0x55,
    SaveValues = 0x61,
    SetSliderValue = 0x62,
    SetEqPresetGain = 0x63,
    SetNoiseGateMode = 0x64,
    SetActiveEqPreset = 0x67,
    GetSliderValue = 0x68,
    GetEqPresetGain = 0x69,
    GetNoiseGateMode = 0x6a,
    GetActiveEqPreset = 0x6c,
    SetEqPresetName = 0x6d,
    GetEqPresetName = 0x6e,
    SetEqPresetFrequency = 0x6f,
    GetEqPresetFrequency = 0x70,
    SetMicrophoneEq = 0x71,
    GetBalance = 0x72,
    SetDefaultBalance = 0x73,
    SetAlertVolume = 0x76,
    GetDefaultBalance = 0x77,
    GetAlertVolume = 0x7a,
    GetMicrophoneEq = 0x7b,
    GetBatteryStatus = 0x7c,
    GetHeadsetFirmwareMinor = 0xd6,
    GetHeadsetFirmwareMajor = 0xda,
};

enum class A50Slider : quint8 {
    StreamMic = 0,
    StreamChat = 1,
    StreamGame = 2,
    StreamAux = 3,
    Microphone = 4,
    Sidetone = 5,
};

enum class NoiseGateMode : quint8 {
    Streaming = 0,
    Night = 1,
    Home = 2,
    Tournament = 3,
};

struct A50EqBand {
    int gainDb = 0;
    int savedGainDb = 0;
    int frequencyHz = 1000;
    int savedFrequencyHz = 1000;
    int bandwidth = 4096;
    int savedBandwidth = 4096;
};

struct A50EqPreset {
    int slot = 1;
    QString name;
    QString savedName;
    std::array<A50EqBand, 5> bands;
};

struct A50Settings {
    int activeEqPreset = 1;
    std::array<A50EqPreset, 3> eqPresets;
    int microphoneEq = 0;
    int savedMicrophoneEq = 0;
    int microphoneLevel = 0;
    int savedMicrophoneLevel = 0;
    int sidetone = 0;
    int savedSidetone = 0;
    int streamMic = 0;
    int savedStreamMic = 0;
    int streamChat = 0;
    int savedStreamChat = 0;
    int streamGame = 0;
    int savedStreamGame = 0;
    int streamAux = 0;
    int savedStreamAux = 0;
    NoiseGateMode noiseGate = NoiseGateMode::Home;
    NoiseGateMode savedNoiseGate = NoiseGateMode::Home;
    int alertVolume = 0;
    int savedAlertVolume = 0;
    int balance = 128;
    int defaultBalance = 128;
    int savedDefaultBalance = 128;
};

struct A50Snapshot {
    bool connected = false;
    bool headsetOn = false;
    bool docked = false;
    bool charging = false;
    int batteryPercent = -1;
    QString baseFirmware;
    QString headsetFirmware;
    A50Settings settings;
};

class A50Protocol
{
public:
    static constexpr quint16 VendorId = 0x9886;
    static constexpr quint16 ProductId = 0x002c;
    static constexpr int Interface = 6;
    static constexpr quint8 EndpointIn = 0x85;
    static constexpr quint8 EndpointOut = 0x05;
    static constexpr int PacketSize = 64;

    static QByteArray encodeRequest(A50Command command, const QByteArray &payload = {});
    static std::optional<QByteArray> decodeResponse(const QByteArray &packet, QString *error);

    static bool validPreset(int preset) { return preset >= 1 && preset <= 3; }
    static bool validBand(int band) { return band >= 1 && band <= 5; }
    static bool validPercent(int value) { return value >= 0 && value <= 100; }
    static bool validGain(int value) { return value >= -7 && value <= 7; }
    static bool validFrequency(int value) { return value >= 80 && value <= 15000; }
    static bool validBandwidth(int band, int value)
    {
        return (band == 1 || band == 5) ? value == 0 : value >= 409 && value <= 12288;
    }
};

} // namespace AstroSpatial
