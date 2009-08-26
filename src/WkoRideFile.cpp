/*
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net),
 *                    Justin F. Knotzke (jknotzke@shampoo.ca)
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

// GENERAL NOTES:
// You will see a lot of 'commented out' code in the parsing functions. This is
// because the decoding is skipped over, but may be useful in the future and
// it helps to reference back to the file format spec. For example a whole
// block of decoding of numbers and texts is replaced with p+= 64 in one part of
// the code and that is not particularly enlightening.
//
// The code is refactored from my wko2csv sourceforge project
//
// Whilst I have applied the coding standards in style.txt to the letter I suspect
// that some of the casting is as a result of poor design and could be removed
// altogether. Naybe when I have more time.

// ISSUES
//
// 1. WKO_HOMEDIR EXTERNAL GLOBAL
// The homedirectory is needed to create rideNotes, but is not available either
// as a global or a passed variable -- since the RideFile function is pure virtual
// I was unable to modify the caller to pass it, so it is stored in an external
// global variable called WKO_HOMEDIR.
//
// 2. UNUSED GRAPHS
// Altitude, Windspeed, Temperature, GPS and other data is available from WKO
// data files but is discarded currently since it is not supported by RideFile
//
// 3. GOTO
// During the ParseHeaderData there are a couple of nasty goto statements
// to breakout of nested loops / if statements.
//
// 4. WKO_device and WKO_GRAPHS STATIC GLOBALS
// Shared between a number of functions to simplify parameter passing and avoid
// refactoring as a class.
//
// 5. ALTITIUDE PATCH SUPPORT
// Needs to be added to appendPoint() call in WkoParseRawData(). Altitude is already
// being decoded
//
// 6. METRIC/IMPERIAL CONVERSION
// Code is available to support conversion from WKO standard of all metric but it is not
// enabled -- need to understand how metric/imperial conversion is supposed to be managed
//

#include "WkoRideFile.h"
#include <QRegExp>
#include <QFile>
#include <QTextStream>
#include <algorithm> // for std::sort
#include <assert.h>
#include "math.h"

// local holding varables shared between WkoParseHeaderData() and WkoParseRawData()
static int WKO_device;                   // Device ID used for this workout
static char WKO_GRAPHS[32];              // GRAPHS available in this workout

static int wkoFileReaderRegistered =
    RideFileFactory::instance().registerReader("wko", new WkoFileReader());


//******************************************************************************
// Main Entry Point from MainWindow() called to read data file
//******************************************************************************
RideFile *WkoFileReader::openRideFile(QFile &file, QStringList &errors) const
{
    unsigned char *headerdata, *rawdata, *footerdata;
    unsigned long version;

    if (!file.open(QFile::ReadOnly)) {
        errors << ("Could not open ride file: \""
                   + file.fileName() + "\"");
        return NULL;
    }

    // read in the whole file and pass to parser routines for decoding
    boost::scoped_array<unsigned char> entirefile(new unsigned char[file.size()]); // with a nod to c++ neophytes

    // read entire raw data stream
    QDataStream *rawstream(new QDataStream(&file));
    headerdata = &entirefile[0];
    rawstream->readRawData(reinterpret_cast<char *>(headerdata), file.size());
    file.close();

    // check it is version 28 of the format (the only version supported right now)
    donumber(headerdata+4, &version);
    if (version < 28) {
        errors << ("Version of file is too old, open and save in WKO then retry: \""
                   + file.fileName() + "\"");
        return NULL;
    } else if (version >28) {
        errors << ("Version of file is new and not supported yet: \"" +
        file.fileName() + "\"");
    return NULL;
    }

    // Golden Cheetah ride file
    RideFile *rideFile = new RideFile;

    // read header data and store details into rideFile structure
    rawdata = WkoParseHeaderData(headerdata, rideFile);

    // Parse raw data (which calls rideFile->appendPoint() with each sample
    if (rawdata) footerdata = WkoParseRawData(rawdata, rideFile);
    else return NULL;

    if (footerdata) return (RideFile *)rideFile;
    else return NULL;
}


/***************************************************************************************
 * PROCESS THE RAW DATA
 *
 * WkoParseRawData() - read through all the raw data adding record points and return
 *                     a pointer to the footer record
 **************************************************************************************/
