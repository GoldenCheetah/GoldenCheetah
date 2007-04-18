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

SUBDIRS=lib cpint cmd srm gui # order is important!

all: qmakefiles subdirs 
.PHONY: all qmakefiles subdirs clean

qmakefiles:
	@for dir in $(SUBDIRS); do cd $$dir; if test -f *.pro; then qmake; fi; cd ..; done

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -wC $$dir clean; done

subdirs:
	@for dir in $(SUBDIRS); do $(MAKE) -wC $$dir; done

