#include "a50manager.h"

#include <QTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

#include <cmath>

namespace AstroSpatial {
namespace {

QString recoveryPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation)
        + QStringLiteral("/astro-manager/runtime-eq.json");
}

QVariantMap bandMap(const A50EqBand &band, int number)
{
    return {
        {QStringLiteral("band"), number},
        {QStringLiteral("gain"), band.gainDb},
        {QStringLiteral("savedGain"), band.savedGainDb},
        {QStringLiteral("frequency"), band.frequencyHz},
        {QStringLiteral("savedFrequency"), band.savedFrequencyHz},
        {QStringLiteral("bandwidth"), band.bandwidth / 4096.0},
        {QStringLiteral("savedBandwidth"), band.savedBandwidth / 4096.0},
    };
}

QVariantMap snapshotMap(const A50Snapshot &snapshot, const A50Settings &desired,
                        bool neutralized)
{
    QVariantList presets;
    bool dirty = false;
    for (const auto &preset : desired.eqPresets) {
        QVariantList bands;
        if (preset.name != preset.savedName)
            dirty = true;
        for (int index = 0; index < 5; ++index) {
            const auto &band = preset.bands[index];
            bands.append(bandMap(band, index + 1));
            dirty = dirty || band.gainDb != band.savedGainDb
                || band.frequencyHz != band.savedFrequencyHz
                || band.bandwidth != band.savedBandwidth;
        }
        presets.append(QVariantMap{
            {QStringLiteral("slot"), preset.slot},
            {QStringLiteral("name"), preset.name},
            {QStringLiteral("savedName"), preset.savedName},
            {QStringLiteral("bands"), bands},
        });
    }
    dirty = dirty
        || desired.microphoneEq != desired.savedMicrophoneEq
        || desired.microphoneLevel != desired.savedMicrophoneLevel
        || desired.sidetone != desired.savedSidetone
        || desired.streamMic != desired.savedStreamMic
        || desired.streamChat != desired.savedStreamChat
        || desired.streamGame != desired.savedStreamGame
        || desired.streamAux != desired.savedStreamAux
        || desired.noiseGate != desired.savedNoiseGate
        || desired.alertVolume != desired.savedAlertVolume
        || desired.defaultBalance != desired.savedDefaultBalance;

    return {
        {QStringLiteral("connected"), snapshot.connected},
        {QStringLiteral("headsetOn"), snapshot.headsetOn},
        {QStringLiteral("docked"), snapshot.docked},
        {QStringLiteral("charging"), snapshot.charging},
        {QStringLiteral("battery"), snapshot.batteryPercent},
        {QStringLiteral("baseFirmware"), snapshot.baseFirmware},
        {QStringLiteral("headsetFirmware"), snapshot.headsetFirmware},
        {QStringLiteral("activeEqPreset"), desired.activeEqPreset},
        {QStringLiteral("eqPresets"), presets},
        {QStringLiteral("microphoneEq"), desired.microphoneEq},
        {QStringLiteral("savedMicrophoneEq"), desired.savedMicrophoneEq},
        {QStringLiteral("microphoneLevel"), desired.microphoneLevel},
        {QStringLiteral("savedMicrophoneLevel"), desired.savedMicrophoneLevel},
        {QStringLiteral("sidetone"), desired.sidetone},
        {QStringLiteral("savedSidetone"), desired.savedSidetone},
        {QStringLiteral("streamMic"), desired.streamMic},
        {QStringLiteral("savedStreamMic"), desired.savedStreamMic},
        {QStringLiteral("streamChat"), desired.streamChat},
        {QStringLiteral("savedStreamChat"), desired.savedStreamChat},
        {QStringLiteral("streamGame"), desired.streamGame},
        {QStringLiteral("savedStreamGame"), desired.savedStreamGame},
        {QStringLiteral("streamAux"), desired.streamAux},
        {QStringLiteral("savedStreamAux"), desired.savedStreamAux},
        {QStringLiteral("noiseGate"), int(desired.noiseGate)},
        {QStringLiteral("savedNoiseGate"), int(desired.savedNoiseGate)},
        {QStringLiteral("alertVolume"), desired.alertVolume},
        {QStringLiteral("savedAlertVolume"), desired.savedAlertVolume},
        {QStringLiteral("balance"), desired.balance},
        {QStringLiteral("defaultBalance"), desired.defaultBalance},
        {QStringLiteral("savedDefaultBalance"), desired.savedDefaultBalance},
        {QStringLiteral("dirty"), dirty},
        {QStringLiteral("neutralized"), neutralized},
    };
}

