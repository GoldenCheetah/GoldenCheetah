# 
# $Id: Makefile,v 1.11 2006/09/06 23:23:03 srhea Exp $
#
# Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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
#

CC=gcc
CFLAGS=-g -W -Wall -Werror -ansi -pedantic -std=c99
CMDS=ptdl ptpk ptunpk cpint
LIBS=../lib/libgc.a
LIBRARIES=-lm
INCLUDES=-I../lib

all: $(CMDS)
.PHONY: all clean

clean:
	rm -f *.o $(CMDS)

%.o: %.c
	$(CC) -c $(CFLAGS) $(INCLUDES) $< 

cpint: cpint.o $(LIBS)
	$(CXX) -o $@ $^ $(LIBRARIES)

ptdl: ptdl.o $(LIBS)
	$(CC) -o $@ $^ $(LIBRARIES)

ptunpk: ptunpk.o $(LIBS)
	$(CC) -o $@ $^ $(LIBRARIES)

ptpk: ptpk.o $(LIBS)
	$(CC) -o $@ $^ $(LIBRARIES)

ptdl.o: ../lib/pt.h
ptunpk.o: ../lib/pt.h

