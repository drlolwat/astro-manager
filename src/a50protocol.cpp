#include "a50protocol.h"

namespace AstroSpatial {

QByteArray A50Protocol::encodeRequest(A50Command command, const QByteArray &payload)
{
    if (payload.size() > PacketSize - 3)
        return {};

    QByteArray request;
    request.reserve(3 + payload.size());
    request.append(char(0x02));
    request.append(char(static_cast<quint8>(command)));
    if (!payload.isEmpty()) {
        request.append(char(payload.size()));
        request.append(payload);
    }
    return request;
}

std::optional<QByteArray> A50Protocol::decodeResponse(const QByteArray &packet, QString *error)
{
    if (packet.size() < 3) {
        if (error) *error = QStringLiteral("The A50 returned a truncated response.");
        return std::nullopt;
    }
    if (quint8(packet[0]) != 0x02) {
        if (error) *error = QStringLiteral("The A50 returned an invalid response marker.");
        return std::nullopt;
    }
    const quint8 status = quint8(packet[1]);
    if (status == 0x01 || (status != 0x00 && status != 0x02)) {
        if (error) *error = QStringLiteral("The A50 rejected the control request (status %1).")
            .arg(status);
        return std::nullopt;
    }
    const int reportedLength = quint8(packet[2]);
    const int availableLength = packet.size() - 3;
    return packet.mid(3, qMin(reportedLength, availableLength));
}

} // namespace AstroSpatial