unsigned char *WkoParseRawData(unsigned char *fb, RideFile *rideFile)
{
    unsigned int WKO_xormasks[32];    // xormasks used all over
    double cad, hr, km, kph, nm, watts, alt, interval;

    int isnull=0;
    unsigned long records, data;
    unsigned char *thelot;
    unsigned short us;

    int imperialflag=0;

    int i,j;
    int bit=0;
    unsigned long inc;
    unsigned long rtime=0,rdist=0;

    int WKO_graphbits[32];            // number of bits for each GRAPH
    unsigned long WKO_nullval[32];    // null value for each graph
    char GRAPHDATA[32][32];           // formatted values for each sample

    // setup decoding controls -- xormasks
    for (i=1; i<32; i++) {
        WKO_xormasks[i]=0;
        for (j=0; j<i; j++) {
            WKO_xormasks[i] <<= 1;
            WKO_xormasks[i] |= 1;
        }
    }

    // setup decoding controls -- bit sizes and null values
    for (i=0; WKO_GRAPHS[i] != '\0'; i++) {
        WKO_nullval[i] = nullvals(WKO_GRAPHS[i]); // setup nullvalue
        if ((WKO_graphbits[i] = bitsize(WKO_GRAPHS[i], WKO_device)) == 0) { // setup & check field size
            fprintf(stderr, "ERROR: Unknown channel '%c' for WKO_device %x.\n", WKO_GRAPHS[i], WKO_device);
            return (NULL);
        }
    }

    /* So how many records are there? */
    fb += doshort(fb, &us);
    if (us == 0xffff) {
        fb += donumber(fb, &records);
    } else {
        records = us;
    }

    /* how much data is there? */
    fb += doshort(fb, &us);
    if (us == 0xffff) {
        fb += donumber(fb, &data);
    } else {
        data = us;
    }

    thelot = fb;

    /* does data stream at offset 0 ? */
    if (get_bit(thelot, 0)){
        bit = 0;
        inc = 1000;
    } else {
        inc = get_bits(thelot, 1, 15); // record interval
        bit = 43;
    }
    interval = inc;
    interval /= 1000;

    rideFile->setRecIntSecs(interval);


    /*------------------------------------------------------------------------------
     * RUN THROUGH EACH RAW DATA RECORD
     *==============================================================================*/
    rdist=rtime=0;
    while (records) {

        unsigned int marker;
        long sval=0;
        unsigned long val=0;


        // reset point values;
        cad= hr= km= kph= nm= watts= 0.0;

        marker = get_bits(thelot, bit++, 1);

        if (marker) {

            // OK so we have a sample to collect, format and get ready to output

            records--;


            for (i=0; WKO_GRAPHS[i] != '\0'; i++) {

                val = get_bits(thelot, bit, WKO_graphbits[i]);
                /* move onto next value */
                bit += WKO_graphbits[i];

                if (WKO_GRAPHS[i] != 'D' && WKO_GRAPHS[i] != 'G' && val==WKO_nullval[i]) {

                    // null value
                    GRAPHDATA[i][0] = '\0';

                } else {

                    /* apply conversion etc */
                    switch (WKO_GRAPHS[i]) {

                    case '+' :  /* Temperature - signed */
                        if (get_bit(thelot, bit-1)) { /* is negative */
                            val ^= WKO_xormasks[WKO_graphbits[i]];
                            sval = val * -1;
                        } else {
                            sval = val;
                        }
                        sprintf(GRAPHDATA[i], "%8ld", sval);
                        break;
                    case '^' : /* Slope */
                    case 'W' : /* Wind speed */
                    case 'A' : /* Altitude */
                        if (get_bit(thelot, bit-1)) { /* is negative */
                            val ^= WKO_xormasks[WKO_graphbits[i]];
                            sval = val * -1;

                            if (imperialflag && WKO_GRAPHS[i]=='A') val = long((double) val * MTOFT);
                            if (imperialflag && WKO_GRAPHS[i]=='W') val = long((double) val * KMTOMI);
                            sprintf(GRAPHDATA[i], "%6ld.%1ld", sval/10, val%10);

                            alt = val; alt /= 10;
                        } else {
                            if (imperialflag && WKO_GRAPHS[i]=='A') val = long((double) val * MTOFT);
                            if (imperialflag && WKO_GRAPHS[i]=='W') val = long((double) val * KMTOMI);
                            sprintf(GRAPHDATA[i], "%6ld.%1ld", val/10, val%10);

                            alt = val; alt /=10;
                        }
                        break;
                    case 'T' : /* Torque */
                        if (imperialflag && WKO_GRAPHS[i]=='S') val = long((double)val * KMTOMI);
                        sprintf(GRAPHDATA[i], "%6ld.%1ld", val/10, val%10);
                        nm = val; nm /=10;
                        break;
                    case 'S' : /* Speed */
                        if (imperialflag && WKO_GRAPHS[i]=='S') val = long((double)val * KMTOMI);
                        sprintf(GRAPHDATA[i], "%6ld.%1ld", val/10, val%10);
                        kph = val; kph/= 10;
                        break;
                    case 'D' : /* Distance - running total to 3 decimal places */
                        double distance;
                        double f, xi, xf;


                        // increment BEFORE not AFTER
                        rdist += val;

                        // conversion may be required
                        distance = rdist;
                        distance /= 100000;
                        if (imperialflag) distance *= KMTOMI;

                        // to max of 3 decimal places
                        xf = modf(distance,&xi);
                        f = floor(xf*1000+0.5)/1000.0;

                        sprintf(GRAPHDATA[i], "%g", xi+f);
                        km = xi+f;


                        break;
                    case 'G' : /* two longs */
                        {
                            signed long llat, llon;
                            double lat,lon;
                            char slat[20], slon[20];

                            // stored 2s complement
                            // conversion is * 180 / 2^31
                            val = get_bits(thelot, bit-64,32);
                            memcpy(&llat, &val, 4);
                            lat = (double)llat;
                            lat *= 0.00000008381903171539306640625;
                            sprintf(slat, "%-3.9g", lat);

                            val = get_bits(thelot, bit-32,32);
                            memcpy(&llon, &val, 4);
                            lon = (double)llon;
                            lon *= 0.00000008381903171539306640625;
                            sprintf(slon, "%-3.9g", lon);

                            sprintf(GRAPHDATA[i], "%13s %13s", slat,slon);

                        }
                        break;
                    case 'P' :
                        watts = val;
                        sprintf(GRAPHDATA[i], "%8lu", val); // just output as numeric
                        break;

                    case 'H' :
                        hr = val;
                        sprintf(GRAPHDATA[i], "%8lu", val); // just output as numeric
                        break;

                    case 'C' :
                        cad = val;
                        sprintf(GRAPHDATA[i], "%8lu", val); // just output as numeric
                        break;
                    }
                }
            }

            // Lets check to see if it is a null record?
            // We ignores checking GPS data cause its too hard to do at this point
            // and rarely (if ever?) affects the outcome
            for (i=0, isnull=1; WKO_GRAPHS[i] != '\0'; i++)
                if (WKO_GRAPHS[i] != 'G' && WKO_GRAPHS[i] != 'D' && GRAPHDATA[i][0] != '\0') isnull=0;

            // Now output this sample if it is not a null record
            if (!isnull) {

                    // !! needs to be modified to support the new alt patch
                    rideFile->appendPoint((double)rtime/1000, cad, hr, km,
                            kph, nm, watts, alt, 1);
            }

            // increment time - even for null records (perhaps especially for null
            // records since they are there specifically to handle pause in recording!
            rtime += inc;

        } else {

            // pause record increments time

            /* set the increment */
            unsigned long pausetime;
            int pausesize;

            if (WKO_device != 0x14) pausesize=42;
            else pausesize=39;

            pausetime = get_bits(thelot, bit, 32);

            /* set increment value -> if followed by a null record
               it is to show a pause in recording -- velotrons seem to cause
               lots and lots of these
            */
            inc = pausetime;
            bit += pausesize;

        }
    }

    return thelot + (bit/8);

}


