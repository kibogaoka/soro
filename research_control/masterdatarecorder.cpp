/*
 * Copyright 2017 The University of Oklahoma.
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

#include "masterdatarecorder.h"
#include "libsoro/logger.h"

#define LOG_TAG "MasterDataRecorder"

namespace Soro {
namespace MissionControl {

MasterDataRecorder::MasterDataRecorder(const Channel* driveChannel, const Channel* sharedChannel, QObject *parent)
    : AbstractDataRecorder(LOG_TAG, "master", parent) {
    _driveChannel = driveChannel;
    _sharedChannel = sharedChannel;
    connect(_driveChannel, &Channel::stateChanged, this, &MasterDataRecorder::driveChannelStateChanged);
    connect(_sharedChannel, &Channel::stateChanged, this, &MasterDataRecorder::sharedChannelStateChanged);

    // Start the timer to poll latency
    START_TIMER(_latencyPollTimerId, 500);
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

void MasterDataRecorder::fakeLatencyUpdated(int latency) {
    if (_fileStream) {
        addTimestamp();
        *_fileStream << Event_FakeLatencyUpdate;
        *_fileStream << latency;
    }
}

void MasterDataRecorder::timerEvent(QTimerEvent *event) {
    if (_fileStream && (_driveChannel->getState() == Channel::ConnectedState) && (event->timerId() == _latencyPollTimerId)) {
        addTimestamp();
        *_fileStream << Event_RealLatencyUpdate;
        *_fileStream << _driveChannel->getLastRtt();
    }
}

} // namespace MissionControl
} // namespace Soro
