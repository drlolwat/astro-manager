#include "appcontroller.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QtMath>

namespace AstroSpatial {
namespace {

constexpr auto SofaPath = "/usr/share/libmysofa/default.sofa";
constexpr auto LimiterTtl = "/usr/lib/lv2/lsp-plugins.lv2/limiter_stereo.ttl";

QString shellQuotedDesktopExec(QString path)
{
    path.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    path.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return QLatin1Char('"') + path + QLatin1Char('"');
}

} // namespace

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_monitor(this)
    , m_systemd(this)
    , m_settings(QStringLiteral("AstroSpatial"), QStringLiteral("AstroSpatial"))
{
    m_mode = modeFromKey(m_settings.value(QStringLiteral("mode"), QStringLiteral("direct")).toString());
    m_endpoint = endpointFromKey(m_settings.value(QStringLiteral("outputEndpoint"), QStringLiteral("chat")).toString());
    m_profileId = m_settings.value(QStringLiteral("profileId"),
                                   ProfileGenerator::defaultProfileId(m_mode)).toString();
    m_eqPresetId = m_settings.value(QStringLiteral("eqPresetId"),
                                    QStringLiteral("bass-heavy")).toString();
    if (!m_settings.contains(QStringLiteral("eqPresetId"))) {
        m_settings.setValue(QStringLiteral("eqPresetId"), m_eqPresetId);
        m_settings.sync();
    }

    connect(&m_monitor, &PipeWireMonitor::stateChanged, this, [this] {
        const bool isConnected = m_monitor.connected();
        emit stateChanged();
        if (isConnected && (!m_wasConnected || !m_hasRestored)) {
            m_hasRestored = true;
            QTimer::singleShot(150, this, &AppController::applyCurrentMode);
        } else if (!isConnected && m_wasConnected) {
            QString ignored;
            m_systemd.stop(&ignored);
        }
        m_wasConnected = isConnected;
    });
    connect(&m_monitor, &PipeWireMonitor::connectionError, this, &AppController::setError);

    QString error;
    if (!m_monitor.start(&error))
        setError(error);
    ensureAutostart();
}

bool AppController::endpointAvailable(OutputEndpoint endpoint) const
{
    return endpoint == OutputEndpoint::Game ? m_monitor.gameAvailable() : m_monitor.chatAvailable();
}

bool AppController::selectedEndpointAvailable() const
{
    return endpointAvailable(m_endpoint);
}

QString AppController::selectedPhysicalNode() const
{
    return m_monitor.nodeForEndpoint(m_endpoint);
}

QString AppController::endpointDisplayName(OutputEndpoint endpoint)
{
    return endpoint == OutputEndpoint::Game ? QStringLiteral("A50 Game") : QStringLiteral("A50 Chat");
}

QVariantList AppController::profileOptions() const
{
    QVariantList result;
    for (const auto &profile : ProfileGenerator::profiles()) {
        result.append(QVariantMap{
            {QStringLiteral("id"), profile.id},
            {QStringLiteral("name"), profile.displayName},
            {QStringLiteral("mode"), static_cast<int>(profile.mode)},
        });
    }
    return result;
}

QVariantList AppController::eqOptions() const
{
    QVariantList result;
    for (const auto &preset : ProfileGenerator::equalizerPresets()) {
        result.append(QVariantMap{
            {QStringLiteral("id"), preset.id},
            {QStringLiteral("name"), preset.displayName},
        });
    }
    return result;
}

QVariantList AppController::channelOptions() const
{
    QVariantList result;
    if (m_mode == Mode::Direct)
        return result;
    for (const auto &channel : ProfileGenerator::profile(m_profileId).channels)
        result << channel.channel;
    return result;
}

bool AppController::limiterInstalled() const
{
    return QFileInfo::exists(QString::fromLatin1(LimiterTtl));
}

QString AppController::statusTitle() const
{
    if (!m_monitor.connected())
        return QStringLiteral("A50 dock disconnected");
    if (!selectedEndpointAvailable())
        return QStringLiteral("Selected A50 output is unavailable");
    if (!m_errorMessage.isEmpty())
        return QStringLiteral("Audio path needs attention");
    if (m_mode == Mode::Direct)
        return QStringLiteral("Direct Stereo via %1").arg(endpointDisplayName(m_endpoint));
    return QStringLiteral("%1 via %2")
        .arg(ProfileGenerator::profile(m_profileId).displayName, endpointDisplayName(m_endpoint));
}

QString AppController::statusDetail() const
{
    if (!m_monitor.connected())
        return QStringLiteral("Connect the dock in PC mode. Your current selection will be restored automatically.");
    if (!selectedEndpointAvailable())
        return QStringLiteral("Choose an A50 endpoint that PipeWire currently exposes.");
    if (m_busy)
        return QStringLiteral("Rebuilding and routing the PipeWire graph…");
    if (m_mode == Mode::Direct)
        return QStringLiteral("48 kHz / 16-bit dock path, unity software volume, no Astro Spatial DSP.");
    return QStringLiteral("32-bit float HRTF processing at 48 kHz with %1 EQ; Chat and microphone streams are not moved into DSP.")
        .arg(ProfileGenerator::equalizerPreset(m_eqPresetId).displayName);
}

QString AppController::configPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
        + QStringLiteral("/astro-spatial/filter-chain.conf");
}

