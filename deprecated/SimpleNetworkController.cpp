/*
 * Copyright (c) 2009 Steve Gribble (gribble [at] cs.washington.edu) and
 *                    Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>

#include "SimpleNetworkController.h"
#include "RealtimeData.h"

SimpleNetworkController::SimpleNetworkController(TrainTool *parent,
                                                 DeviceConfiguration *dc)
  : RealtimeController(parent), parent(parent), client(parent, dc), state(DISCONNECTED)
{
}

int SimpleNetworkController::start() {
  if (state != DISCONNECTED) {
    // can't connect if I'm already connected, return failure
    return -1;
  }

  if (client.start()) {
    state = RUNNING;
    return 0;
  } else {
qDebug()<<"Client didn't start!";
  }
  return -1;
}

int SimpleNetworkController::stop() {
  if (state == DISCONNECTED) {
    // can't disconnected if I'm disconnected, return failure
    return -1;
  }
  client.closeAndExit();
  state = DISCONNECTED;
  return 0;
}

int SimpleNetworkController::pause() {
  if (state != RUNNING) {
    // can't pause if I'm not running
    return -1;
  }
  state = PAUSED;
  return 0;
}

int SimpleNetworkController::restart() {
  if (state != PAUSED) {
    // can't resume if I'm not paused
    return -1;
  }
  state = RUNNING;
  return 0;
}

void SimpleNetworkController::getRealtimeData(RealtimeData &rtData) {
  if (state == RUNNING) {

    // did the thread die?
    if(!client.isRunning())
    {
        QMessageBox msgBox;
        msgBox.setText("Cannot Connect to peer");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        parent->Stop(DEVICE_OK);
    }

    if (!client.getRealtimeData(data_cache)) {
      // client has disconnected since the last poll
      printf("getRealtimeData invoked, and client failed\n");
      this->stop();
    }
  }
  rtData = data_cache;
}

void SimpleNetworkController::pushRealtimeData(RealtimeData &rtData) {
  if (state == RUNNING) {
    if (!client.pushRealtimeData(rtData)) {
      // client has disconnected since the last push
      printf("pushRealtimeData invoked, and client failed\n");
      this->stop();
      return;
    }
  } else {
    qDebug()<<"Pushed but not running!";
  }
}