void copySavedValues(A50Settings &target, const A50Settings &source)
{
    for (int preset = 0; preset < 3; ++preset) {
        target.eqPresets[preset].savedName = source.eqPresets[preset].savedName;
        for (int band = 0; band < 5; ++band) {
            target.eqPresets[preset].bands[band].savedGainDb = source.eqPresets[preset].bands[band].savedGainDb;
            target.eqPresets[preset].bands[band].savedFrequencyHz = source.eqPresets[preset].bands[band].savedFrequencyHz;
            target.eqPresets[preset].bands[band].savedBandwidth = source.eqPresets[preset].bands[band].savedBandwidth;
        }
    }
    target.savedMicrophoneEq = source.savedMicrophoneEq;
    target.savedMicrophoneLevel = source.savedMicrophoneLevel;
    target.savedSidetone = source.savedSidetone;
    target.savedStreamMic = source.savedStreamMic;
    target.savedStreamChat = source.savedStreamChat;
    target.savedStreamGame = source.savedStreamGame;
    target.savedStreamAux = source.savedStreamAux;
    target.savedNoiseGate = source.savedNoiseGate;
    target.savedAlertVolume = source.savedAlertVolume;
    target.savedDefaultBalance = source.savedDefaultBalance;
}

} // namespace

class A50Worker : public QObject
{
    Q_OBJECT

public slots:
    void start()
    {
        m_timer = new QTimer(this);
        m_timer->setInterval(2000);
        connect(m_timer, &QTimer::timeout, this, &A50Worker::poll);
        m_timer->start();
        refresh(true);
    }

    void stop()
    {
        if (m_neutralized)
            restoreDesiredOutput();
        m_device.close();
    }

    void refresh(bool forceFull = true)
    {
        setBusy(true);
        QString error;
        if (!m_device.isOpen() && !m_device.open(&error)) {
            disconnected(error);
            return;
        }

        A50Snapshot current = m_snapshot;
        bool ok = forceFull ? m_device.readSnapshot(&current, &error)
                            : m_device.readDynamic(&current, &error);
        if (!ok) {
            // The Gen 4 dock occasionally returns status 1 during the handoff
            // from docked to wireless mode. Reclaiming interface 6 and retrying
            // once keeps that transient from becoming a red disconnect loop.
            m_device.close();
            QThread::msleep(100);
            QString retryError;
            if (m_device.open(&retryError)) {
                current = m_snapshot;
                ok = forceFull ? m_device.readSnapshot(&current, &retryError)
                               : m_device.readDynamic(&current, &retryError);
            }
            if (!ok) {
                // Keep the last good snapshot through the short interval where
                // the base switches between cradle and wireless control. Only
                // call it disconnected after three consecutive poll failures.
                ++m_readFailures;
                if (m_readFailures >= 3)
                    disconnected(retryError.isEmpty() ? error : retryError);
                else {
                    emit errorChanged({});
                    if (m_snapshot.connected)
                        emitState();
                    setBusy(false);
                }
                return;
            }
        }
        m_readFailures = 0;
        m_snapshot = current;
        if (!m_hasDesired) {
            m_desired = current.settings;
            m_hasDesired = true;
            loadRecovery();
        } else if (forceFull) {
            copySavedValues(m_desired, current.settings);
            m_desired.balance = current.settings.balance;
            if (!m_neutralized)
                m_desired = current.settings;
            else if (current.settings.activeEqPreset != m_desired.activeEqPreset) {
                m_desired.activeEqPreset = current.settings.activeEqPreset;
                applyFlatOutput();
            }
        } else {
            m_desired.balance = current.settings.balance;
            if (current.settings.activeEqPreset != m_desired.activeEqPreset) {
                m_desired.activeEqPreset = current.settings.activeEqPreset;
                if (m_neutralized)
                    applyFlatOutput();
            }
        }
        if (m_spatialActive && !m_auditioning && !m_neutralized)
            applyFlatOutput();
        else if ((!m_spatialActive || m_auditioning) && m_neutralized)
            restoreDesiredOutput();
        m_error.clear();
        emit errorChanged({});
        emitState();
        setBusy(false);
    }

