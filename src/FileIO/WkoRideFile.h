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
#include <cmath>
#include <memory>
#include <QtEndian>

struct WkoFileReader : public RideFileReader {
    virtual RideFile *openRideFile(QFile &file, QStringList &errors, QList<RideFile*>* = 0) const; 
    bool hasWrite() const { return false; }
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
        int charts;
        long bufferSize;

        // used by all the parsers as temporary storage
        unsigned char txtbuf[102400]; // text buffer
        WKO_ULONG num;
        WKO_ULONG ul;
        WKO_USHORT us;

        // Header data parsing - the first section of
        //                       a WKO file contains general
        //                       ride/athlete data, intervals
        //                       device specific data and then
        //                       charts and caches
        WKO_UCHAR *parseHeaderData(WKO_UCHAR *data);
	    WKO_UCHAR *parseCRideSettingsConfig(WKO_UCHAR *data);
	    WKO_UCHAR *parseCRideGoalConfig(WKO_UCHAR *data);
	    WKO_UCHAR *parseCRideNotesConfig(WKO_UCHAR *data);
	    WKO_UCHAR *parseCDistributionChartConfig(WKO_UCHAR *data);
	    WKO_UCHAR *parseCRideSummaryConfig(WKO_UCHAR *data);
	    WKO_UCHAR *parseCMeanMaxChartConfig(WKO_UCHAR *data);
	    WKO_UCHAR *parseCMeanMaxChartCache(WKO_UCHAR *data);
	    WKO_UCHAR *parseCDistributionChartCache(WKO_UCHAR *data);

        // re-used across all the segment parsers above
        WKO_UCHAR *parsePerspective(WKO_UCHAR *p);
        WKO_UCHAR *parseChart(WKO_UCHAR *p, int type = 0);

        // Basic Field decoding
        unsigned int doshort(WKO_UCHAR *p, WKO_USHORT *pnum);
        unsigned int donumber(WKO_UCHAR *p, WKO_ULONG *pnum);
        unsigned int dotext(WKO_UCHAR *p, WKO_UCHAR *txt);
        unsigned int dofloat(WKO_UCHAR *p, float *pnum);
        unsigned int dodouble(WKO_UCHAR *p, double *pnum);
        unsigned int optpad(WKO_UCHAR *p),
                    optpad2(WKO_UCHAR *p);

        // Telemetry data parsing - the second section of a
        //                          WKO file contains the ride
        //                          telemetry, but bit packed to
        //                          save storage space
        WKO_UCHAR *parseRawData(WKO_UCHAR *data);

        // Bit twiddling functions
        void setxormasks();
        int get_bit(WKO_UCHAR *data, unsigned bitoffset);
        unsigned int get_bits(WKO_UCHAR* data, unsigned bitOffset, unsigned numBits); 
        unsigned long bitget(char *thelot, int offset, int count);

        // Utilities to return bit packed field sizes based
        // upon the original device type (data is stored in
        // different lengths (8bit, 11bit, 22bit etc)
        WKO_ULONG nullvals(char graph, WKO_ULONG version);
        unsigned int bitsize(char graph, int device, WKO_ULONG version);
        void pbin(WKO_UCHAR x); // for debugging

        // Different chart types
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
