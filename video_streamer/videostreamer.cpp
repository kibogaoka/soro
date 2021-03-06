/*
 * Copyright 2016 The University of Oklahoma.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "videostreamer.h"
#include "libsoro/logger.h"
#include "libsoro/constants.h"

namespace Soro {
namespace Rover {

VideoStreamer::VideoStreamer(QGst::ElementPtr source, VideoFormat format, SocketAddress bindAddress, SocketAddress address, quint16 ipcPort, QObject *parent)
        : Soro::Gst::MediaStreamer("VideoStreamer", parent) {
    if (!connectToParent(ipcPort)) return;

    LOG_I(LOG_TAG, "Creating pipeline");
    _pipeline = createPipeline();

    // create gstreamer command
    QString binStr = "%1 ! udpsink bind-address=%2 bind-port=%3 host=%4 port=%5";
    binStr = binStr.arg(format.createGstEncodingArgs(),
                        bindAddress.host.toString(),
                        QString::number(bindAddress.port),
                        address.host.toString(),
                        QString::number(address.port));

    QGst::BinPtr encoder = QGst::Bin::fromDescription(binStr);

    LOG_I(LOG_TAG, "Created gstreamer bin <source> ! " + binStr);

    // link elements
    _pipeline->add(source, encoder);
    source->link(encoder);

    LOG_I(LOG_TAG, "Elements linked on pipeline");

    // play<source> ! " +
    _pipeline->setState(QGst::StatePlaying);

    LOG_I(LOG_TAG, "Stream started");
}

VideoStreamer::VideoStreamer(QString sourceDevice, VideoFormat format, SocketAddress bindAddress, SocketAddress address, quint16 ipcPort, QObject *parent)
        : Soro::Gst::MediaStreamer("VideoStreamer", parent) {
    if (!connectToParent(ipcPort)) return;

     LOG_I(LOG_TAG, "Creating pipeline");
    _pipeline = createPipeline();

    // create gstreamer command
    QString binStr = "v4l2src device=%1 ! %2 ! udpsink bind-address=%3 bind-port=%4 host=%5 port=%6";
    binStr = binStr.arg(sourceDevice,
                        format.createGstEncodingArgs(),
                        bindAddress.host.toString(),
                        QString::number(bindAddress.port),
                        address.host.toString(),
                        QString::number(address.port));

    QGst::BinPtr encoder = QGst::Bin::fromDescription(binStr);

    LOG_I(LOG_TAG, "Created gstreamer bin " + binStr);

    _pipeline->add(encoder);

    LOG_I(LOG_TAG, "Elements linked on pipeline");

    // play
    _pipeline->setState(QGst::StatePlaying);

    LOG_I(LOG_TAG, "Stream started");

}

} // namespace Rover
} // namespace Soro