/**********************************************************************
 * ZIP THROUGH ALL THE WKO SPECIFIC DATA, LAND ON THE RAW DATA
 * AND RETURN A POINTER TO THE FIRST BYTE OF THE RAW DATA
 *
 * WkoParseHeadeData() - read through file and land on the raw data
 *
 *********************************************************************/
unsigned char *WkoParseHeaderData(unsigned char *fb, RideFile *rideFile)
{
    unsigned long julian, sincemidnight;
    unsigned char *goal, *notes, *code; // save location of WKO metadata

    enum configtype lastchart;
    unsigned int num,x,z,i;

    /* utility holders */
    unsigned long ul;
    unsigned short us;
    double g;

    boost::scoped_array<unsigned char> txtbuf(new unsigned char[1024000]);

    /* working through filebuf */
    unsigned char *p = fb;

    /***************************************************
     * 0:  FILE HEADER
     ***************************************************/
    p += donumber(p, &ul); /* 1: magic */
    p += donumber(p, &ul); /* 2: version */

    /***************************************************
     * 1:  JOURNAL TAB
     ***************************************************/
    p += donumber(p, &ul); /* 3: date days since 01/01/1901 */
    julian = ul + 2415386; // 1/1/1901 is day 2415386 in julian days god bless google.

    goal = p; p += dotext(p, &txtbuf[0]); /* 4: goal */
    notes = p; p += dotext(p, &txtbuf[0]); /* 5: notes */

    p += dotext(p, &txtbuf[0]); /* 6: graphs */
    strcpy(reinterpret_cast<char *>(WKO_GRAPHS), reinterpret_cast<char *>(&txtbuf[0])); // save those graphs away

    p += donumber(p, &ul); /* 7: sport */
    code = p; p += dotext(p, &txtbuf[0]); /* 8: workout code */
    p += donumber(p, &ul); /* 9: duration 000s of seconds */
    p += dotext(p, &txtbuf[0]); /* 10: lastname */
    p += dotext(p, &txtbuf[0]); /* 11: firstname */

    p += donumber(p, &ul); /* 12: time of day */
    sincemidnight = ul;

    // Set start time
        QDateTime datetime(QDate::fromJulianDay(julian),
               QTime(sincemidnight/360000, (sincemidnight%360000)/6000, (sincemidnight%6000)/100));
        rideFile->setStartTime(datetime);


    // Create a Notes file for Goal, Notes and Workout Code from original
        QChar zero = QLatin1Char('0');
        QString notesFileName = QString("%1_%2_%3_%4_%5_%6.notes")
            .arg(rideFile->startTime().date().year(), 4, 10, zero)
            .arg(rideFile->startTime().date().month(), 2, 10, zero)
            .arg(rideFile->startTime().date().day(), 2, 10, zero)
            .arg(rideFile->startTime().time().hour(), 2, 10, zero)
            .arg(rideFile->startTime().time().minute(), 2, 10, zero)
            .arg(rideFile->startTime().time().second(), 2, 10, zero);

        // Create the Notes file ONLY IF IT DOES NOT ALREADY EXIST
        QFile notesFile(WKO_HOMEDIR + "/" + notesFileName);
    if (!notesFile.exists()) {
        notesFile.open(QFile::WriteOnly);
                QTextStream out(&notesFile);
        QString scode, sgoal, snote;

        // Workout Code
        dotext(code, &txtbuf[0]);
        scode = (const char *)&txtbuf[0];
        out << "WORKOUT CODE: " << scode << endl;

        dotext(goal, &txtbuf[0]);
        sgoal = (const char *)&txtbuf[0];
        out << "WORKOUT GOAL" << endl << sgoal << endl;

        dotext(notes, &txtbuf[0]);
        snote = (const char *)&txtbuf[0];
        out << endl << "WORKOUT NOTES" << endl << snote << endl;

        notesFile.close();
        }

    p += donumber(p, &ul); /* 13: distance travelled in meters */
    p += donumber(p, &ul); /* 14: device recording interval */
    p += donumber(p, &ul); /* 15: athlete max heartrate */
    p += donumber(p, &ul); /* 16: athlete threshold heart rate */
    p += donumber(p, &ul); /* 17: athlete threshold power */
    p += dodouble(p, &g);  /* 18: athlete threshold pace */
    p += donumber(p, &ul); /* 19: weight in grams/10 */

    p += 28;
    //p += donumber(p, &ul); /* 20: unknown */
    //p += donumber(p, &ul); /* 21: unknown */
    //p += donumber(p, &ul); /* 22: unknown */
    //p += donumber(p, &ul); /* 23: unknown */
    //p += donumber(p, &ul); /* 24: unknown */
    //p += donumber(p, &ul); /* 25: unknown */
    //p += donumber(p, &ul); /* 26: unknown */

    /***************************************************
     * 2:  GRAPH TAB - MAX OF 16 CHARTING GRAPHS
     ***************************************************/
    p += donumber(p, &ul); /* 27: graph view */
    p += donumber(p, &ul); /* 28: WKO_device type */
    WKO_device = ul; // save WKO_device

#if 0
    switch (WKO_device) {
    case 0x01 : rideFile->setDeviceType("Powertap (via WKO)"); break;
    case 0x04 : rideFile->setDeviceType("SRM (via WKO)"); break;
    case 0x05 : rideFile->setDeviceType("Polar (via WKO)"); break;
    case 0x06 : rideFile->setDeviceType("Computrainer/Velotron (via WKO)"); break;
    case 0x11 : rideFile->setDeviceType("Ergomo (via WKO)"); break;
    case 0x12 : rideFile->setDeviceType("Garmin Edge 205/305 (via WKO)"); break;
    case 0x13 : rideFile->setDeviceType("Garmin Edge 705 (via WKO)"); break;
    case 0x14 : rideFile->setDeviceType("Ergomo (via WKO)"); break;
    case 0x19 : rideFile->setDeviceType("Ergomo (via WKO)"); break;
    default : rideFile->setDeviceType("Unknown Device (via WKO)"); break;
    }
#else
    rideFile->setDeviceType("WKO");
#endif

    p += donumber(p, &ul); /* 29: unknown */

    for (i=0; i< 16; i++) { // 16 types of chart data

        p += 44;
        //p += dofloat(p, &f) /* 30: unknown */
        //p += dofloat(p, &f) /* 31: unknown */
        //p += dofloat(p, &f) /* 32: unknown */
        //p += dofloat(p, &f) /* 33: unknown */
        //p += dofloat(p, &f) /* 34: unknown */
        //p += dofloat(p, &f) /* 35: unknown */
        //p += dofloat(p, &f) /* 36: unknown */
        //p += donumber(p, &ul); /* 37: x-axis maximum */
        //p += donumber(p, &ul); /* 38: show chart? */
        //p += donumber(p, &ul); /* 39: autoscale? */
        //p += donumber(p, &ul); /* 40: autogridlines? */

        p += doshort(p, &us); /* 41: number of gridlines XXVARIABLEXX */
        p += (us * 8); // 2 longs x number of gridlines
    }


    /* Ranges */
    p += doshort(p, &us); /* 237: Number of ranges XXVARIABLEXX */
    for (i=0; i<us; i++) {

        p += 12;
        //p += donumber(p, &ul);
        //p += donumber(p, &ul);
        //p += donumber(p, &ul);

        p += dotext(p, &txtbuf[0]);
        p += dotext(p, &txtbuf[0]);

        p += 24;
        //p += donumber(p, &ul);
        //p += donumber(p, &ul);
        //p += donumber(p, &ul);
        //p += donumber(p, &ul);
        //p += donumber(p, &ul);
        //p += donumber(p, &ul);
    }

    /***************************************************
     * 3: DEVICE SPECIFIC DATA
     ***************************************************/
    p += doshort(p, &us); /* 249: Device/Token pairs XXVARIABLEXX */
    for (i=0; i<us; i++) {
        p += dotext(p, &txtbuf[0]);
        p += dotext(p, &txtbuf[0]);
    }

    /***************************************************
     * 4: PERSPECTIVE CHARTS & CACHES
     ***************************************************/

    p += doshort(p, &us);       /* 251: Number of charts XXVARIABLEXX */
    z = us;

    num=0;

    while (z) {     /* Until we hit first chart data */

        unsigned int prev=0;
        enum configtype type=INVALID;


next:
        prev=num;
        p += donumber(p, &ul);
        num = ul;
        lastchart = OTHER;

        if (num==131071) {

            char buf[32];

            /* Config */
            p += doshort(p, &us); // got here...
            strncpy (reinterpret_cast<char *>(buf), reinterpret_cast<char *>(p), us); buf[us]=0;
            p += us;


            /* What type? */
            if (!strcmp(buf, "CRideSettingsConfig")) lastchart=type=CRIDESETTINGSCONFIG;
            if (!strcmp(buf, "CRideGoalConfig")) lastchart=type=CRIDEGOALCONFIG;
            if (!strcmp(buf, "CRideNotesConfig")) lastchart=type=CRIDENOTESCONFIG;
            if (!strcmp(buf, "CDistributionChartConfig")) lastchart=type=CDISTRIBUTIONCHARTCONFIG;
            if (!strcmp(buf, "CRideSummaryConfig")) lastchart=type=CRIDESUMMARYCONFIG;
            if (!strcmp(buf, "CMeanMaxChartConfig"))lastchart=type=CMEANMAXCHARTCONFIG;
            if (!strcmp(buf, "CMeanMaxChartCache")) lastchart=type=CMEANMAXCHARTCACHE;
            if (!strcmp(buf, "CDistributionChartCache")) lastchart=type=CDISTRIBUTIONCHARTCACHE;


            if (type == CDISTRIBUTIONCHARTCACHE) {

                p += 346;
                //p += donumber(p, &ul); /* always 2 */
                //p += donumber(p, &ul); /* always 1 */
                //_read(fd,buf,338);   /* dist cache data */

                /* handle "optional padding" */
                p += optpad(p);

            } else if (type == CMEANMAXCHARTCACHE) {

                unsigned long recs, term;

                p += 20;
                //_read(fd,buf,20); /* unknown 20 bytes */

                p += donumber(p, &ul);
                p += (ul * 8);

                p += 28;
                //_read(fd,buf,28); /* further cached data */

                p += donumber(p, &ul);
                p += (ul * 8);
                //_read(fd,buf,recs*8); /* first data block */

                /* now its the main structures from a meanmax chart */

                /* the code below is cut and paste from the meanmax chart
                 * code below
                 */
                /* How many meanmax records in first set ? */
                p += doshort(p, &us);
                recs = us;

                while (recs--) {

                    p += donumber(p, &ul);
                    if (!ul) goto next;
                    else {

                        /* Read that row */
                        p += 28;
                        //p += donumber(p, &ul); /* unknown */
                        //p += donumber(p, &ul); /* unknown */
                        //p += donumber(p, &ul); /* unknown */
                        //p += dodouble(p, &g);  /* value! */
                        //p += donumber(p, &ul); /* unknown */
                        //p += donumber(p, &ul); /* unknown */

                    }
                }

                /* 12 byte preamble */
                p += 12;
                //p += donumber(p, &ul);
                //p += donumber(p, &ul);
                //p += donumber(p, &ul);

                /* How many meanmax records in second set ? */
                p += doshort(p, &us);
                recs = us;
                term=1;

                while (recs--) {

                    p += donumber(p, &ul);
                    if (!ul) break;
                    else {

                        /* Read that row */
                        p += 28;
                        //p += donumber(p, &ul); /* unknown */
                        //p += donumber(p, &ul); /* unknown */
                        //p += donumber(p, &ul); /* unknown */
                        //p += dodouble(p, &g);  /* value! */
                        //p += donumber(p, &ul); /* unknown */
                        //p += donumber(p, &ul); /* unknown */

                    }
                }


                /* as long as it didn't get terminated we need to read the end of the data */
                if (term) p +=24; //_read(fd, buf, 24);

                /* might still be padding ? */
                p += optpad(p);

            } else {

                p += donumber(p, &ul); /* always 2 */
                p += dotext(p, &txtbuf[0]); /* perspective */

                switch (type) {

                case CRIDESUMMARYCONFIG:
                    p += donumber(p, &ul);
                    p += donumber(p, &ul);
                    z--;
                    /* might still be padding ? */
                    p += optpad(p);
                    break;

                case CRIDENOTESCONFIG:
                case CRIDEGOALCONFIG:
                    p += donumber(p, &ul);
                case CRIDESETTINGSCONFIG:
                    p += donumber(p, &ul);
                    z--;
                case CDISTRIBUTIONCHARTCONFIG:
                case CMEANMAXCHARTCONFIG:
                    /* already handled in previous if clause */
                default:
                    break;
                }

            }

        } else if (num==2) {

            /* Perspective */
            p += dotext(p, &txtbuf[0]);

        } else if (num==3) {

            z--;

            /* Chart */
            p += dotext(p, &txtbuf[0]);

            /* now lets skip the config and labels etc */
            p += 32; //for (i=0; i<16; i++) doshort(fd,buf,0);

            /* what type is it? */
            p += donumber(p, &ul);
            x = ul;

            if (x == 0x02) {        /* Read through Distribution Chart */

                p += 64;
                // /* distribution chart */
                //p += donumber(p, &ul); /* chart type bin/pie1 */
                //p += donumber(p, &ul); /* channel */
                //p += donumber(p, &ul); /* percent/absolute */
                //p += donumber(p, &ul); /* u1 */
                //p += donumber(p, &ul); /* Units */
                //p += donumber(p, &ul); /* Conversion type */
                //p += dodouble(p, &g); /* first converter */
                //p += donumber(p, &ul); /* bin mode auto/zone/manual */
                ///* remaining 28 bytes of config data */
                //p += donumber(p, &ul); /* include zeros */
                //p += donumber(p, &ul); /* u2 */
                //p += donumber(p, &ul); /* lower limit */
                //p += donumber(p, &ul); /* u3 */
                //p += donumber(p, &ul);  /* upper limit */
                //p += donumber(p, &ul); /* u4 */
                //p += donumber(p, &ul); /* increments */


                /* number of labels */
                p += doshort(p, &us);

                for (i=0; i<us; i++) {
                    p += donumber(p, &ul);
                    p += dotext(p, &txtbuf[0]);
                    p += dotext(p, &txtbuf[0]);
                    p += donumber(p, &ul);
                    p += donumber(p, &ul);
                }


                p += optpad(p);

            } else if (x == 0x0c) {     /* Read through Mean Max Chart */

                unsigned long recs, term;
                unsigned long two;

                p += donumber(p, &ul); /* 253.4: Log or Linear meanmax ? */
                p += donumber(p, &ul); /* 253.4: Which Channel ? */
                p += donumber(p, &ul); /* 253.4: UNKNOWN */
                p += donumber(p, &ul); /* 253.4: Units */
                p += donumber(p, &ul); /* 253.4: Conversion type ? */
                two = ul;

                p += dodouble(p, &g);  /* 253.4: Conversion Factor 1 */

                if (two) p += donumber(p, &ul);  /* 253.4: Conversion Factor 2 */

                /* How many meanmax records in first set ? */
                p += doshort(p, &us);
                recs = us;

                if (recs) {
                while (recs--) {

                    p += donumber(p, &ul);
                    term = ul;
                    if (!term) goto next;
                    else {

                        /* Read that row */
                        p += 28;
                        //p += donumber(p, &ul); /* unknown */
                        //p += donumber(p, &ul); /* unknown */
                        //p += donumber(p, &ul); /* unknown */
                        //p += dodouble(p, &g); /* value! */
                        //p += donumber(p, &ul); /* unknown */
                        //p += donumber(p, &ul); /* unknown */

                    }
                }

                /* 12 byte preamble */
                p += 12;
                //p += donumber(p, &ul);
                //p += donumber(p, &ul);
                //p += donumber(p, &ul);

                /* How many meanmax records in second set ? */
                p += doshort(p, &us);
                recs=us;
                term=1;

                while (recs--) {

                    p += donumber(p, &ul);
                    term=ul;
                    if (!term) {
                        break;
                    } else {

                        /* Read that row */
                        p += 28;
                        //p += donumber(p, &ul); /* unknown */
                        //p += donumber(p, &ul); /* unknown */
                        //p += donumber(p, &ul); /* unknown */
                        //dodouble(fd,buf,0); /* value! */
                        //p += donumber(p, &ul); /* unknown */
                        //p += donumber(p, &ul); /* unknown */
                    }
                }


                /* as long as it didn't get terminated we need to read the end of the data */
                if (term) p += 24; //_read(fd, buf, 24);

                }

                /* might still be padding ? */
                p += optpad(p);

            } else {

                goto breakout;
            }

        } else {
            fprintf(stderr, "ERROR: unrecognised segment %x at %x aborting file\n", num, p-fb);
            return NULL;
        }
    }

breakout:

    if (WKO_GRAPHS[0] == '\0') {
        fprintf(stderr, "ERROR: file contains no GRAPHS\n");
        return (unsigned char *)NULL;
    } else {
        /* PHEW! We're on the raw data, out job here is done */
        return (p);
    }
}

