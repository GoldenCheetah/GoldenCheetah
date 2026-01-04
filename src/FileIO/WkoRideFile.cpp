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

#include "WkoRideFile.h"
#include "Units.h"

#include <QRegExp>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <algorithm> // for std::sort
#include "cmath"

#ifdef Q_OS_WIN
// 'sprintf': This function or variable may be unsafe.
// 'strcpy': This function or variable may be unsafe.
// 'strncpy': This function or variable may be unsafe.
#pragma warning(disable:4996)
#endif

static int wkoFileReaderRegistered = RideFileFactory::instance().registerReader(
                                     "wko", "WKO+ Files", new WkoFileReader());

RideFile *WkoFileReader::openRideFile(QFile &file,
                                      QStringList &errors,
                                      QList<RideFile*>*rides) const
{
    WkoParser parser(file,errors,rides);
    return parser.result();
}

/*----------------------------------------------------------------------
 * WKO Parser for parsing .wko file format, ride is parsed as part
 * of the constructor, so no need to explicitly call any parsing functions
 *--------------------------------------------------------------------*/

WkoParser::WkoParser(QFile &file, QStringList &errors, QList<RideFile*>*rides)
          : file(file), results(NULL), errors(errors), rides(rides)
{
    // we reference the filename to parse start date/time
    filename = file.fileName();
    QFileInfo fileinfo(file);

    // open the source file, we never update so read only and then
    // read in the whole file, proved faster when bit twiddling the
    // raw data and avoids unbounded memory allocations when ride data
    // is corrupt or we parse it incorrectly
    if (!file.open(QFile::ReadOnly)) {
        errors << ("Could not open ride file: \"" + file.fileName() + "\"");
        return;
    }

    // is it big enough?
    if (file.size() < 0x200) {
        file.close();
        errors << "Not a WKO+ file";
        return;
    }

    bufferSize = file.size();
    QScopedArrayPointer<WKO_UCHAR> entirefile(new WKO_UCHAR[bufferSize]);
    QDataStream *rawstream(new QDataStream(&file));
    headerdata = &entirefile[0];
    rawstream->readRawData(reinterpret_cast<char *>(headerdata), file.size());
    file.close();


    // Check the header to make sure it really is a WKO
    // file, the magic number for WKO is 'W' 'K' 'O' ^Z
    if (*headerdata != 'W' ||
        *(headerdata+1) != 'K' ||
        *(headerdata+2) != 'O' ||
        *(headerdata+3) != 0x1A) {
        errors << "Not a WKO+ file";
        return;
    }

    // We only support versions of the WKO file format we have seen
    // sufficient source files to reverse engineer and test these
    // are for CP v1.0 and v1.1 and then WKO v2.2 *or higher*
    donumber(headerdata+4, &version);

    // versions we don't support are rejected
    if (version < 28 && version != 1 && version != 12 && version != 7) {
        errors << (QString("Version of file (%1) is too old, open and save in WKO then retry: \"").arg(version) + file.fileName() + "\"");
        return;

    // later versions may change so support but warn
    } else if (version >31) {
        errors << ("Version of file is new and not fully supported yet: \"" +
        file.fileName() + "\"");
    }

    // Allocate space for newly parsed ride
    results = new RideFile;
    //results->setTag("File Format", QString("WKO v%1").arg(version));
    results->setFileFormat(QString("WKO v%1 (wko)").arg(version));

    // read header data and store details into rideFile structure
    rawdata = parseHeaderData(headerdata);

    // Parse raw data (which calls rideFile->appendPoint() with each sample
    if (rawdata) footerdata = parseRawData(rawdata);
    else {
        delete results;
        results = NULL;
        return;
    }

    // is recIntSecs a daft value?
    if (results->recIntSecs() < 0.1) {

        // lets see what the most popular recording interval is...
        QMap<double, int> ints;

        bool first = true;
        double last = 0;

        foreach(RideFilePoint *p, results->dataPoints()) {

            if (first) {
                last = p->secs;
                first = false;
            } else {
                double delta = p->secs-last;
                last = p->secs;

                // lookup
                int count = ints.value(delta);
                count++;
                ints.insert(delta, count);
            }
        }

        // which is most popular?
        double populardelta=1.0;
        int count=0;
        QMapIterator<double, int> i(ints);
        while (i.hasNext()) {
            i.next();

            if (i.value() > count) {
                count = i.value();
                populardelta = i.key();
            }
        }
        results->setRecIntSecs(populardelta);
    }

    // adjust times to start at zero, some ridefiles have
    // a start time that really blows up the CPX calculator
    // e.g. first sample at 19 hrs ..
    if (results->dataPoints().count() && results->dataPoints().first()->secs > 0) {
        double sub = results->dataPoints().first()->secs;

        foreach(RideFilePoint *p, results->dataPoints())
            p->secs -= sub;
    }

    // Post process  the ride intervals to convert from point offsets to time in seconds
    QVector<RideFilePoint*> datapoints = results->dataPoints();
    for (int i=0; i<references.count(); i++) {
        RideFileInterval add;

        // name is ok, but start stop need to be in time rather than datapoint no.
        add.name = references.at(i)->name;

        if (references.at(i)->start < datapoints.count())
            add.start = datapoints.at(references.at(i)->start)->secs;
        else continue;

        if (references.at(i)->stop < datapoints.count())
            add.stop = datapoints.at(references.at(i)->stop)->secs + results->recIntSecs()-.001;
        else continue;

        results->addInterval(RideFileInterval::DEVICE, add.start, add.stop, add.name);
    }

    // free up temporary storage for range post processing
    for (int i=0; i<references.count(); i++) delete references.at(i);
    references.clear();

    // if we got to the end then close with success
    if (footerdata) return; 
    else {
        // failed, so wipe what we got and return
        delete results;
        results = NULL;
        return;
    }
}


