#pragma once

#include "a50device.h"

#include <QObject>
#include <QThread>
#include <QVariantMap>

namespace AstroSpatial {

class A50Worker;

class A50Manager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY stateChanged)
    Q_PROPERTY(bool neutralized READ neutralized NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap state READ state NOTIFY stateChanged)

public:
    explicit A50Manager(QObject *parent = nullptr);
    ~A50Manager() override;

    bool connected() const { return m_state.value(QStringLiteral("connected")).toBool(); }
    bool busy() const { return m_busy; }
    bool dirty() const { return m_state.value(QStringLiteral("dirty")).toBool(); }
    bool neutralized() const { return m_state.value(QStringLiteral("neutralized")).toBool(); }
    QString errorMessage() const { return m_error; }
    QVariantMap state() const { return m_state; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setSpatialActive(bool active);
    Q_INVOKABLE void setActiveEqPreset(int preset);
    Q_INVOKABLE void setEqPresetName(int preset, const QString &name);
    Q_INVOKABLE void setEqGain(int preset, int band, int gainDb);
    Q_INVOKABLE void setEqBand(int preset, int band, int frequencyHz, double bandwidth);
    Q_INVOKABLE void setSlider(int slider, int value);
    Q_INVOKABLE void setNoiseGate(int mode);
    Q_INVOKABLE void setMicrophoneEq(int preset);
    Q_INVOKABLE void setAlertVolume(int value);
    Q_INVOKABLE void setDefaultBalance(int value);
    Q_INVOKABLE void saveToHeadset();
    Q_INVOKABLE void revertToSaved();
    Q_INVOKABLE void auditionHardwareEq(bool enabled);
    Q_INVOKABLE void applyProfile(const QVariantMap &hardwareProfile);

signals:
    void stateChanged();

private slots:
    void acceptState(const QVariantMap &state);
    void acceptBusy(bool busy);
    void acceptError(const QString &error);

private:
    template<typename Functor> void queue(Functor &&functor)
    {
        QMetaObject::invokeMethod(m_worker, std::forward<Functor>(functor), Qt::QueuedConnection);
    }

    QThread m_thread;
    A50Worker *m_worker = nullptr;
    QVariantMap m_state;
    QString m_error;
    bool m_busy = false;
};

} // namespace AstroSpatial