/*==============================================================================
 * UTILITY FUNCTIONS
 *==============================================================================*/

/**********************************************************************************
 * SETUP DECODING CONTROLS
 *
 * bitsize() - return size in bits of a given graph for a specific WKO_device
 * nullvals() - return nullvalue for this GRAPH on this WKO_device
 **********************************************************************************/
unsigned int bitsize(char g, int WKO_device)
{
    switch (g) {

    case 'P' : return (12); break;
    case 'H' : return (8); break;
    case 'C' : return (8); break;
    case 'S' : return (11); break;
    case 'A' : return (20); break;
    case 'T' : return (11); break;
    case 'D' :
        /* distance */
        switch (WKO_device) {
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x11:
        case 0x19:
        case 0x1a:
        default:
            return 19;
            break;
        case 0x01:
        case 0x00:
        case 0x12: // Garmin Edge 205/305
        case 0x13:
        case 0x14:
            return 22;
            break;
        }
        break;

    case 'G' : return (64); break;
    case 'W' : return (11); break;
    case '+' : return (8); break;
    case '^' : return (20); break;
    case 'm' : return (1); break;
    default:
        return (0);
    }
}

unsigned long nullvals(char g)
{
    switch (g) {

    case 'P' : return (4095L); break;
    case 'H' : return (255L); break;
    case 'C' : return (255L); break;
    case 'S' : return (2047L); break;
    case 'A' : return (524287L); break; // max is for 19 bits, sign bit 0 for null values
    case 'T' : return (2047L); break;
    case 'D' : return (0L); break; // distance is ignored for null purposes
    case 'G' : return (0L); break; // GPS is ignored for null purposes
    case 'W' : return (2047L); break;
    case '+' : return (127L); break;     // max is for 7 bits, sign bit 0 for null value
    case '^' : return (524287L); break; // max is for 19 bits, sign bit 0 for null values
    case 'm' : return (1L); break;
    default:
        return (0);
    }
}

