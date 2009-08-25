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

#include "RideFile.h"
#include <QDataStream>
#include <string.h>
#include <math.h>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>

struct WkoFileReader : public RideFileReader {
    RideFile *openRideFile(QFile &file, QStringList &errors) const;

};

typedef std::auto_ptr<QDataStream> QDataStreamPtr;

unsigned char *WkoParseHeaderData(unsigned char *data, RideFile *rideFile) ;
unsigned char *WkoParseRawData(unsigned char *data, RideFile *rideFile) ;

// Some Globals -- try and remove them as I refactor code from the original WKO2CSV source
QString WKO_HOMEDIR;

// UTILITY FUNCTIONS
// Field decoding
unsigned int doshort(unsigned char *p, unsigned short *pnum);
unsigned int donumber(unsigned char *p, unsigned long *pnum);
unsigned int dotext(unsigned char *p, unsigned char *txt);unsigned int dofloat(unsigned char *p, float *pnum);
unsigned int dodouble(unsigned char *p, double *pnum);
unsigned int optpad(unsigned char *p),
             optpad2(unsigned char *p);

// Bit twiddlers
int get_bit(unsigned char *data, unsigned bitoffset); // returns 0 or 1
unsigned int get_bits(unsigned char* data, unsigned bitOffset, unsigned numBits); // returns 32 bit unsigned

// Decoding setup
void setxormasks();
unsigned long nullvals(char graph);
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

