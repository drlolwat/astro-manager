#pragma once

#include "types.h"

#include <QObject>
#include <QString>

typedef struct _WpCore WpCore;
typedef struct _WpObjectManager WpObjectManager;
typedef struct _WpMetadata WpMetadata;

namespace AstroSpatial {

class PipeWireMonitor : public QObject
{
    Q_OBJECT

public:
    static constexpr auto ChatNode = "alsa_output.usb-Astro_Gaming_Astro_A50-00.stereo-chat";
    static constexpr auto GameNode = "alsa_output.usb-Astro_Gaming_Astro_A50-00.stereo-game";
    static constexpr auto SpatialNode = "astro_spatial_sink";
    static constexpr auto SpatialOutputNode = "astro_spatial_output";

    explicit PipeWireMonitor(QObject *parent = nullptr);
    ~PipeWireMonitor() override;

    bool start(QString *error = nullptr);
    bool connected() const { return m_chatAvailable || m_gameAvailable; }
    bool chatAvailable() const { return m_chatAvailable; }
    bool gameAvailable() const { return m_gameAvailable; }
    bool spatialAvailable() const { return m_spatialAvailable; }
    bool metadataAvailable() const { return m_defaultMetadata != nullptr; }
    QString defaultNodeName() const;
    QString nodeForEndpoint(OutputEndpoint endpoint) const;
    quint32 idForEndpoint(OutputEndpoint endpoint) const;

    bool setDefaultTarget(const QString &nodeName, QString *error = nullptr);
    int moveEligiblePlaybackStreams(const QString &nodeName, QString *error = nullptr);

signals:
    void stateChanged();
    void connectionError(const QString &message);

private:
    void scanObjects();
    void pumpMainContext();
    static QString property(void *object, const char *key);

    WpCore *m_core = nullptr;
    WpObjectManager *m_objectManager = nullptr;
    WpMetadata *m_defaultMetadata = nullptr;
    bool m_chatAvailable = false;
    bool m_gameAvailable = false;
    bool m_spatialAvailable = false;
    quint32 m_chatId = 0;
    quint32 m_gameId = 0;
};

} // namespace AstroSpatial
