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
// Windspeed, Temperature, Slope et al are available from WKO
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

#include "WkoRideFile.h"
#include <QRegExp>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <algorithm> // for std::sort
#include <assert.h>
#include "math.h"

// local holding varables shared between WkoParseHeaderData() and WkoParseRawData()
static WKO_ULONG WKO_device;                   // Device ID used for this workout
static char WKO_GRAPHS[32];              // GRAPHS available in this workout
static QList<RideFileInterval *> references; // saved with data point references

static int wkoFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "wko", "WKO+ Files", new WkoFileReader());


//******************************************************************************
// Main Entry Point from MainWindow() called to read data file
//******************************************************************************
RideFile *WkoFileReader::openRideFile(QFile &file, QStringList &errors) const
{
    WKO_UCHAR *headerdata, *rawdata, *footerdata;
    WKO_ULONG version;
    QFileInfo fileinfo(file);

    if (!file.open(QFile::ReadOnly)) {
        errors << ("Could not open ride file: \""
                   + file.fileName() + "\"");
        return NULL;
    }

    // read in the whole file and pass to parser routines for decoding
    boost::scoped_array<WKO_UCHAR> entirefile(new WKO_UCHAR[file.size()]); // with a nod to c++ neophytes

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
    } else if (version >29) {
        errors << ("Version of file is new and not fully supported yet: \"" +
        file.fileName() + "\"");
    }

    // Golden Cheetah ride file
    RideFile *rideFile = new RideFile;

    // read header data and store details into rideFile structure
    rawdata = WkoParseHeaderData(fileinfo.fileName(), headerdata, rideFile, errors);

    // Parse raw data (which calls rideFile->appendPoint() with each sample
    if (rawdata) footerdata = WkoParseRawData(rawdata, rideFile, errors);
    else return NULL;

    // Post process  the ride intervals to convert from point
    // offsets to time in seconds
    QVector<RideFilePoint*> datapoints = rideFile->dataPoints();
    for (int i=0; i<references.count(); i++) {
        RideFileInterval add;

        // name is ok, but start stop need to be in time rather than datapoint no.
        add.name = references.at(i)->name;

        if (references.at(i)->start < datapoints.count())
            add.start = datapoints.at(references.at(i)->start)->secs;
        else
            continue;   // out of bounds

        if (references.at(i)->stop < datapoints.count())
            add.stop = datapoints.at(references.at(i)->stop)->secs + rideFile->recIntSecs()-.001;
        else
            continue;   // out of bounds

        rideFile->addInterval(add.start, add.stop, add.name);
    }

    // free up memory
    for (int i=0; i<references.count(); i++) delete references.at(i);
    references.clear();

    if (footerdata) return (RideFile *)rideFile;
    else return NULL;
}


/***************************************************************************************
 * PROCESS THE RAW DATA
 *
 * WkoParseRawData() - read through all the raw data adding record points and return
 *                     a pointer to the footer record
 **************************************************************************************/
