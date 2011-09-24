/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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
#include "GoldenCheetah.h"

#include <stdint.h>
#include "RideFile.h"
#include <QDataStream>
#include <string.h>
#include <math.h>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <QtEndian>

struct WkoFileReader : public RideFileReader {
    virtual RideFile *openRideFile(QFile &file, QStringList &errors, QList<RideFile*>* = 0) const; 
};

typedef std::auto_ptr<QDataStream> QDataStreamPtr;

// Explicitly define types using unistd.h definitions to
// ensure char IS 8 bits and short IS 16 bits and long IS 32 bits
#define WKO_UCHAR   uint8_t
#define WKO_CHAR   int8_t
#define WKO_USHORT uint16_t
#define WKO_SHORT  int16_t
#define WKO_ULONG  uint32_t
#define WKO_LONG   int32_t
#define	KMTOMI 0.6213
#define MTOFT  3.2808

class WkoParser
{
    public:
        WkoParser(QFile &file, QStringList &errors, QList<RideFile*>*rides = 0);
        RideFile *result() { return results; }

    private:

        QFile &file;
        QString filename;
        RideFile *results;
        QStringList &errors;
        QList<RideFile*>*rides;

        // state data during parsing
        WKO_UCHAR *headerdata, *rawdata, *footerdata;
        WKO_ULONG version;
        WKO_ULONG WKO_device;
        char WKO_GRAPHS[32];
        QList<RideFileInterval *> references;

        // Header data parsing
        WKO_UCHAR *parseHeaderData(WKO_UCHAR *data);

        // Raw bit packed data parsing
        WKO_UCHAR *parseRawData(WKO_UCHAR *data);

        // Basic Field decoding
        unsigned int doshort(WKO_UCHAR *p, WKO_USHORT *pnum);
        unsigned int donumber(WKO_UCHAR *p, WKO_ULONG *pnum);
        unsigned int dotext(WKO_UCHAR *p, WKO_UCHAR *txt);
        unsigned int dofloat(WKO_UCHAR *p, float *pnum);
        unsigned int dodouble(WKO_UCHAR *p, double *pnum);
        unsigned int optpad(WKO_UCHAR *p),
                    optpad2(WKO_UCHAR *p);

        // Bit twiddling functions
        int get_bit(WKO_UCHAR *data, unsigned bitoffset);
        unsigned int get_bits(WKO_UCHAR* data, unsigned bitOffset, unsigned numBits); 

        // Decoding setup
        void setxormasks();
        WKO_ULONG nullvals(char graph, WKO_ULONG version);
        unsigned long bitget(char *thelot, int offset, int count);
        unsigned int bitsize(char graph, int device, WKO_ULONG version);
        void pbin(WKO_UCHAR x); // for debugging

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
};

#endif // _WkoRideFile_h