/***************************************************************************************
 * PROCESS THE RAW DATA
 *
 * parseRawData() - read through all the raw data adding record points and return
 *                     a pointer to the footer record
 **************************************************************************************/
WKO_UCHAR *
WkoParser::parseRawData(WKO_UCHAR *fb)
{
    WKO_ULONG WKO_xormasks[32];    // xormasks used all over
    double cad=0, hr=0, km=0, kph=0, nm=0, watts=0, slope=0, alt=0, lon=0, lat=0, wind=0, temp=RideFile::NA, interval=0;

    int isnull=0;
    WKO_ULONG records=0, data;
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
        WKO_nullval[i] = nullvals(WKO_GRAPHS[i], version); // setup nullvalue
        if ((WKO_graphbits[i] = bitsize(WKO_GRAPHS[i], WKO_device, version)) == 0) { // setup & check field size
            errors << QString("Unknown channel %1").arg(WKO_GRAPHS[i]);
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
        if (version == 1) bit = 32;
        else bit = 43;
    }
    interval = inc;
    interval /= 1000;

    results->setRecIntSecs(interval);

#if 0
// used when debugging
    qDebug()<<QString("0x%1").arg(WKO_device,0,16)<<version<<WKO_GRAPHS<<"records="<<records;
    for (int xbit=bit; xbit < 500; xbit++)
    if (get_bits(thelot, xbit, 1)) fprintf(stderr, "1");
        else fprintf(stderr, "0");
    qDebug()<<"";
#endif

    int maxbit = 8 * (bufferSize - (rawdata-headerdata));

    /*------------------------------------------------------------------------------
     * RUN THROUGH EACH RAW DATA RECORD
     *==============================================================================*/
    rdist=rtime=0;
    while (records && bit < maxbit) {

        unsigned int marker;
        WKO_LONG sval=0;
        WKO_ULONG val=0;
	    unsigned long valp; // for printf
	    long svalp; // for printf

        // reset point values;
        alt = slope = wind = cad= hr= km= kph= nm= watts= 0.0;
        temp= RideFile::NA;

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
                        temp = sval;
                        sprintf(GRAPHDATA[i], "%8ld", svalp);
                        break;
                    case '^' : /* Slope */
                        if (get_bit(thelot, bit-1)) { /* is negative */
                            val ^= WKO_xormasks[WKO_graphbits[i]];
                            sval = val * -1;
                        } else sval = val;
                        slope = sval;
                        slope /=10;
                        break;
                    case 'W' : /* Wind speed */
                        if (get_bit(thelot, bit-1)) { /* is negative */
                            val ^= WKO_xormasks[WKO_graphbits[i]];
                            sval = val * -1;
                        } else sval = val;
                        wind = sval;
                        wind /=10;
                        break;
                    case 'A' : /* Altitude */
                        if (get_bit(thelot, bit-1)) { /* is negative */
                            val ^= WKO_xormasks[WKO_graphbits[i]];
                            sval = val * -1;
                        } else sval = val;
                        alt = sval;
                        alt /=10;
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
                        if (version == 1 || version == 7) distance /= 1000;
                        else distance /= 100000;

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
                    results->appendPoint((double)rtime/1000, cad, hr, km,
                            kph, nm, watts, alt, lon, lat, wind, slope, temp, 0.0, 
                            0.0,0.0,0.0,0.0, // vector pedal torque eff and smoothness not supported in WKO (?)
                            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                            0.0,0.0,
                            0.0,0.0,0.0, // running dynamics
                            0.0, // tcore
                            0);
            }

            // increment time - even for null records (perhaps especially for null
            // records since they are there specifically to handle pause in recording!
            rtime += inc;

        } else {
            // pause record increments time

            /* set the increment */
            unsigned long pausetime;
            int pausesize;

            // pause record different in version 1
            if (version == 1) pausesize=31;
            else pausesize=42;

            /* set increment value -> if followed by a null record
               it is to show a pause in recording -- velotrons seem to cause
               lots and lots of these
            */
            pausetime = get_bits(thelot, bit, 32);
            if (version !=  1) inc = pausetime;
#if 0
                fprintf(stderr, "pausetime: ");
                for (int i=0; i<pausesize+42; i++) {
                    int x = get_bits(thelot, bit+i, 1);
                    fputc(x ? '1' : '0', stderr);
                }
                fprintf(stderr, " %ld ", pausetime);
                fputc('\n', stderr);
#endif
            bit += pausesize;
        }
    }

    return thelot + (bit/8);

}


