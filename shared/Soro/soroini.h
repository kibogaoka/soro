#ifndef SOROINI_H
#define SOROINI_H

#include <QtCore>

#include "iniparser.h"
#include "logger.h"

#define SOROINI_PATH "config/soro.ini"
#define SOROINI_TAG_SERVER_ADDRESS "ServerAddress"
#define SOROINI_TAG_SERVER_SIDE "ServerSide"
#define SOROINI_TAG_ARM_SERVER_PORT "ArmChannelPort"
#define SOROINI_TAG_DRIVE_SERVER_PORT "DriveChannelPort"
#define SOROINI_TAG_GIMBAL_SERVER_PORT "GimbalChannelPort"
#define SOROINI_TAG_SHARED_SERVER_PORT "SharedChannelPort"
#define SOROINI_TAG_VIDEO_SERVER_ADDRESS "VideoServerAddress"
#define SOROINI_TAG_ARM_VIDEO_PORT "ArmVideoPort"
#define SOROINI_TAG_DRIVE_VIDEO_PORT "DriveVideoPort"
#define SOROINI_TAG_GIMBAL_VIDEO_PORT "GimbalVideoPort"
#define SOROINI_TAG_LOG_LEVEL "LogLevel"
#define SOROINI_VALUE_ROVER_SERVER "Rover"
#define SOROINI_VALUE_MC_SERVER "MissionControl"
#define SOROINI_VALUE_LOG_LEVEL_DEBUG "Debug"
#define SOROINI_VALUE_LOG_LEVEL_INFO "Information"
#define SOROINI_VALUE_LOG_LEVEL_WARN "Warning"
#define SOROINI_VALUE_LOG_LEVEL_ERROR "Error"
#define SOROINI_VALUE_LOG_LEVEL_DISABLED "Disabled"
#define SOROINI_TAG_ARM_MBED_PORT "ArmMbedPort"
#define SOROINI_TAG_DRIVE_MBED_PORT "DriveMbedPort"
#define SOROINI_TAG_GIMBAL_MBED_PORT "GimbalMbedPort"

namespace Soro {

/* Struct to contain and parse the configuration file that is common
 * to mission control and the rover (stuff like the server IP, which side should
 * act as server, ports, etc)
 */
struct SoroIniConfig {

    enum EndPoint {
        RoverEndPoint, MissionControlEndPoint
    };

    QHostAddress ServerAddress;
    QHostAddress VideoServerAddress;
    EndPoint ServerSide;
    int LogLevel;
    quint16 ArmChannelPort, DriveChannelPort, GimbalChannelPort, SharedChannelPort;
    quint16 ArmVideoPort, DriveVideoPort, GimbalVideoPort;
    quint16 ArmMbedPort, DriveMbedPort, GimbalMbedPort;

