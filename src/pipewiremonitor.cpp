#include "pipewiremonitor.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

#ifdef signals
#undef signals
#endif
#include <wp/wp.h>

namespace AstroSpatial {
namespace {

bool isCommunicationRole(const QString &role)
{
    return role.compare(QLatin1String("Communication"), Qt::CaseInsensitive) == 0
        || role.compare(QLatin1String("Phone"), Qt::CaseInsensitive) == 0
        || role.compare(QLatin1String("Chat"), Qt::CaseInsensitive) == 0;
}

QString jsonNodeName(const QString &name)
{
    return QString::fromUtf8(QJsonDocument(QJsonObject{{QStringLiteral("name"), name}})
                                 .toJson(QJsonDocument::Compact));
}

} // namespace

PipeWireMonitor::PipeWireMonitor(QObject *parent)
    : QObject(parent)
{
}

PipeWireMonitor::~PipeWireMonitor()
{
    if (m_defaultMetadata)
        g_object_unref(m_defaultMetadata);
    if (m_objectManager)
        g_object_unref(m_objectManager);
    if (m_core) {
        wp_core_disconnect(m_core);
        g_object_unref(m_core);
    }
}

bool PipeWireMonitor::start(QString *error)
{
    wp_init(WP_INIT_ALL);
    m_core = wp_core_new(nullptr, nullptr, nullptr);
    if (!m_core) {
        if (error) *error = QStringLiteral("Could not create a WirePlumber core.");
        return false;
    }

    if (!wp_core_connect(m_core)) {
        if (error) *error = QStringLiteral("Could not connect to the current PipeWire session.");
        return false;
    }

    m_objectManager = wp_object_manager_new();
    wp_object_manager_add_interest(m_objectManager, WP_TYPE_NODE, nullptr);
    wp_object_manager_add_interest(m_objectManager, WP_TYPE_METADATA, nullptr);
    wp_object_manager_request_object_features(
        m_objectManager, WP_TYPE_NODE, WP_PIPEWIRE_OBJECT_FEATURES_MINIMAL);
    wp_object_manager_request_object_features(
        m_objectManager, WP_TYPE_METADATA,
        static_cast<WpObjectFeatures>(WP_PROXY_FEATURE_BOUND)
            | static_cast<WpObjectFeatures>(WP_METADATA_FEATURE_DATA));

    g_signal_connect_swapped(m_objectManager, "objects-changed",
                             G_CALLBACK(+[](PipeWireMonitor *self) { self->scanObjects(); }), this);
    g_signal_connect_swapped(m_objectManager, "installed",
                             G_CALLBACK(+[](PipeWireMonitor *self) { self->scanObjects(); }), this);
    g_signal_connect_swapped(m_core, "disconnected",
                             G_CALLBACK(+[](PipeWireMonitor *self) {
                                 emit self->connectionError(QStringLiteral("PipeWire disconnected."));
                             }), this);
    wp_core_install_object_manager(m_core, m_objectManager);

    auto *pump = new QTimer(this);
    pump->setInterval(10);
    connect(pump, &QTimer::timeout, this, &PipeWireMonitor::pumpMainContext);
    pump->start();
    return true;
}

void PipeWireMonitor::pumpMainContext()
{
    auto *context = wp_core_get_g_main_context(m_core);
    int iterations = 0;
    while (context && g_main_context_pending(context) && iterations++ < 64)
        g_main_context_iteration(context, false);
}

QString PipeWireMonitor::property(void *object, const char *key)
{
    if (!object)
        return {};
    const char *value = nullptr;
    if (WP_IS_PIPEWIRE_OBJECT(object))
        value = wp_pipewire_object_get_property(WP_PIPEWIRE_OBJECT(object), key);
    if (!value && WP_IS_GLOBAL_PROXY(object)) {
        WpProperties *properties = wp_global_proxy_get_global_properties(WP_GLOBAL_PROXY(object));
        if (properties)
            value = wp_properties_get(properties, key);
    }
    return value ? QString::fromUtf8(value) : QString{};
}

void PipeWireMonitor::scanObjects()
{
    bool chat = false;
    bool game = false;
    bool spatial = false;
    quint32 chatId = 0;
    quint32 gameId = 0;
    WpMetadata *metadata = nullptr;

    WpIterator *iterator = wp_object_manager_new_iterator(m_objectManager);
    GValue item = G_VALUE_INIT;
    while (wp_iterator_next(iterator, &item)) {
        auto *object = g_value_get_object(&item);
        if (WP_IS_NODE(object)) {
            const QString nodeName = property(object, "node.name");
            const auto id = wp_proxy_get_bound_id(WP_PROXY(object));
            if (nodeName == QLatin1String(ChatNode)) {
                chat = true;
                chatId = id;
            } else if (nodeName == QLatin1String(GameNode)) {
                game = true;
                gameId = id;
            } else if (nodeName == QLatin1String(SpatialNode)) {
                spatial = true;
            }
        } else if (WP_IS_METADATA(object)) {
            const QString metadataName = property(object, "metadata.name");
            if (metadataName == QLatin1String("default"))
                metadata = WP_METADATA(object);
        }
        g_value_unset(&item);
    }
    wp_iterator_unref(iterator);

    if (metadata != m_defaultMetadata) {
        if (metadata) g_object_ref(metadata);
        if (m_defaultMetadata) g_object_unref(m_defaultMetadata);
        m_defaultMetadata = metadata;
    }

    const bool changed = chat != m_chatAvailable || game != m_gameAvailable
        || spatial != m_spatialAvailable || chatId != m_chatId || gameId != m_gameId;
    m_chatAvailable = chat;
    m_gameAvailable = game;
    m_spatialAvailable = spatial;
    m_chatId = chatId;
    m_gameId = gameId;
    if (changed)
        emit stateChanged();
}

QString PipeWireMonitor::defaultNodeName() const
{
    if (!m_defaultMetadata)
        return {};
    const char *type = nullptr;
    const char *value = wp_metadata_find(m_defaultMetadata, 0,
                                         "default.configured.audio.sink", &type);
    if (!value)
        value = wp_metadata_find(m_defaultMetadata, 0, "default.audio.sink", &type);
    if (!value)
        return {};
    const auto document = QJsonDocument::fromJson(QByteArray(value));
    return document.object().value(QStringLiteral("name")).toString();
}

QString PipeWireMonitor::nodeForEndpoint(OutputEndpoint endpoint) const
{
    return endpoint == OutputEndpoint::Game ? QString::fromLatin1(GameNode)
                                            : QString::fromLatin1(ChatNode);
}

quint32 PipeWireMonitor::idForEndpoint(OutputEndpoint endpoint) const
{
    return endpoint == OutputEndpoint::Game ? m_gameId : m_chatId;
}

bool PipeWireMonitor::setDefaultTarget(const QString &nodeName, QString *error)
{
    if (!m_defaultMetadata) {
        if (error) *error = QStringLiteral("WirePlumber default-node metadata is unavailable.");
        return false;
    }
    const QByteArray value = jsonNodeName(nodeName).toUtf8();
    wp_metadata_set(m_defaultMetadata, 0, "default.configured.audio.sink",
                    "Spa:String:JSON", value.constData());
    wp_metadata_set(m_defaultMetadata, 0, "default.audio.sink",
                    "Spa:String:JSON", value.constData());
    return true;
}

int PipeWireMonitor::moveEligiblePlaybackStreams(const QString &nodeName, QString *error)
{
    if (!m_defaultMetadata) {
        if (error) *error = QStringLiteral("WirePlumber stream-routing metadata is unavailable.");
        return -1;
    }

    int moved = 0;
    const QByteArray target = nodeName.toUtf8();
    WpIterator *iterator = wp_object_manager_new_iterator(m_objectManager);
    GValue item = G_VALUE_INIT;
    while (wp_iterator_next(iterator, &item)) {
        auto *object = g_value_get_object(&item);
        if (!WP_IS_NODE(object)) {
            g_value_unset(&item);
            continue;
        }

        const QString mediaClass = property(object, "media.class");
        const QString role = property(object, "media.role");
        const QString application = property(object, "application.name");
        const QString nodeName = property(object, "node.name");
        if (mediaClass == QLatin1String("Stream/Output/Audio")
            && !isCommunicationRole(role)
            && application != QLatin1String("Astro Spatial")
            && application != QLatin1String("Astro Manager for Linux")
            && nodeName != QLatin1String(SpatialOutputNode)) {
            const auto id = wp_proxy_get_bound_id(WP_PROXY(object));
            if (id != G_MAXUINT32) {
                wp_metadata_set(m_defaultMetadata, id, "target.object", "Spa:String", target.constData());
                ++moved;
            }
        }
        g_value_unset(&item);
    }
    wp_iterator_unref(iterator);
    return moved;
}

} // namespace AstroSpatial
