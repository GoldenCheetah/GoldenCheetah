/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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

#ifndef Gc_GcUpgrade_h
#define Gc_GcUpgrade_h
#include "GoldenCheetah.h"

// Build ID History
//
// 3001 - V3.0 RC1
// 3002 - V3.0 RC2
// 3003 - V3.0 RC3
// 3004 - V3.0 RC4 / 4X
// 3005 - V3.0 RC5 / 5X
// 3006 - V3.0 RC6
// 3007 - V3.0 RC7
// 3010 - V3.0 RELEASE (June 7 2013)
// 3020 - V3.0 SP1 RC1
// 3030 - V3.0 SP1 RELEASE (December 2013)
// 3032 - V3.0 SP2 RELEASE (March 2014)
// 3040 - V3.1 DEVELOPMENT
// 3050 - V3.1 RC1

#define VERSION3_BUILD 3010 // released
#define VERSION3_SP1   3030 // released
#define VERSION3_SP2   3032 // released

// these three will change until we release
#define VERSION31_BUILD 3050
#define VERSION_LATEST 3050
#define VERSION_STRING "V3.1 (RC1)"

class GcUpgrade
{
	public:
        GcUpgrade() {}
        int upgrade(const QDir &home);
        static int version() { return VERSION_LATEST; }
        static QString versionString() { return VERSION_STRING; }
        void removeIndex(QFile&);
};

#endif
