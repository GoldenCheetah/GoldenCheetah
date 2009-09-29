/*
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _WkoRideFile_h
#define _WkoRideFile_h

#include <stdint.h>
#include "RideFile.h"
#include <QDataStream>
#include <string.h>
#include <math.h>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <QtEndian>

// Explicitly define types using unistd.h definitions to
// ensure char IS 8 bits and short IS 16 bits and long IS 32 bits
#define WKO_UCHAR   uint8_t
#define WKO_CHAR   int8_t
#define WKO_USHORT uint16_t
#define WKO_SHORT  int16_t
#define WKO_ULONG  uint32_t
#define WKO_LONG   int32_t
struct WkoFileReader : public RideFileReader {
    RideFile *openRideFile(QFile &file, QStringList &errors) const;

};

typedef std::auto_ptr<QDataStream> QDataStreamPtr;

WKO_UCHAR *WkoParseHeaderData(QString filename, WKO_UCHAR *data, RideFile *rideFile, QStringList &errors) ;
WKO_UCHAR *WkoParseRawData(WKO_UCHAR *data, RideFile *rideFile, QStringList &errors) ;

// Some Globals -- try and remove them as I refactor code from the original WKO2CSV source
QString WKO_HOMEDIR;

// UTILITY FUNCTIONS
// Field decoding
unsigned int doshort(WKO_UCHAR *p, WKO_USHORT *pnum);
unsigned int donumber(WKO_UCHAR *p, WKO_ULONG *pnum);
unsigned int dotext(WKO_UCHAR *p, WKO_UCHAR *txt);
unsigned int dofloat(WKO_UCHAR *p, float *pnum);
unsigned int dodouble(WKO_UCHAR *p, double *pnum);
unsigned int optpad(WKO_UCHAR *p),
             optpad2(WKO_UCHAR *p);

// Bit twiddlers
int get_bit(WKO_UCHAR *data, unsigned bitoffset); // returns 0 or 1
unsigned int get_bits(WKO_UCHAR* data, unsigned bitOffset, unsigned numBits); // returns 32 bit unsigned

// Decoding setup
void setxormasks();
WKO_ULONG nullvals(char graph);
unsigned long bitget(char *thelot, int offset, int count);
unsigned int bitsize(char graph, int device);

// different Chart segment types
enum configtype {
	CRIDESETTINGSCONFIG,
	CRIDEGOALCONFIG,
	CRIDENOTESCONFIG,
	CDISTRIBUTIONCHARTCONFIG,
	CRIDESUMMARYCONFIG,
	CMEANMAXCHARTCONFIG,
	CMEANMAXCHARTCACHE,
	CDISTRIBUTIONCHARTCACHE,
	OTHER,
	INVALID
};

#define	KMTOMI 0.6213
#define MTOFT  3.2808


#endif // _WkoRideFile_h

