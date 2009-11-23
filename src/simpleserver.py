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

# this class keeps track of the state of a connected peer, including
# the socket used to talk to it, it's name, and a queue of records waiting
# to be written into it.
class PeerTracker():
    def __init__(self):
        self.writequeue = [ ]   # list of records to be sent TO peer

# The main "threaded web server" class.  Has a condition variable and
# a list of connected peers.
class CheetahServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    def init_cheetah(self):
        self.cond = threading.Condition()   # used to coordinate client lists
        self.peers = [ ]  # a list of connected peers

# A new ThreadedCheetahHandler is instantiated for each incoming connection,
# and a new thread is dispatched to handle() to service that connection.
class ThreadedCheetahHandler(SocketServer.BaseRequestHandler):
    # if something goes wrong with this connection, removepeer is called
    # to clean up out of the server list.
    def removepeer(self):
        self.server.cond.acquire()
        print "(" + self.me_peer.peername + ")", \
            "bogus or broken input, dropping.\n"
        self.server.peers.remove(self.me_peer)
        self.server.cond.release()

    # a line of data was received; this method is invoked to parse it
    # and route it to the appropriate peer queue. if parsing fails, the
    # peer is killed off.  if parsing succeeds but only one peer is
    # connected, the record is dropped but the peer stays up.
    def processinput(self, data):
        # make sure data has 6 fields. if not, drop peer.
        components = data.split()
        if (len(components) != 6):
            self.removepeer()
            return True
        # try to parse the fields.  if fail, drop peer.
        try:
            valdict = { }
            valdict['watts'] = float(components[0])
            valdict['hr'] = float(components[1])
            valdict['time'] = int(components[2])
            valdict['speed'] = float(components[3])
            valdict['rpm'] = float(components[4])
            valdict['load'] = float(components[5])

            # parse succeeded, so add to peer's input queue
            print "(" + self.me_peer.peername + ")", \
                "well-formed record arrived..."
            print "  ... watts:%.2f hr:%.2f time:%ld speed:%.2f rpm:%.2f load:%.3f" % \
                (valdict['watts'], valdict['hr'], valdict['time'], \
                     valdict['speed'], valdict['rpm'], valdict['load'])
            self.server.cond.acquire()
            if (len(self.server.peers) == 2):
                if (self.server.peers[0] == self.me_peer):
                    otherpeer = self.server.peers[1]
                else:
                    otherpeer = self.server.peers[0]
                otherpeer.writequeue.append(valdict)

                # notify writing thread that there is work to do
                self.server.cond.notify()
                print "  ...queued for", otherpeer.peername + "\n"
            else:
                print "  ...but other peer not connected,", \
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
        if (len(self.server.peers) < 2):
            # We have room.  Create record for new peer.
            self.me_peer = PeerTracker()
            self.me_peer.socket = self.request
            if (len(self.server.peers) == 0):
                self.me_peer.peername = "Peer A"
            elif (self.server.peers[0].peername == "Peer A"):
                self.me_peer.peername = "Peer B"
            else:
                self.me_peer.peername = "Peer A"
            self.server.peers.append(self.me_peer)
            print "  ...added peer #", str(len(self.server.peers)), \
                "named \"" + self.me_peer.peername + "\"\n"
            done = False
        else:
            # Server is full, so bonk out.
            print "  ...but server already has 2 peers, so dropping.\n"
            done = True
        self.server.cond.release()

        # while this peer is connected, process requests
        socketfile = self.request.makefile()
        while not done:
            try:
                data = socketfile.readline()
                done = self.processinput(data)
            except socket.error, msg:
                self.removepeer()
                break;

# this utility method is used by thread_write to actually write a
# record into a peer's socket.  if the write fails, we ignore for now,
# and rely on the read on the socket to fail and clean up later.
def write_to_peer(item, peer):
    s = "%.2f %.2f %d %.2f %.2f %.3f\n" % \
        (item['watts'], item['hr'], item['time'], \
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
        for peer in server.peers:
            while len(peer.writequeue) > 0:
                next = peer.writequeue.pop(0)
                write_to_peer(next, peer)
    server.cond.release()

# main() invokes this to spawn the web server and the writer thread.
def run_server(port):
    # initialize the web server
    server = CheetahServer(("", port), ThreadedCheetahHandler)
    server.allow_reuse_address = True
    ip, port = server.server_address
    print "(server) running in thread:", \
        threading.currentThread().getName() + "\n"
    server.init_cheetah()

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
    print "usage: ./simpleserver.py listen_port"
    sys.exit(1)   # failure

def main(argv):
    # validate arguments
    if (len(argv) != 1):
        usage()
    try:
        port = int(argv[0])
    except ValueError:
        usage()
    if ((port < 1) or (port > 65535)):
        usage()

    # great!
    run_server(port)

if __name__ == "__main__":
    main(sys.argv[1:])
