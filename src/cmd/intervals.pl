#!/usr/bin/perl -w
# 
# $Id: intervals.pl,v 1.2 2006/08/11 19:53:50 srhea Exp $
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

my $interval = -1;
my $time_start = 0; 
my $time_end = 0; 
my $mile_start = 0; 
my $mile_end = 0; 
my $watts_max = 0; 
my $watts_sum = 0; 
my $watts_cnt = 0; 
my $hrate_max = 0; 
my $hrate_sum = 0; 
my $hrate_cnt = 0; 
my $caden_max = 0; 
my $caden_sum = 0; 
my $caden_cnt = 0; 
my $speed_max = 0; 
my $speed_sum = 0; 
my $speed_cnt = 0; 

sub sumarize {
    my $dur = $time_end - $time_start;
    my $len = $mile_end - $mile_start;
    my $minutes = int($dur);
    my $seconds = int(60 * ($dur - int($dur)));
    my $watts_avg = ($watts_cnt == 0) ? 0 : int($watts_sum / $watts_cnt);
    my $hrate_avg = ($hrate_cnt == 0) ? 0 : int($hrate_sum / $hrate_cnt);
    my $caden_avg = ($caden_cnt == 0) ? 0 : int($caden_sum / $caden_cnt);
    my $speed_avg = int($speed_sum / $speed_cnt);
    printf "%2d\t%2d:%02d\t%5.1f\t%4d\t%4d\t%3d\t%3d\t%3d\t%3d\t%0.1f\t%0.1f\n", $interval, $minutes, $seconds, $len, $watts_avg, $watts_max, $hrate_avg, $hrate_max, $caden_avg, $caden_max, $speed_avg, $speed_max;
    $watts_sum = 0;
    $watts_cnt = 0;
    $watts_max = 0;
    $hrate_max = 0; 
    $hrate_sum = 0; 
    $hrate_cnt = 0; 
    $caden_max = 0; 
    $caden_sum = 0; 
    $caden_cnt = 0; 
    $speed_max = 0; 
    $speed_sum = 0; 
    $speed_cnt = 0; 
}

print "\t\t\tPower\t\tHeart Rate\tCadence\t\tSpeed\n";
print "Int\t Dur\tDist\t Avg\t Max\tAvg\tMax\tAvg\tMax\tAvg\tMax\n";

while (<>) {
    if (m/^#/) {
    }
    else {
        my @cols = split;
        if ($#cols != 7) {
            print STDERR "Wrong number of columns: $_";
            exit 1;
        }
        my ($min, $torq, $speed, $watts, $miles, $caden, $hrate, $id) = @cols;
        if ($id != $interval) {
            if ($interval != -1) {
                sumarize();
            }
            $interval = $id;
            $time_start = $min;
            $mile_start = $miles;
        }
        $mile_end = $miles;
        $time_end = $min;
        if ($watts != "NaN") {
            $watts_sum += $watts;
            $watts_cnt += 1;
            if ($watts > $watts_max) { $watts_max = $watts; }
        }
        if ($hrate != "NaN") {
            $hrate_sum += $hrate;
            $hrate_cnt += 1;
            if ($hrate > $hrate_max) { $hrate_max = $hrate; }
        }
        if ($caden != "NaN") {
            $caden_sum += $caden;
            $caden_cnt += 1;
            if ($caden > $caden_max) { $caden_max = $caden; }
        }
        if ($speed != "NaN") {
            $speed_sum += $speed;
            $speed_cnt += 1;
            if ($speed > $speed_max) { $speed_max = $speed; }
        }
    }
}

sumarize();