WKO_UCHAR *WkoParseRawData(WKO_UCHAR *fb, RideFile *rideFile, QStringList &errors)
{
    WKO_ULONG WKO_xormasks[32];    // xormasks used all over
    double cad=0, hr=0, km=0, kph=0, nm=0, watts=0, alt=0, lon=0, lat=0, slope=0, wind=0, interval=0;

    int isnull=0;
    WKO_ULONG records, data;
    WKO_UCHAR *thelot;
    WKO_USHORT us;

    int imperialflag=0;

    int i,j;
    int bit=0;
    int calculate_distance=0;         // if speed is available but not distance we need to calculate it
    unsigned long inc;
    unsigned long rtime=0,rdist=0;

    int WKO_graphbits[32];            // number of bits for each GRAPH
    WKO_ULONG WKO_nullval[32];    // null value for each graph
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
            errors << ("Unknown channel " + WKO_GRAPHS[i]);
            return (NULL);
        }
    }

    // do we need to calculate distance ?
    if (!strchr(WKO_GRAPHS, 'D') && strchr(WKO_GRAPHS, 'S')) calculate_distance = 1;
    else calculate_distance = 0;

    /* So how many records are there? */
    fb += doshort(fb, &us);
    if (us == 0xffff) {
        fb += donumber(fb, &records);
    } else {
        records = us;
    }

    if (records == 0) {
        errors << ("Workout is empty.");
        return NULL;
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
        WKO_LONG sval=0;
        WKO_ULONG val=0;
	unsigned long valp; // for printf 
	long svalp; // for printf

        // reset point values;
        alt = slope = wind = cad= hr= km= kph= nm= watts= 0.0;

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
			svalp = sval;
                        sprintf(GRAPHDATA[i], "%8ld", svalp);
                        break;
                    case '^' : /* Slope */
                        if (get_bit(thelot, bit-1)) { /* is negative */
                            val ^= WKO_xormasks[WKO_graphbits[i]];
                            sval = val * -1;
                        } else sval = val;
                        slope = sval;
                        slope /= 10;
                        break;
                    case 'W' : /* Wind speed */
                        if (get_bit(thelot, bit-1)) { /* is negative */
                            val ^= WKO_xormasks[WKO_graphbits[i]];
                            sval = val * -1;
                        } else sval = val;
                        wind = sval;
                        wind /= 10;
                        break;
                    case 'A' : /* Altitude */
                        if (get_bit(thelot, bit-1)) { /* is negative */
                            val ^= WKO_xormasks[WKO_graphbits[i]];
                            sval = val * -1;
                        } else sval = val;
                        alt = sval;
                        alt /= 10;
                        break;
                    case 'T' : /* Torque */
                        if (imperialflag && WKO_GRAPHS[i]=='S') val = long((double)val * KMTOMI);
                        valp=val;
                        sprintf(GRAPHDATA[i], "%6ld.%1ld", valp/10, valp%10);
                        nm = val; nm /=10;
                        break;
                    case 'S' : /* Speed */
                        if (imperialflag && WKO_GRAPHS[i]=='S') val = long((double)val * KMTOMI);
                        valp=val;
                        sprintf(GRAPHDATA[i], "%6ld.%1ld", valp/10, valp%10);
                        kph = val; kph/= 10;

                        // distance is not available so we need to calculate it
                        if (calculate_distance) {
                            double distance;
                            double f, xi, xf;

                            // inc is in 1000ths of a second kph val is kph*10
                            rdist += (inc * kph) / 36;
                            distance = rdist; 
                            distance /= 100000;

                            // convert to imperial units ?
                            if (imperialflag) distance *= KMTOMI;

                            // round to max of 3 decimal places
                            xf = modf(distance,&xi);
                            f = floor(xf*1000+0.5)/1000.0;

                            sprintf(GRAPHDATA[i], "%g", xi+f);
                            km = xi+f;
                        } 
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
                            int32_t llat, llon;
                            char slat[20], slon[20];

                            // stored 2s complement
                            // conversion is * 180 / 2^31
                            val = get_bits(thelot, bit-64,32);
                            memcpy(&llat, &val, 4);
                            lat = (double)llat;
                            lat *= 0.00000008381903171539306640625;

                            val = get_bits(thelot, bit-32,32);
                            memcpy(&llon, &val, 4);
                            lon = (double)llon;
                            lon *= 0.00000008381903171539306640625;

                            // WKO handles drops in recording of GPS data
                            // as 180,180 -- we expect 0,0
                            llat=round(lat); llon=round(lon);
                            if (llat == 180 && llon == 180) lat=lon=0;

                            sprintf(slon, "%-3.9g", lon);
                            sprintf(slat, "%-3.9g", lat);

                            sprintf(GRAPHDATA[i], "%13s %13s", slat,slon);

                        }
                        break;
                    case 'P' :
                        watts = val;
                        valp=val;
                        sprintf(GRAPHDATA[i], "%8lu", valp); // just output as numeric
                        break;

                    case 'H' :
                        hr = val;
                        valp=val;
                        sprintf(GRAPHDATA[i], "%8lu", valp); // just output as numeric
                        break;

                    case 'C' :
                        cad = val;
			valp = val;
                        sprintf(GRAPHDATA[i], "%8lu", valp); // just output as numeric
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
                            kph, nm, watts, alt, lon, lat, wind, 0);
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
WKO_UCHAR *WkoParseHeaderData(QString fname, WKO_UCHAR *fb, RideFile *rideFile, QStringList &errors)
{
    unsigned long julian, sincemidnight;
    WKO_UCHAR *goal, *notes, *code; // save location of WKO metadata

    enum configtype lastchart;
    unsigned int x,z,i;

    /* utility holders */
    WKO_ULONG num;
    WKO_ULONG ul;
    WKO_ULONG sport;
    WKO_USHORT us;
    double g;

    boost::scoped_array<WKO_UCHAR> txtbuf(new WKO_UCHAR[1024000]);

    /* working through filebuf */
    WKO_UCHAR *p = fb;

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
    rideFile->setTag("Objective", (const char*)&txtbuf[0]);
    notes = p; p += dotext(p, &txtbuf[0]); /* 5: notes */

    p += dotext(p, &txtbuf[0]); /* 6: graphs */
    strcpy(reinterpret_cast<char *>(WKO_GRAPHS), reinterpret_cast<char *>(&txtbuf[0])); // save those graphs away

    p += donumber(p, &sport); /* 7: sport */
    switch (sport) {
        case 0x01 : rideFile->setTag("Sport", "Swim") ; break;
        case 0x02 : rideFile->setTag("Sport", "Bike") ; break;
        case 0x03 : rideFile->setTag("Sport", "Run") ; break;
        case 0x04 : rideFile->setTag("Sport", "Brick") ; break;
        case 0x05 : rideFile->setTag("Sport", "Cross Train") ; break;
        case 0x06 : rideFile->setTag("Sport", "Race ") ; break;
        case 0x07 : rideFile->setTag("Sport", "Day Off") ; break;
        case 0x08 : rideFile->setTag("Sport", "Mountain Bike") ; break;
        case 0x09 : rideFile->setTag("Sport", "Strength") ; break;
        case 0x0B : rideFile->setTag("Sport", "XC Ski") ; break;
        case 0x0C : rideFile->setTag("Sport", "Rowing") ; break;
        default   :
        case 0x64 : rideFile->setTag("Sport", "Other"); break;

    }
    code = p; p += dotext(p, &txtbuf[0]); /* 8: workout code */
    rideFile->setTag("Workout Code", (const char*)&txtbuf[0]);

    p += donumber(p, &ul); /* 9: duration 000s of seconds */
    p += dotext(p, &txtbuf[0]); /* 10: lastname */
    p += dotext(p, &txtbuf[0]); /* 11: firstname */

    p += donumber(p, &ul); /* 12: time of day */
    sincemidnight = ul;

    char rideFileRegExp[] = "^((\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)"
                                   "_(\\d\\d)_(\\d\\d)_(\\d\\d))\\.(.+)$";
    QRegExp rx(rideFileRegExp);

    if (rx.exactMatch(fname)) {

        // set the date and time from the filename
        QDate date(rx.cap(2).toInt(), rx.cap(3).toInt(),rx.cap(4).toInt());
        QTime time(rx.cap(5).toInt(), rx.cap(6).toInt(),rx.cap(7).toInt());
        QDateTime datetime(date, time);

        rideFile->setStartTime(datetime);

    } else {

        // date not available from filename use WKO metadata
        QDateTime datetime(QDate::fromJulianDay(julian),
               QTime(sincemidnight/360000, (sincemidnight%360000)/6000, (sincemidnight%6000)/100));
        rideFile->setStartTime(datetime);
    }

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

        // Sport type
        switch (sport) {

        case 0x01 : out << "Swim " ; break;
        case 0x02 : out << "Bike " ; break;
        case 0x03 : out << "Run " ; break;
        case 0x04 : out << "Brick " ; break;
        case 0x05 : out << "Cross Train " ; break;
        case 0x06 : out << "Race " ; break;
        case 0x07 : out << "Day Off " ; break;
        case 0x08 : out << "Mountain Bike " ; break;
        case 0x09 : out << "Strength " ; break;
        case 0x0B : out << "XC Ski " ; break;
        case 0x0C : out << "Rowing " ; break;
        default   :
        case 0x64 : out << "Other" << endl; break;

        }

        // Workout Code
        dotext(code, &txtbuf[0]);
        scode = (const char *)&txtbuf[0];
        out << scode << endl;

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
    rideFile->setTag("Weight", QString("%1").arg((double)ul/100.00));

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

    switch (WKO_device) {
    case 0x01 : rideFile->setDeviceType("Powertap"); break;
    case 0x04 : rideFile->setDeviceType("SRM"); break;
    case 0x05 : rideFile->setDeviceType("Polar"); break;
    case 0x06 : rideFile->setDeviceType("Computrainer/Velotron"); break;
    case 0x11 : rideFile->setDeviceType("Ergomo"); break;
    case 0x12 : rideFile->setDeviceType("Garmin Edge 205/305"); break;
    case 0x13 : rideFile->setDeviceType("Garmin Edge 705"); break;
    case 0x14 : rideFile->setDeviceType("iBike"); break;
    case 0x16 : rideFile->setDeviceType("Cycleops 300PT"); break;
    case 0x19 : rideFile->setDeviceType("Ergomo"); break;
    default : rideFile->setDeviceType("WKO"); break;
    }

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

    // The ranges are stored as references to data points
    // whilst the RideFileInterval structure uses start and stop
    // in seconds. We cannot translate from one to the other at this
    // point because the raw data has not been parsed yet.
    // So intervals are created here with point references
    // and they are post-processed after the call to
    // WkoParseRawData in openRideFile above.
    p += doshort(p, &us); /* 237: Number of ranges XXVARIABLEXX */
    for (i=0; i<us; i++) {
        RideFileInterval *add = new RideFileInterval();    // add new interval

        p += 12;
        //p += donumber(p, &ul);
        //p += donumber(p, &ul);
        //p += donumber(p, &ul);

        p += dotext(p, &txtbuf[0]);
        add->name = reinterpret_cast<char *>(&txtbuf[0]);
        p += dotext(p, &txtbuf[0]);

        p += donumber(p, &ul);
        p += donumber(p, &ul);
        add->start = ul;

        p += donumber(p, &ul);
        add->stop = ul;

        p += 12;
        //p += donumber(p, &ul);
        //p += donumber(p, &ul);
        //p += donumber(p, &ul);

        // add to intervals - referencing data point for interval
        references.append(add);
    }

    /***************************************************
     * 3: DEVICE SPECIFIC DATA
     ***************************************************/
    QString deviceInfo;
    p += doshort(p, &us); /* 249: Device/Token pairs XXVARIABLEXX */
    for (i=0; i<us; i++) {
        p += dotext(p, &txtbuf[0]);
        deviceInfo += QString("%1 = ").arg((char*)&txtbuf[0]);
        p += dotext(p, &txtbuf[0]);
        deviceInfo += QString("%1\n").arg((char*)&txtbuf[0]);
    }
    rideFile->setTag("Device Info", deviceInfo);

    /***************************************************
     * 4: PERSPECTIVE CHARTS & CACHES
     ***************************************************/

    p += doshort(p, &us);       /* 251: Number of charts XXVARIABLEXX */
    z = us;

    num=0;

    while (z) {     /* Until we hit first chart data */

        WKO_ULONG prev=0;
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

                WKO_ULONG recs, term;

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

                /* there is more data in there! */
                doshort(p, &us);
                if (us == 1) {
                    p += 6;
                    doshort(p, &us);
                    if (us != 0xffff) {
                        p += 18;
                        p += doshort(p, &us);
                        p += us*8;
                    }
                }

                p += optpad(p);

            } else if (x == 0x0c) {     /* Read through Mean Max Chart */

                WKO_ULONG recs, term;
                WKO_ULONG two;

                p += donumber(p, &ul); /* 253.4: Log or Linear meanmax ? */
                p += donumber(p, &ul); /* 253.4: Which Channel ? */
                p += donumber(p, &ul); /* 253.4: UNKNOWN */
                p += donumber(p, &ul); /* 253.4: Units */
                p += donumber(p, &ul); /* 253.4: Conversion type ? */
                two = ul;

                p += dodouble(p, &g);  /* 253.4: Conversion Factor 1 */

                if (two == 2) p += donumber(p, &ul);  /* 253.4: Conversion Factor 2 */

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
            errors << ("Unrecognised segment");
            return NULL;
        }
    }

