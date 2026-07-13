#include "profilegenerator.h"

#include <QFileInfo>
#include <QStringList>

namespace AstroSpatial {
namespace {

QString quoted(QString value)
{
    value.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    value.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return QLatin1Char('"') + value + QLatin1Char('"');
}

QString channelList(const ProfileSpec &profile)
{
    QStringList result;
    for (const auto &position : profile.channels)
        result << position.channel;
    return result.join(QLatin1Char(' '));
}

QString directionNode(const ChannelPosition &position, const QString &sofaPath)
{
    return QStringLiteral(R"(
                    { type = sofa label = spatializer name = spatial_%1
                        config = {
                            filename = %2
                            blocksize = 128
                            tailsize = 1024
                            gain = 0.5
                            normalize = true
                            latency = 0
                        }
                        control = { "Azimuth" = %3 "Elevation" = %4 "Radius" = 1.0 }
                    })")
        .arg(position.channel, quoted(sofaPath))
        .arg(position.azimuth)
        .arg(position.elevation);
}

QString equalizerNode(const QString &name, const EqualizerPreset &preset)
{
    QStringList filters;
    for (const auto &band : preset.bands) {
        filters << QStringLiteral("                            { type = %1 freq = %2 gain = %3 q = %4 }")
                       .arg(band.type)
                       .arg(band.frequency, 0, 'f', 1)
                       .arg(band.gain, 0, 'f', 1)
                       .arg(band.q, 0, 'f', 3);
    }
    return QStringLiteral(R"(
                    { type = builtin label = param_eq name = %1
                        config = {
                            filters = [
%2
                            ]
                        }
                    })")
        .arg(name, filters.join(QLatin1Char('\n')));
}

} // namespace

QList<ProfileSpec> ProfileGenerator::profiles()
{
    return {
        {
            QStringLiteral("reference-5.1"),
            QStringLiteral("5.1 Reference"),
            Mode::Surround51,
            {
                {QStringLiteral("FL"), 30, 0},
                {QStringLiteral("FR"), 330, 0},
                {QStringLiteral("FC"), 0, 0},
                {QStringLiteral("LFE"), 0, 0},
                {QStringLiteral("SL"), 110, 0},
                {QStringLiteral("SR"), 250, 0},
            },
        },
        {
            QStringLiteral("reference-7.1"),
            QStringLiteral("7.1 Reference"),
            Mode::Surround71,
            {
                {QStringLiteral("FL"), 30, 0},
                {QStringLiteral("FR"), 330, 0},
                {QStringLiteral("FC"), 0, 0},
                {QStringLiteral("LFE"), 0, 0},
                {QStringLiteral("RL"), 150, 0},
                {QStringLiteral("RR"), 210, 0},
                {QStringLiteral("SL"), 90, 0},
                {QStringLiteral("SR"), 270, 0},
            },
        },
        {
            QStringLiteral("wide-7.1"),
            QStringLiteral("7.1 Wide"),
            Mode::Surround71,
            {
                {QStringLiteral("FL"), 35, 0},
                {QStringLiteral("FR"), 325, 0},
                {QStringLiteral("FC"), 0, 0},
                {QStringLiteral("LFE"), 0, 0},
                {QStringLiteral("RL"), 145, 0},
                {QStringLiteral("RR"), 215, 0},
                {QStringLiteral("SL"), 100, 0},
                {QStringLiteral("SR"), 260, 0},
            },
        },
    };
}

ProfileSpec ProfileGenerator::profile(const QString &id)
{
    for (const auto &candidate : profiles()) {
        if (candidate.id == id)
            return candidate;
    }
    return profiles().at(1);
}

QString ProfileGenerator::defaultProfileId(Mode mode)
{
    return mode == Mode::Surround51 ? QStringLiteral("reference-5.1")
                                    : QStringLiteral("reference-7.1");
}

QList<EqualizerPreset> ProfileGenerator::equalizerPresets()
{
    return {
        {
            QStringLiteral("bass-heavy"),
            QStringLiteral("Bass Heavy (Default)"),
            {
                {QStringLiteral("bq_highshelf"), 0.0, -5.0, 0.707},
                {QStringLiteral("bq_lowshelf"), 110.0, 7.5, 0.707},
                {QStringLiteral("bq_peaking"), 55.0, 2.5, 1.000},
                {QStringLiteral("bq_peaking"), 3500.0, -2.0, 0.900},
                {QStringLiteral("bq_highshelf"), 8000.0, -2.0, 0.707},
            },
        },
        {
            QStringLiteral("balanced"),
            QStringLiteral("Balanced Warm"),
            {
                {QStringLiteral("bq_highshelf"), 0.0, -2.0, 0.707},
                {QStringLiteral("bq_lowshelf"), 105.0, 3.5, 0.707},
                {QStringLiteral("bq_peaking"), 5000.0, -1.0, 0.900},
                {QStringLiteral("bq_highshelf"), 9000.0, -0.5, 0.707},
            },
        },
        {QStringLiteral("flat"), QStringLiteral("Flat / EQ Off"), {}},
    };
}

EqualizerPreset ProfileGenerator::equalizerPreset(const QString &id)
{
    for (const auto &preset : equalizerPresets()) {
        if (preset.id == id)
            return preset;
    }
    return equalizerPresets().constFirst();
}

bool ProfileGenerator::validateInputs(const ProfileSpec &profile,
                                      const QString &sofaPath,
                                      const QString &targetNode,
                                      QString *error)
{
    if (profile.mode == Mode::Direct || profile.channels.isEmpty()) {
        if (error) *error = QStringLiteral("A surround profile must contain channels.");
        return false;
    }
    if (!QFileInfo::exists(sofaPath)) {
        if (error) *error = QStringLiteral("SOFA HRTF file not found: %1").arg(sofaPath);
        return false;
    }
    if (targetNode.trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("The physical output node is empty.");
        return false;
    }
    return true;
}

QString ProfileGenerator::generate(const ProfileSpec &profile,
                                   const QString &sofaPath,
                                   const QString &targetNode,
                                   bool includeLimiter,
                                   const QString &equalizerPresetId)
{
    QStringList nodes;
    QStringList links;
    QStringList inputs;
    int mixerInput = 1;

    for (const auto &position : profile.channels) {
        if (position.channel == QLatin1String("LFE")) {
            nodes << QStringLiteral(R"pw(
                    { type = builtin label = bq_lowpass name = lfe_lowpass
                        control = { "Freq" = 120.0 "Q" = 0.707 "Gain" = 0.0 }
                    }
                    { type = builtin label = delay name = lfe_delay
                        config = { "max-delay" = 0.05 "latency" = 0.011625 }
                        control = { "Delay (s)" = 0.011625 "Feedback" = 0.0 "Feedforward" = 0.0 }
                    }
                    { type = builtin label = copy name = lfe_copy })pw");
            links << QStringLiteral("                    { output = \"lfe_lowpass:Out\" input = \"lfe_delay:In\" }");
            links << QStringLiteral("                    { output = \"lfe_delay:Out\" input = \"lfe_copy:In\" }");
            links << QStringLiteral("                    { output = \"lfe_copy:Out\" input = \"mixL:In %1\" }").arg(mixerInput);
            links << QStringLiteral("                    { output = \"lfe_copy:Out\" input = \"mixR:In %1\" }").arg(mixerInput);
            inputs << QStringLiteral("\"lfe_lowpass:In\"");
        } else {
            nodes << directionNode(position, sofaPath);
            links << QStringLiteral("                    { output = \"spatial_%1:Out L\" input = \"mixL:In %2\" }")
                         .arg(position.channel).arg(mixerInput);
            links << QStringLiteral("                    { output = \"spatial_%1:Out R\" input = \"mixR:In %2\" }")
                         .arg(position.channel).arg(mixerInput);
            inputs << QStringLiteral("\"spatial_%1:In\"").arg(position.channel);
        }
        ++mixerInput;
    }

    nodes << QStringLiteral(R"(
                    { type = builtin label = mixer name = mixL }
                    { type = builtin label = mixer name = mixR })");

    const auto eqPreset = equalizerPreset(equalizerPresetId);
    QString leftOutput = QStringLiteral("mixL:Out");
    QString rightOutput = QStringLiteral("mixR:Out");
    if (!eqPreset.bands.isEmpty()) {
        nodes << equalizerNode(QStringLiteral("eqL"), eqPreset);
        nodes << equalizerNode(QStringLiteral("eqR"), eqPreset);
        links << QStringLiteral("                    { output = \"mixL:Out\" input = \"eqL:In 1\" }");
        links << QStringLiteral("                    { output = \"mixR:Out\" input = \"eqR:In 1\" }");
        leftOutput = QStringLiteral("eqL:Out 1");
        rightOutput = QStringLiteral("eqR:Out 1");
    }

    QString outputs;
    if (includeLimiter) {
        nodes << QStringLiteral(R"(
                    { type = lv2 name = safety_limiter
                        plugin = "http://lsp-plug.in/plugins/lv2/limiter_stereo"
                        control = {
                            "enabled" = 1
                            "alr" = 0
                            "mode" = 0
                            "th" = 0.89125094
                            "boost" = 0
                            "lk" = 1.0
                            "at" = 1.0
                            "rt" = 5.0
                            "ovs" = 21
                            "dith" = 6
                            "g_in" = 1.0
                            "g_out" = 1.0
                        }
                    })");
        links << QStringLiteral("                    { output = \"%1\" input = \"safety_limiter:in_l\" }").arg(leftOutput);
        links << QStringLiteral("                    { output = \"%1\" input = \"safety_limiter:in_r\" }").arg(rightOutput);
        outputs = QStringLiteral("[ \"safety_limiter:out_l\" \"safety_limiter:out_r\" ]");
    } else {
        outputs = QStringLiteral("[ \"%1\" \"%2\" ]").arg(leftOutput, rightOutput);
    }

    const int channels = profile.channels.size();
    return QStringLiteral(R"(# Generated by Astro Manager. Manual changes will be overwritten.
context.properties = {
    log.level = 2
}

context.spa-libs = {
    audio.convert.* = audioconvert/libspa-audioconvert
    support.* = support/libspa-support
}

context.modules = [
    { name = libpipewire-module-rt flags = [ ifexists nofail ] }
    { name = libpipewire-module-protocol-native }
    { name = libpipewire-module-client-node }
    { name = libpipewire-module-adapter }
    { name = libpipewire-module-filter-chain
        args = {
            node.description = "Astro Manager %1"
            media.name = "Astro Manager %1"
            filter.graph = {
                nodes = [
%2
                ]
                links = [
%3
                ]
                inputs = [ %4 ]
                outputs = %5
            }
            capture.props = {
                node.name = "astro_spatial_sink"
                node.description = "Astro Manager %1"
                media.class = Audio/Sink
                node.virtual = true
                audio.rate = 48000
                audio.channels = %6
                audio.position = [ %7 ]
                stream.dont-remix = true
                node.latency = 256/48000
                resample.quality = 10
            }
            playback.props = {
                node.name = "astro_spatial_output"
                node.description = "Astro Manager to A50"
                node.passive = true
                target.object = %8
                audio.rate = 48000
                audio.channels = 2
                audio.position = [ FL FR ]
                node.latency = 256/48000
                resample.quality = 10
            }
        }
    }
]
)")
        .arg(profile.displayName,
             nodes.join(QLatin1Char('\n')),
             links.join(QLatin1Char('\n')),
             inputs.join(QLatin1Char(' ')),
             outputs)
        .arg(channels)
        .arg(channelList(profile), quoted(targetNode));
}

} // namespace AstroSpatial
