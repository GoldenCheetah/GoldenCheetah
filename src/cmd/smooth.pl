#!/usr/bin/perl -w
# 
# $Id: smooth.pl,v 1.1 2006/05/16 14:24:50 srhea Exp $
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

use strict;

if ($#ARGV < 0) {
    print "usage: smooth.pl <window length in seconds>\n";
    exit 1;
}

my $len = $ARGV[0] / 60.0;
my $count = 0;
my $total = 0;
my $start_time = 0;

while (<STDIN>) {
    if (m/^#/) {
    }
    else {
        my @cols = split;
        if (!($cols[1] eq "NaN")) {
            ++$count;
            $total += $cols[1];
        }
        if ($cols[0] >= $start_time + $len) {
            if ($count > 0) {
                printf "$start_time %f\n", $total / $count;
            }
            $start_time = $cols[0];
            $total = 0; 
            $count = 0;
        }
    }
}