void AppController::setError(const QString &message)
{
    m_errorMessage = message;
    m_busy = false;
    emit stateChanged();
}

void AppController::clearError()
{
    if (!m_errorMessage.isEmpty()) {
        m_errorMessage.clear();
        emit stateChanged();
    }
}

void AppController::saveState()
{
    m_settings.setValue(QStringLiteral("schemaVersion"), 1);
    m_settings.setValue(QStringLiteral("mode"), modeKey(m_mode));
    m_settings.setValue(QStringLiteral("outputEndpoint"), endpointKey(m_endpoint));
    m_settings.setValue(QStringLiteral("profileId"), m_profileId);
    m_settings.setValue(QStringLiteral("eqPresetId"), m_eqPresetId);
    m_settings.sync();
}

void AppController::activateMode(int value)
{
    if (value < static_cast<int>(Mode::Direct) || value > static_cast<int>(Mode::Surround71))
        return;
    m_mode = static_cast<Mode>(value);
    if (m_mode != Mode::Direct) {
        const auto selected = ProfileGenerator::profile(m_profileId);
        if (selected.mode != m_mode)
            m_profileId = ProfileGenerator::defaultProfileId(m_mode);
    }
    saveState();
    emit stateChanged();
    applyCurrentMode();
}

void AppController::selectOutputEndpoint(int value)
{
    if (value < static_cast<int>(OutputEndpoint::Chat)
        || value > static_cast<int>(OutputEndpoint::Game))
        return;
    m_endpoint = static_cast<OutputEndpoint>(value);
    saveState();
    emit stateChanged();
    applyCurrentMode();
}

void AppController::configure(int endpoint, int mode)
{
    if (endpoint < static_cast<int>(OutputEndpoint::Chat)
        || endpoint > static_cast<int>(OutputEndpoint::Game)
        || mode < static_cast<int>(Mode::Direct)
        || mode > static_cast<int>(Mode::Surround71))
        return;
    m_endpoint = static_cast<OutputEndpoint>(endpoint);
    m_mode = static_cast<Mode>(mode);
    if (m_mode != Mode::Direct) {
        const auto selected = ProfileGenerator::profile(m_profileId);
        if (selected.mode != m_mode)
            m_profileId = ProfileGenerator::defaultProfileId(m_mode);
    }
    saveState();
    emit stateChanged();
    applyCurrentMode();
}

void AppController::selectProfile(const QString &profileId)
{
    const auto profile = ProfileGenerator::profile(profileId);
    m_profileId = profile.id;
    m_mode = profile.mode;
    saveState();
    emit stateChanged();
    applyCurrentMode();
}

void AppController::selectEqPreset(const QString &presetId)
{
    m_eqPresetId = ProfileGenerator::equalizerPreset(presetId).id;
    saveState();
    emit stateChanged();
    if (m_mode != Mode::Direct)
        applyCurrentMode();
}

void AppController::applyCurrentMode()
{
    if (!m_monitor.connected() || !selectedEndpointAvailable())
        return;
    const quint64 generation = ++m_activationGeneration;
    clearError();
    if (m_mode == Mode::Direct)
        activateDirect(generation);
    else
        activateSurround(generation);
}

