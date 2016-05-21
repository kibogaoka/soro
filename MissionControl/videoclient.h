#ifndef VIDEOCLIENT_H
#define VIDEOCLIENT_H

#include <QObject>
#include <QUdpSocket>
#include <QDataStream>
#include <QByteArray>
#include <QList>

#include "soro_global.h"
#include "channel.h"
#include "socketaddress.h"
#include "mediaencoding.h"
#include "mediaclient.h"

namespace Soro {
namespace MissionControl {

class VideoClient : public MediaClient {
    Q_OBJECT

public:
    explicit VideoClient(int mediaId, SocketAddress server, QHostAddress host, Logger *log = 0, QObject *parent = 0);

    VideoFormat getVideoFormat() const;

private:
    VideoFormat _format;

protected:
    void onServerStreamingMessageInternal(QDataStream& stream) Q_DECL_OVERRIDE;
    void onServerStartMessageInternal() Q_DECL_OVERRIDE;
    void onServerEosMessageInternal() Q_DECL_OVERRIDE;
    void onServerErrorMessageInternal() Q_DECL_OVERRIDE;
    void onServerConnectedInternal() Q_DECL_OVERRIDE;
    void onServerDisconnectedInternal() Q_DECL_OVERRIDE;
};

} // namespace MissionControl
} // namespace Soro

#endif // VIDEOCLIENT_H