    void poll()
    {
        if (m_busy)
            return;
        ++m_pollCount;
        // A reclaimed interface only needs a quick dynamic probe when we
        // already have a full snapshot. The periodic full refresh still runs
        // every ten seconds and initial/reconnected discovery remains full.
        refresh(!m_hasDesired || m_pollCount % 5 == 0);
    }

    void setSpatialActive(bool active)
    {
        m_spatialActive = active;
        if (!m_device.isOpen() || !m_hasDesired)
            return;
        if (active && !m_auditioning)
            applyFlatOutput();
        else
            restoreDesiredOutput();
        emitState();
    }

    void auditionHardwareEq(bool enabled)
    {
        m_auditioning = enabled;
        setSpatialActive(m_spatialActive);
    }

    void setActiveEqPreset(int preset)
    {
        runWrite([&](QString *error) {
            if (!m_device.setActiveEqPreset(preset, error))
                return false;
            m_desired.activeEqPreset = preset;
            return !m_neutralized || applyFlatOutput(error);
        });
    }

    void setEqPresetName(int preset, const QString &name)
    {
        runWrite([&](QString *error) {
            if (!m_device.setEqPresetName(preset, name, error))
                return false;
            m_desired.eqPresets[preset - 1].name = name;
            return true;
        });
    }

    void setEqGain(int preset, int band, int gain)
    {
        runWrite([&](QString *error) {
            if (!A50Protocol::validPreset(preset) || !A50Protocol::validBand(band)
                || !A50Protocol::validGain(gain)) {
                *error = QStringLiteral("Invalid hardware EQ gain.");
                return false;
            }
            m_desired.eqPresets[preset - 1].bands[band - 1].gainDb = gain;
            std::array<int, 5> gains;
            for (int index = 0; index < 5; ++index)
                gains[index] = m_neutralized ? 0 : m_desired.eqPresets[preset - 1].bands[index].gainDb;
            if (!m_device.setEqPresetGains(preset, gains, error))
                return false;
            if (m_neutralized)
                writeRecovery();
            return true;
        });
    }

    void setEqBand(int preset, int band, int frequency, double bandwidth)
    {
        runWrite([&](QString *error) {
            const int wireBandwidth = (band == 1 || band == 5) ? 0 : qRound(bandwidth * 4096.0);
            if (!m_device.setEqBand(preset, band, frequency, wireBandwidth, error))
                return false;
            auto &target = m_desired.eqPresets[preset - 1].bands[band - 1];
            target.frequencyHz = frequency;
            target.bandwidth = wireBandwidth;
            return true;
        });
    }

    void setSlider(int slider, int value)
    {
        runWrite([&](QString *error) {
            if (slider < int(A50Slider::StreamMic) || slider > int(A50Slider::Sidetone))
                return false;
            if (!m_device.setSlider(A50Slider(slider), value, error))
                return false;
            switch (A50Slider(slider)) {
            case A50Slider::StreamMic: m_desired.streamMic = value; break;
            case A50Slider::StreamChat: m_desired.streamChat = value; break;
            case A50Slider::StreamGame: m_desired.streamGame = value; break;
            case A50Slider::StreamAux: m_desired.streamAux = value; break;
            case A50Slider::Microphone: m_desired.microphoneLevel = value; break;
            case A50Slider::Sidetone: m_desired.sidetone = value; break;
            }
            return true;
        });
    }

    void setNoiseGate(int mode)
    {
        runWrite([&](QString *error) {
            if (mode < 0 || mode > 3 || !m_device.setNoiseGate(NoiseGateMode(mode), error))
                return false;
            m_desired.noiseGate = NoiseGateMode(mode);
            return true;
        });
    }

