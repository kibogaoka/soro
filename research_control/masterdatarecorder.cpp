#include "masterdatarecorder.h"
#include "libsoro/logger.h"

#define LOG_TAG "MasterDataRecorder"

namespace Soro {
namespace MissionControl {

MasterDataRecorder::MasterDataRecorder(const Channel* driveChannel, const Channel* sharedChannel, QObject *parent) : AbstractDataRecorder(LOG_TAG, parent) {
    connect(driveChannel, SIGNAL(stateChanged(Channel::State)),
            this, SLOT(driveChannelStateChanged(Channel::State)));
    connect(sharedChannel, SIGNAL(stateChanged(Channel::State)),
            this, SLOT(sharedChannelStateChanged(Channel::State)));
}

void MasterDataRecorder::driveChannelStateChanged(Channel::State state) {
    if (_fileStream) {
        addTimestamp();
        switch (state) {
        case Channel::ConnectedState:
            *_fileStream << Event_DriveChannelConnected;
            break;
        default:
            *_fileStream << Event_DriveChannelDisconnected;
            break;
        }
    }
}

void MasterDataRecorder::sharedChannelStateChanged(Channel::State state) {
    if (_fileStream) {
        addTimestamp();
        switch (state) {
        case Channel::ConnectedState:
            *_fileStream << Event_SharedChannelConnected;
            break;
        default:
            *_fileStream << Event_SharedChannelDisconnected;
            break;
        }
    }
}

void MasterDataRecorder::addComment(QString comment) {
    if (_fileStream) {
        addTimestamp();
        *_fileStream << Event_CommentEntered;
        *_fileStream << comment;
    }
}

} // namespace MissionControl
} // namespace Soro