/************************************************************************
 * HANDLE OPTIONAL CHARTING DATA
 *
 * optpad() - main entry point for handling optional chart data
 * optpad2() - called by optpad for extended data
 ***********************************************************************/

unsigned int optpad(unsigned char *p)
{
    unsigned short int us;
    unsigned long int ul;
    unsigned int bytes = 0;

    /* Opening bytes are
     * ffff - gone too far!
     * 8007 - stop
     * 800f - data cache
     * 0001 - onebyte field
     * Any other value and you've gone
     * too far and need to rewind.
     */
    p += doshort(p, &us);
    bytes = 2;

    switch (us) {

    case 0x8007 :       /* all done */
    case 0x800c :
    case 0x800b :
        break;

    case 0x0001 :       /* 4byte follows */
        p += donumber(p, &ul);
        bytes += 4;
        bytes += optpad2(p);
        break;

    case 0x0000 :
        bytes += doshort(p, &us);
        p += 4;
        break;

    case 0xffff :       /* too far, rewind */
    default :
        bytes -= 2;
        break;
    }
    return (bytes);
}

unsigned int optpad2(unsigned char *p)
{
    unsigned short int us;
    unsigned int bytes=0;

    /* Opening bytes are
     * ffff - gone too far!
     * 8007 - stop
     * 800f - data cache
     * 0001 - onebyte field
     * Any other value and you've gone
     * too far and need to rewind.
     */
    bytes = doshort(p, &us);
    p += bytes;

    switch (us) {

    case 0x800f :       /* data cache */
    case 0x800d :
    case 0x800e :
    case 0x8018 :
    case 0x8017 :
    case 0x8016 :
    case 0x8015 :
    case 0x8011 :
    case 0x8010 :

        bytes += 16;
        p += 16;

        bytes += doshort(p, &us);
        p += 2;

        p += ((us *8) + 2);//_read(fd,buf,(count*8)+2);
        bytes += ((us * 8) +2);

        break;

    case 0x8007 :       /* all done */
    case 0x800c :
    case 0x800b :
        break;

    default :
    case 0xffff :       /* too far, rewind */
        bytes -= 2;
        p -=2;
        break;
    }
    return(bytes);
}
/************************************************************************************
 * BIT TWIDDLING FUNCTIONS
 *
 * get_bit() - return 0 or 1 for the given bit from a large array
 * get_bits() - return a range of bits read right to left (high bit first)
 *
 ************************************************************************************/
