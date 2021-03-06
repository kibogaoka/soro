#ifndef SORO_MISSIONCONTROL_DRIVECONTROLSYSTEM_H
#define SORO_MISSIONCONTROL_DRIVECONTROLSYSTEM_H

#include <QObject>
#include <QTimerEvent>

#include "libsoro/channel.h"
#include "libsoro/logger.h"
#include "libsoro/drivemessage.h"
#include "libsoro/constants.h"
#include "libsoro/enums.h"

#include "soro_missioncontrol_global.h"
#include "gamepadmanager.h"
#include "controlsystem.h"

namespace Soro {
namespace MissionControl {

class LIBSOROMC_EXPORT DriveControlSystem : public ControlSystem {
    Q_OBJECT
public:
    explicit DriveControlSystem(const QHostAddress& roverAddress, GamepadManager *input, QObject *parent = 0);
    bool init(QString *errorString);
    void enable();
    void disable();
    void setMode(DriveGamepadMode mode);
    DriveGamepadMode getMode() const;
    void setMiddleSkidFactor(float factor);
    float getMiddleSkidFactor() const;

protected:
    void timerEvent(QTimerEvent *e);

signals:
    void driveMessageSent(const char *message, int size);

private:
    DriveGamepadMode _mode = DualStickDrive;
    int _controlSendTimerId = TIMER_INACTIVE;
    float _midSkidFactor = 0.2; //The higher this is, the slower the middle wheels turn while skid steering
    const GamepadManager *_input = nullptr;
    char _buffer[256];
};

}
}

#endif // SORO_MISSIONCONTROL_DRIVECONTROLSYSTEM_H
