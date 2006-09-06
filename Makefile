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
CFLAGS=-g -W -Wall -Werror -ansi -pedantic
CXXFLAGS=-g -W -Wall -Werror

all: ptdl ptpk ptunpk cpint subdirs
.PHONY: all clean subdirs

clean:
	rm -f *.o ptdl ptpk ptunpk cpint
	$(MAKE) -wC gui clean

subdirs:
	@if test -d gui; \
	then \
		if ! test -f gui/Makefile; then cd gui; qmake; cd -; fi; \
		$(MAKE) -wC gui; \
	fi

cpint: cpint-cmd.o cpint.o pt.o
	$(CXX) -o $@ $^

ptdl: ptdl.o pt.o
	$(CC) -o $@ $^

ptunpk: ptunpk.o pt.o
	$(CC) -o $@ $^

ptpk: ptpk.o pt.o
	$(CC) -o $@ $^

ptdl.o: pt.h
ptunpk.o: pt.h