breakout:

    if (WKO_GRAPHS[0] == '\0') {
        errors << ("File contains no GRAPHS");
        return (WKO_UCHAR *)NULL;
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
        case 0x19:
        default:
            return 19;
            break;
        case 0x01:
        case 0x16: // Cycleops PT300
        case 0x11:
        case 0x00:
        case 0x12: // Garmin Edge 205/305
        case 0x13:
        case 0x14:
        case 0x1a: // SRM Powercontrol VI - moved to support files
                   // supplied by Alex Simmons Oct 2010, was
                   // previously set as 19 bits above.
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

WKO_ULONG nullvals(char g)
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
 ***********************************************************************/

unsigned int optpad(WKO_UCHAR *p)
{
    WKO_USHORT us;
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
    case 0x800a :       /* after fixup for distchart Jan 2010 */
    case 0x800b :
    case 0x800c :       /* after fixup for distchart Jan 2010 */
    case 0x800d :       /* from Jim B 2nd Oct 2009 */
    case 0x800e :       /* from Phil S 4th Oct 2009 */
    case 0x800f :       /* after fixup for distchart Jan 2010 */
    case 0x8010 :       /* after fixup for distchart Jan 2010 */
    case 0x8011 :       /* after fixup for distchart Jan 2010 */
    case 0x8012 :       /* after fixup for distchart Jan 2010 */
    case 0x8013 :       /* after fixup for distchart Jan 2010 */
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

/************************************************************************************
 * BIT TWIDDLING FUNCTIONS
 *
 * get_bit() - return 0 or 1 for the given bit from a large array
 * get_bits() - return a range of bits read right to left (high bit first)
 *
 ************************************************************************************/
int get_bit(WKO_UCHAR *data, unsigned bitoffset) // returns the n-th bit
{
    WKO_UCHAR c = data[bitoffset >> 3]; // X>>3 is X/8
    WKO_UCHAR bitmask = 1 << (bitoffset %8);  // X&7 is X%8
    return ((c & bitmask)!=0) ? 1 : 0;
}

unsigned int get_bits(WKO_UCHAR* data, unsigned bitOffset, unsigned numBits)
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
unsigned int dofloat(WKO_UCHAR *p, float *pnum)
{
    memcpy(pnum, p, 4);
    return 4;
}

unsigned int dodouble(WKO_UCHAR *p, double *pnum)
{
    memcpy(pnum, p, 8);
    return 8;
}

unsigned int donumber(WKO_UCHAR *p, WKO_ULONG *pnum)
{
    *pnum = qFromLittleEndian<quint32>(p);
    return 4;
}

void pbin(WKO_UCHAR x)
{
    static WKO_UCHAR bits[]={ 128, 64, 32, 16, 8, 4, 2, 1 };
    int i;
    for (i=0; i<8; i++)
        printf("%c", ((x&bits[i]) == bits[i]) ? '1' : '0');

}


unsigned int dotext(WKO_UCHAR *p, WKO_UCHAR *buf)
{
    WKO_USHORT us;

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

unsigned int doshort(WKO_UCHAR *p, WKO_USHORT *pnum)
{
    *pnum = qFromLittleEndian<quint16>(p);
    return 2;
}

