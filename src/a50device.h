#pragma once

#include "a50protocol.h"

#include <QByteArray>

struct libusb_context;
struct libusb_device_handle;

namespace AstroSpatial {

class A50Device
{
public:
    A50Device();
    ~A50Device();

    A50Device(const A50Device &) = delete;
    A50Device &operator=(const A50Device &) = delete;

    bool open(QString *error);
    void close();
    bool isOpen() const { return m_handle != nullptr; }

    bool readSnapshot(A50Snapshot *snapshot, QString *error);
    bool readDynamic(A50Snapshot *snapshot, QString *error);
    bool readSettings(A50Settings *settings, QString *error);

    bool setActiveEqPreset(int preset, QString *error);
    bool setEqPresetName(int preset, const QString &name, QString *error);
    bool setEqPresetGains(int preset, const std::array<int, 5> &gains, QString *error);
    bool setEqBand(int preset, int band, int frequency, int bandwidth, QString *error);
    bool setSlider(A50Slider slider, int value, QString *error);
    bool setNoiseGate(NoiseGateMode mode, QString *error);
    bool setMicrophoneEq(int preset, QString *error);
    bool setAlertVolume(int value, QString *error);
    bool setDefaultBalance(int value, QString *error);
    bool save(QString *error);

private:
    std::optional<QByteArray> request(A50Command command, const QByteArray &payload,
                                      int timeoutMs, QString *error);
    bool readEqPreset(int preset, A50EqPreset *result, QString *error);
    bool readSlider(A50Slider slider, int *active, int *saved, QString *error);

    libusb_context *m_context = nullptr;
    libusb_device_handle *m_handle = nullptr;
    bool m_claimed = false;
};

} // namespace AstroSpatial
