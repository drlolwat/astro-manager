#pragma once

#include "a50manager.h"
#include "pipewiremonitor.h"
#include "profilegenerator.h"
#include "profilestore.h"
#include "systemdmanager.h"
#include "types.h"

#include <QObject>
#include <QSettings>
#include <QVariantList>
#include <QUrl>

namespace AstroSpatial {

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY stateChanged)
    Q_PROPERTY(bool chatAvailable READ chatAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool gameAvailable READ gameAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool selectedEndpointAvailable READ selectedEndpointAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool spatialAvailable READ spatialAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(int mode READ mode NOTIFY stateChanged)
    Q_PROPERTY(int outputEndpoint READ outputEndpoint NOTIFY stateChanged)
    Q_PROPERTY(QString profileId READ profileId NOTIFY stateChanged)
    Q_PROPERTY(QString eqPresetId READ eqPresetId NOTIFY stateChanged)
    Q_PROPERTY(QVariantList profileOptions READ profileOptions CONSTANT)
    Q_PROPERTY(QVariantList eqOptions READ eqOptions CONSTANT)
    Q_PROPERTY(QVariantList channelOptions READ channelOptions NOTIFY stateChanged)
    Q_PROPERTY(QString statusTitle READ statusTitle NOTIFY stateChanged)
    Q_PROPERTY(QString statusDetail READ statusDetail NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(QString verificationReport READ verificationReport NOTIFY stateChanged)
    Q_PROPERTY(bool limiterInstalled READ limiterInstalled NOTIFY stateChanged)
    Q_PROPERTY(QObject *hardware READ hardware CONSTANT)
    Q_PROPERTY(QVariantList savedProfiles READ savedProfiles NOTIFY stateChanged)
    Q_PROPERTY(QString profileError READ profileError NOTIFY stateChanged)

public:
    explicit AppController(QObject *parent = nullptr);

    bool connected() const { return m_monitor.connected(); }
    bool chatAvailable() const { return m_monitor.chatAvailable(); }
    bool gameAvailable() const { return m_monitor.gameAvailable(); }
    bool selectedEndpointAvailable() const;
    bool spatialAvailable() const { return m_monitor.spatialAvailable(); }
    bool busy() const { return m_busy; }
    int mode() const { return static_cast<int>(m_mode); }
    int outputEndpoint() const { return static_cast<int>(m_endpoint); }
    QString profileId() const { return m_profileId; }
    QString eqPresetId() const { return m_eqPresetId; }
    QVariantList profileOptions() const;
    QVariantList eqOptions() const;
    QVariantList channelOptions() const;
    QString statusTitle() const;
    QString statusDetail() const;
    QString errorMessage() const { return m_errorMessage; }
    QString verificationReport() const { return m_verificationReport; }
    bool limiterInstalled() const;
    QObject *hardware() { return &m_hardware; }
    A50Manager *hardwareManager() { return &m_hardware; }
    QVariantList savedProfiles() const { return m_profileStore.profiles(); }
    QString profileError() const { return m_profileError; }

    Q_INVOKABLE void activateMode(int mode);
    Q_INVOKABLE void selectOutputEndpoint(int endpoint);
    Q_INVOKABLE void configure(int endpoint, int mode);
    Q_INVOKABLE void selectProfile(const QString &profileId);
    Q_INVOKABLE void selectEqPreset(const QString &presetId);
    Q_INVOKABLE void playChannel(const QString &channel);
    Q_INVOKABLE void runSelfTest();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void createUnifiedProfile(const QString &name);
    Q_INVOKABLE void applyUnifiedProfile(const QString &id);
    Q_INVOKABLE void deleteUnifiedProfile(const QString &id);
    Q_INVOKABLE void importUnifiedProfile(const QUrl &url);
    Q_INVOKABLE void exportUnifiedProfile(const QString &id, const QUrl &url);

signals:
    void stateChanged();

private:
    void applyCurrentMode();
    void activateDirect(quint64 generation);
    void activateSurround(quint64 generation);
    void waitForSpatialSink(quint64 generation, int remainingAttempts);
    bool writeDspConfiguration(QString *error);
    bool ensureAutostart(QString *error = nullptr);
    bool endpointAvailable(OutputEndpoint endpoint) const;
    QString selectedPhysicalNode() const;
    QString configPath() const;
    void setError(const QString &message);
    void clearError();
    void saveState();
    static QString endpointDisplayName(OutputEndpoint endpoint);

    PipeWireMonitor m_monitor;
    SystemdManager m_systemd;
    A50Manager m_hardware;
    ProfileStore m_profileStore;
    QSettings m_settings;
    Mode m_mode = Mode::Direct;
    OutputEndpoint m_endpoint = OutputEndpoint::Chat;
    QString m_profileId = QStringLiteral("reference-7.1");
    QString m_eqPresetId = QStringLiteral("bass-heavy");
    bool m_busy = false;
    bool m_wasConnected = false;
    bool m_hasRestored = false;
    QString m_errorMessage;
    QString m_verificationReport;
    QString m_profileError;
    quint64 m_activationGeneration = 0;
};

} // namespace AstroSpatial
