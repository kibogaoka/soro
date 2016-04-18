#include "roverworker.h"

#define LOG_TAG "Rover"

using namespace Soro::Rover;

RoverWorker::RoverWorker(QObject *parent) : QObject(parent) {
    _log = new Logger(this);
    _log->setLogfile(QCoreApplication::applicationDirPath() + "/rover_" + QDateTime::currentDateTime().toString("M-dd_h:mm:AP") + ".log");
    _log->RouteToQtLogger = true;
    LOG_I("-------------------------------------------------------");
    LOG_I("-------------------------------------------------------");
    LOG_I("-------------------------------------------------------");
    LOG_I("Starting up...");

    //Must initialize from the event loop
    START_TIMER(_initTimerId, 1);
}

void RoverWorker::timerEvent(QTimerEvent *e) {
    QObject::timerEvent(e);
    if (e->timerId() == _initTimerId) {
        KILL_TIMER(_initTimerId); //single shot

        QString err = QString::null;
        if (!_soroIniConfig.load(&err)) {
            LOG_E(err);
            exit(1); return;
        }
        _soroIniConfig.applyLogLevel(_log);
        LOG_I("Configuration has been loaded successfully");
        Channel::EndPoint commEndPoint =
                _soroIniConfig.ServerSide == SoroIniLoader::RoverEndPoint ?
                    Channel::ServerEndPoint : Channel::ClientEndPoint;

        //create network channels
        _armChannel = new Channel(this, SocketAddress(_soroIniConfig.ServerAddress, _soroIniConfig.ArmChannelPort), CHANNEL_NAME_ARM,
                                  Channel::UdpProtocol, commEndPoint, QHostAddress::Any, _log);
        _driveChannel = new Channel(this, SocketAddress(_soroIniConfig.ServerAddress, _soroIniConfig.DriveChannelPort), CHANNEL_NAME_DRIVE,
                                  Channel::UdpProtocol, commEndPoint, QHostAddress::Any, _log);
        _gimbalChannel = new Channel(this, SocketAddress(_soroIniConfig.ServerAddress, _soroIniConfig.GimbalChannelPort), CHANNEL_NAME_GIMBAL,
                                  Channel::UdpProtocol, commEndPoint, QHostAddress::Any, _log);
        _sharedChannel = new Channel(this, SocketAddress(_soroIniConfig.ServerAddress, _soroIniConfig.SharedChannelPort), CHANNEL_NAME_SHARED,
                                  Channel::TcpProtocol, commEndPoint, QHostAddress::Any, _log);

        if (_armChannel->getState() == Channel::ErrorState) {
            LOG_E("The arm channel experienced a fatal error during initialization");
            exit(1); return;
        }
        if (_driveChannel->getState() == Channel::ErrorState) {
            LOG_E("The drive channel experienced a fatal error during initialization");
            exit(1); return;
        }
        if (_gimbalChannel->getState() == Channel::ErrorState) {
            LOG_E("The gimbal channel experienced a fatal error during initialization");
            exit(1); return;
        }
        if (_sharedChannel->getState() == Channel::ErrorState) {
            LOG_E("The shared channel experienced a fatal error during initialization");
            exit(1); return;
        }
        LOG_I("All network channels initialized successfully");

        //create mbed channels
        _armControllerMbed = new MbedChannel(SocketAddress(QHostAddress::Any, _soroIniConfig.ArmMbedPort), MBED_ID_ARM, this, _log);
        _driveControllerMbed = new MbedChannel(SocketAddress(QHostAddress::Any, _soroIniConfig.DriveMbedPort), MBED_ID_DRIVE, this, _log);
        _gimbalControllerMbed = new MbedChannel(SocketAddress(QHostAddress::Any, _soroIniConfig.GimbalMbedPort), MBED_ID_GIMBAL, this, _log);

        //observers for network channels message received
        connect(_armChannel, SIGNAL(messageReceived(const char*, Channel::MessageSize)),
                 this, SLOT(armChannelMessageReceived(const char*, Channel::MessageSize)));
        connect(_driveChannel, SIGNAL(messageReceived(const char*, Channel::MessageSize)),
                 this, SLOT(driveChannelMessageReceived(const char*, Channel::MessageSize)));
        connect(_gimbalChannel, SIGNAL(messageReceived(const char*, Channel::MessageSize)),
                 this, SLOT(gimbalChannelMessageReceived(const char*, Channel::MessageSize)));
        connect(_sharedChannel, SIGNAL(messageReceived(const char*, Channel::MessageSize)),
                 this, SLOT(sharedChannelMessageReceived(const char*, Channel::MessageSize)));
        //observers for network connectivity changes
        connect(_armChannel, SIGNAL(messageReceived(const char*, Channel::MessageSize)),
                 this, SLOT(armChannelMessageReceived(const char*, Channel::MessageSize)));
        connect(_driveChannel, SIGNAL(messageReceived(const char*, Channel::MessageSize)),
                 this, SLOT(driveChannelMessageReceived(const char*, Channel::MessageSize)));
        connect(_gimbalChannel, SIGNAL(messageReceived(const char*, Channel::MessageSize)),
                 this, SLOT(gimbalChannelMessageReceived(const char*, Channel::MessageSize)));
        connect(_sharedChannel, SIGNAL(messageReceived(const char*, Channel::MessageSize)),
                 this, SLOT(sharedChannelMessageReceived(const char*, Channel::MessageSize)));

        _armChannel->open();
        _driveChannel->open();
        _gimbalChannel->open();
        _sharedChannel->open();

        _gpsServer = new GpsServer(this, SocketAddress(QHostAddress::Any, 5499), _log);

        LOG_I("Waiting for connections...");
    }
}