    void setMicrophoneEq(int preset)
    {
        runWrite([&](QString *error) {
            if (!m_device.setMicrophoneEq(preset, error))
                return false;
            m_desired.microphoneEq = preset;
            return true;
        });
    }

    void setAlertVolume(int value)
    {
        runWrite([&](QString *error) {
            if (!m_device.setAlertVolume(value, error))
                return false;
            m_desired.alertVolume = value;
            return true;
        });
    }

    void setDefaultBalance(int value)
    {
        runWrite([&](QString *error) {
            if (!m_device.setDefaultBalance(value, error))
                return false;
            m_desired.defaultBalance = value;
            return true;
        });
    }

    void saveToHeadset()
    {
        runWrite([&](QString *error) {
            const bool wasNeutralized = m_neutralized;
            if (wasNeutralized && !restoreDesiredOutput(error))
                return false;
            if (!applyDesired(error) || !m_device.save(error))
                return false;
            A50Snapshot verified;
            if (!m_device.readSnapshot(&verified, error))
                return false;
            m_snapshot = verified;
            m_desired = verified.settings;
            if (m_spatialActive && !m_auditioning && !applyFlatOutput(error))
                return false;
            return true;
        });
    }

    void revertToSaved()
    {
        runWrite([&](QString *error) {
            for (auto &preset : m_desired.eqPresets) {
                preset.name = preset.savedName;
                for (auto &band : preset.bands) {
                    band.gainDb = band.savedGainDb;
                    band.frequencyHz = band.savedFrequencyHz;
                    band.bandwidth = band.savedBandwidth;
                }
            }
            m_desired.microphoneEq = m_desired.savedMicrophoneEq;
            m_desired.microphoneLevel = m_desired.savedMicrophoneLevel;
            m_desired.sidetone = m_desired.savedSidetone;
            m_desired.streamMic = m_desired.savedStreamMic;
            m_desired.streamChat = m_desired.savedStreamChat;
            m_desired.streamGame = m_desired.savedStreamGame;
            m_desired.streamAux = m_desired.savedStreamAux;
            m_desired.noiseGate = m_desired.savedNoiseGate;
            m_desired.alertVolume = m_desired.savedAlertVolume;
            m_desired.defaultBalance = m_desired.savedDefaultBalance;
            if (!applyDesired(error))
                return false;
            if (m_spatialActive && !m_auditioning)
                return applyFlatOutput(error);
            return true;
        });
    }