    /* Loads the configuration from the default path, and returns true if successful.
     * If there is an error, err will contain a summary of what went wrong.
     */
    bool load(QString *err) {
        QString appPath = QCoreApplication::applicationDirPath();
        IniParser configParser;
        QFile configFile(appPath + "/" + SOROINI_PATH);
        if (!configParser.load(configFile)) {
            *err = "The configuration file " + appPath + "/" + SOROINI_PATH + " is missing or invalid";
            return false;
        }

        int tmp;
        if (!configParser.valueAsIP(SOROINI_TAG_SERVER_ADDRESS, &ServerAddress, true)) {
            *err = "No server address found in configuration file";
            return false;
        }
        QString serverSide = configParser.value(SOROINI_TAG_SERVER_SIDE);
        if (QString::compare(serverSide, SOROINI_VALUE_ROVER_SERVER, Qt::CaseInsensitive) == 0) {
            ServerSide = RoverEndPoint;
            //we cannot know were to send the video when we act as the server.
            //wait for mission control to connect first, then send it to that address.
            VideoServerAddress = QHostAddress::Null;
        }
        else {
            //default to mission control as server
            ServerSide = MissionControlEndPoint;
            if (!configParser.valueAsIP(SOROINI_TAG_VIDEO_SERVER_ADDRESS, &VideoServerAddress, true)) {
                //assume video server address is same as main server address
                VideoServerAddress = ServerAddress;
            }
        }
        QString logLevel = configParser.value(SOROINI_TAG_LOG_LEVEL);
        if (QString::compare(logLevel, SOROINI_VALUE_LOG_LEVEL_DEBUG, Qt::CaseInsensitive) == 0) {
            LogLevel = LOG_LEVEL_DEBUG;
        }
        else if (QString::compare(logLevel, SOROINI_VALUE_LOG_LEVEL_WARN, Qt::CaseInsensitive) == 0) {
            LogLevel = LOG_LEVEL_WARN;
        }
        else if (QString::compare(logLevel, SOROINI_VALUE_LOG_LEVEL_ERROR, Qt::CaseInsensitive) == 0) {
            LogLevel = LOG_LEVEL_ERROR;
        }
        else if (QString::compare(logLevel, SOROINI_VALUE_LOG_LEVEL_DISABLED, Qt::CaseInsensitive) == 0) {
            LogLevel = LOG_LEVEL_DISABLED;
        }
        else {
            //default to info level
            LogLevel = LOG_LEVEL_INFORMATION;
        }
        if (!configParser.valueAsInt(SOROINI_TAG_ARM_SERVER_PORT, &tmp)) {
            *err = "No arm channel port found in configuration file";
            return false;
        }
        ArmChannelPort = tmp;
        if (!configParser.valueAsInt(SOROINI_TAG_DRIVE_SERVER_PORT, &tmp)) {
            *err = "No drive channel port found in configuration file";
            return false;
        }
        DriveChannelPort = tmp;
        if (!configParser.valueAsInt(SOROINI_TAG_GIMBAL_SERVER_PORT, &tmp)) {
            *err = "No gimbal channel port found in configuration file";
            return false;
        }
        GimbalChannelPort = tmp;
        if (!configParser.valueAsInt(SOROINI_TAG_SHARED_SERVER_PORT, &tmp)) {
            *err = "No shared channel port found in configuration file";
            return false;
        }
        SharedChannelPort = tmp;
        if (!configParser.valueAsInt(SOROINI_TAG_ARM_VIDEO_PORT, &tmp)) {
            *err = "No arm video port found in configuration file";
            return false;
        }
        ArmVideoPort = tmp;
        if (!configParser.valueAsInt(SOROINI_TAG_DRIVE_VIDEO_PORT, &tmp)) {
            *err = "No drive video port found in configuration file";
            return false;
        }
        DriveVideoPort = tmp;
        if (!configParser.valueAsInt(SOROINI_TAG_GIMBAL_VIDEO_PORT, &tmp)) {
            *err = "No gimbal video port found in configuration file";
            return false;
        }
        GimbalVideoPort = tmp;
        if (!configParser.valueAsInt(SOROINI_TAG_ARM_MBED_PORT, &tmp)) {
            *err = "No arm mbed port found in configuration file";
            return false;
        }
        ArmMbedPort = tmp;
        if (!configParser.valueAsInt(SOROINI_TAG_DRIVE_MBED_PORT, &tmp)) {
            *err = "No drive mbed port found in configuration file";
            return false;
        }
        DriveMbedPort = tmp;
        if (!configParser.valueAsInt(SOROINI_TAG_GIMBAL_MBED_PORT, &tmp)) {
            *err = "No gimbal mbed port found in configuration file";
            return false;
        }
        GimbalMbedPort = tmp;
        return true;
    }

    /* Configures a logger object based on the log level specified in the
     * config file. If it is set to LOG_LEVEL_DISABLED, this will delete &
     * null the logger!!!
     */
    void applyLogLevel(Logger*& log) {
        if (log == NULL) return;
        switch (LogLevel) {
        case LOG_LEVEL_DISABLED:
            log->i("SoroIniConfig", "The configuration file specifies to disable logging, goodbye!");
            delete log;
            log = NULL;
            break;
        case LOG_LEVEL_DEBUG:
            log->i("SoroIniConfig", "The maximum log level for this file is DEBUG");
            log->MaxLevel = LOG_LEVEL_DEBUG;
            break;
        case LOG_LEVEL_INFORMATION:
            log->i("SoroIniConfig", "The maximum log level for this file is INFORMATION");
            log->MaxLevel = LOG_LEVEL_INFORMATION;
            break;
        case LOG_LEVEL_WARN:
            log->i("SoroIniConfig", "The maximum log level for this file is WARNING");
            log->MaxLevel = LOG_LEVEL_WARN;
            break;
        case LOG_LEVEL_ERROR:
            log->i("SoroIniConfig", "The maximum log level for this file is ERROR");
            log->MaxLevel = LOG_LEVEL_ERROR;
            break;
        }
    }
};

}

#endif // SOROINI_H