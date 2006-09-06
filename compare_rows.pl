#!/usr/bin/perl -w
#
# $Id: compare_rows.pl,v 1.1 2006/05/12 21:50:51 srhea Exp $
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

use strict;

if ($#ARGV < 1) {
    print STDERR "usage: compare_rows.pl <file 1> <file 2> [column]\n";
    exit 1;
}

my $file1 = $ARGV[0];
my $file2 = $ARGV[1];
my $start_col = 0;
my $stop_col = 8;
if ($#ARGV >= 2) {
    my $stop_col = $ARGV[2];
    my $start_col = $stop_col - 1;
}

open(FILE1, $file1) or die "Couldn't open $file1\n";
open(FILE2, $file2) or die "Couldn't open $file2\n";

my $row = 0;
while(<FILE1>) {
    ++$row;
    $_ =~ s/^\s+//;
    my @cols1 = split /\s+/, $_;
    my $row2 = <FILE2>;
    if ($row2) {
        my @cols2 = split /\s+/, $row2;
        my $col;
        for ($col = $start_col; $col < $stop_col; ++$col) {
            if (($cols1[$col] =~ m/[0-9.]+/) && ($cols2[$col] =~ m/[0-9.]+/)) {
                if ($col == 0) {
                    # Times are sometimes off by a bit; just make sure they're
                    # within a tenth of a percent.
                    if (($cols1[$col] * 0.999 > $cols2[$col]) 
                        || ($cols1[$col] * 1.001 < $cols2[$col])) {
                        print "$file1: @cols1\n";
                        print "$file2: @cols2\n";
                    }
                }
                else {
                    # All other columns should match exactly.
                    if ($cols1[$col] != $cols2[$col]) {
                        print "$file1: @cols1\n";
                        print "$file2: @cols2\n";
                    }
                }
            }
            elsif (!($cols1[$col] eq $cols2[$col])) {
                print "$file1: @cols1\n";
                print "$file2: @cols2\n";
            }
        }
    }
    else {
        print "$file1 is longer.\n";
    }
}

if (<FILE2>) {
    print "$file2 is longer.\n";
}

