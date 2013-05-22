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

// Version management was introduced in V3.0 RC3
// We expect number series to be along the lines of;
//
// 3001 - V3 RC1
// 3002 - V3 RC2
// 3003 - V3 RC3
// 3004 - V3 RC4 / 4X
// 3005 - V3 RC5 / 5X
// 3010 - V3.0 Full Release
// 3011 - V3.0 SP1
// 301n - V3.0 SPn
// 3101 - V3.1 RC1
// 310n - V3.1 RCn
//

#define VERSION_LATEST 3005
#define VERSION_STRING "V3.0 RC5X"

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