    void applyProfile(const QVariantMap &profile)
    {
        runWrite([&](QString *error) {
            A50Settings candidate = m_desired;
            const int activePreset = profile.value(QStringLiteral("activeEqPreset")).toInt();
            const auto presets = profile.value(QStringLiteral("eqPresets")).toList();
            if (!A50Protocol::validPreset(activePreset) || presets.size() != 3) {
                *error = QStringLiteral("The profile has invalid hardware EQ data.");
                return false;
            }
            candidate.activeEqPreset = activePreset;
            for (const auto &presetValue : presets) {
                const auto presetMap = presetValue.toMap();
                const int slot = presetMap.value(QStringLiteral("slot")).toInt();
                const auto bands = presetMap.value(QStringLiteral("bands")).toList();
                if (!A50Protocol::validPreset(slot) || bands.size() != 5) {
                    *error = QStringLiteral("The profile has an invalid EQ slot.");
                    return false;
                }
                auto &preset = candidate.eqPresets[slot - 1];
                preset.name = presetMap.value(QStringLiteral("name")).toString();
                for (const auto &bandValue : bands) {
                    const auto bandMap = bandValue.toMap();
                    const int number = bandMap.value(QStringLiteral("band")).toInt();
                    const int gain = bandMap.value(QStringLiteral("gain")).toInt();
                    const int frequency = bandMap.value(QStringLiteral("frequency")).toInt();
                    const int bandwidth = (number == 1 || number == 5) ? 0
                        : qRound(bandMap.value(QStringLiteral("bandwidth")).toDouble() * 4096.0);
                    if (!A50Protocol::validBand(number) || !A50Protocol::validGain(gain)
                        || !A50Protocol::validFrequency(frequency)
                        || !A50Protocol::validBandwidth(number, bandwidth)) {
                        *error = QStringLiteral("The profile has an invalid EQ band.");
                        return false;
                    }
                    auto &band = preset.bands[number - 1];
                    band.gainDb = gain;
                    band.frequencyHz = frequency;
                    band.bandwidth = bandwidth;
                }
            }

            auto percent = [&](const QString &key, int *target) {
                const int value = profile.value(key).toInt();
                if (!A50Protocol::validPercent(value)) {
                    *error = QStringLiteral("The profile has an invalid %1 value.").arg(key);
                    return false;
                }
                *target = value;
                return true;
            };
            if (!percent(QStringLiteral("microphoneLevel"), &candidate.microphoneLevel)
                || !percent(QStringLiteral("sidetone"), &candidate.sidetone)
                || !percent(QStringLiteral("streamMic"), &candidate.streamMic)
                || !percent(QStringLiteral("streamChat"), &candidate.streamChat)
                || !percent(QStringLiteral("streamGame"), &candidate.streamGame)
                || !percent(QStringLiteral("streamAux"), &candidate.streamAux)
                || !percent(QStringLiteral("alertVolume"), &candidate.alertVolume))
                return false;
            candidate.microphoneEq = profile.value(QStringLiteral("microphoneEq")).toInt();
            candidate.noiseGate = NoiseGateMode(profile.value(QStringLiteral("noiseGate")).toInt());
            candidate.defaultBalance = profile.value(QStringLiteral("defaultBalance")).toInt();
            if (candidate.microphoneEq < 0 || candidate.microphoneEq > 2
                || int(candidate.noiseGate) < 0 || int(candidate.noiseGate) > 3
                || candidate.defaultBalance < 0 || candidate.defaultBalance > 255) {
                *error = QStringLiteral("The profile contains an invalid device setting.");
                return false;
            }

            m_desired = candidate;
            if (!applyDesired(error))
                return false;
            if (m_spatialActive && !m_auditioning)
                return applyFlatOutput(error);
            return true;
        });
    }

signals:
    void stateReady(const QVariantMap &state);
    void busyChanged(bool busy);
    void errorChanged(const QString &error);

private:
    template<typename Operation> void runWrite(Operation &&operation)
    {
        if (!m_device.isOpen() || !m_hasDesired) {
            emit errorChanged(QStringLiteral("The A50 control interface is not connected."));
            return;
        }
        setBusy(true);
        QString error;
        if (!operation(&error))
            emit errorChanged(error.isEmpty() ? QStringLiteral("The A50 rejected the requested change.") : error);
        else
            emit errorChanged({});
        emitState();
        setBusy(false);
    }

    bool applyFlatOutput(QString *error = nullptr)
    {
        writeRecovery();
        std::array<int, 5> flat = {0, 0, 0, 0, 0};
        if (!m_device.setActiveEqPreset(m_desired.activeEqPreset, error)
            || !m_device.setEqPresetGains(m_desired.activeEqPreset, flat, error))
            return false;
        m_neutralized = true;
        return true;
    }

    bool restoreDesiredOutput(QString *error = nullptr)
    {
        if (!m_device.isOpen() || !m_hasDesired)
            return false;
        std::array<int, 5> gains;
        const auto &preset = m_desired.eqPresets[m_desired.activeEqPreset - 1];
        for (int index = 0; index < 5; ++index)
            gains[index] = preset.bands[index].gainDb;
        if (!m_device.setActiveEqPreset(m_desired.activeEqPreset, error)
            || !m_device.setEqPresetGains(m_desired.activeEqPreset, gains, error))
            return false;
        m_neutralized = false;
        QFile::remove(recoveryPath());
        return true;
    }