//observers for network channels message received

void RoverWorker::armChannelStateChanged(Channel::State state) {
    //TODO
}

void RoverWorker::driveChannelStateChanged(Channel::State state) {
    //TODO
}

void RoverWorker::gimbalChannelStateChanged(Channel::State state) {
    //TODO
}

void RoverWorker::sharedChannelStateChanged(Channel::State state) {
    if ((state == Channel::ConnectedState) && (_soroIniConfig.VideoServerAddress == QHostAddress::Null)) {
        //If the rover acts as a server, the only way we can deduce where to send the video stream is by waiting
        //for mission control to connect
        _soroIniConfig.VideoServerAddress = _sharedChannel->getPeerAddress().host;
    }
    //TODO
}

//observers for network channels message received

void RoverWorker::armChannelMessageReceived(const char *message, Channel::MessageSize size) {
    switch (message[0]) {
    case ArmMessage::Header_Gamepad:
    case ArmMessage::Header_Master:
        _armControllerMbed->sendMessage(message, (int)size);
        break;
    default:
        LOG_E("Received invalid message from mission control on arm control channel");
        break;
    }
}

void RoverWorker::driveChannelMessageReceived(const char *message, Channel::MessageSize size) {
    if (message[0] == DriveMessage::Header) {
        _driveControllerMbed->sendMessage(message, (int)size);
    }
    else {
        LOG_E("Received invalid message from mission control on drive control channel");
    }
}

void RoverWorker::gimbalChannelMessageReceived(const char *message, Channel::MessageSize size) {
    if (message[0] == GimbalMessage::Header) {
        _gimbalControllerMbed->sendMessage(message, (int)size);
    }
    else {
        LOG_E("Received invalid message from mission control on gimbal control channel");
    }
}

void RoverWorker::sharedChannelMessageReceived(const char *message, Channel::MessageSize size) {
    //TODO
}

RoverWorker::~RoverWorker() {
    if (_log != NULL) delete _log;
    if (_armChannel != NULL) delete _armChannel;
    if (_driveChannel != NULL) delete _driveChannel;
    if (_gimbalChannel != NULL) delete _gimbalChannel;
    if (_sharedChannel != NULL) delete _sharedChannel;
    if (_armControllerMbed != NULL) delete _armControllerMbed;
    if (_armControllerMbed != NULL) delete _driveControllerMbed;
    if (_armControllerMbed != NULL) delete _gimbalControllerMbed;
    if (_gpsServer != NULL) delete _gpsServer;
}