void AppController::activateDirect(quint64 generation)
{
    qInfo().noquote() << "Astro Spatial: activating Direct via" << endpointDisplayName(m_endpoint)
                      << "generation" << generation;
    m_busy = true;
    emit stateChanged();

    QString error;
    m_systemd.stop(&error); // A missing/inactive unit is harmless; routing still proceeds.
    if (!m_monitor.setDefaultTarget(selectedPhysicalNode(), &error)) {
        setError(error);
        return;
    }
    m_monitor.moveEligiblePlaybackStreams(selectedPhysicalNode(), &error);

    const quint32 id = m_monitor.idForEndpoint(m_endpoint);
    if (id != 0) {
        QProcess::execute(QStringLiteral("/usr/bin/wpctl"),
                          {QStringLiteral("set-volume"), QString::number(id), QStringLiteral("1.0")});
    }
    m_busy = false;
    emit stateChanged();
}

bool AppController::writeDspConfiguration(QString *error)
{
    const auto profile = ProfileGenerator::profile(m_profileId);
    if (!ProfileGenerator::validateInputs(profile, QString::fromLatin1(SofaPath),
                                          selectedPhysicalNode(), error))
        return false;
    if (!limiterInstalled()) {
        if (error) {
            *error = QStringLiteral("The safety limiter is missing. Install it with: sudo pacman -S lsp-plugins-lv2");
        }
        return false;
    }

    const QFileInfo info(configPath());
    if (!QDir().mkpath(info.absolutePath())) {
        if (error) *error = QStringLiteral("Could not create %1").arg(info.absolutePath());
        return false;
    }
    const QString generated = ProfileGenerator::generate(
        profile, QString::fromLatin1(SofaPath), selectedPhysicalNode(), true, m_eqPresetId);
    QSaveFile file(configPath());
    const QByteArray bytes = generated.toUtf8();
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)
        || file.write(bytes) != bytes.size() || !file.commit()) {
        if (error) *error = QStringLiteral("Could not write the DSP configuration: %1").arg(file.errorString());
        return false;
    }
    return true;
}

void AppController::activateSurround(quint64 generation)
{
    qInfo().noquote() << "Astro Spatial: activating" << ProfileGenerator::profile(m_profileId).displayName
                      << "with" << ProfileGenerator::equalizerPreset(m_eqPresetId).displayName << "EQ"
                      << "via" << endpointDisplayName(m_endpoint) << "generation" << generation;
    m_busy = true;
    emit stateChanged();

    QString error;
    if (!writeDspConfiguration(&error)
        || !m_systemd.installUserUnit(configPath(), &error)
        || !m_systemd.restart(&error)) {
        m_systemd.stop(nullptr);
        m_monitor.setDefaultTarget(selectedPhysicalNode(), nullptr);
        setError(error);
        return;
    }
    waitForSpatialSink(generation, 30);
}

void AppController::waitForSpatialSink(quint64 generation, int remainingAttempts)
{
    if (generation != m_activationGeneration)
        return;
    if (m_monitor.spatialAvailable()) {
        QString error;
        if (!m_monitor.setDefaultTarget(QString::fromLatin1(PipeWireMonitor::SpatialNode), &error)) {
            setError(error);
            return;
        }
        m_monitor.moveEligiblePlaybackStreams(QString::fromLatin1(PipeWireMonitor::SpatialNode), &error);
        m_busy = false;
        emit stateChanged();
        return;
    }
    if (remainingAttempts <= 0) {
        QString ignored;
        m_systemd.stop(&ignored);
        m_monitor.setDefaultTarget(selectedPhysicalNode(), &ignored);
        setError(QStringLiteral("The Astro Spatial sink did not appear. Check the user service log with: journalctl --user -u astro-spatial-dsp"));
        return;
    }
    QTimer::singleShot(100, this, [this, generation, remainingAttempts] {
        waitForSpatialSink(generation, remainingAttempts - 1);
    });
}

void AppController::refresh()
{
    clearError();
    applyCurrentMode();
}