    void writeRecovery()
    {
        QVariantList presets;
        for (const auto &preset : m_desired.eqPresets) {
            QVariantList gains;
            for (const auto &band : preset.bands)
                gains.append(band.gainDb);
            presets.append(QVariantMap{
                {QStringLiteral("slot"), preset.slot},
                {QStringLiteral("gains"), gains},
            });
        }
        const QVariantMap recovery = {
            {QStringLiteral("baseFirmware"), m_snapshot.baseFirmware},
            {QStringLiteral("headsetFirmware"), m_snapshot.headsetFirmware},
            {QStringLiteral("activeEqPreset"), m_desired.activeEqPreset},
            {QStringLiteral("presets"), presets},
        };
        const QFileInfo info(recoveryPath());
        QDir().mkpath(info.absolutePath());
        QSaveFile file(recoveryPath());
        const QByteArray bytes = QJsonDocument::fromVariant(recovery).toJson(QJsonDocument::Compact);
        if (file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size())
            file.commit();
    }

    void loadRecovery()
    {
        QFile file(recoveryPath());
        if (!file.open(QIODevice::ReadOnly))
            return;
        const auto recovery = QJsonDocument::fromJson(file.readAll()).toVariant().toMap();
        // Firmware metadata is unavailable during wireless use on affected
        // Gen 4 bases. The exact USB VID/PID already identifies the supported
        // hardware, so metadata placeholders must not invalidate the crash
        // journal that remembers the user's pre-DSP hardware gains.
        const int active = recovery.value(QStringLiteral("activeEqPreset")).toInt();
        const auto presets = recovery.value(QStringLiteral("presets")).toList();
        if (!A50Protocol::validPreset(active) || presets.size() != 3)
            return;
        for (const auto &value : presets) {
            const auto map = value.toMap();
            const int slot = map.value(QStringLiteral("slot")).toInt();
            const auto gains = map.value(QStringLiteral("gains")).toList();
            if (!A50Protocol::validPreset(slot) || gains.size() != 5)
                return;
            for (int band = 0; band < 5; ++band) {
                const int gain = gains[band].toInt();
                if (!A50Protocol::validGain(gain))
                    return;
                m_desired.eqPresets[slot - 1].bands[band].gainDb = gain;
            }
        }
        m_desired.activeEqPreset = active;
        bool activeIsFlat = true;
        for (const auto &band : m_snapshot.settings.eqPresets[active - 1].bands)
            activeIsFlat = activeIsFlat && band.gainDb == 0;
        if (activeIsFlat)
            m_neutralized = true;
        else {
            m_desired = m_snapshot.settings;
            QFile::remove(recoveryPath());
        }
    }

    bool applyDesired(QString *error)
    {
        if (!m_device.setActiveEqPreset(m_desired.activeEqPreset, error))
            return false;
        for (const auto &preset : m_desired.eqPresets) {
            if (!m_device.setEqPresetName(preset.slot, preset.name, error))
                return false;
            std::array<int, 5> gains;
            for (int index = 0; index < 5; ++index)
                gains[index] = preset.bands[index].gainDb;
            if (!m_device.setEqPresetGains(preset.slot, gains, error))
                return false;
            for (int index = 0; index < 5; ++index) {
                const auto &band = preset.bands[index];
                if (!m_device.setEqBand(preset.slot, index + 1, band.frequencyHz,
                                        band.bandwidth, error))
                    return false;
            }
        }
        return m_device.setSlider(A50Slider::Microphone, m_desired.microphoneLevel, error)
            && m_device.setSlider(A50Slider::Sidetone, m_desired.sidetone, error)
            && m_device.setSlider(A50Slider::StreamMic, m_desired.streamMic, error)
            && m_device.setSlider(A50Slider::StreamChat, m_desired.streamChat, error)
            && m_device.setSlider(A50Slider::StreamGame, m_desired.streamGame, error)
            && m_device.setSlider(A50Slider::StreamAux, m_desired.streamAux, error)
            && m_device.setNoiseGate(m_desired.noiseGate, error)
            && m_device.setMicrophoneEq(m_desired.microphoneEq, error)
            && m_device.setAlertVolume(m_desired.alertVolume, error)
            && m_device.setDefaultBalance(m_desired.defaultBalance, error);
    }

