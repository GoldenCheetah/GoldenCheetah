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


#ifndef _GC_SimpleNetworkController_h
#define _GC_SimpleNetworkController_h 1
#include "GoldenCheetah.h"

#include <QString>
#include <QDebug>

#include "RealtimeController.h"
#include "RealtimeData.h"
#include "SimpleNetworkClient.h"
#include "DeviceTypes.h"
#include "DeviceConfiguration.h"

// This class serves as our first simple cut as a realtime
// network device controller.  This class opens up a TCP connection
// to a server, and then starts pulling telemetry updates from a single
// peer via the server.

// This class currently uses a pull model.  As well, GC should push
// realtime data drawn from a local bike into this class, causing that
// data to be relayed to the server (and from there to the peer).

class SimpleNetworkController : public RealtimeController
{
 public:

  TrainTool *parent;

  // hostname and port are the hostname/port of the server to which
  // this SimpleNetworkControlller should connect.
  SimpleNetworkController(TrainTool *parent,
                          DeviceConfiguration *dc);
  ~SimpleNetworkController() { }

  // Connect to the server; blocks until connection finishes or fails.
  //
  // Returns 0 on success, non-zero on failure.
  int start();

  // Disconnect from the server; blocks until disconnect finishes or
  // fails.
  //
  // Returns 0 on successful disconnection, non-zero if the controller
  // wasn't disconnected to begin with.
  int stop();

  // If the controller is connected to the server and running, this
  // method causes the controller to "pause", i.e., to ignore updates
  // flowing from the server locally.
  //
  // Returns 0 if the pause took effect, non-zero if the pause isn't
  // meaningful (i.e., the controller isn't connected, or it's already
  // paused).
  int pause();

  // If the controller is connected and paused, this method causes the
  // controller to unpause and resume processing updates from the server.
  //
  // Returns 0 if the restart succeeds, non-zero otherwise.
  int restart();

  // XXX -- NOT SURE WHAT THIS METHOD IS FOR.  I'VE CURRENTLY STUBBED
  // IT OUT TO RETURN TRUE.
  bool discover(char *) {  return true;  }

  // The SimpleNetworkController is currently a pull mode controller.
  bool doesPush() {  return false; }
  bool doesPull() {  return true; }
  bool doesLoad() {  return false; }
  void setLoad(double) { return; }

  // When called, this method will fill in rtData with the latest
  // realtime data from the remote peer.
  //
  // XXX -- should probably have a return value so that the caller
  // can learn when the data source has unexpectedly disconnected.
  void getRealtimeData(RealtimeData &rtData);

  // When called, this method will push the realtime data in
  // rtData to the server.
  void pushRealtimeData(RealtimeData &rtData);

 private:
  SimpleNetworkClient client;
  enum {DISCONNECTED, RUNNING, PAUSED} state;
  RealtimeData data_cache;
};


#endif // _GC_SimpleNetworkController_h
