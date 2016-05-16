#ifndef SORO_GLOBAL_H
#define SORO_GLOBAL_H

#include <climits>

#ifdef QT_CORE_LIB ///////////////////////////////////////////////////////////////////////////////

//QObject timer macros to make shit easier
#define TIMER_INACTIVE -1
#define START_TIMER(X,Y) if (X == TIMER_INACTIVE) X = startTimer(Y)
#define KILL_TIMER(X) if (X != TIMER_INACTIVE) { killTimer(X); X = TIMER_INACTIVE; }

//channel names, must be the same on the rover and mission control builds
#define CHANNEL_NAME_ARM "Soro_ArmChannel"
#define CHANNEL_NAME_DRIVE "Soro_DriveChannel"
#define CHANNEL_NAME_GIMBAL "Soro_GimbalChannel"
#define CHANNEL_NAME_SHARED "Soro_SharedTcpChannel"
#define CHANNEL_NAME_SECONDARY_COMPUTER "Soro_SecondaryComputerChannel"

#define STREAMPROCESS_ERR_NOT_ENOUGH_ARGUMENTS 91
#define STREAMPROCESS_ERR_INVALID_ARGUMENT 92
#define STREAMPROCESS_ERR_GSTREAMER_EOS 93
#define STREAMPROCESS_ERR_GSTREAMER_ERROR 94
#define STREAMPROCESS_ERR_UNKNOWN_CODEC 95
#define STREAMPROCESS_ERR_FLYCAP_ERROR 96
#define STREAMPROCESS_ERR_SOCKET_ERROR 97

#define STREAMPROCESS_IPC_START 's'
#define STREAMPROCESS_IPC_STREAMING 'v'
#define STREAMPROCESS_IPC_EXIT 'e'

enum SharedMessageType {
    SharedMessage_RoverSharedChannelStateChanged = 1,
    SharedMessage_RoverStatusUpdate,
    SharedMessage_RoverDisconnected,
    SharedMessage_RoverGpsUpdate,
    SharedMessage_MissionControlConnected,
    SharedMessage_MissionControlDisconnected,
    SharedMessage_RequestActivateCamera,
    SharedMessage_RequestDeactivateCamera,
    SharedMessage_RoverVideoServerError,
    SharedMessage_MissionControlChat,
    SharedMessage_CameraChanged,
    SharedMessage_BitrateUpdate
};

enum RoverSubsystemState {
    NormalSubsystemState, MalfunctionSubsystemState, UnknownSubsystemState
};

enum RoverCameraState {
    StreamingCameraState, DisabledCameraState, UnavailableCameraState
};

enum DriveGamepadMode {
    SingleStickDrive, DualStickDrive
};

enum Role {
    ArmOperatorRole, DriverRole, CameraOperatorRole, SpectatorRole
};

enum NotificationType {
    RoverNotification, MCCNotification, ChatNotification
};

#ifdef QT_WIDGETS_LIB ///////////////////////////////////////////////////////////////////////////////

#include <QWidget>
#include <QGraphicsDropShadowEffect>

namespace Soro {

inline void addWidgetShadow(QWidget *target, int radius, int offset) {
    QGraphicsDropShadowEffect* ef = new QGraphicsDropShadowEffect;
    ef->setBlurRadius(radius);
    ef->setColor(QColor::fromHsv(0, 0, 0, 255));
    ef->setOffset(offset);
    target->setGraphicsEffect(ef);
}

}

#endif // QT_WIDGETS_LIB ///////////////////////////////////////////////////////////////////////////////

#endif // QT_CORE_LIB ///////////////////////////////////////////////////////////////////////////////

#define MBED_ID_MASTER_ARM 1
#define MBED_ID_ARM 2
#define MBED_ID_DRIVE_CAMERA 3

namespace Soro {

template <typename T>
inline void serialize(char *arr, T data) {
    int max = sizeof(T) - 1;
    for (int i = 0; i <= max; i++) {
        arr[max - i] = (data >> (i * 8)) & 0xFF;
    }
}

template <typename T>
inline T deserialize(const char *arr) {
    T result;
    for (unsigned int i = 0; i < sizeof(T); i++) {
        result = (result << 8) + reinterpret_cast<const unsigned char&>(arr[i]);
    }
    return result;
}

/* Converts a 16 bit signed integer ranging from -32768 to 32767
 * into an unsigned char, ranging from 0 to 200
 *
 * This is designed for converting SDL axis values.
 */
inline char axisShortToAxisByte(short val) {
    val /= (SHRT_MAX / 100);
    val += 100;
    unsigned char uc = (unsigned char)val;
    return reinterpret_cast<char&>(uc);
}

/* Converts a float ranging from -1 to 1 into an unsigned char,
 * ranging from 0 to 200
 *
 * This is designed for converting GLFW axis values.
 */
inline char axisFloatToAxisByte(float val) {
    val = (val + 1) * 100;
    unsigned char uc = (unsigned char)val;
    return reinterpret_cast<char&>(uc);
}

/* Converts an int ranging from -100 to 100 into an unsigned char,
 * ranging from 0 to 200
 */
inline char axisIntToAxisByte(int val) {
    val += 100;
    unsigned char uc = (unsigned char)val;
    return reinterpret_cast<char&>(uc);
}

/* Converts an int ranging from -100 to 100 into an unsigned char,
 * ranging from 0 to 200
 */
inline int axisFloatToAxisInt(float val) {
    return (int)(val * 100);
}

/* Converts a byte encoded joystick axis (see joyFloatToByte) back
 * into it's original float value
 */
inline float axisByteToAxisFloat(char val) {
    return (float)((int)reinterpret_cast<unsigned char&>(val) - 100.0) / 100.0;
}

/* Converts a byte encoded joystick axis (see joyFloatToByte) into
 * an int ranging from -100 to 100
 */
inline int axisByteToAxisInt(char val) {
    return (int)reinterpret_cast<unsigned char&>(val) - 100;
}

inline float axisShortToAxisFloat(short val) {
    return (float)val / (float)SHRT_MAX;
}

}

#endif // SORO_GLOBAL_H
