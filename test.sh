#!/bin/sh
#
# $Id: test.sh,v 1.2 2006/05/14 14:17:21 srhea Exp $
#
# Copyright (c) 2006 Russ Cox (rsc@swtch.com) and Sean Rhea (srhea@srhea.net)
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

grep 'Rider' $1.xml | sed -e 's/<[^>]*>/ /g' > win      
if ! ./ptunpk -c < $1.raw | grep -v '^#' > mac
then
    echo "Failed to unpack $1.raw";
    exit 1
fi
./compare_rows.pl win mac $2
rm win mac