/**********************************************************************
 * ZIP THROUGH ALL THE WKO SPECIFIC DATA, LAND ON THE RAW DATA
 * AND RETURN A POINTER TO THE FIRST BYTE OF THE RAW DATA
 *
 * parseHeadeData() - read through file and land on the raw data
 *
 *********************************************************************/
WKO_UCHAR *
WkoParser::parseHeaderData(WKO_UCHAR *fb)
{
    unsigned long julian, sincemidnight;
    WKO_UCHAR *goal=NULL, *notes=NULL, *code=NULL; // save location of WKO metadata

    /* utility holders */
    WKO_ULONG num;
    WKO_ULONG ul;
    WKO_ULONG sport;
    WKO_USHORT us;
    double g;

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
    results->setTag("Objective", (const char*)&txtbuf[0]);
    notes = p; p += dotext(p, &txtbuf[0]); /* 5: notes */

    p += dotext(p, &txtbuf[0]); /* 6: graphs */
    strcpy(reinterpret_cast<char *>(WKO_GRAPHS), reinterpret_cast<char *>(&txtbuf[0])); // save those graphs away

    if (version != 1) { //!!! Version 1 beta support
        p += donumber(p, &sport); /* 7: sport */
    } else {
        sport = 0x02; // only bike was supported in files this old
    }


    switch (sport) {
        case 0x01 : results->setTag("Sport", "Swim") ; break;
        case 0x02 : results->setTag("Sport", "Bike") ; break;
        case 0x03 : results->setTag("Sport", "Run") ; break;
        case 0x04 : results->setTag("Sport", "Brick") ; break;
        case 0x05 : results->setTag("Sport", "Cross Train") ; break;
        case 0x06 : results->setTag("Sport", "Race ") ; break;
        case 0x07 : results->setTag("Sport", "Day Off") ; break;
        case 0x08 : results->setTag("Sport", "Mountain Bike") ; break;
        case 0x09 : results->setTag("Sport", "Strength") ; break;
        case 0x0B : results->setTag("Sport", "XC Ski") ; break;
        case 0x0C : results->setTag("Sport", "Rowing") ; break;
        default   :
        case 0x64 : results->setTag("Sport", "Other"); break;

    }

    QString notesTag;
    notesTag = results->getTag("Sport", "Bike"); // we just set it, so default is meaningless.
    notesTag += "\n";


    if (version != 1) { //!!! Version 1 beta support

        // workout code and duration not in v1 files
        code = p; p += dotext(p, &txtbuf[0]); /* 8: workout code */
        results->setTag("Workout Code", (const char*)&txtbuf[0]);
        p += donumber(p, &ul); /* 9: duration 000s of seconds */
    }

    p += dotext(p, &txtbuf[0]); /* 10: lastname */
    p += dotext(p, &txtbuf[0]); /* 11: firstname */

    p += donumber(p, &ul); /* 12: time of day */
    sincemidnight = ul;

    char rideFileRegExp[] = "^((\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)"
                                   "_(\\d\\d)_(\\d\\d)_(\\d\\d))\\.(.+)$";
    QRegExp rx(rideFileRegExp);

    if (rx.exactMatch(filename)) {

        // set the date and time from the filename
        QDate date(rx.cap(2).toInt(), rx.cap(3).toInt(),rx.cap(4).toInt());
        QTime time(rx.cap(5).toInt(), rx.cap(6).toInt(),rx.cap(7).toInt());
        QDateTime datetime(date, time);

        results->setStartTime(datetime);

    } else {

        // date not available from filename use WKO metadata
        // Note: WKO format sure is special, tested with file 2008_06_25_20_59_49.wko renamed to 'my.wko'. 
        QDateTime datetime(QDate::fromJulianDay(julian),
            QTime(sincemidnight/MS_IN_WKO_HOURS, (sincemidnight%MS_IN_WKO_HOURS)/6000, (sincemidnight%6000)/100));
                
        results->setStartTime(datetime);
    }

    QString scode, sgoal, snote;

    if (version != 1) {
        // Workout Code
        dotext(code, &txtbuf[0]);
        scode = (const char *)&txtbuf[0];
        notesTag += scode; notesTag += "\n";
    }

    dotext(goal, &txtbuf[0]);
    sgoal = (const char *)&txtbuf[0];
    notesTag += "WORKOUT GOAL"; notesTag += "\n";
    notesTag += sgoal; notesTag += "\n";

    dotext(notes, &txtbuf[0]);
    snote = (const char *)&txtbuf[0];
    notesTag += "WORKOUT NOTES"; notesTag += "\n";
    notesTag += snote; notesTag += "\n";

    results->setTag("Notes", notesTag);

    if (version != 1) {
        p += donumber(p, &ul); /* 13: distance travelled in meters */
        p += donumber(p, &ul); /* 14: device recording interval */
        p += donumber(p, &ul); /* 15: athlete max heartrate */
        p += donumber(p, &ul); /* 16: athlete threshold heart rate */
        p += donumber(p, &ul); /* 17: athlete threshold power */
        if (version != 12 && version != 7)
            p += dodouble(p, &g);  /* 18: athlete threshold pace */
        p += donumber(p, &ul); /* 19: weight in grams/10 */
        results->setTag("Weight", QString("%1").arg((double)ul/100.00));

        p += 28;
        //p += donumber(p, &ul); /* 20: unknown */
        //p += donumber(p, &ul); /* 21: unknown */
        //p += donumber(p, &ul); /* 22: unknown */
        //p += donumber(p, &ul); /* 23: unknown */
        //p += donumber(p, &ul); /* 24: unknown */
        //p += donumber(p, &ul); /* 25: unknown */
        //p += donumber(p, &ul); /* 26: unknown */

    } else { //!!! Version 1 Beta Support
        p += 52; // not decoded at present
    }

    /***************************************************
     * 2:  GRAPH TAB - MAX OF 16 CHARTING GRAPHS
     ***************************************************/
    p += donumber(p, &ul); /* 27: graph view */
    p += donumber(p, &ul); /* 28: WKO_device type */
    WKO_device = ul; // save WKO_device

    switch (WKO_device) {
    case 0x00 : // early versions set device to zero for powertap
    case 0x01 : results->setDeviceType("Powertap"); break;
    case 0x1a : // also SRM - PowerControl VI
    case 0x04 : results->setDeviceType("SRM"); break; // powercontrol V
    case 0x05 : results->setDeviceType("Polar"); break;
    case 0x06 : results->setDeviceType("Computrainer/Velotron"); break;
    case 0x0e : // also ergomo
    case 0x11 : results->setDeviceType("Ergomo"); break;
    case 0x12 : results->setDeviceType("Garmin Edge 205/305"); break;
    case 0x13 : results->setDeviceType("Garmin Edge 705"); break;
    case 0x14 : results->setDeviceType("iBike"); break;
    case 0x15 : results->setDeviceType("Suunto"); break;
    case 0x16 : results->setDeviceType("Cycleops 300PT"); break;
    case 0x19 : results->setDeviceType("Ergomo"); break;
    default : results->setDeviceType(QString("WKO (0x%1)").arg(WKO_device,0,16)); break;
    }

    p += donumber(p, &ul); // not in version 12?

    int arraysize = 0;
    if (version < 12) arraysize = 8;
    if (version == 12) arraysize = 14;
    if (version >= 28) arraysize = 16;

    for (int i=0; i< arraysize; i++) { // 16 types of chart data

        p += 44;

        // gridlines in WKO+ become references in the ridefile
        // we only support Power, but have never seen it for other
        // series in cycling files, EVER. (>8000 files)
        p += doshort(p, &us); /* 41: number of gridlines XXVARIABLEXX */
        for(int j=0; j<us; j++) {

            WKO_ULONG val, series;
            p += donumber(p, &val); // value first
            p += donumber(p, &series); // series second

            // series of 0 or 1 seems to be power
            if (val > 0 && val < 2500 && series < 2) {
                RideFilePoint ref;
                ref.watts = val;
                results->appendReference(ref);
            }
        }
    }

    /* Ranges */

    // The ranges are stored as references to data points
    // whilst the RideFileInterval structure uses start and stop
    // in seconds. We cannot translate from one to the other at this
    // point because the raw data has not been parsed yet.
    // So intervals are created here with point references
    // and they are post-processed after the call to
    // parseRawData in openRideFile above.
    p += doshort(p, &us); /* 237: Number of ranges XXVARIABLEXX */
    for (int i=0; i<us; i++) {
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

        if (version != 1) { //!!! Version 1 Beta Support
            p += 12;
        } else {
            p += 8;
        }
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

    for (int i=0; i<us; i++) {
        p += dotext(p, &txtbuf[0]);
        deviceInfo += QString("%1 = ").arg((char*)&txtbuf[0]);
        p += dotext(p, &txtbuf[0]);
        deviceInfo += QString("%1\n").arg((char*)&txtbuf[0]);
    }
    results->setTag("Device Info", deviceInfo);

    /***************************************************
     * 4: PERSPECTIVE CHARTS & CACHES
     ***************************************************/

    p += doshort(p, &us);       /* 251: Number of charts XXVARIABLEXX */
    charts = us;

    num=0;

    while (charts) {     // keep parsing until we have no charts left

        // basic bounds check
        if ((p - fb) > bufferSize) {
            errors << "Buffer overrun, file may be corrupt / truncated";
            qDebug()<<"buffer overrun";
            return NULL;
        }

        enum configtype type=INVALID;

        p += donumber(p, &ul);
        num = ul;

        // Each config section is preceded with
        // 0xff 0xff 0x01 0x00 
        if (num==0x01ffff) {

            char buf[32];

            // Config Type
            p += doshort(p, &us); // got here...
            strncpy (reinterpret_cast<char *>(buf), reinterpret_cast<char *>(p), us); buf[us]=0;
            p += us;

            /* What type? */
            if (!strcmp(buf, "CRideSettingsConfig")) type=CRIDESETTINGSCONFIG;
            if (!strcmp(buf, "CRideGoalConfig")) type=CRIDEGOALCONFIG;
            if (!strcmp(buf, "CRideNotesConfig")) type=CRIDENOTESCONFIG;
            if (!strcmp(buf, "CDistributionChartConfig")) type=CDISTRIBUTIONCHARTCONFIG;
            if (!strcmp(buf, "CRideSummaryConfig")) type=CRIDESUMMARYCONFIG;
            if (!strcmp(buf, "CMeanMaxChartConfig"))type=CMEANMAXCHARTCONFIG;
            if (!strcmp(buf, "CMeanMaxChartCache")) type=CMEANMAXCHARTCACHE;
            if (!strcmp(buf, "CDistributionChartCache")) type=CDISTRIBUTIONCHARTCACHE;

            if (type == CDISTRIBUTIONCHARTCACHE) {

                p = parseCDistributionChartCache(p);

            } else if (type == CMEANMAXCHARTCACHE) {

                p = parseCMeanMaxChartCache(p);

            } else if (type == CRIDESUMMARYCONFIG) {

                p = parseCRideSummaryConfig(p);

            } else if (type == CRIDENOTESCONFIG) {

                p = parseCRideNotesConfig(p);

            } else if (type == CRIDEGOALCONFIG) {

                p = parseCRideGoalConfig(p);

            } else if (type == CRIDESETTINGSCONFIG) {

                p = parseCRideSettingsConfig(p);

            } else if (type == CDISTRIBUTIONCHARTCONFIG) {

                p = parseCDistributionChartConfig(p);

            } else if (type == CMEANMAXCHARTCONFIG) {

                p = parseCMeanMaxChartConfig(p);

            }

        } else if (num == 1) {

            // version 12 leaves this at the end of ride summary

        } else if (num==2) {

            /* Perspective */
            p += dotext(p, &txtbuf[0]);

        } else if (num==3) {

            p = parseChart(p);

        } else {

            errors << "Could not parse chart data"
                   << QString("0x%1 @0x%2").arg(num, 0, 16).arg(p-fb, 0, 16);
            return NULL;
        }
    }

    if (WKO_GRAPHS[0] == '\0') {
        errors << ("Manual files not supported");
        return (WKO_UCHAR *)NULL;
    } else {
        /* PHEW! We're on the raw data, out job here is done */
        return (p);
    }
}


/*==============================================================================
 * Parse chart segments - the really hard stuff!
 *==============================================================================*/
WKO_UCHAR *WkoParser::parsePerspective(WKO_UCHAR *p)
{
    // perspective - not present in early releases
    if (version != 1 && version != 7 && version != 12) {
        p += donumber(p, &ul); /* always 2 */
        p += dotext(p, &txtbuf[0]); /* perspective */
    } 
    return p;
}

WKO_UCHAR *WkoParser::parseChart(WKO_UCHAR *p, int type)
{
    charts--;

    // chart name
    p += dotext(p, &txtbuf[0]);

    // preamble
    if (version == 1) p+= 24;
    else p += 32;

    // earlier versions don't have a type marker
    // so we let the caller tell us what we're parsing
    // otherwise we work it when we can
    if (!type) {
        p += donumber(p, &ul);
        type = ul;
    } else {
        p+= 4;
    }

    if (type == 0x02) { // distribution chart
        p += 64;

        // record count
        p += doshort(p, &us);

        for (int i=0; i<us; i++) {
            p += donumber(p, &ul);
            p += dotext(p, &txtbuf[0]);
            p += dotext(p, &txtbuf[0]);
            p += donumber(p, &ul);
            p += donumber(p, &ul);
        }

        // trailing data, especially for speed/pace distribution
        doshort(p, &us);

        // version 1 trailing data is different
        if (version == 1 && us == 0) p += 8;
        else if (version != 1 && us == 1) {

            p += 6;
            doshort(p, &us);
            if (us != 0xffff) {
                p += 18;
                p += doshort(p, &us);
                p += us*8;
            }
        }

        p += optpad(p);

    } else {     // Mean Max Chart

        WKO_ULONG recs, term;
        WKO_ULONG two;
        double g;

        p += donumber(p, &ul); /* 253.4: Log or Linear meanmax ? */
        p += donumber(p, &ul); /* 253.4: Which Channel ? */
        p += donumber(p, &ul); /* 253.4: UNKNOWN */
        p += donumber(p, &ul); /* 253.4: Units */
        p += donumber(p, &ul); /* 253.4: Conversion type ? */
        two = ul;

        p += dodouble(p, &g);  /* 253.4: Conversion Factor 1 */

        if (two == 2) p += donumber(p, &ul);  /* 253.4: Conversion Factor 2 */

        // record count
        p += doshort(p, &us);
        recs = us;

        // loop through the records
        if (recs) {
        while (recs--) {

            p += donumber(p, &ul);
            term = ul;
            if (!term) return p;
            else {

                p += 28;
            }
        }

        // preamble
        p += 12;

        // get record count
        p += doshort(p, &us);
        recs=us;
        term=1;

        // loop through the records
        while (recs--) {
            p += donumber(p, &ul);
            term=ul;
            if (!term) {
                break;
            } else {
                p += 28;
            }
        }

        /* as long as it didn't get terminated we need to read the end of the data */
        if (term) {
            if (version != 1) p += 24; //_read(fd, buf, 24);
            else p += 10;
        }
        }
        /* might still be padding ? */
        p += optpad(p);

    }

    return p;
}

WKO_UCHAR *WkoParser::parseCRideSettingsConfig(WKO_UCHAR *p)
{
    p = parsePerspective(p);
    p += donumber(p, &ul);

    // additional number in earlier release
    if (version == 12 || version == 1 || version == 7) p += donumber(p,&ul);
    charts--;
    return p;
}

WKO_UCHAR *WkoParser::parseCRideGoalConfig(WKO_UCHAR *p)
{
    p = parsePerspective(p);
    p += donumber(p, &ul);
    p += donumber(p, &ul);

    // additional number in earlier release
    if (version == 12 || version == 1 || version == 7) p += donumber(p,&ul);

    charts--;
    return p;
}

WKO_UCHAR *WkoParser::parseCRideNotesConfig(WKO_UCHAR *p)
{
    p = parsePerspective(p);
    p += donumber(p, &ul);
    p += donumber(p, &ul);

    // additional number in earlier release
    if (version == 12 || version == 1 || version == 7) p += donumber(p,&ul);
    charts--;
    return p;
}

WKO_UCHAR *WkoParser::parseCDistributionChartConfig(WKO_UCHAR *p)
{
    do {

        // always an extra number in earlier versions
        if (version == 1 || version == 7 || version == 12) p += donumber(p,&ul);
        p = parsePerspective(p);

        p += donumber(p, &ul);

        if (version != 1 && ul != 3) {
            errors << QString("Error Parsing CDistributionChartConfig got 0x%1 expected 0x03").arg(ul,0,16);
            return p;
        }

        p = parseChart(p, version == 1 ? 2 : 0);

        doshort(p, &us);

    } while (charts && us != 0xffff);

    return p;
}


WKO_UCHAR *WkoParser::parseCRideSummaryConfig(WKO_UCHAR *p)
{
    p = parsePerspective(p);
    if (version == 1 || version == 12 || version == 7) {
        p += 12;
        charts--;

    } else {
        p += donumber(p, &ul);
        p += donumber(p, &ul);
        charts--;
        /* might still be padding ? */
    }
    p += optpad(p);
    return p;
}

WKO_UCHAR *WkoParser::parseCMeanMaxChartConfig(WKO_UCHAR *p)
{
    do {

        // always an extra number in earlier versions
        if (version == 1 || version == 7 || version == 12) p += donumber(p,&ul);
        p = parsePerspective(p);

        p += donumber(p, &ul);

        if (version != 1 && ul != 3) {
            errors << QString("Error Parsing CMeanMaxChartConfig got 0x%1 expected 0x03").arg(ul,0,16);
            return p;
        }

        p = parseChart(p, version == 1 ? 0x0c : 0);

        doshort(p, &us);

    } while (charts && us != 0xffff);


    return p;
}

WKO_UCHAR *WkoParser::parseCMeanMaxChartCache(WKO_UCHAR *p) // only present in v28 onwards
{

    WKO_ULONG recs, term;

    p += 20;

    p += donumber(p, &ul);
    p += (ul * 8);

    p += 28;

    p += donumber(p, &ul);
    p += (ul * 8);

    /* now its the main structures from a meanmax chart */

    /* How many meanmax records in first set ? */
    p += doshort(p, &us);
    recs = us;

    while (recs--) {

        p += donumber(p, &ul);
        if (!ul) return p;
        else {

            p += 28;
        }
    }

    // preamble
    p += 12;

    // record count
    p += doshort(p, &us);
    recs = us;
    term=1;

    // loop over records
    while (recs--) {

        p += donumber(p, &ul);
        if (!ul) break;
        else {

            p += 28;
        }
    }

    // trail (if not a truncated data set
    if (term) p += 24;

    // skip over markers
    p += optpad(p);

    return p;
}

WKO_UCHAR *WkoParser::parseCDistributionChartCache(WKO_UCHAR *p) // only in v2.2 onwards
{

    // we just position after the initial block since it is then
    // followed by chart arrays that can be handled in the main loop
    // of parseHeaderData (num ==2 and num == 3)
    p += donumber(p, &ul);
    p += donumber(p, &ul);
    p += donumber(p, &ul);
    p += donumber(p, &ul);

    // count
    p += doshort(p, &us);

    while (us) {
        us--;
        p += donumber(p, &ul);
        p += donumber(p, &ul);
    }

    p += optpad(p);

    return p;
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
unsigned int
WkoParser::bitsize(char g, int WKO_device, WKO_ULONG version)
{
    if (version != 1 && version != 7) {
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
            case 0x11:
            case 0x00:
            case 0x12: // Garmin Edge 205/305
            case 0x13:
            case 0x14:
            case 0x15: // suunto
            case 0x16: // Cycleops PT300
            case 0x1a: // SRM Powercontrol VI
                       // Alex Simmons files Oct 2010
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
    } else { //!!! version 1

        switch (g) {
        case 'P' : if (version == 1) return 11;
                   else if (version == 7) return 12; //  was 12?
                   break;
        case 'H' : return (8); break;
        case 'C' : return (8); break;
        case 'S' : return (11); break;
        case 'A' : if (version == 7) return 16;
                   else return(14);
                   break; 
        case 'T' : return (11); break;
        case 'D' :
            /* distance */
            switch (WKO_device) {
            case 0x04:
            case 0x05:
            case 0x06:
            case 0x19:
            case 0x1a:
            default:
                return 11;
                break;
            case 0x01:
            case 0x11:
            case 0x16: // Cycleops PT300
            case 0x00:
            case 0x12: // Garmin Edge 205/305
            case 0x13:
            case 0x14:
                return 11;
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
    return 0; // impossible (in theory) but keeps compiler happy
}

WKO_ULONG
WkoParser::nullvals(char g, WKO_ULONG version)
{
    switch (g) {

    case 'P' : if (version != 1) return (4095L);
               else return (2047L);
               break;
    case 'H' : return (255L); break;
    case 'C' : return (255L); break;
    case 'S' : return (2047L); break;
    case 'A' : if (version > 7) return (524287L); // max is for 19 bits, sign bit 0 for null values
               else return (32767L);              // max is for 15 bits, sign bit 0 for null values
               break;
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

unsigned int
WkoParser::optpad(WKO_UCHAR *p)
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
    case 0x8009 :       /* Patrick McNally files Jan 2014 */
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
    case 0x8014 :       /* Q's v2 file */
    case 0x8015 :       /* Q's v2 file */
    case 0x8016 :       /* Q's v2 file */
    case 0x8017 :       /* Q's v2 file */
    case 0x8018 :       /* Q's v2 file */
    case 0x8019 :       /* Rainer's running file */
    case 0x801a :       /* Rainer's running file */
    case 0x801b :       /* Rainer's running file */
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
int
WkoParser::get_bit(WKO_UCHAR *data, unsigned bitoffset) // returns the n-th bit
{
    WKO_UCHAR c = data[bitoffset >> 3]; // X>>3 is X/8
    WKO_UCHAR bitmask = 1 << (bitoffset %8);  // X&7 is X%8
    return ((c & bitmask)!=0) ? 1 : 0;
}

unsigned int
WkoParser::get_bits(WKO_UCHAR* data, unsigned bitOffset, unsigned numBits)
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
unsigned int
WkoParser::dofloat(WKO_UCHAR *p, float *pnum)
{
    memcpy(pnum, p, 4);
    return 4;
}

unsigned int
WkoParser::dodouble(WKO_UCHAR *p, double *pnum)
{
    memcpy(pnum, p, 8);
    return 8;
}

unsigned int
WkoParser::donumber(WKO_UCHAR *p, WKO_ULONG *pnum)
{
    *pnum = qFromLittleEndian<quint32>(p);
    return 4;
}

void
WkoParser::pbin(WKO_UCHAR x)
{
    static WKO_UCHAR bits[]={ 128, 64, 32, 16, 8, 4, 2, 1 };
    int i;
    for (i=0; i<8; i++)
        printf("%c", ((x&bits[i]) == bits[i]) ? '1' : '0');

}


unsigned int
WkoParser::dotext(WKO_UCHAR *p, WKO_UCHAR *buf)
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

unsigned int
WkoParser::doshort(WKO_UCHAR *p, WKO_USHORT *pnum)
{
    *pnum = qFromLittleEndian<quint16>(p);
    return 2;
}
