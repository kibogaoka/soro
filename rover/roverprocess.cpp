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

#include "roverprocess.h"

#define LOG_TAG "Rover"

#define DEFAULT_AUDIO_DEVICE "hw:1"

namespace Soro {
namespace Rover {

RoverProcess::RoverProcess(QObject *parent) : QObject(parent) {

    // Must initialize once the event loop has started.
    // This can be accomplished using a single shot timer.
    QTimer::singleShot(1, this, SLOT(init()));
}

void RoverProcess::init() {
    LOG_I(LOG_TAG, "*****************Loading Configuration*******************");

    QString error;
    if (!_config.load(&error)) {
        LOG_E(LOG_TAG, error);
        QCoreApplication::exit(1);
        return;
    }

    LOG_I(LOG_TAG, "*************Initializing core networking*****************");

    _armChannel = Channel::createServer(this, NETWORK_ALL_ARM_CHANNEL_PORT, CHANNEL_NAME_ARM,
                              Channel::UdpProtocol, QHostAddress::Any);
    _driveChannel = Channel::createServer(this, NETWORK_ALL_DRIVE_CHANNEL_PORT, CHANNEL_NAME_DRIVE,
                              Channel::UdpProtocol, QHostAddress::Any);
    _gimbalChannel = Channel::createServer(this, NETWORK_ALL_GIMBAL_CHANNEL_PORT, CHANNEL_NAME_GIMBAL,
                              Channel::UdpProtocol, QHostAddress::Any);
    _sharedChannel = Channel::createServer(this, NETWORK_ALL_SHARED_CHANNEL_PORT, CHANNEL_NAME_SHARED,
                              Channel::TcpProtocol, QHostAddress::Any);
    _secondaryComputerChannel = Channel::createServer(this, NETWORK_ROVER_COMPUTER2_PORT, CHANNEL_NAME_SECONDARY_COMPUTER,
                                    Channel::TcpProtocol, QHostAddress::Any);

    if (_armChannel->getState() == Channel::ErrorState) {
        LOG_E(LOG_TAG, "The arm channel experienced a fatal error during initialization");
        exit(1); return;
    }
    if (_driveChannel->getState() == Channel::ErrorState) {
        LOG_E(LOG_TAG, "The drive channel experienced a fatal error during initialization");
        exit(1); return;
    }
    if (_gimbalChannel->getState() == Channel::ErrorState) {
        LOG_E(LOG_TAG, "The gimbal channel experienced a fatal error during initialization");
        exit(1); return;
    }
    if (_sharedChannel->getState() == Channel::ErrorState) {
        LOG_E(LOG_TAG, "The shared channel experienced a fatal error during initialization");
        exit(1); return;
    }

    _armChannel->open();
    _driveChannel->open();
    _gimbalChannel->open();
    _sharedChannel->open();
    _secondaryComputerChannel->open();

    // create the udp broadcast socket that will listen for the secondary computer
    _secondaryComputerBroadcastSocket = new QUdpSocket(this);

    // observers for network channel connectivity changes
    connect(_sharedChannel, &Channel::stateChanged, this, &RoverProcess::sharedChannelStateChanged);
    connect(_secondaryComputerChannel, &Channel::stateChanged, this, &RoverProcess::secondaryComputerStateChanged);
    connect(_secondaryComputerBroadcastSocket, &QUdpSocket::readyRead, this, &RoverProcess::secondaryComputerBroadcastSocketReadyRead);
    connect(_secondaryComputerBroadcastSocket, static_cast<void (QUdpSocket::*)(QUdpSocket::SocketError)>(&QUdpSocket::error), this, &RoverProcess::secondaryComputerBroadcastSocketError);


    beginSecondaryComputerListening();

    LOG_I(LOG_TAG, "All network channels initialized successfully");

    LOG_I(LOG_TAG, "*****************Initializing MBED systems*******************");

    // create mbed channels
    _armControllerMbed = new MbedChannel(SocketAddress(QHostAddress::Any, NETWORK_ROVER_ARM_MBED_PORT), MBED_ID_ARM, this);
    _driveGimbalControllerMbed = new MbedChannel(SocketAddress(QHostAddress::Any, NETWORK_ROVER_DRIVE_MBED_PORT), MBED_ID_DRIVE_CAMERA, this);

    // observers for mbed connectivity changes
    connect(_armControllerMbed, &MbedChannel::stateChanged, this, &RoverProcess::mbedChannelStateChanged);
    connect(_driveGimbalControllerMbed, &MbedChannel::stateChanged, this, &RoverProcess::mbedChannelStateChanged);

    // observers for network channels message received
    connect(_armChannel, &Channel::messageReceived, this, &RoverProcess::armChannelMessageReceived);
    connect(_driveChannel, &Channel::messageReceived, this, &RoverProcess::driveChannelMessageReceived);
    connect(_gimbalChannel, &Channel::messageReceived, this, &RoverProcess::gimbalChannelMessageReceived);
    connect(_sharedChannel, &Channel::messageReceived, this, &RoverProcess::sharedChannelMessageReceived);

    LOG_I(LOG_TAG, "*****************Initializing GPS system*******************");

    _gpsServer = new GpsServer(SocketAddress(QHostAddress::Any, NETWORK_ROVER_GPS_PORT), this);
    connect(_gpsServer, &GpsServer::gpsUpdate, this, &RoverProcess::gpsUpdate);

    LOG_I(LOG_TAG, "*****************Initializing Video system*******************");

    _videoServers = new VideoServerArray(this);
    _videoServers->populate(_config.getBlacklistedCameras(), NETWORK_ALL_CAMERA_PORT_1, 0);

    connect(_videoServers, &VideoServerArray::videoServerError, this, &RoverProcess::mediaServerError);

    if (_videoServers->serverCount() > _config.getComputer1CameraCount()) {
        LOG_E(LOG_TAG, "The configuration specifies less cameras than this, the last ones will be removed");
        while (_videoServers->serverCount() > _config.getComputer1CameraCount()) {
            _videoServers->remove(_videoServers->serverCount() - 1);
        }
    }
    else if (_videoServers->serverCount() < _config.getComputer1CameraCount()) {
        LOG_E(LOG_TAG, "The configuration specifies more cameras than this, check cable connections");
    }

    LOG_I(LOG_TAG, "*****************Initializing Audio system*******************");

    _audioServer = new AudioServer(MEDIAID_AUDIO, SocketAddress(QHostAddress::Any, NETWORK_ALL_AUDIO_PORT), this);
    connect(_audioServer, &AudioServer::error, this, &RoverProcess::mediaServerError);

    LOG_I(LOG_TAG, "-------------------------------------------------------");
    LOG_I(LOG_TAG, "-------------------------------------------------------");
    LOG_I(LOG_TAG, "-------------------------------------------------------");
    LOG_I(LOG_TAG, "Initialization complete");
    LOG_I(LOG_TAG, "-------------------------------------------------------");
    LOG_I(LOG_TAG, "-------------------------------------------------------");
    LOG_I(LOG_TAG, "-------------------------------------------------------");
}

void RoverProcess::beginSecondaryComputerListening() {
    if (_secondaryComputerBroadcastSocket) {
        _secondaryComputerBroadcastSocket->abort();
    }
    _secondaryComputerBroadcastSocket->bind(QHostAddress::Any, NETWORK_ROVER_COMPUTER2_PORT);
    _secondaryComputerBroadcastSocket->open(QIODevice::ReadWrite);
}

void RoverProcess::secondaryComputerBroadcastSocketError(QAbstractSocket::SocketError err) {
    Q_UNUSED(err);
    LOG_E(LOG_TAG, "Error on secondary computer broadcast socket: " + _secondaryComputerBroadcastSocket->errorString());
    QTimer::singleShot(500, this, SLOT(beginSecondaryComputerListening()));
}

void RoverProcess::secondaryComputerStateChanged(Channel::State state) {
    Q_UNUSED(state);
    sendSystemStatusMessage();
}

void RoverProcess::sharedChannelStateChanged(Channel::State state) {
    if (state == Channel::ConnectedState) {
        // send all status information since we just connected
        QTimer::singleShot(1000, this, SLOT(sendSystemStatusMessage()));
    }
}

void RoverProcess::mbedChannelStateChanged(MbedChannel::State state) {
    Q_UNUSED(state);
    sendSystemStatusMessage();
}

void RoverProcess::sendSystemStatusMessage() {
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    SharedMessageType messageType = SharedMessage_RoverStatusUpdate;
    bool armState = _armControllerMbed->getState() == MbedChannel::ConnectedState;
    bool driveGimbalState = _driveGimbalControllerMbed->getState() == MbedChannel::ConnectedState;
    bool secondaryComputerState = _secondaryComputerChannel->getState() == Channel::ConnectedState;

    stream << static_cast<qint32>(messageType);
    stream << armState;
    stream << driveGimbalState;
    stream << secondaryComputerState;
    _sharedChannel->sendMessage(message);
}

// observers for network channels message received

void RoverProcess::armChannelMessageReceived(const char *message, Channel::MessageSize size) {
    char header = message[0];
    MbedMessageType messageType;
    reinterpret_cast<qint32&>(messageType) = (qint32)reinterpret_cast<unsigned char&>(header);
    switch (messageType) {
    case MbedMessage_ArmGamepad:
    case MbedMessage_ArmMaster:
        _armControllerMbed->sendMessage(message, (int)size);
        break;
    default:
        LOG_E(LOG_TAG, "Received invalid message from mission control on arm control channel");
        break;
    }
}

void RoverProcess::driveChannelMessageReceived(const char *message, Channel::MessageSize size) {
    char header = message[0];
    MbedMessageType messageType;
    reinterpret_cast<qint32&>(messageType) = (qint32)reinterpret_cast<unsigned char&>(header);
    switch (messageType) {
    case MbedMessage_Drive:
        _driveGimbalControllerMbed->sendMessage(message, (int)size);
        break;
    default:
        LOG_E(LOG_TAG, "Received invalid message from mission control on drive control channel");
        break;
    }
}

void RoverProcess::gimbalChannelMessageReceived(const char *message, Channel::MessageSize size) {
    char header = message[0];
    MbedMessageType messageType;
    reinterpret_cast<qint32&>(messageType) = (qint32)reinterpret_cast<unsigned char&>(header);
    switch (messageType) {
    case MbedMessage_Gimbal:
        _driveGimbalControllerMbed->sendMessage(message, (int)size);
        break;
    default:
        LOG_E(LOG_TAG, "Received invalid message from mission control on gimbal control channel");
        break;
    }
}

void RoverProcess::sharedChannelMessageReceived(const char *message, Channel::MessageSize size) {
    QByteArray byteArray = QByteArray::fromRawData(message, size);
    QDataStream stream(byteArray);
    SharedMessageType messageType;

    stream >> reinterpret_cast<qint32&>(messageType);
    switch (messageType) {
    case SharedMessage_RequestActivateCamera: {
        qint32 camera;
        QString formatString;
        VideoFormat format;
        stream >> camera;
        stream >> formatString;
        format.deserialize(formatString);
        if (camera >= _config.getComputer1CameraCount()) {
            // this is the second odroid's camera
            QByteArray byteArray2;
            QDataStream stream2(&byteArray2, QIODevice::WriteOnly);
            stream2 << static_cast<qint32>(messageType);
            stream2 << camera;
            stream2 << format.serialize();
            _secondaryComputerChannel->sendMessage(byteArray2);
        }
        else {
            _videoServers->activate(camera, format);
        }
    }
        break;
    case SharedMessage_RequestDeactivateCamera:
        qint32 camera;
        stream >> camera;
        if (camera >= _config.getComputer1CameraCount()) {
            // this is the second odroid's camera
            QByteArray byteArray2;
            QDataStream stream2(&byteArray2, QIODevice::WriteOnly);
            stream2 << static_cast<qint32>(messageType);
            stream2 << camera;
            _secondaryComputerChannel->sendMessage(byteArray2);
        }
        else {
            _videoServers->deactivate(camera);
        }
        break;
    case SharedMessage_RequestActivateAudioStream: {
        AudioFormat format;
        stream >> reinterpret_cast<qint32&>(format);
        _audioServer->start(DEFAULT_AUDIO_DEVICE, format);
    }
        break;
    case SharedMessage_RequestDeactivateAudioStream:
        _audioServer->stop();
        break;
    default:
        LOG_W(LOG_TAG, "Got unknown shared channel message");
        break;
    }
}

void RoverProcess::secondaryComputerBroadcastSocketReadyRead() {
    char buffer[100];
    SocketAddress peer;
    while (_secondaryComputerBroadcastSocket->hasPendingDatagrams()) {
        int len = _secondaryComputerBroadcastSocket->readDatagram(&buffer[0], 100, &peer.host, &peer.port);
        if (strncmp(SECONDARY_COMPUTER_BROADCAST_STRING, buffer, len) == 0) {
            // secondary computer is broadcasting, respond
            LOG_I(LOG_TAG, "Getting broadcast from secondary computer");
            _secondaryComputerBroadcastSocket->writeDatagram(MASTER_COMPUTER_BROADCAST_STRING, strlen(MASTER_COMPUTER_BROADCAST_STRING) + 1, peer.host, peer.port);
        }
    }
}

void RoverProcess::mediaServerError(MediaServer *server, QString message) {
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    SharedMessageType messageType = SharedMessage_RoverMediaServerError;
    stream.setByteOrder(QDataStream::BigEndian);

    stream << static_cast<qint32>(messageType);
    stream << static_cast<qint32>(server->getMediaId());
    stream << message;

    _sharedChannel->sendMessage(byteArray);
}

void RoverProcess::gpsUpdate(NmeaMessage message) {
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    SharedMessageType messageType = SharedMessage_RoverGpsUpdate;
    stream.setByteOrder(QDataStream::BigEndian);

    stream << static_cast<qint32>(messageType);
    stream << message;

    _sharedChannel->sendMessage(byteArray);
}

RoverProcess::~RoverProcess() {

}

} // namespace Rover
} // namespace Soro