    void disconnected(const QString &error)
    {
        m_snapshot = {};
        m_hasDesired = false;
        m_neutralized = false;
        emit errorChanged(error);
        emit stateReady(QVariantMap{{QStringLiteral("connected"), false}});
        setBusy(false);
    }

    void emitState()
    {
        emit stateReady(snapshotMap(m_snapshot, m_desired, m_neutralized));
    }

    void setBusy(bool busy)
    {
        if (m_busy == busy)
            return;
        m_busy = busy;
        emit busyChanged(busy);
    }

    A50Device m_device;
    QTimer *m_timer = nullptr;
    A50Snapshot m_snapshot;
    A50Settings m_desired;
    QString m_error;
    bool m_hasDesired = false;
    bool m_busy = false;
    bool m_spatialActive = false;
    bool m_neutralized = false;
    bool m_auditioning = false;
    int m_pollCount = 0;
    int m_readFailures = 0;
};

A50Manager::A50Manager(QObject *parent)
    : QObject(parent)
    , m_worker(new A50Worker)
{
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::started, m_worker, &A50Worker::start);
    connect(m_worker, &A50Worker::stateReady, this, &A50Manager::acceptState);
    connect(m_worker, &A50Worker::busyChanged, this, &A50Manager::acceptBusy);
    connect(m_worker, &A50Worker::errorChanged, this, &A50Manager::acceptError);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread.setObjectName(QStringLiteral("A50 USB worker"));
    m_thread.start();
}

A50Manager::~A50Manager()
{
    if (m_thread.isRunning()) {
        QMetaObject::invokeMethod(m_worker, &A50Worker::stop, Qt::BlockingQueuedConnection);
        m_thread.quit();
        m_thread.wait();
    }
}

void A50Manager::acceptState(const QVariantMap &state)
{
    m_state = state;
    emit stateChanged();
}

void A50Manager::acceptBusy(bool busy)
{
    m_busy = busy;
    emit stateChanged();
}

void A50Manager::acceptError(const QString &error)
{
    m_error = error;
    emit stateChanged();
}

void A50Manager::refresh() { queue([worker = m_worker] { worker->refresh(true); }); }
void A50Manager::setSpatialActive(bool value) { queue([worker = m_worker, value] { worker->setSpatialActive(value); }); }
void A50Manager::setActiveEqPreset(int value) { queue([worker = m_worker, value] { worker->setActiveEqPreset(value); }); }
void A50Manager::setEqPresetName(int preset, const QString &name) { queue([worker = m_worker, preset, name] { worker->setEqPresetName(preset, name); }); }
void A50Manager::setEqGain(int preset, int band, int gain) { queue([worker = m_worker, preset, band, gain] { worker->setEqGain(preset, band, gain); }); }
void A50Manager::setEqBand(int preset, int band, int frequency, double bandwidth) { queue([worker = m_worker, preset, band, frequency, bandwidth] { worker->setEqBand(preset, band, frequency, bandwidth); }); }
void A50Manager::setSlider(int slider, int value) { queue([worker = m_worker, slider, value] { worker->setSlider(slider, value); }); }
void A50Manager::setNoiseGate(int mode) { queue([worker = m_worker, mode] { worker->setNoiseGate(mode); }); }
void A50Manager::setMicrophoneEq(int preset) { queue([worker = m_worker, preset] { worker->setMicrophoneEq(preset); }); }
void A50Manager::setAlertVolume(int value) { queue([worker = m_worker, value] { worker->setAlertVolume(value); }); }
void A50Manager::setDefaultBalance(int value) { queue([worker = m_worker, value] { worker->setDefaultBalance(value); }); }
void A50Manager::saveToHeadset() { queue([worker = m_worker] { worker->saveToHeadset(); }); }
void A50Manager::revertToSaved() { queue([worker = m_worker] { worker->revertToSaved(); }); }
void A50Manager::auditionHardwareEq(bool enabled) { queue([worker = m_worker, enabled] { worker->auditionHardwareEq(enabled); }); }
void A50Manager::applyProfile(const QVariantMap &profile) { queue([worker = m_worker, profile] { worker->applyProfile(profile); }); }

} // namespace AstroSpatial

#include "a50manager.moc"