void AppController::runSelfTest()
{
    bool exact = true;
    for (int sample = -32768; sample <= 32767; ++sample) {
        const float normalized = static_cast<float>(sample) / 32768.0f;
        const int restored = qRound(normalized * 32768.0f);
        if (restored != sample) {
            exact = false;
            break;
        }
    }

    QStringList report;
    report << (m_monitor.connected() ? QStringLiteral("PASS  A50 dock detected")
                                     : QStringLiteral("FAIL  A50 dock not detected"));
    report << (selectedEndpointAvailable()
                   ? QStringLiteral("PASS  %1 endpoint available").arg(endpointDisplayName(m_endpoint))
                   : QStringLiteral("FAIL  Selected endpoint unavailable"));
    report << (QFileInfo::exists(QString::fromLatin1(SofaPath))
                   ? QStringLiteral("PASS  KEMAR SOFA HRTF available")
                   : QStringLiteral("FAIL  KEMAR SOFA HRTF missing"));
    report << (limiterInstalled() ? QStringLiteral("PASS  LSP true-peak limiter available")
                                  : QStringLiteral("FAIL  lsp-plugins-lv2 not installed"));
    report << (exact ? QStringLiteral("PASS  Every S16 sample survives identity float conversion bit-for-bit")
                     : QStringLiteral("FAIL  S16 identity conversion mismatch"));
    if (m_mode == Mode::Direct && m_monitor.defaultNodeName() == selectedPhysicalNode())
        report << QStringLiteral("PASS  Direct mode targets the selected physical endpoint");
    else if (m_mode == Mode::Direct)
        report << QStringLiteral("WARN  Direct mode is not the configured default yet");
    else
        report << QStringLiteral("INFO  Surround mode intentionally changes samples");
    report << QStringLiteral("BOUNDARY  The proprietary dock-to-headset wireless hop cannot be measured by Linux");
    m_verificationReport = report.join(QLatin1Char('\n'));
    emit stateChanged();
}

void AppController::playChannel(const QString &channel)
{
    if (m_mode == Mode::Direct || !m_monitor.spatialAvailable())
        return;
    const auto profile = ProfileGenerator::profile(m_profileId);
    int channelIndex = -1;
    QStringList channelMap;
    for (int i = 0; i < profile.channels.size(); ++i) {
        channelMap << profile.channels.at(i).channel;
        if (profile.channels.at(i).channel == channel)
            channelIndex = i;
    }
    if (channelIndex < 0)
        return;

    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(cacheDir);
    const QString filePath = cacheDir + QStringLiteral("/channel-test-%1.raw").arg(channel);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(file.errorString());
        return;
    }
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    constexpr int frames = 24000;
    constexpr double frequency = 700.0;
    for (int frame = 0; frame < frames; ++frame) {
        const double edge = qMin(qMin(frame / 1200.0, (frames - frame - 1) / 1200.0), 1.0);
        const qint16 tone = static_cast<qint16>(qSin(2.0 * M_PI * frequency * frame / 48000.0)
                                               * 8192.0 * qMax(0.0, edge));
        for (int output = 0; output < profile.channels.size(); ++output)
            stream << static_cast<qint16>(output == channelIndex ? tone : 0);
    }
    file.close();

    auto *player = new QProcess(this);
    connect(player, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [player, filePath](int, QProcess::ExitStatus) {
                QFile::remove(filePath);
                player->deleteLater();
            });
    player->start(QStringLiteral("/usr/bin/pw-play"), {
        QStringLiteral("--target"), QString::fromLatin1(PipeWireMonitor::SpatialNode),
        QStringLiteral("--raw"),
        QStringLiteral("--rate"), QStringLiteral("48000"),
        QStringLiteral("--channels"), QString::number(profile.channels.size()),
        QStringLiteral("--channel-map"), channelMap.join(QLatin1Char(',')),
        QStringLiteral("--format"), QStringLiteral("s16"),
        filePath,
    });
}

bool AppController::ensureAutostart(QString *error)
{
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
        + QStringLiteral("/autostart");
    if (!QDir().mkpath(directory)) {
        if (error) *error = QStringLiteral("Could not create the autostart directory.");
        return false;
    }
    const QString path = directory + QStringLiteral("/io.github.drlolwat.AstroSpatial.desktop");
    QFile::remove(directory + QStringLiteral("/io.github.jabby.AstroSpatial.desktop"));
    const QString contents = QStringLiteral(R"([Desktop Entry]
Type=Application
Name=Astro Spatial
Comment=Restore the selected Astro A50 audio path
Exec=%1 --background
Icon=audio-headphones
Terminal=false
X-KDE-autostart-after=panel
X-KDE-StartupNotify=false
)").arg(shellQuotedDesktopExec(QCoreApplication::applicationFilePath()));
    QSaveFile file(path);
    const QByteArray bytes = contents.toUtf8();
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)
        || file.write(bytes) != bytes.size() || !file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

} // namespace AstroSpatial
