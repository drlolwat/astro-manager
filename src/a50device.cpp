#include "a50device.h"

#include <libusb-1.0/libusb.h>

#include <QScopeGuard>

namespace AstroSpatial {
namespace {

quint16 readLe16(const QByteArray &data, int offset)
{
    return quint8(data[offset]) | (quint16(quint8(data[offset + 1])) << 8);
}

quint32 readLe32(const QByteArray &data, int offset)
{
    return quint8(data[offset]) | (quint32(quint8(data[offset + 1])) << 8)
        | (quint32(quint8(data[offset + 2])) << 16)
        | (quint32(quint8(data[offset + 3])) << 24);
}

void appendLe16(QByteArray &data, int value)
{
    data.append(char(value & 0xff));
    data.append(char((value >> 8) & 0xff));
}

QString usbError(const QString &operation, int code)
{
    return QStringLiteral("%1: %2").arg(operation, QString::fromLatin1(libusb_error_name(code)));
}

bool beginsWith(const QByteArray &data, std::initializer_list<int> bytes)
{
    if (data.size() < qsizetype(bytes.size()))
        return false;
    int index = 0;
    for (const int byte : bytes) {
        if (quint8(data[index++]) != byte)
            return false;
    }
    return true;
}

} // namespace

A50Device::A50Device() = default;

A50Device::~A50Device()
{
    close();
}

bool A50Device::open(QString *error)
{
    if (isOpen())
        return true;

    int result = libusb_init(&m_context);
    if (result != LIBUSB_SUCCESS) {
        if (error) *error = usbError(QStringLiteral("Could not initialize libusb"), result);
        return false;
    }

    m_handle = libusb_open_device_with_vid_pid(m_context, A50Protocol::VendorId,
                                                A50Protocol::ProductId);
    if (!m_handle) {
        if (error) {
            *error = QStringLiteral("A50 Gen 4 control interface unavailable. Connect the dock in PC mode or check USB permissions.");
        }
        close();
        return false;
    }

    libusb_set_auto_detach_kernel_driver(m_handle, 1);
    result = libusb_claim_interface(m_handle, A50Protocol::Interface);
    if (result != LIBUSB_SUCCESS) {
        if (error) {
            if (result == LIBUSB_ERROR_ACCESS)
                *error = QStringLiteral("Permission denied opening A50 controls. Install the Astro Manager udev rule, then reconnect the dock.");
            else if (result == LIBUSB_ERROR_BUSY)
                *error = QStringLiteral("Another application currently owns the A50 control interface.");
            else
                *error = usbError(QStringLiteral("Could not claim A50 interface 6"), result);
        }
        close();
        return false;
    }
    m_claimed = true;

    const auto info = request(A50Command::GetDeviceInfo, {}, 1000, error);
    if (!info || info->size() < 8
        || readLe16(*info, 4) != A50Protocol::VendorId
        || readLe16(*info, 6) != A50Protocol::ProductId) {
        if (error && (info && info->size() >= 8))
            *error = QStringLiteral("The USB control device did not identify as a supported A50 Gen 4 dock.");
        close();
        return false;
    }
    return true;
}

void A50Device::close()
{
    if (m_handle && m_claimed)
        libusb_release_interface(m_handle, A50Protocol::Interface);
    m_claimed = false;
    if (m_handle)
        libusb_close(m_handle);
    m_handle = nullptr;
    if (m_context)
        libusb_exit(m_context);
    m_context = nullptr;
}

std::optional<QByteArray> A50Device::request(A50Command command, const QByteArray &payload,
                                             int timeoutMs, QString *error)
{
    if (!m_handle) {
        if (error) *error = QStringLiteral("The A50 control interface is not connected.");
        return std::nullopt;
    }

    QByteArray outgoing = A50Protocol::encodeRequest(command, payload);
    if (outgoing.isEmpty()) {
        if (error) *error = QStringLiteral("The A50 request payload is too large.");
        return std::nullopt;
    }

    int transferred = 0;
    int result = libusb_interrupt_transfer(
        m_handle, A50Protocol::EndpointOut,
        reinterpret_cast<unsigned char *>(outgoing.data()), outgoing.size(), &transferred, timeoutMs);
    if (result != LIBUSB_SUCCESS || transferred != outgoing.size()) {
        if (error) *error = usbError(QStringLiteral("A50 USB write failed"), result);
        return std::nullopt;
    }

    QByteArray incoming(A50Protocol::PacketSize, Qt::Uninitialized);
    transferred = 0;
    result = libusb_interrupt_transfer(
        m_handle, A50Protocol::EndpointIn,
        reinterpret_cast<unsigned char *>(incoming.data()), incoming.size(), &transferred, timeoutMs);
    if (result != LIBUSB_SUCCESS) {
        if (error) *error = usbError(QStringLiteral("A50 USB response failed"), result);
        return std::nullopt;
    }
    incoming.resize(transferred);
    return A50Protocol::decodeResponse(incoming, error);
}

bool A50Device::readDynamic(A50Snapshot *snapshot, QString *error)
{
    const auto status = request(A50Command::GetHeadsetStatus, {}, 1000, error);
    if (!status || status->isEmpty())
        return false;
    snapshot->docked = quint8((*status)[0]) & 0x01;
    snapshot->headsetOn = quint8((*status)[0]) & 0x02;

    const auto battery = request(A50Command::GetBatteryStatus, {}, 1000, error);
    if (!battery || battery->isEmpty())
        return false;
    snapshot->charging = quint8((*battery)[0]) & 0x80;
    snapshot->batteryPercent = quint8((*battery)[0]) & 0x7f;

    const auto balance = request(A50Command::GetBalance, {}, 1000, error);
    if (!balance || balance->isEmpty())
        return false;
    snapshot->settings.balance = quint8((*balance)[0]);

    const auto activeEq = request(A50Command::GetActiveEqPreset, {}, 1000, error);
    if (!activeEq || activeEq->isEmpty() || !A50Protocol::validPreset(quint8((*activeEq)[0])))
        return false;
    snapshot->settings.activeEqPreset = quint8((*activeEq)[0]);
    snapshot->connected = true;
    return true;
}

bool A50Device::readSlider(A50Slider slider, int *active, int *saved, QString *error)
{
    QByteArray payload(1, char(slider));
    const auto data = request(A50Command::GetSliderValue, payload, 1000, error);
    if (!data || data->size() < 4
        || !beginsWith(*data, {int(A50Command::GetSliderValue), int(slider)})) {
        if (error && data) *error = QStringLiteral("Invalid A50 slider response.");
        return false;
    }
    *active = quint8((*data)[2]);
    *saved = quint8((*data)[3]);
    if (!A50Protocol::validPercent(*active) || !A50Protocol::validPercent(*saved)) {
        if (error) *error = QStringLiteral("The A50 returned an out-of-range slider value.");
        return false;
    }
    return true;
}

bool A50Device::readEqPreset(int preset, A50EqPreset *result, QString *error)
{
    result->slot = preset;
    for (const bool saved : {false, true}) {
        QByteArray payload;
        payload.append(char(preset));
        payload.append(char(saved));
        const auto data = request(A50Command::GetEqPresetName, payload, 1000, error);
        if (!data || data->size() < 3
            || !beginsWith(*data, {int(A50Command::GetEqPresetName), preset})) {
            if (error && data) *error = QStringLiteral("Invalid A50 EQ-name response.");
            return false;
        }
        QByteArray name = data->mid(2);
        const int terminator = name.indexOf('\0');
        if (terminator >= 0)
            name.truncate(terminator);
        if (saved)
            result->savedName = QString::fromUtf8(name);
        else
            result->name = QString::fromUtf8(name);
    }

    const auto gain = request(A50Command::GetEqPresetGain, QByteArray(1, char(preset)), 1000, error);
    if (!gain || gain->size() < 12
        || !beginsWith(*gain, {int(A50Command::GetEqPresetGain), preset})) {
        if (error && gain) *error = QStringLiteral("Invalid A50 EQ-gain response.");
        return false;
    }
    for (int band = 0; band < 5; ++band) {
        result->bands[band].gainDb = quint8((*gain)[2 + band]) - 12;
        result->bands[band].savedGainDb = quint8((*gain)[7 + band]) - 12;
        if (!A50Protocol::validGain(result->bands[band].gainDb)
            || !A50Protocol::validGain(result->bands[band].savedGainDb)) {
            if (error) *error = QStringLiteral("The A50 returned an out-of-range EQ gain.");
            return false;
        }
    }

    for (int band = 1; band <= 5; ++band) {
        QByteArray payload;
        payload.append(char(preset));
        payload.append(char(band));
        const auto data = request(A50Command::GetEqPresetFrequency, payload, 1000, error);
        if (!data || data->size() < 11
            || !beginsWith(*data, {int(A50Command::GetEqPresetFrequency), preset, band})) {
            if (error && data) *error = QStringLiteral("Invalid A50 EQ-frequency response.");
            return false;
        }
        auto &target = result->bands[band - 1];
        target.bandwidth = readLe16(*data, 3);
        target.savedBandwidth = readLe16(*data, 5);
        target.frequencyHz = readLe16(*data, 7);
        target.savedFrequencyHz = readLe16(*data, 9);
        if (!A50Protocol::validFrequency(target.frequencyHz)
            || !A50Protocol::validFrequency(target.savedFrequencyHz)
            || !A50Protocol::validBandwidth(band, target.bandwidth)
            || !A50Protocol::validBandwidth(band, target.savedBandwidth)) {
            if (error) *error = QStringLiteral("The A50 returned invalid EQ frequency data.");
            return false;
        }
    }
    return true;
}

bool A50Device::readSettings(A50Settings *settings, QString *error)
{
    const auto active = request(A50Command::GetActiveEqPreset, {}, 1000, error);
    if (!active || active->isEmpty() || !A50Protocol::validPreset(quint8((*active)[0])))
        return false;
    settings->activeEqPreset = quint8((*active)[0]);

    for (int preset = 1; preset <= 3; ++preset) {
        if (!readEqPreset(preset, &settings->eqPresets[preset - 1], error))
            return false;
    }

    if (!readSlider(A50Slider::Microphone, &settings->microphoneLevel,
                    &settings->savedMicrophoneLevel, error)
        || !readSlider(A50Slider::Sidetone, &settings->sidetone, &settings->savedSidetone, error)
        || !readSlider(A50Slider::StreamMic, &settings->streamMic, &settings->savedStreamMic, error)
        || !readSlider(A50Slider::StreamChat, &settings->streamChat, &settings->savedStreamChat, error)
        || !readSlider(A50Slider::StreamGame, &settings->streamGame, &settings->savedStreamGame, error)
        || !readSlider(A50Slider::StreamAux, &settings->streamAux, &settings->savedStreamAux, error))
        return false;

    const auto gate = request(A50Command::GetNoiseGateMode, {}, 1000, error);
    if (!gate || gate->size() < 3 || quint8((*gate)[0]) != quint8(A50Command::GetNoiseGateMode))
        return false;
    if (quint8((*gate)[1]) > 3 || quint8((*gate)[2]) > 3) {
        if (error) *error = QStringLiteral("The A50 returned an unknown noise-gate mode.");
        return false;
    }
    settings->noiseGate = NoiseGateMode(quint8((*gate)[1]));
    settings->savedNoiseGate = NoiseGateMode(quint8((*gate)[2]));

    for (const bool saved : {false, true}) {
        const QByteArray flag(1, char(saved));
        const auto micEq = request(A50Command::GetMicrophoneEq, flag, 1000, error);
        const auto alert = request(A50Command::GetAlertVolume, flag, 1000, error);
        const auto defaultBalance = request(A50Command::GetDefaultBalance, flag, 1000, error);
        if (!micEq || micEq->isEmpty() || !alert || alert->isEmpty()
            || !defaultBalance || defaultBalance->isEmpty())
            return false;
        if (quint8((*micEq)[0]) > 2 || !A50Protocol::validPercent(quint8((*alert)[0]))) {
            if (error) *error = QStringLiteral("The A50 returned an out-of-range device setting.");
            return false;
        }
        if (saved) {
            settings->savedMicrophoneEq = quint8((*micEq)[0]);
            settings->savedAlertVolume = quint8((*alert)[0]);
            settings->savedDefaultBalance = quint8((*defaultBalance)[0]);
        } else {
            settings->microphoneEq = quint8((*micEq)[0]);
            settings->alertVolume = quint8((*alert)[0]);
            settings->defaultBalance = quint8((*defaultBalance)[0]);
        }
    }
    return true;
}

bool A50Device::readSnapshot(A50Snapshot *snapshot, QString *error)
{
    if (!readDynamic(snapshot, error))
        return false;

    const auto info = request(A50Command::GetDeviceInfo, {}, 1000, error);
    const auto baseMinor = request(A50Command::GetBaseFirmwareMinor, {}, 1000, error);
    if (!info || info->size() < 25 || !baseMinor || baseMinor->isEmpty())
        return false;
    snapshot->baseFirmware = QStringLiteral("%1.%2")
        .arg(readLe32(*info, 21)).arg(quint8((*baseMinor)[0]));

    const auto headsetMajor = request(A50Command::GetHeadsetFirmwareMajor, QByteArray(1, char(0x0a)), 1000, error);
    const auto headsetMinor = request(A50Command::GetHeadsetFirmwareMinor, QByteArray(1, char(0x0a)), 1000, error);
    if (headsetMajor && headsetMajor->size() >= 4 && headsetMinor && !headsetMinor->isEmpty()) {
        snapshot->headsetFirmware = QStringLiteral("%1.%2")
            .arg(readLe32(*headsetMajor, 0)).arg(quint8((*headsetMinor)[0]));
    } else {
        snapshot->headsetFirmware = QStringLiteral("Unavailable");
    }

    return readSettings(&snapshot->settings, error);
}

bool A50Device::setActiveEqPreset(int preset, QString *error)
{
    if (!A50Protocol::validPreset(preset)) {
        if (error) *error = QStringLiteral("EQ slot must be between 1 and 3.");
        return false;
    }
    const auto data = request(A50Command::SetActiveEqPreset, QByteArray(1, char(preset)), 1000, error);
    return data && beginsWith(*data, {int(A50Command::SetActiveEqPreset), preset});
}

bool A50Device::setEqPresetName(int preset, const QString &name, QString *error)
{
    const QByteArray encoded = name.toUtf8();
    if (!A50Protocol::validPreset(preset) || encoded.isEmpty() || encoded.size() > 56 || encoded.contains('\0')) {
        if (error) *error = QStringLiteral("EQ names must contain 1 to 56 UTF-8 bytes.");
        return false;
    }
    QByteArray payload;
    payload.append(char(preset));
    payload.append(char(encoded.size() + 1));
    payload.append(encoded);
    payload.append('\0');
    const auto data = request(A50Command::SetEqPresetName, payload, 1000, error);
    return data && beginsWith(*data, {int(A50Command::SetEqPresetName), preset});
}

bool A50Device::setEqPresetGains(int preset, const std::array<int, 5> &gains, QString *error)
{
    if (!A50Protocol::validPreset(preset))
        return false;
    QByteArray payload(1, char(preset));
    for (const int gain : gains) {
        if (!A50Protocol::validGain(gain)) {
            if (error) *error = QStringLiteral("Hardware EQ gain must be between -7 and +7 dB.");
            return false;
        }
        payload.append(char(gain + 12));
    }
    const auto data = request(A50Command::SetEqPresetGain, payload, 1000, error);
    return data && beginsWith(*data, {int(A50Command::SetEqPresetGain), preset});
}

bool A50Device::setEqBand(int preset, int band, int frequency, int bandwidth, QString *error)
{
    if (!A50Protocol::validPreset(preset) || !A50Protocol::validBand(band)
        || !A50Protocol::validFrequency(frequency) || !A50Protocol::validBandwidth(band, bandwidth)) {
        if (error) *error = QStringLiteral("Invalid hardware EQ frequency or bandwidth.");
        return false;
    }
    QByteArray payload;
    payload.append(char(preset));
    payload.append(char(band));
    appendLe16(payload, bandwidth);
    appendLe16(payload, frequency);
    const auto data = request(A50Command::SetEqPresetFrequency, payload, 1000, error);
    return data && beginsWith(*data, {int(A50Command::SetEqPresetFrequency), preset, band, 0});
}

bool A50Device::setSlider(A50Slider slider, int value, QString *error)
{
    if (!A50Protocol::validPercent(value)) {
        if (error) *error = QStringLiteral("A50 levels must be between 0 and 100 percent.");
        return false;
    }
    QByteArray payload;
    payload.append(char(slider));
    payload.append(char(value));
    const auto data = request(A50Command::SetSliderValue, payload, 1000, error);
    return data && beginsWith(*data, {int(A50Command::SetSliderValue), int(slider)});
}

bool A50Device::setNoiseGate(NoiseGateMode mode, QString *error)
{
    const int value = int(mode);
    if (value < 0 || value > 3)
        return false;
    const auto data = request(A50Command::SetNoiseGateMode, QByteArray(1, char(value)), 1000, error);
    return data && !data->isEmpty() && quint8((*data)[0]) == value;
}

bool A50Device::setMicrophoneEq(int preset, QString *error)
{
    if (preset < 0 || preset > 2)
        return false;
    const auto data = request(A50Command::SetMicrophoneEq, QByteArray(1, char(preset)), 1000, error);
    return data && !data->isEmpty() && quint8((*data)[0]) == quint8(A50Command::SetMicrophoneEq);
}

bool A50Device::setAlertVolume(int value, QString *error)
{
    if (!A50Protocol::validPercent(value))
        return false;
    const auto data = request(A50Command::SetAlertVolume, QByteArray(1, char(value)), 1000, error);
    return data && !data->isEmpty() && quint8((*data)[0]) == quint8(A50Command::SetAlertVolume);
}

bool A50Device::setDefaultBalance(int value, QString *error)
{
    if (value < 0 || value > 255)
        return false;
    const auto data = request(A50Command::SetDefaultBalance, QByteArray(1, char(value)), 1000, error);
    return data && !data->isEmpty() && quint8((*data)[0]) == quint8(A50Command::SetDefaultBalance);
}

bool A50Device::save(QString *error)
{
    const auto data = request(A50Command::SaveValues, {}, 5000, error);
    return data && beginsWith(*data, {int(A50Command::SaveValues), 0});
}

} // namespace AstroSpatial
