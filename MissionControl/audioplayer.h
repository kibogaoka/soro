#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>

#include <Qt5GStreamer/QGst/Ui/VideoWidget>
#include <Qt5GStreamer/QGst/Pipeline>
#include <Qt5GStreamer/QGst/Element>
#include <Qt5GStreamer/QGst/ElementFactory>
#include <Qt5GStreamer/QGst/Bin>
#include <Qt5GStreamer/QGst/Bus>
#include <Qt5GStreamer/QGlib/RefPointer>
#include <Qt5GStreamer/QGlib/Error>
#include <Qt5GStreamer/QGlib/Connect>
#include <Qt5GStreamer/QGst/Message>

#include "socketaddress.h"
#include "soro_global.h"

namespace Soro {
namespace MissionControl {

class AudioPlayer : public QObject
{
    Q_OBJECT
public:
    explicit AudioPlayer(QObject *parent = 0);
    ~AudioPlayer();

    void play(SocketAddress address, AudioFormat encoding);
    void stop();
    bool isPlaying();

private:
    QGst::PipelinePtr _pipeline;
    bool _isPlaying = false;
    void resetPipeline();

private slots:
    /* Recieves messages from the gstreamer pipeline bus
     */
    void onBusMessage(const QGst::MessagePtr & message);

signals:
    /* Emitted when the player receives an end-of-stream message
     */
    void eosMessage();

    /* Emitted when the player encounters a playback error
     */
    void error();

};

}
}

#endif // AUDIOPLAYER_H