int get_bit(unsigned char *data, unsigned bitoffset) // returns the n-th bit
{
    unsigned char c = data[bitoffset >> 3]; // X>>3 is X/8
    unsigned char bitmask = 1 << (bitoffset %8);  // X&7 is X%8
    return ((c & bitmask)!=0) ? 1 : 0;
}

unsigned int get_bits(unsigned char* data, unsigned bitOffset, unsigned numBits)
{
    unsigned int bits = 0;
    unsigned int currentbit;

    bits=0;
    for (currentbit = bitOffset+numBits-1; numBits--; currentbit--) {
        bits = bits << 1;
        bits = bits | get_bit(data, currentbit);
    }
    return bits;
}

/*****************************************************************************
 * READ NUMBERS AND TEXTS FROM THE FILE, OPTIONALLY OUTPUTTING
 * IN DIFFERENT FORMATS TO DEBUG OR ANALYSE DATA
 *
 * dofloat() - read and retuen a 4 byte float
 * dodouble() - read and return an 8 byte double
 * donumber() - read and return a 4 byte long
 * doshort() - read and return a 2 byte short
 * dotext() - read a wko text (1byte len, n bytes string without terminator)
 * pbin() - print a number in binary
 ****************************************************************************/
unsigned int dofloat(unsigned char *p, float *pnum)
{
    memcpy(pnum, p, 4);
    return 4;
}

unsigned int dodouble(unsigned char *p, double *pnum)
{
    memcpy(pnum, p, 8);
    return 8;
}

unsigned int donumber(unsigned char *p, unsigned long *pnum)
{
    memcpy(pnum, p, 4);
    return 4;
}

void pbin(unsigned char x)
{
    static unsigned char bits[]={ 128, 64, 32, 16, 8, 4, 2, 1 };
    int i;
    for (i=0; i<8; i++)
        printf("%c", ((x&bits[i]) == bits[i]) ? '1' : '0');

}


unsigned int dotext(unsigned char *p, unsigned char *buf)
{
    unsigned short us;

    if (*p == 0) {
            *buf = '\0';
            return 1;
    } else if (*p < 255) {
        strncpy(reinterpret_cast<char *>(buf), reinterpret_cast<char *>(p+1), *p);
        buf[*p]=0;
        return (*p)+1;
    } else {
        p += 1;
        p += doshort(p, &us);
        strncpy(reinterpret_cast<char *>(buf), reinterpret_cast<char *>(p), us);
        buf[us]=0;
        return (us)+3;
    }
}

unsigned int doshort(unsigned char *p, unsigned short *pnum)
{
    memcpy(pnum, p, 2);
    return 2;
}

