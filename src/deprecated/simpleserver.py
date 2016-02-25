#!/usr/bin/env python

# simpleserver.py
#
# Acts as a simple server for coupling two golden cheetah peers together
# usage:  ./simpleserver.py listen_port
#
# Copyright (c) 2009 Steve Gribble (gribble [at] cs.washington.edu) and
#                    Mark Liversedge (liversedge@gmail.com)
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc., 51
# Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

import socket
import threading
import SocketServer
import sys

MAXPEERS = 4096

# this class keeps track of the state of a peer, including the socket
# used to talk to it, it's name, and a queue of records waiting to be
# written into it.
class PeerTracker():
    def __init__(self, name):
        self.writequeue = [ ]   # list of records to be sent TO peer
        self.connected = False
        self.peername = name

    # scrub my state when the peer disconnects
    def clean(self):
        self.writequeue = [ ]
        self.connected = False
        self.socket = None

# The main "threaded server" class.  Has a condition variable and a
# dict of connected peers.
class CheetahServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    def init_cheetah(self, maxpeers):
        self.cond = threading.Condition()   # used to coordinate client lists
        self.peers = { }  # a dict of peers
        for i in range(0, maxpeers):
            self.peers[i] = PeerTracker("Peer " + str(i))

# A new ThreadedCheetahHandler is instantiated for each incoming connection,
# and a new thread is dispatched to handle() to service that connection.
class ThreadedCheetahHandler(SocketServer.BaseRequestHandler):
    # if something goes wrong with this connection, removepeer is called
    # to clean up out of the server list.
    def removepeer(self):
        self.server.cond.acquire()
        print "(" + self.me_peer.peername + ")", \
            "bogus or broken input, dropping.\n"
        self.me_peer.clean()
        self.server.cond.release()

    # a line of data was received; this method is invoked to parse it
    # and route it to the appropriate peer queue. if parsing fails, the
    # peer is killed off.  if parsing succeeds but only one peer is
    # connected, the record is dropped but the peer stays up.
    def processinput(self, data):
        # make sure data has 7 fields. if not, drop peer.
        components = data.split()
        if (len(components) != 7):
            self.removepeer()
            return True
        # try to parse the fields.  if fail, drop peer.
        try:
            valdict = { }
            valdict['peername'] = components[0]
            valdict['watts'] = float(components[1])
            valdict['hr'] = float(components[2])
            valdict['time'] = int(components[3])
            valdict['speed'] = float(components[4])
            valdict['rpm'] = float(components[5])
            valdict['load'] = float(components[6])

            # parse succeeded, so add to peer's input queue
            print "(" + self.me_peer.peername + ")", \
                "well-formed record arrived..."
            print "  ... name:%s watts:%.2f hr:%.2f time:%ld speed:%.2f rpm:%.2f load:%.3f" % \
                (valdict['peername'], valdict['watts'], valdict['hr'], valdict['time'], \
                     valdict['speed'], valdict['rpm'], valdict['load'])
            self.server.cond.acquire()
            numqueued = 0
            for i in range(0, len(self.server.peers)):
                if (not (self.server.peers[i] == self.me_peer)) and (self.server.peers[i].connected):
                    self.server.peers[i].writequeue.append(valdict)
                    print "  ...queued for", self.server.peers[i].peername + "\n"
                    numqueued = numqueued + 1
            if numqueued > 0:
                # notify writing thread that there is work to do
                self.server.cond.notify()
            else:
                print "  ...but no other peers are connected,", \
                    "so dropped it.\n"
            self.server.cond.release()
        except ValueError:
            # couldn't parse line, so drop connection
            self.removepeer()
            return True

        # finished with record, but not done with getting input
        # records from peer, so return and tell caller we're not
        # done yet
        return False

    # this method gets invoked by the web server when a new peer
    # connects to it.  we check to see if we have room.  if so,
    # we add the peer, then start processing input.
    def handle(self):
        # New connection arrived.
        print "(server) new incoming connection..."
        self.server.cond.acquire()
        # If the server is full, bonk out, else add.
        addedpeer = False
        for i in range(0, len(self.server.peers)):
            if (not addedpeer) and (self.server.peers[i].connected == False):
                # We have room.  Create record for new peer in slot i
                self.me_peer = self.server.peers[i]
                self.me_peer.connected = True
                self.me_peer.socket = self.request
                print "  ...connected " + self.me_peer.peername + "\n"
                addedpeer = True
                done = False
        if addedpeer == False:
            # Server is full, so bonk out.
            print "  ...but server already has max peers, so dropping.\n"
            done = True
        self.server.cond.release()

        # while this peer is connected, process requests
        socketfile = self.request.makefile()
        while not done:
            try:
                data = socketfile.readline()
                print '[debug][' + self.me_peer.peername + '] got: \"' + data + '\"\n'
                done = self.processinput(data)
            except socket.error, msg:
                self.removepeer()
                break;

# this utility method is used by thread_write to actually write a
# record into a peer's socket.  if the write fails, we ignore for now,
# and rely on the read on the socket to fail and clean up later.
def write_to_peer(item, peer):
    s = "%s %.2f %.2f %d %.2f %.2f %.3f\n" % \
        (item['peername'], item['watts'], item['hr'], item['time'], \
             item['speed'], item['rpm'], item['load'])
    try:
        peer.socket.sendall(s)
    except socket.error, msg:
        pass

# we spawn a thread to service writes -- when woken up by a signal on
# the server condition variable, the writer thread checks the peers'
# queues for work, and if it finds it, writes into the peer's socket.
# the writer thread finishes writing before it relinquishes the lock
# by waiting.  this thread is a daemon, so we don't have to worry
# about making it exit on error.
def thread_write(server):
    print "(writer) awake and waiting for business\n"
    server.cond.acquire()

    # spin forever waiting for work
    while True:
        server.cond.wait()
        # see if the first peer has some work
        for peer in server.peers.itervalues():
            while len(peer.writequeue) > 0:
                next = peer.writequeue.pop(0)
                write_to_peer(next, peer)
    server.cond.release()

# main() invokes this to spawn the server and the writer thread.
def run_server(port, maxpeers):
    # initialize the server
    server = CheetahServer(("", port), ThreadedCheetahHandler)
    server.allow_reuse_address = True
    ip, port = server.server_address
    print "(server) running in thread:", \
        threading.currentThread().getName() + "\n"
    server.init_cheetah(maxpeers)

    # fire up the writer thread
    writer_thread = threading.Thread(target=thread_write, args=(server,))
    writer_thread.daemon = True
    writer_thread.start()

    # open for business and get incoming connections
    server.serve_forever()


###############################
# everything below here is roughly equivalent to "main()" from C
###############################

def usage():
    print "usage: ./simpleserver.py listen_port max_num_peers"
    print "   where 2 <= maxpeers < " + str(MAXPEERS) + "\n"
    sys.exit(1)   # failure

def main(argv):
    # validate arguments
    if (len(argv) != 2):
        usage()
    try:
        port = int(argv[0])
        maxpeers = int(argv[1])
    except ValueError:
        usage()
    if ((port < 1) or (port > 65535) or (maxpeers < 2) or (maxpeers > MAXPEERS)):
        usage()

    # great!
    run_server(port, maxpeers)

if __name__ == "__main__":
    main(sys.argv[1:])
