#include <PythonEmbed.h>
#include "Context.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "Athlete.h"
#include "GcUpgrade.h"
#include "PythonChart.h"
#include "Colors.h"
#include "RideCache.h"
#include "DataFilter.h"
#include "PMCData.h"
#include "Season.h"
#include "WPrime.h"
#include "Zones.h"
#include "HrZones.h"
#include "PaceZones.h"
#include "DataProcessor.h"
#include "Perspective.h"

#include "Bindings.h"

#include <QWebEngineView>
#include <QUrl>
#include <datetime.h> // for Python datetime macros

long Bindings::threadid() const
{
    // Get current thread ID via Python thread functions
    PyObject* thread = PyImport_ImportModule("_thread");
    PyObject* get_ident = PyObject_GetAttrString(thread, "get_ident");
    PyObject* ident = PyObject_CallObject(get_ident, 0);
    Py_DECREF(get_ident);
    long t = PyLong_AsLong(ident);
    Py_DECREF(ident);
    return t;
}

long Bindings::build() const
{
    return VERSION_LATEST;
}

QString Bindings::version() const
{
    return VERSION_STRING;
}

bool
Bindings::webpage(QString url) const
{
    if (!python->chart) return false; // Do nothing when no chart is avaliable

#ifdef Q_OS_WIN
    url = url.replace("://C:", ":///C:"); // plotly fails to use enough slashes
    url = url.replace("\\", "/");
#endif

    QUrl p(url);
    python->chart->emitUrl(p);
    return true;
}

bool 
Bindings::configChart(QString title, int type, bool animate, int pos, bool stack, int orientation) const
{
    if (!python->chart) return false; // Do nothing when no chart is avaliable
    python->chart->emitChart(title, type, animate, pos, stack, orientation);
    return true;
}

bool 
Bindings::setCurve(QString name, PyObject *xseries, PyObject *yseries, QStringList fseries, QString xname, QString yname,
                      QStringList labels,  QStringList colors,
                      int line, int symbol, int size, QString color, int opacity, bool opengl, bool legend, bool datalabels, bool fill) const
{
    if (!python->chart) return false; // Do nothing when no chart is avaliable

    QVector<double>xs, ys;

    // xseries type conversion
    if (!PyList_Check(xseries)) {
         PyErr_Format(PyExc_TypeError, "The argument 'xseries' must be of list or subtype of list");
         return false;
    } else {
        // convert to QVector TODO convert in SIP
        for (Py_ssize_t i=0; i <PyList_Size(xseries); i++) {
            PyObject *it = PyList_GET_ITEM(xseries,i);
            if (it==NULL) break;
            xs << PyFloat_AsDouble(it);
        }
    }

    // yseries type conversion
    if (!PyList_Check(yseries)) {
         PyErr_Format(PyExc_TypeError, "The argument 'yseries' must be of list or subtype of list");
         return false;
    } else {
        // convert to QVector TODO convert in SIP
        for (Py_ssize_t i=0; i <PyList_Size(yseries); i++) {
            PyObject *it = PyList_GET_ITEM(yseries,i);
            if (it==NULL) break;
            ys << PyFloat_AsDouble(it);
        }
    }

    // now just add via the chart
    python->chart->emitCurve(name, xs, ys, fseries, xname, yname, labels, colors, line, symbol, size, color, opacity, opengl, legend, datalabels, fill);
    return true;
}

bool
Bindings::configAxis(QString name, bool visible, int align, double min, double max,
                      int type, QString labelcolor, QString color, bool log, QStringList categories)
{
    if (!python->chart) return false; // Do nothing when no chart is avaliable
    python->chart->emitAxis(name, visible, align, min, max, type, labelcolor, color, log, categories);
    return false;
}

bool
Bindings::addAnnotation(QString, QString s1, QString s2, double)
{
    if (!python->chart) return false; // Do nothing when no chart is avaliable

    // we will reuse later but for now just assume its a label
    // will likely need to refactor all of this and create an
    // annotation class to throw around, but lets wait till we
    // get there !
    QStringList labels;
    labels << s2;
    python->chart->emitAnnotation(s1, labels);

    return true;
}

void
Bindings::result(double value)
{
    python->result = value;
}

// get athlete data
PyObject* Bindings::athlete() const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL) return NULL;

    PyObject* dict = PyDict_New();
    if (dict == NULL) return dict;

    // NAME
    PyDict_SetItemString_Steal(dict, "name", PyUnicode_FromString(context->athlete->cyclist.toUtf8().constData()));

    // HOME
    PyDict_SetItemString_Steal(dict, "home", PyUnicode_FromString(context->athlete->home->root().absolutePath().toUtf8().constData()));

    // DOB
    if (PyDateTimeAPI == NULL) PyDateTime_IMPORT;// import datetime if necessary
    QDate d = appsettings->cvalue(context->athlete->cyclist, GC_DOB).toDate();
    PyDict_SetItemString_Steal(dict, "dob", PyDate_FromDate(d.year(), d.month(), d.day()));

    // WEIGHT
    PyDict_SetItemString_Steal(dict, "weight", PyFloat_FromDouble(appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT).toDouble()));

    // HEIGHT
    PyDict_SetItemString_Steal(dict, "height", PyFloat_FromDouble(appsettings->cvalue(context->athlete->cyclist, GC_HEIGHT).toDouble()));

    // GENDER
    int isfemale = appsettings->cvalue(context->athlete->cyclist, GC_SEX).toInt();
    PyDict_SetItemString_Steal(dict, "gender", PyUnicode_FromString(isfemale ? "female" : "male"));

    return dict;
}

// one entry per sport per date for hr/power/pace
class gcZoneConfig {
    public:
    gcZoneConfig(QString sport) : sport(sport), date(QDate(01,01,01)), cp(0), wprime(0), pmax(0), aetp(0), ftp(0),lthr(0),aethr(0),rhr(0),hrmax(0),cv(0),aetv(0) {}
    bool operator<(gcZoneConfig rhs) const { return date < rhs.date; }
    QString sport;
    QDate date;
    QList<int> zoneslow;
    QList<int> hrzoneslow;
    QList<double> pacezoneslow;
    int cp, wprime, pmax,aetp,ftp,lthr,aethr,rhr,hrmax;
    double cv,aetv;
};

// Return a dataframe with:
// date, sport, cp, w', pmax, aetp, ftp, lthr, aethr, rhr, hrmax, cv, aetv, zoneslow, hrzoneslow, pacezoneslow zonescolor
PyObject*
Bindings::athleteZones(PyObject* date, QString sport) const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL) return NULL;

    // import datetime if necessary
    if (PyDateTimeAPI == NULL) PyDateTime_IMPORT;

    // COLLECT ALL THE CONFIG TOGETHER
    QList<gcZoneConfig> config;

    // for a specific date?
    if (date != NULL && PyDate_Check(date)) {

        // convert PyDate to QDate
        QDate forDate(QDate(PyDateTime_GET_YEAR(date), PyDateTime_GET_MONTH(date), PyDateTime_GET_DAY(date)));

        // Look for the range in Power, HR and Pace zones for each sport
        foreach (QString sp, GlobalContext::context()->rideMetadata->sports()) {

            // Power Zones
            if (context->athlete->zones(sp)) {

                // run through the power zones
                int range=context->athlete->zones(sp)->whichRange(forDate);
                if (range >= 0) {

                    gcZoneConfig c(sp);

                    c.date = forDate;
                    c.cp = context->athlete->zones(sp)->getCP(range);
                    c.wprime = context->athlete->zones(sp)->getWprime(range);
                    c.pmax = context->athlete->zones(sp)->getPmax(range);
                    c.aetp = context->athlete->zones(sp)->getAeT(range);
                    c.ftp = context->athlete->zones(sp)->getFTP(range);
                    c.zoneslow = context->athlete->zones(sp)->getZoneLows(range);

                    config << c;
                }
            }

            // HR Zones
            if (context->athlete->hrZones(sp)) {

                int range=context->athlete->hrZones(sp)->whichRange(forDate);
                if (range >= 0) {

                    gcZoneConfig c(sp);

                    c.date = forDate;
                    c.lthr = context->athlete->hrZones(sp)->getLT(range);
                    c.aethr = context->athlete->hrZones(sp)->getAeT(range);
                    c.rhr = context->athlete->hrZones(sp)->getRestHr(range);
                    c.hrmax = context->athlete->hrZones(sp)->getMaxHr(range);
                    c.hrzoneslow = context->athlete->hrZones(sp)->getZoneLows(range);

                    config << c;
                }
            }

            // Pace Zones
            if ((sp == "Run" || sp == "Swim") && context->athlete->paceZones(sp=="Swim")) {

                int range=context->athlete->paceZones(sp=="Swim")->whichRange(forDate);
                if (range >= 0) {

                    gcZoneConfig c(sp);

                    c.date =  forDate;
                    c.cv =  context->athlete->paceZones(sp=="Swim")->getCV(range);
                    c.aetv =  context->athlete->paceZones(sp=="Swim")->getAeT(range);
                    c.pacezoneslow = context->athlete->paceZones(sp=="Swim")->getZoneLows(range);

                    config << c;
                }
            }
        }
    } else {

        // Look for the ranges in Power, HR and Pace zones for each sport
        foreach (QString sp, GlobalContext::context()->rideMetadata->sports()) {

            // Power Zones
            if (context->athlete->zones(sp)) {

                for (int range=0; range < context->athlete->zones(sp)->getRangeSize(); range++) {

                    // run through the bike zones
                    gcZoneConfig c(sp);

                    c.date =  context->athlete->zones(sp)->getStartDate(range);
                    c.cp = context->athlete->zones(sp)->getCP(range);
                    c.wprime = context->athlete->zones(sp)->getWprime(range);
                    c.pmax = context->athlete->zones(sp)->getPmax(range);
                    c.aetp = context->athlete->zones(sp)->getAeT(range);
                    c.ftp = context->athlete->zones(sp)->getFTP(range);
                    c.zoneslow = context->athlete->zones(sp)->getZoneLows(range);

                    config << c;
                }
            }

            // HR Zones
            if (context->athlete->hrZones(sp)) {

                for (int range=0; range < context->athlete->hrZones(sp)->getRangeSize(); range++) {

                    gcZoneConfig c(sp);

                    c.date =  context->athlete->hrZones(sp)->getStartDate(range);
                    c.lthr =  context->athlete->hrZones(sp)->getLT(range);
                    c.aethr =  context->athlete->hrZones(sp)->getAeT(range);
                    c.rhr =  context->athlete->hrZones(sp)->getRestHr(range);
                    c.hrmax =  context->athlete->hrZones(sp)->getMaxHr(range);
                    c.hrzoneslow = context->athlete->hrZones(sp)->getZoneLows(range);

                    config << c;
                }
            }

            // Pace Zones
            if ((sp == "Run" || sp == "Swim") && context->athlete->paceZones(sp=="Swim")) {

                for (int range=0; range < context->athlete->paceZones(sp=="Swim")->getRangeSize(); range++) {

                    gcZoneConfig c(sp);

                    c.date =  context->athlete->paceZones(sp=="Swim")->getStartDate(range);
                    c.cv =  context->athlete->paceZones(sp=="Swim")->getCV(range);
                    c.aetv =  context->athlete->paceZones(sp=="Swim")->getAeT(range);
                    c.pacezoneslow = context->athlete->paceZones(sp=="Swim")->getZoneLows(range);

                    config << c;
                }
            }
        }
    }

    // no config ?
    if (config.count() == 0) return NULL;

    // COMPRESS CONFIG TOGETHER BY SPORT
    QList<gcZoneConfig> compressed;
    std::sort(config.begin(),config.end());

    foreach (QString sp, GlobalContext::context()->rideMetadata->sports()) {

        // will have date zero
        gcZoneConfig last(sp);

        foreach(gcZoneConfig x, config) {

            if (x.sport == sp && (sport=="" || QString::compare(sport, sp, Qt::CaseInsensitive)==0)) {

                // new date so save what we have collected
                if (x.date > last.date) {

                    if (last.date > QDate(01,01,01))  compressed << last;
                    last.date = x.date;
                }

                // merge new values
                if (x.date == last.date) {
                    // merge with prior
                    if (x.cp) last.cp = x.cp;
                    if (x.wprime) last.wprime = x.wprime;
                    if (x.pmax) last.pmax = x.pmax;
                    if (x.aetp) last.aetp = x.aetp;
                    if (x.ftp) last.ftp = x.ftp;
                    if (x.lthr) last.lthr = x.lthr;
                    if (x.aethr) last.aethr = x.aethr;
                    if (x.rhr) last.rhr = x.rhr;
                    if (x.hrmax) last.hrmax = x.hrmax;
                    if (x.cv) last.cv = x.cv;
                    if (x.aetv) last.cv = x.aetv;
                    if (x.zoneslow.length()) last.zoneslow = x.zoneslow;
                    if (x.hrzoneslow.length()) last.hrzoneslow = x.hrzoneslow;
                    if (x.pacezoneslow.length()) last.pacezoneslow = x.pacezoneslow;
                }
            }
        }

        if (last.date > QDate(01,01,01)) compressed << last;
    }

    // now use the new compressed ones
    config = compressed;
    std::sort(config.begin(),config.end());
    int size = config.count();

    // CREATE A DICT OF CONFIG
    PyObject* dict = PyDict_New();
    if (dict == NULL) return dict;

    // 15 lists
    PyObject* dates = PyList_New(size);
    PyObject* sports = PyList_New(size);
    PyObject* cp = PyList_New(size);
    PyObject* wprime = PyList_New(size);
    PyObject* pmax = PyList_New(size);
    PyObject* aetp = PyList_New(size);
    PyObject* ftp = PyList_New(size);
    PyObject* lthr = PyList_New(size);
    PyObject* aethr = PyList_New(size);
    PyObject* rhr = PyList_New(size);
    PyObject* hrmax = PyList_New(size);
    PyObject* cv = PyList_New(size);
    PyObject* aetv = PyList_New(size);
    PyObject* zoneslow = PyList_New(size);
    PyObject* hrzoneslow = PyList_New(size);
    PyObject* pacezoneslow = PyList_New(size);
    PyObject* zonescolor = PyList_New(size);

    int index=0;
    foreach(gcZoneConfig x, config) {

        // update the lists
        PyList_SET_ITEM(dates, index, PyDate_FromDate(x.date.year(), x.date.month(), x.date.day()));
        PyList_SET_ITEM(sports, index, PyUnicode_FromString(x.sport.toUtf8().constData()));
        PyList_SET_ITEM(cp, index, PyFloat_FromDouble(x.cp));
        PyList_SET_ITEM(wprime, index, PyFloat_FromDouble(x.wprime));
        PyList_SET_ITEM(pmax, index, PyFloat_FromDouble(x.pmax));
        PyList_SET_ITEM(aetp, index, PyFloat_FromDouble(x.aetp));
        PyList_SET_ITEM(ftp, index, PyFloat_FromDouble(x.ftp));
        PyList_SET_ITEM(lthr, index, PyFloat_FromDouble(x.lthr));
        PyList_SET_ITEM(aethr, index, PyFloat_FromDouble(x.aethr));
        PyList_SET_ITEM(rhr, index, PyFloat_FromDouble(x.rhr));
        PyList_SET_ITEM(hrmax, index, PyFloat_FromDouble(x.hrmax));
        PyList_SET_ITEM(cv, index, PyFloat_FromDouble(x.cv));
        PyList_SET_ITEM(aetv, index, PyFloat_FromDouble(x.aetv));

        PyObject* lows = PyList_New(x.zoneslow.length());
        PyObject* hrlows = PyList_New(x.hrzoneslow.length());
        PyObject* pacelows = PyList_New(x.pacezoneslow.length());
        PyObject* colors = PyList_New(x.zoneslow.length());
        int indexlow=0;
        foreach(int low, x.zoneslow) {
            PyList_SET_ITEM(lows, indexlow, PyFloat_FromDouble(low));
            PyList_SET_ITEM(colors, indexlow, PyUnicode_FromString(zoneColor(indexlow, x.zoneslow.length()).name().toUtf8().constData()));
            indexlow++;
        }
        indexlow=0;
        foreach(int low, x.hrzoneslow) {
            PyList_SET_ITEM(hrlows, indexlow, PyFloat_FromDouble(low));
            indexlow++;
        }
        indexlow=0;
        foreach(double low, x.pacezoneslow) {
            PyList_SET_ITEM(pacelows, indexlow, PyFloat_FromDouble(low));
            indexlow++;
        }
        PyList_SET_ITEM(zoneslow, index, lows);
        PyList_SET_ITEM(hrzoneslow, index, hrlows);
        PyList_SET_ITEM(pacezoneslow, index, pacelows);
        PyList_SET_ITEM(zonescolor, index, colors);

        index++;
    }

    // add to dict
    PyDict_SetItemString_Steal(dict, "date", dates);
    PyDict_SetItemString_Steal(dict, "sport", sports);
    PyDict_SetItemString_Steal(dict, "cp", cp);
    PyDict_SetItemString_Steal(dict, "wprime", wprime);
    PyDict_SetItemString_Steal(dict, "pmax", pmax);
    PyDict_SetItemString_Steal(dict, "aetp", aetp);
    PyDict_SetItemString_Steal(dict, "ftp", ftp);
    PyDict_SetItemString_Steal(dict, "lthr", lthr);
    PyDict_SetItemString_Steal(dict, "aethr", aethr);
    PyDict_SetItemString_Steal(dict, "rhr", rhr);
    PyDict_SetItemString_Steal(dict, "hrmax", hrmax);
    PyDict_SetItemString_Steal(dict, "cv", cv);
    PyDict_SetItemString_Steal(dict, "aetv", aetv);
    PyDict_SetItemString_Steal(dict, "zoneslow", zoneslow);
    PyDict_SetItemString_Steal(dict, "hrzoneslow", hrzoneslow);
    PyDict_SetItemString_Steal(dict, "pacezoneslow", pacezoneslow);
    PyDict_SetItemString_Steal(dict, "zonescolor", zonescolor);

    return dict;
}

// get a list of activities (Date&Time)
PyObject*
Bindings::activities(QString filter) const
{
    Context *context = python->contexts.value(threadid()).context;

    if (context && context->athlete && context->athlete->rideCache) {

        // import datetime if necessary
        if (PyDateTimeAPI == NULL) PyDateTime_IMPORT;

        // filters
        // apply any global filters
        Specification specification;
        FilterSet fs;
        fs.addFilter(context->isfiltered, context->filters);
        fs.addFilter(context->ishomefiltered, context->homeFilters);
        if (python->perspective) fs.addFilter(python->perspective->isFiltered(), python->perspective->filterlist(DateRange(QDate(1,1,1970),QDate(31,12,3000))));

        // did call contain any filters?
        if (filter != "") {

            DataFilter dataFilter(python->chart, context);
            QStringList files;
            dataFilter.parseFilter(context, filter, &files);
            fs.addFilter(true, files);
        }
        specification.setFilterSet(fs);

        // how many pass?
        int count = 0;
        foreach(RideItem *item, context->athlete->rideCache->rides()) {

            // apply filters
            if (!specification.pass(item)) continue;
            count++;
        }

        // allocate space for a list of dates
        PyObject* dates = PyList_New(count);

        // fill with values for date and class
        int idx = 0;
        foreach(RideItem *item, context->athlete->rideCache->rides()) {

            // apply filters
            if (!specification.pass(item)) continue;

            // add datetime to the list
            QDate d = item->dateTime.date();
            QTime t = item->dateTime.time();
            PyList_SET_ITEM(dates, idx++, PyDateTime_FromDateAndTime(d.year(), d.month(), d.day(), t.hour(), t.minute(), t.second(), t.msec()*10));
        }

        return dates;
    }

    return NULL;
}

RideItem*
Bindings::fromDateTime(PyObject* activity) const
{
    Context *context = python->contexts.value(threadid()).context;

    // import datetime if necessary
    if (PyDateTimeAPI == NULL) PyDateTime_IMPORT;

    if (context !=NULL && activity != NULL && PyDate_Check(activity)) {

        // convert PyDateTime to QDateTime
        QDateTime dateTime(QDate(PyDateTime_GET_YEAR(activity), PyDateTime_GET_MONTH(activity), PyDateTime_GET_DAY(activity)),
                           QTime(PyDateTime_DATE_GET_HOUR(activity), PyDateTime_DATE_GET_MINUTE(activity), PyDateTime_DATE_GET_SECOND(activity), PyDateTime_DATE_GET_MICROSECOND(activity)/10));

        // search the RideCache
        foreach(RideItem*item, context->athlete->rideCache->rides())
            if (item->dateTime.toUTC() == dateTime.toUTC())
                return const_cast<RideItem*>(item);
    }

    return NULL;
}

RideFile *
Bindings::selectRideFile(PyObject *activity) const
{
    RideFile *f;
    RideItem* item = fromDateTime(activity);
    if (item && item->ride()) return item->ride();

    f = python->contexts.value(threadid()).rideFile;
    if (f) return f;

    item = python->contexts.value(threadid()).item;
    if (item && item->ride()) return item->ride();

    Context *context = python->contexts.value(threadid()).context;
    if (context) {
        item = const_cast<RideItem*>(context->currentRideItem());
        if (item && item->ride()) return item->ride();
    }

    return nullptr;
}

// get the data series for the currently selected ride
PythonDataSeries*
Bindings::series(int type, PyObject* activity) const
{
    RideFile *f = selectRideFile(activity);
    if (f == nullptr) return nullptr;

    // count the included points, create data series output and copy data
    int pCount = 0;
    RideFileIterator it(f, python->contexts.value(threadid()).spec);
    while (it.hasNext()) { it.next(); pCount++; }
    RideFile::SeriesType seriesType = static_cast<RideFile::SeriesType>(type);
    bool readOnly = python->contexts.value(threadid()).readOnly;
    QList<RideFile *> *editedRideFiles = python->contexts.value(threadid()).editedRideFiles;
    if (!readOnly && editedRideFiles && !editedRideFiles->contains(f)) {
        f->command->startLUW(QString("Python_%1").arg(threadid()));
        editedRideFiles->append(f);
    }

    PythonDataSeries* ds = new PythonDataSeries(seriesName(type), pCount, readOnly, seriesType, f);
    it.toFront();
    for(int i=0; i<pCount && it.hasNext(); i++) {
        struct RideFilePoint *point = it.next();
        ds->data[i] = point->value(seriesType);
    }

    return ds;
}

// get the wbal series for the currently selected ride
PythonDataSeries*
Bindings::activityWbal(PyObject* activity) const
{
    RideFile *f = selectRideFile(activity);
    if (f == nullptr) return nullptr;

    f->recalculateDerivedSeries();
    WPrime *w = f->wprimeData();
    if (w == NULL) return NULL;

    // count the included points, create data series output and copy data
    int pCount = 0;
    int idxStart = 0;
    int secsStart = python->contexts.value(threadid()).spec.secsStart();
    int secsEnd = python->contexts.value(threadid()).spec.secsEnd();
    for(int i=0; i<w->xdata(false).count(); i++) {
        if (w->xdata(false)[i] < secsStart) continue;
        if (secsEnd >= 0 && w->xdata(false)[i] > secsEnd) break;
        if (pCount == 0) idxStart = i;
        pCount++;
    }
    PythonDataSeries* ds = new PythonDataSeries("WBal", pCount);
    for(int i=0; i<pCount; i++) ds->data[i] = w->ydata()[i+idxStart];

    return ds;
}

// get the xdata series for the currently selected ride
PythonDataSeries*
Bindings::xdata(QString name, QString series, QString join, PyObject* activity) const
{
    // XDATA join method
    RideFile::XDataJoin xjoin;
    QStringList xdataValidSymbols;
    xdataValidSymbols << "sparse" << "repeat" << "interpolate" << "resample";
    int xx = xdataValidSymbols.indexOf(join, Qt::CaseInsensitive);
    switch(xx) {
        case 0: xjoin = RideFile::SPARSE; break;
        default:
        case 1: xjoin = RideFile::REPEAT; break;
        case 2: xjoin = RideFile::INTERPOLATE; break;
        case 3: xjoin = RideFile::RESAMPLE; break;
    }

    RideFile *f = selectRideFile(activity);
    if (f == nullptr) return nullptr;

    if (!f->xdata().contains(name)) return NULL; // No such XData series
    XDataSeries *xds = f->xdata()[name];

    if (!xds->valuename.contains(series)) return NULL; // No such XData name

    // count the included points, create data series output and copy data
    int pCount = 0;
    RideFileIterator it(f, python->contexts.value(threadid()).spec);
    while (it.hasNext()) { it.next(); pCount++; }
    PythonDataSeries* ds = new PythonDataSeries(QString("%1_%2").arg(name).arg(series), pCount);
    it.toFront();
    int idx = 0;
    for(int i=0; i<pCount && it.hasNext(); i++) {
        struct RideFilePoint *point = it.next();
        double val = f->xdataValue(point, idx, name, series, xjoin);
        ds->data[i] = (val == RideFile::NA) ? sqrt(-1) : val; // NA => NaN
    }

    return ds;
}

// get the xdata series for the currently selected ride, without interpolation
PythonXDataSeries *Bindings::xdataSeries(QString name, QString series, PyObject* activity) const
{
    RideFile *f = selectRideFile(activity);
    if (f == nullptr) return nullptr;

    if (!f->xdata().contains(name)) return NULL; // No such XData series
    XDataSeries *xds = f->xdata()[name];

    int valueIdx = -1;
    if (xds->valuename.contains(series))
        valueIdx = xds->valuename.indexOf(series);
    else if (series != "secs" && series != "km")
        return NULL; // No such XData name

    bool readOnly = python->contexts.value(threadid()).readOnly;
    QList<RideFile *> *editedRideFiles = python->contexts.value(threadid()).editedRideFiles;
    if (!readOnly && editedRideFiles && !editedRideFiles->contains(f)) {
        f->command->startLUW(QString("Python_%1").arg(threadid()));
        editedRideFiles->append(f);
    }

    // count the included points, create data series output and copy data
    Specification spec(python->contexts.value(threadid()).spec);
    IntervalItem* it = spec.interval();
    int pCount = 0;
    foreach(XDataPoint* p, xds->datapoints) {
        if (it && p->secs < it->start) continue;
        if (it && p->secs > it->stop) break;
        pCount++;
    }

    PythonXDataSeries* ds = new PythonXDataSeries(name, series, valueIdx >= 0 ? xds->unitname[valueIdx] : "",
                                                  pCount, readOnly, f);

    int idx = 0;
    foreach(XDataPoint* p, xds->datapoints) {
        if (it && p->secs < it->start) continue;
        if (it && p->secs > it->stop) break;
        double val = sqrt(-1); // NA => NaN
        if (valueIdx >= 0) val = p->number[valueIdx];
        else if (series == "secs") val = p->secs;
        else if (series == "km") val = p->km;
        ds->set(idx++, val);
    }

    return ds;
}

PyObject*
Bindings::xdataNames(QString name, PyObject* activity) const
{
    RideFile *f = selectRideFile(activity);
    if (f == nullptr) return nullptr;

    QStringList namelist;
    if (name.isEmpty())
        namelist = f->xdata().keys();
    else if (f->xdata().contains(name))
        namelist = f->xdata()[name]->valuename;
    else
        return NULL; // No such XData series

    PyObject* list = PyList_New(namelist.size());
    for (int i = 0; i < namelist.size(); i++)
        PyList_SET_ITEM(list, i, PyUnicode_FromString(namelist.at(i).toUtf8().constData()));

    return list;
}

int
Bindings::seriesLast() const
{
    return static_cast<int>(RideFile::none);
}

QString
Bindings::seriesName(int type) const
{
    return RideFile::seriesName(static_cast<RideFile::SeriesType>(type), true);
}

bool
Bindings::seriesPresent(int type, PyObject* activity) const
{
    RideFile *f = selectRideFile(activity);
    if (f == nullptr) return false;

    return f->isDataPresent(static_cast<RideFile::SeriesType>(type));
}

PythonDataSeries::PythonDataSeries(QString name, Py_ssize_t count, bool readOnly, RideFile::SeriesType seriesType, RideFile *rideFile)
    : name(name), count(count), data(NULL), readOnly(readOnly), seriesType(seriesType), rideFile(rideFile)
{
    if (count > 0) data = new double[count];
}

PythonDataSeries::PythonDataSeries(QString name, Py_ssize_t count) : name(name), count(count), data(NULL),
    readOnly(true), seriesType(RideFile::none), rideFile(NULL)
{
    if (count > 0) data = new double[count];
}

// default constructor and copy constructor
PythonDataSeries::PythonDataSeries() : name(QString()), count(0), data(NULL),
    readOnly(true), seriesType(RideFile::none), rideFile(NULL) {}
PythonDataSeries::PythonDataSeries(PythonDataSeries *clone)
{
    if (clone) *this = *clone;
    else {
        name = QString();
        count = 0;
        data = NULL;
    }
}

PythonDataSeries::~PythonDataSeries()
{
    if (data) delete[] data;
    data=NULL;
    rideFile = NULL;
}

PythonXDataSeries::PythonXDataSeries(QString xdata, QString series, QString unit, int count, bool readOnly, RideFile *rideFile)
    : xdata(xdata), series(series), colIdx(-1), unit(unit), readOnly(readOnly), rideFile(rideFile), shape(1), data(count)
{
    shape[0] = count >= 0 ? count : 0;
}

PythonXDataSeries::PythonXDataSeries(PythonXDataSeries *clone)
{
    if (clone) *this = *clone;
    else {
        colIdx = -1;
        readOnly = true;
        rideFile = nullptr;
        shape = QVector<Py_ssize_t>(1);
    }
}

PythonXDataSeries::PythonXDataSeries()
    : colIdx(-1), readOnly(true), rideFile(nullptr), shape(1)
{
}

bool PythonXDataSeries::set(int i, double value)
{
    if (!readOnly && rideFile) {
        if (!setColIdx()){
            return false;
        }

        rideFile->command->setXDataPointValue(xdata, i, colIdx, value);
    }

    data[i] = value;
    return true;
}

bool PythonXDataSeries::add(double value)
{
    if (rideFile) {
        if (!setColIdx()){
            return false;
        }

        QVector<XDataPoint *> xDataPoints(1);
        xDataPoints[0] = new XDataPoint;
        int i = rideFile->xdata(xdata)->datapoints.count();

        rideFile->command->appendXDataPoints(xdata, xDataPoints);
        rideFile->command->setXDataPointValue(xdata, i, colIdx, value);
    }

    data.append(value);
    shape[0] = data.count();
    return true;
}

bool PythonXDataSeries::remove(int i)
{
    if (rideFile) {
        if (!setColIdx()){
            return false;
        }

        rideFile->command->deleteXDataPoints(xdata, i, 1);
    }

    data.removeAt(i);
    shape[0] = data.count();
    return true;
}

bool PythonXDataSeries::setColIdx()
{
    if (colIdx > -1) {
        return true;
    }

    if (colIdx == -1) {
        XDataSeries *xds = rideFile->xdata(xdata);
        if (xds->valuename.contains(series))
            colIdx = xds->valuename.indexOf(series) + 2;
        else if (series == "secs") colIdx = 0;
        else if (series == "km") colIdx = 1;
        else colIdx = -2;
    }

    if (colIdx == -2) {
        return false; // No such XData series
    }

    return true;
}

PyObject*
Bindings::activityMetrics(bool compare) const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL) return NULL;

    // want a list of compares
    if (compare) {

        // only return compares if its actually active
        if (context->isCompareIntervals) {

            // how many to return?
            int count = 0;
            foreach(CompareInterval p, context->compareIntervals) if (p.isChecked()) count++;
            PyObject* list = PyList_New(count);

            // create a dict for each and add to list as tuple (metrics, color)
            long idx = 0;
            foreach(CompareInterval p, context->compareIntervals) {
                if (p.isChecked()) {

                    // create a tuple (metrics, color)
                    PyObject* am = activityMetrics(p.rideItem);
                    PyObject* tuple = Py_BuildValue("(Os)", am, p.color.name().toUtf8().constData());
                    Py_DECREF(am);
                    PyList_SET_ITEM(list, idx++, tuple);
                }
            }

            return list;
        } else { // compare isn't active...

            // otherwise return the current metrics in a compare list
            if (context->currentRideItem()==NULL) return NULL;
            RideItem *item = const_cast<RideItem*>(context->currentRideItem());
            PyObject* list = PyList_New(1);

            PyObject* am = activityMetrics(item);
            PyObject* tuple = Py_BuildValue("(Os)", am, "#FF00FF");
            Py_DECREF(am);
            PyList_SET_ITEM(list, 0, tuple);

            return list;
        }
    } else {

        // not compare, so just return a dict
        RideItem *item = python->contexts.value(threadid()).item;
        if (item == NULL) item = const_cast<RideItem*>(context->currentRideItem());

        return activityMetrics(item);
    }
}

PyObject*
Bindings::activityMetrics(RideItem* item) const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL) return NULL;

    PyObject* dict = PyDict_New();
    if (dict == NULL) return dict;

    if (item == NULL) return NULL;

    const RideMetricFactory &factory = RideMetricFactory::instance();

    //
    // Date and Time
    //
    if (PyDateTimeAPI == NULL) PyDateTime_IMPORT;// import datetime if necessary
    QDate d = item->dateTime.date();
    PyDict_SetItemString_Steal(dict, "date", PyDate_FromDate(d.year(), d.month(), d.day()));
    QTime t = item->dateTime.time();
    PyDict_SetItemString_Steal(dict, "time", PyTime_FromTime(t.hour(), t.minute(), t.second(), t.msec()*10));

    //
    // METRICS
    //
    for(int i=0; i<factory.metricCount();i++) {

        QString symbol = factory.metricName(i);
        const RideMetric *metric = factory.rideMetric(symbol);
        QString name = GlobalContext::context()->specialFields.internalName(factory.rideMetric(symbol)->name());
        name = name.replace(" ","_");
        name = name.replace("'","_");

        bool useMetricUnits = GlobalContext::context()->useMetricUnits;
        double value = item->metrics()[i] * (useMetricUnits ? 1.0f : metric->conversion()) + (useMetricUnits ? 0.0f : metric->conversionSum());

        // Override if we have precomputed values in ScriptContext (UserMetric)
        if (python->contexts.value(threadid()).metrics && python->contexts.value(threadid()).metrics->contains(symbol)) {
            const RideMetric *metric = python->contexts.value(threadid()).metrics->value(symbol);
            value = metric->value(useMetricUnits);
        }

        // add to the dict
        PyDict_SetItemString_Steal(dict, name.toUtf8().constData(), PyFloat_FromDouble(value));
    }

    //
    // META
    //
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {

        // don't add incomplete meta definitions or metric override fields
        if (field.name == "" || field.tab == "" ||
            GlobalContext::context()->specialFields.isMetric(field.name)) continue;

        // add to the dict
        PyDict_SetItemString_Steal(dict, field.name.replace(" ","_").toUtf8().constData(), PyUnicode_FromString(item->getText(field.name, "").toUtf8().constData()));
    }

    //
    // add Color
    //
    QString color;

    // apply item color, remembering that 1,1,1 means use default (reverse in this case)
    if (item->color == QColor(1,1,1,1)) {

        // use the inverted color, not plot marker as that hideous
        QColor col =GCColor::invertColor(GColor(CPLOTBACKGROUND));

        // white is jarring on a dark background!
        if (col==QColor(Qt::white)) col=QColor(127,127,127);

        color = col.name();
    } else
        color = item->color.name();

    // add to the dict
    PyDict_SetItemString_Steal(dict, "color", PyUnicode_FromString(color.toUtf8().constData()));

    return dict;
}

PyObject*
Bindings::seasonMetrics(bool all, QString filter, bool compare) const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL) return NULL;

    // want a list of compares
    if (compare) {

        // only return compares if its actually active
        if (context->isCompareDateRanges) {

            // how many to return?
            int count=0;
            foreach(CompareDateRange p, context->compareDateRanges) if (p.isChecked()) count++;

            // cool we can return a list of intervals to compare
            PyObject* list = PyList_New(count);
            int idx = 0;

            // create a dict for each and add to list
            foreach(CompareDateRange p, context->compareDateRanges) {
                if (p.isChecked()) {

                    // create a tuple (metrics, color)
                    PyObject* sm = seasonMetrics(all, DateRange(p.start, p.end), filter);
                    PyObject* tuple = Py_BuildValue("(Os)", sm, p.color.name().toUtf8().constData());
                    Py_DECREF(sm);
                    // add to back and move on
                    PyList_SET_ITEM(list, idx++, tuple);
                }
            }

            return list;

        } else { // compare isn't active...

            // otherwise return the current metrics in a compare list
            PyObject* list = PyList_New(1);

            // create a tuple (metrics, color)
            DateRange range = context->currentDateRange();
            PyObject* sm = seasonMetrics(all, range, filter);
            PyObject* tuple = Py_BuildValue("(Os)", sm, "#FF00FF");
            Py_DECREF(sm);
            // add to back and move on
            PyList_SET_ITEM(list, 0, tuple);

            return list;
        }

    } else {

        // just a dict of metrics
        DateRange range = context->currentDateRange();
        return seasonMetrics(all, range, filter);
    }
}

PyObject*
Bindings::seasonMetrics(bool all, DateRange range, QString filter) const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL || context->athlete == NULL || context->athlete->rideCache == NULL) return NULL;

    // how many rides to return if we're limiting to the
    // currently selected date range ?

    // apply any global filters
    Specification specification;
    FilterSet fs;
    fs.addFilter(context->isfiltered, context->filters);
    fs.addFilter(context->ishomefiltered, context->homeFilters);
    if (python->perspective) fs.addFilter(python->perspective->isFiltered(), python->perspective->filterlist(DateRange(QDate(1,1,1970),QDate(31,12,3000))));

    // did call contain a filter?
    if (filter != "") {

        DataFilter dataFilter(python->chart, context);
        QStringList files;
        dataFilter.parseFilter(context, filter, &files);
        fs.addFilter(true, files);
    }

    specification.setFilterSet(fs);

    // we need to count rides that are in range...
    int rides = 0;
    foreach(RideItem *ride, context->athlete->rideCache->rides()) {
        if (!specification.pass(ride)) continue;
        if (all || range.pass(ride->dateTime.date())) rides++;
    }

    PyObject* dict = PyDict_New();
    if (dict == NULL) return dict;

    //
    // Date, Time and Color
    //
    if (PyDateTimeAPI == NULL) PyDateTime_IMPORT;// import datetime if necessary

    PyObject* datelist = PyList_New(rides);
    PyObject* timelist = PyList_New(rides);
    PyObject* colorlist = PyList_New(rides);

    int idx = 0;
    foreach(RideItem *ride, context->athlete->rideCache->rides()) {
        if (!specification.pass(ride)) continue;
        if (all || range.pass(ride->dateTime.date())) {

            QDate d = ride->dateTime.date();
            PyList_SET_ITEM(datelist, idx, PyDate_FromDate(d.year(), d.month(), d.day()));

            QTime t = ride->dateTime.time();
            PyList_SET_ITEM(timelist, idx, PyTime_FromTime(t.hour(), t.minute(), t.second(), t.msec()*10));

            // apply item color, remembering that 1,1,1 means use default (reverse in this case)
            QString color;

            if (ride->color == QColor(1,1,1,1)) {

                // use the inverted color, not plot marker as that hideous
                QColor col =GCColor::invertColor(GColor(CPLOTBACKGROUND));

                // white is jarring on a dark background!
                if (col==QColor(Qt::white)) col=QColor(127,127,127);

                color = col.name();
            } else
                color = ride->color.name();

            PyList_SET_ITEM(colorlist, idx, PyUnicode_FromString(color.toUtf8().constData()));

            idx++;
        }
    }

    PyDict_SetItemString_Steal(dict, "date", datelist);
    PyDict_SetItemString_Steal(dict, "time", timelist);
    PyDict_SetItemString_Steal(dict, "color", colorlist);

    //
    // METRICS
    //
    const RideMetricFactory &factory = RideMetricFactory::instance();
    bool useMetricUnits = GlobalContext::context()->useMetricUnits;
    for(int i=0; i<factory.metricCount();i++) {

        QString symbol = factory.metricName(i);
        const RideMetric *metric = factory.rideMetric(symbol);
        QString name = GlobalContext::context()->specialFields.internalName(factory.rideMetric(symbol)->name());
        name = name.replace(" ","_");
        name = name.replace("'","_");

        // set a list of metric values
        PyObject* metriclist = PyList_New(rides);

        int idx = 0;
        foreach(RideItem *item, context->athlete->rideCache->rides()) {
            if (!specification.pass(item)) continue;
            if (all || range.pass(item->dateTime.date())) {
                PyList_SET_ITEM(metriclist, idx++, PyFloat_FromDouble(item->metrics()[i] * (useMetricUnits ? 1.0f : metric->conversion()) + (useMetricUnits ? 0.0f : metric->conversionSum())));
            }
        }

        // add to the dict
        PyDict_SetItemString_Steal(dict, name.toUtf8().constData(), metriclist);
    }

    //
    // META
    //
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {

        // don't add incomplete meta definitions or metric override fields
        if (field.name == "" || field.tab == "" ||
            GlobalContext::context()->specialFields.isMetric(field.name)) continue;

        // Create a string list
        PyObject* metalist = PyList_New(rides);

        int idx = 0;
        foreach(RideItem *item, context->athlete->rideCache->rides()) {
            if (!specification.pass(item)) continue;
            if (all || range.pass(item->dateTime.date())) {
                PyList_SET_ITEM(metalist, idx++, PyUnicode_FromString(item->getText(field.name, "").toUtf8().constData()));
            }
        }

        // add to the dict
        PyDict_SetItemString_Steal(dict, field.name.replace(" ","_").toUtf8().constData(), metalist);
    }

    return dict;
}

PyObject*
Bindings::seasonIntervals(QString type, bool compare) const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL) return NULL;

    // want a list of compares
    if (compare) {

        // only return compares if its actually active
        if (context->isCompareDateRanges) {

            // how many to return?
            int count=0;
            foreach(CompareDateRange p, context->compareDateRanges) if (p.isChecked()) count++;

            // cool we can return a list of intervals to compare
            PyObject* list = PyList_New(count);
            int idx = 0;

            // create a dict for each and add to list
            foreach(CompareDateRange p, context->compareDateRanges) {
                if (p.isChecked()) {

                    // create a tuple (metrics, color)
                    PyObject* si = seasonIntervals(DateRange(p.start, p.end), type);
                    PyObject* tuple = Py_BuildValue("(Os)", si, p.color.name().toUtf8().constData());
                    Py_DECREF(si);
                    // add to back and move on
                    PyList_SET_ITEM(list, idx++, tuple);
                }
            }

            return list;

        } else { // compare isn't active...

            // otherwise return the current metrics in a compare list
            PyObject* list = PyList_New(1);

            // create a tuple (metrics, color)
            DateRange range = context->currentDateRange();
            PyObject* si = seasonIntervals(range, type);
            PyObject* tuple = Py_BuildValue("(Os)", si, "#FF00FF");
            Py_DECREF(si);
            // add to back and move on
            PyList_SET_ITEM(list, 0, tuple);

            return list;
        }

    } else {

        // just a dict of metrics
        DateRange range = context->currentDateRange();
        return seasonIntervals(range, type);
    }
}

PyObject*
Bindings::seasonIntervals(DateRange range, QString type) const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL || context->athlete == NULL || context->athlete->rideCache == NULL) return NULL;

    const RideMetricFactory &factory = RideMetricFactory::instance();
    int intervals = 0;

    // how many interval to return in the currently selected date range ?

    // apply any global filters
    Specification specification;
    FilterSet fs;
    fs.addFilter(context->isfiltered, context->filters);
    fs.addFilter(context->ishomefiltered, context->homeFilters);
    if (python->perspective) fs.addFilter(python->perspective->isFiltered(), python->perspective->filterlist(DateRange(QDate(1,1,1970),QDate(31,12,3000))));
    specification.setFilterSet(fs);

    // we need to count intervals that are in range...
    intervals = 0;
    foreach(RideItem *ride, context->athlete->rideCache->rides()) {
        if (!specification.pass(ride)) continue;
        if (!range.pass(ride->dateTime.date())) continue;

        if (type.isEmpty()) intervals += ride->intervals().count();
        else {
            foreach(IntervalItem *item, ride->intervals())
                if (type == RideFileInterval::typeDescription(item->type))
                    intervals++;
        }
    }

    PyObject* dict = PyDict_New();
    if (dict == NULL) return dict;

    //
    // Date, Time, Name Type and Color
    //
    if (PyDateTimeAPI == NULL) PyDateTime_IMPORT;// import datetime if necessary

    PyObject* datelist = PyList_New(intervals);
    PyObject* timelist = PyList_New(intervals);
    PyObject* namelist = PyList_New(intervals);
    PyObject* typelist = PyList_New(intervals);
    PyObject* colorlist = PyList_New(intervals);

    int idx=0;
    foreach(RideItem *ride, context->athlete->rideCache->rides()) {
        if (!specification.pass(ride)) continue;
        if (range.pass(ride->dateTime.date())) {
            foreach(IntervalItem *item, ride->intervals())
                if (type.isEmpty() || type == RideFileInterval::typeDescription(item->type)) {

                    // DATE
                    QDate d = ride->dateTime.date();
                    PyList_SET_ITEM(datelist, idx, PyDate_FromDate(d.year(), d.month(), d.day()));

                    // TIME - time offsets by time of interval
                    QTime t = ride->dateTime.time().addSecs(item->start);
                    PyList_SET_ITEM(timelist, idx, PyTime_FromTime(t.hour(), t.minute(), t.second(), t.msec()*10));

                    // NAME
                    PyList_SET_ITEM(namelist, idx, PyUnicode_FromString(item->name.toUtf8().constData()));

                    // TYPE
                    PyList_SET_ITEM(typelist, idx, PyUnicode_FromString(RideFileInterval::typeDescription(item->type).toUtf8().constData()));

                    // apply item color, remembering that 1,1,1 means use default (reverse in this case)
                    QString color;
                    if (item->color == QColor(1,1,1,1)) {
                        // use the inverted color, not plot marker as that hideous
                        QColor col =GCColor::invertColor(GColor(CPLOTBACKGROUND));
                        // white is jarring on a dark background!
                        if (col==QColor(Qt::white)) col=QColor(127,127,127);
                        color = col.name();
                    } else
                        color = item->color.name();
                    PyList_SET_ITEM(colorlist, idx, PyUnicode_FromString(color.toUtf8().constData()));

                    idx++;
                }
        }
    }

    PyDict_SetItemString_Steal(dict, "date", datelist);
    PyDict_SetItemString_Steal(dict, "time", timelist);
    PyDict_SetItemString_Steal(dict, "name", namelist);
    PyDict_SetItemString_Steal(dict, "type", typelist);
    PyDict_SetItemString_Steal(dict, "color", colorlist);

    //
    // METRICS
    //
    for(int i=0; i<factory.metricCount();i++) {

        // set a list of metric values
        PyObject* metriclist = PyList_New(intervals);

        QString symbol = factory.metricName(i);
        const RideMetric *metric = factory.rideMetric(symbol);
        QString name = GlobalContext::context()->specialFields.internalName(factory.rideMetric(symbol)->name());
        name = name.replace(" ","_");
        name = name.replace("'","_");

        bool useMetricUnits = GlobalContext::context()->useMetricUnits;

        int index=0;
        foreach(RideItem *item, context->athlete->rideCache->rides()) {
            if (!specification.pass(item)) continue;
            if (range.pass(item->dateTime.date())) {

                foreach(IntervalItem *interval, item->intervals()) {
                    if (type.isEmpty() || type == RideFileInterval::typeDescription(interval->type))
                        PyList_SET_ITEM(metriclist, index++, PyFloat_FromDouble(interval->metrics()[i] * (useMetricUnits ? 1.0f : metric->conversion()) + (useMetricUnits ? 0.0f : metric->conversionSum())));
                }
            }
        }

        // add to the dict
        PyDict_SetItemString_Steal(dict, name.toUtf8().constData(), metriclist);
    }

    return dict;
}

QString
Bindings::intervalType(int type) const
{
    return RideFileInterval::typeDescription(static_cast<RideFileInterval::IntervalType>(type));
}

PyObject*
Bindings::activityIntervals(QString type, PyObject* activity) const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL) return NULL;

    RideItem* ride = fromDateTime(activity);
    if (ride == NULL) ride = python->contexts.value(threadid()).item;
    if (ride == NULL) ride = const_cast<RideItem*>(context->currentRideItem());
    if (ride == NULL) return NULL;

    const RideMetricFactory &factory = RideMetricFactory::instance();
    int intervals = 0;

    // how many interval to return in the currently selected RideItem ?

    // we need to count intervals that are of requested type
    intervals = 0;
    if (type.isEmpty()) intervals = ride->intervals().count();
    else foreach(IntervalItem *item, ride->intervals()) if (type == RideFileInterval::typeDescription(item->type)) intervals++;

    PyObject* dict = PyDict_New();
    if (dict == NULL) return dict;

    //
    // Start, Stop, Name Type and Color
    //
    if (PyDateTimeAPI == NULL) PyDateTime_IMPORT;// import datetime if necessary

    PyObject* startlist = PyList_New(intervals);
    PyObject* stoplist = PyList_New(intervals);
    PyObject* namelist = PyList_New(intervals);
    PyObject* typelist = PyList_New(intervals);
    PyObject* colorlist = PyList_New(intervals);
    PyObject* selectedlist = PyList_New(intervals);

    int idx=0;
    foreach(IntervalItem *item, ride->intervals())
        if (type.isEmpty() || type == RideFileInterval::typeDescription(item->type)) {

            // START
            PyList_SET_ITEM(startlist, idx, PyFloat_FromDouble(item->start));

            // STOP
            PyList_SET_ITEM(stoplist, idx, PyFloat_FromDouble(item->stop));

            // NAME
            PyList_SET_ITEM(namelist, idx, PyUnicode_FromString(item->name.toUtf8().constData()));

            // TYPE
            PyList_SET_ITEM(typelist, idx, PyUnicode_FromString(RideFileInterval::typeDescription(item->type).toUtf8().constData()));

            // apply item color, remembering that 1,1,1 means use default (reverse in this case)
            QString color;
            if (item->color == QColor(1,1,1,1)) {
                // use the inverted color, not plot marker as that hideous
                QColor col =GCColor::invertColor(GColor(CPLOTBACKGROUND));
                // white is jarring on a dark background!
                if (col==QColor(Qt::white)) col=QColor(127,127,127);
                color = col.name();
            } else
                color = item->color.name();
            PyList_SET_ITEM(colorlist, idx, PyUnicode_FromString(color.toUtf8().constData()));

            // SELECTED
            PyList_SET_ITEM(selectedlist, idx, PyBool_FromLong(item->selected));

            idx++;
        }

    PyDict_SetItemString_Steal(dict, "start", startlist);
    PyDict_SetItemString_Steal(dict, "stop", stoplist);
    PyDict_SetItemString_Steal(dict, "name", namelist);
    PyDict_SetItemString_Steal(dict, "type", typelist);
    PyDict_SetItemString_Steal(dict, "color", colorlist);
    PyDict_SetItemString_Steal(dict, "selected", selectedlist);

    //
    // METRICS
    //
    for(int i=0; i<factory.metricCount();i++) {

        // set a list of metric values
        PyObject* metriclist = PyList_New(intervals);

        QString symbol = factory.metricName(i);
        const RideMetric *metric = factory.rideMetric(symbol);
        QString name = GlobalContext::context()->specialFields.internalName(factory.rideMetric(symbol)->name());
        name = name.replace(" ","_");
        name = name.replace("'","_");

        bool useMetricUnits = GlobalContext::context()->useMetricUnits;

        int index=0;
        foreach(IntervalItem *item, ride->intervals()) {
            if (type.isEmpty() || type == RideFileInterval::typeDescription(item->type))
                PyList_SET_ITEM(metriclist, index++, PyFloat_FromDouble(item->metrics()[i] * (useMetricUnits ? 1.0f : metric->conversion()) + (useMetricUnits ? 0.0f : metric->conversionSum())));
        }

        // add to the dict
        PyDict_SetItemString_Steal(dict, name.toUtf8().constData(), metriclist);
    }

    return dict;
}

bool
Bindings::createXDataSeries(QString name, QString series, QString seriesUnit, PyObject *activity) const
{
    bool readOnly = python->contexts.value(threadid()).readOnly;
    if (readOnly) return false;

    if (series == "secs" || series == "km")
        return false; // invalid series name

    RideFile *f = selectRideFile(activity);
    if (f == nullptr) return false;

    XDataSeries *xds = nullptr;
    if (f->xdata().contains(name)) {
        xds = f->xdata()[name];
        if (xds->valuename.contains(series))
            return false; // XData series exists already
    }

    QList<RideFile *> *editedRideFiles = python->contexts.value(threadid()).editedRideFiles;
    if (editedRideFiles && !editedRideFiles->contains(f)) {
        f->command->startLUW(QString("Python_%1").arg(threadid()));
        editedRideFiles->append(f);
    }

    // create xdata if not exists
    if (xds == nullptr) {
        xds = new XDataSeries();
        xds->name = name;
        xds->valuename << series;
        xds->unitname << seriesUnit;
        f->command->addXData(xds);
    } else {
        // add series for existing xdata
        f->command->addXDataSeries(name, series, seriesUnit);
    }

    return true;
}

bool
Bindings::deleteActivitySample(int index, PyObject *activity) const
{
    bool readOnly = python->contexts.value(threadid()).readOnly;
    if (readOnly) return false;

    RideFile *f = selectRideFile(activity);
    if (f == nullptr) return false;

    // count the samples
    int pCount = 0;
    RideFileIterator it(f, python->contexts.value(threadid()).spec);
    while (it.hasNext()) { it.next(); pCount++; }

    if (index < 0) index += pCount;
    if (index < 0 || index >= pCount) return false;

    QList<RideFile *> *editedRideFiles = python->contexts.value(threadid()).editedRideFiles;
    if (!readOnly && editedRideFiles && !editedRideFiles->contains(f)) {
        f->command->startLUW(QString("Python_%1").arg(threadid()));
        editedRideFiles->append(f);
    }

    f->command->deletePoints(index, 1);
    return true;
}

bool
Bindings::deleteSeries(int type, PyObject *activity) const
{
    bool readOnly = python->contexts.value(threadid()).readOnly;
    if (readOnly) return false;

    RideFile *f = selectRideFile(activity);
    if (f == nullptr) return false;

    QList<RideFile *> *editedRideFiles = python->contexts.value(threadid()).editedRideFiles;
    if (!readOnly && editedRideFiles && !editedRideFiles->contains(f)) {
        f->command->startLUW(QString("Python_%1").arg(threadid()));
        editedRideFiles->append(f);
    }

    RideFile::SeriesType seriesType = static_cast<RideFile::SeriesType>(type);
    f->command->setDataPresent(seriesType, false);
    return true;
}

bool
Bindings::postProcess(QString processor, PyObject *activity) const
{
    bool readOnly = python->contexts.value(threadid()).readOnly;
    if (readOnly) return false;

    RideFile *f = selectRideFile(activity);
    if (f == nullptr) return false;

    DataProcessor* dp = DataProcessorFactory::instance().getProcessors().value(processor, nullptr);
    if (!dp || !dp->isCoreProcessor()) return false; // Python DPs are disabled due to #4095
    return dp->postProcess(f, nullptr, "PYTHON");
}

bool
Bindings::setTag(QString name, QString value, PyObject *activity) const
{
    bool readOnly = python->contexts.value(threadid()).readOnly;
    if (readOnly) return false;

    Context *context = python->contexts.value(threadid()).context;
    if (context == nullptr) return false;

    RideItem* m = fromDateTime(activity);
    if (m == nullptr) m = const_cast<RideItem*>(context->currentRideItem());
    if (m == nullptr) return false;

    RideFile *f = m->ride();
    if (f == nullptr) return false;

    name = name.replace("_"," ");
    if (GlobalContext::context()->specialFields.isMetric(name)) {

        QString symbol = GlobalContext::context()->specialFields.metricSymbol(name);
        // lets set the override
        QMap<QString,QString> override;
        override  = f->metricOverrides.value(symbol);

        // clear and reset override value for this metric
        override.insert("value", QString("%1").arg(value.toDouble()));

        // update overrides for this metric in the main QMap
        f->metricOverrides.insert(symbol, override);

    } else {

        f->setTag(name, value);
    }

    // rideFile is now dirty!
    m->setDirty(true);
    // get refresh done, coz overrides state has changed
    m->notifyRideMetadataChanged();

    return true;
}

bool
Bindings::delTag(QString name, PyObject *activity) const
{
    bool readOnly = python->contexts.value(threadid()).readOnly;
    if (readOnly) return false;

    Context *context = python->contexts.value(threadid()).context;
    if (context == nullptr) return false;

    RideItem* m = fromDateTime(activity);
    if (m == nullptr) m = const_cast<RideItem*>(context->currentRideItem());
    if (m == nullptr) return false;

    RideFile *f = m->ride();
    if (f == nullptr) return false;

    name = name.replace("_"," ");
    if (GlobalContext::context()->specialFields.isMetric(name)) {

        if (f->metricOverrides.remove(GlobalContext::context()->specialFields.metricSymbol(name))) {

            // rideFile is now dirty!
            m->setDirty(true);
            // get refresh done, coz overrides state has changed
            m->notifyRideMetadataChanged();

            return true;
        }
        return false;

    } else {

        if (f->removeTag(name)) {

            // rideFile is now dirty!
            m->setDirty(true);
            // get refresh done, coz overrides state has changed
            m->notifyRideMetadataChanged();

            return true;
        }
        return false;
    }
}

bool
Bindings::hasTag(QString name, PyObject *activity) const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == nullptr) return false;

    RideItem* m = fromDateTime(activity);
    if (m == nullptr) m = const_cast<RideItem*>(context->currentRideItem());
    if (m == nullptr) return false;

    name = name.replace("_"," ");
    if (GlobalContext::context()->specialFields.isMetric(name)) {
        return m->overrides_.contains(GlobalContext::context()->specialFields.metricSymbol(name));
    } else {
        return m->hasText(name);
    }
}

PythonDataSeries*
Bindings::metrics(QString metric, bool all, QString filter) const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL || context->athlete == NULL || context->athlete->rideCache == NULL) return NULL;

    // how many rides to return if we're limiting to the
    // currently selected date range ?
    DateRange range = context->currentDateRange();

    // apply any global filters
    Specification specification;
    FilterSet fs;
    fs.addFilter(context->isfiltered, context->filters);
    fs.addFilter(context->ishomefiltered, context->homeFilters);
    if (python->perspective) fs.addFilter(python->perspective->isFiltered(), python->perspective->filterlist(DateRange(QDate(1,1,1970),QDate(31,12,3000))));

    // did call contain a filter?
    if (filter != "") {

        DataFilter dataFilter(python->chart, context);
        QStringList files;
        dataFilter.parseFilter(context, filter, &files);
        fs.addFilter(true, files);
    }

    specification.setFilterSet(fs);

    // we need to count rides that are in range...
    int rides = 0;
    foreach(RideItem *ride, context->athlete->rideCache->rides()) {
        if (!specification.pass(ride)) continue;
        if (all || range.pass(ride->dateTime.date())) rides++;
    }

    const RideMetricFactory &factory = RideMetricFactory::instance();
    bool useMetricUnits = GlobalContext::context()->useMetricUnits;
    for(int i=0; i<factory.metricCount();i++) {

        QString symbol = factory.metricName(i);
        const RideMetric *m = factory.rideMetric(symbol);
        QString name = GlobalContext::context()->specialFields.internalName(factory.rideMetric(symbol)->name());
        name = name.replace(" ","_");
        name = name.replace("'","_");

        if (name == metric) {

            // found, set an array of metric values
            PythonDataSeries* pds = new PythonDataSeries(name, rides);

            int idx = 0;
            foreach(RideItem *item, context->athlete->rideCache->rides()) {
                if (!specification.pass(item)) continue;
                if (all || range.pass(item->dateTime.date())) {
                    pds->data[idx++] = item->metrics()[i] * (useMetricUnits ? 1.0f : m->conversion()) + (useMetricUnits ? 0.0f : m->conversionSum());
                }
            }

            // Done, return the series
            return pds;
        }
    }

    // Not found, nothing to return
    return NULL;
}

PyObject*
Bindings::activityMeanmax(bool compare) const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL) return NULL;

    // want a list of compares
    if (compare) {

        // only return compares if its actually active
        if (context->isCompareIntervals) {

            // how many to return?
            int count = 0;
            foreach(CompareInterval p, context->compareIntervals) if (p.isChecked()) count++;
            PyObject* list = PyList_New(count);

            // create a dict for each and add to list as tuple (meanmax, color)
            long idx = 0;
            foreach(CompareInterval p, context->compareIntervals) {
                if (p.isChecked()) {

                    // create a tuple (meanmax, color)
                    PyObject* am = activityMeanmax(p.rideItem);
                    PyObject* tuple = Py_BuildValue("(Os)", am, p.color.name().toUtf8().constData());
                    Py_DECREF(am);
                    PyList_SET_ITEM(list, idx++, tuple);
                }
            }

            return list;
        } else { // compare isn't active...

            // otherwise return the current meanmax in a compare list
            if (context->currentRideItem()==NULL) return NULL;
            PyObject* list = PyList_New(1);

            PyObject* am = activityMeanmax(context->currentRideItem());
            PyObject* tuple = Py_BuildValue("(Os)", am, "#FF00FF");
            Py_DECREF(am);
            PyList_SET_ITEM(list, 0, tuple);

            return list;
        }
    } else {

        // not compare, so just return a dict
        RideItem *item = python->contexts.value(threadid()).item;
        if (item == NULL) item = const_cast<RideItem*>(context->currentRideItem());
        return activityMeanmax(item);
    }
}

PyObject*
Bindings::seasonMeanmax(bool all, QString filter, bool compare) const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL) return NULL;

    // want a list of compares
    if (compare && context) {

        // only return compares if its actually active
        if (context->isCompareDateRanges) {

            // how many to return?
            int count=0;
            foreach(CompareDateRange p, context->compareDateRanges) if (p.isChecked()) count++;

            // cool we can return a list of meanaxes to compare
            PyObject* list = PyList_New(count);
            int idx = 0;

            // create a dict for each and add to list
            foreach(CompareDateRange p, context->compareDateRanges) {
                if (p.isChecked()) {

                    // create a tuple (meanmax, color)
                    PyObject* sm = seasonMeanmax(all, DateRange(p.start, p.end), filter);
                    PyObject* tuple = Py_BuildValue("(Os)", sm, p.color.name().toUtf8().constData());
                    Py_DECREF(sm);
                    // add to back and move on
                    PyList_SET_ITEM(list, idx++, tuple);
                }
            }

            return list;

        } else { // compare isn't active...

            // otherwise return the current season meanmax in a compare list
            PyObject* list = PyList_New(1);

            // create a tuple (meanmax, color)
            DateRange range = context->currentDateRange();
            PyObject* sm = seasonMeanmax(all, range, filter);
            PyObject* tuple = Py_BuildValue("(Os)", sm, "#FF00FF");
            Py_DECREF(sm);
            // add to back and move on
            PyList_SET_ITEM(list, 0, tuple);

            return list;
        }

    } else {

        // just a datafram of meanmax
        DateRange range = context->currentDateRange();

        return seasonMeanmax(all, range, filter);
    }
}

PyObject*
Bindings::seasonMeanmax(bool all, DateRange range, QString filter) const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL) return NULL;

    // construct the date range and then get a ridefilecache
    if (all) range = DateRange(QDate(1900,01,01), QDate(2100,01,01));

    // did call contain any filters?
    QStringList filelist;
    bool filt=false;

    if (python->perspective) {
        filelist = python->perspective->filterlist(DateRange(QDate(1,1,1970),QDate(31,12,3000)));
        filt = python->perspective->isFiltered();
    }

    // if not empty write a filter
    if (filter != "") {

        DataFilter dataFilter(python->chart, context);
        dataFilter.parseFilter(context, filter, &filelist);
        filt=true;
    }

    // RideFileCache for a date range with our filters (if any)
    RideFileCache cache(context, range.from, range.to, filt, filelist, true, NULL);

    return rideFileCacheMeanmax(&cache);
}

PyObject*
Bindings::activityMeanmax(const RideItem* item) const
{
    return rideFileCacheMeanmax(const_cast<RideItem*>(item)->fileCache());
}

PyObject*
Bindings::rideFileCacheMeanmax(RideFileCache* cache) const
{
    if (PyDateTimeAPI == NULL) PyDateTime_IMPORT;// import datetime if necessary

    if (cache == NULL) return NULL;

    // we return a dict
    PyObject* ans = PyDict_New();

    //
    // Now we need to add lists to the ans dict...
    //
    foreach(RideFile::SeriesType series, cache->meanMaxList()) {

        QVector <double> values = cache->meanMaxArray(series);

        // don't add empty ones but we always add power
        if (series != RideFile::watts && values.count()==0) continue;


        // set a list
        PyObject* list = PyList_New(values.count());

        // will have different sizes e.g. when a daterange
        // since longest ride with e.g. power may be different
        // to longest ride with heartrate
        for(int j=0; j<values.count(); j++) PyList_SET_ITEM(list, j, PyFloat_FromDouble(values[j]));

        // add to the dict
        PyDict_SetItemString_Steal(ans, RideFile::seriesName(series, true).toUtf8().constData(), list);

        // if is power add the dates
        if(series == RideFile::watts) {

            // dates
            QVector<QDate> dates = cache->meanMaxDates(series);
            PyObject* datelist = PyList_New(dates.count());
            // will have different sizes e.g. when a daterange
            // since longest ride with e.g. power may be different
            // to longest ride with heartrate
            for(int j=0; j<dates.count(); j++) {

                // make sure its a valid date
                if (j==0) dates[j] = QDate::currentDate();
                PyList_SET_ITEM(datelist, j, PyDate_FromDate(dates[j].year(), dates[j].month(), dates[j].day()));
            }

            // add to the dict
            PyDict_SetItemString_Steal(ans, "power_date", datelist);
        }
    }

    // return a valid result
    return ans;
}

PyObject*
Bindings::seasonPmc(bool all, QString metric) const
{
    Context *context = python->contexts.value(threadid()).context;

    // return a dict with PMC data for all or the current season
    // XXX uses the default half-life
    if (context) {

        // import datetime if necessary
        if (PyDateTimeAPI == NULL) PyDateTime_IMPORT;

        // get the currently selected date range
        DateRange range(context->currentDateRange());

        // convert the name to a symbol, if not found just leave as it is
        const RideMetricFactory &factory = RideMetricFactory::instance();
        for (int i=0; i<factory.metricCount(); i++) {
            QString symbol = factory.metricName(i);
            QString name = GlobalContext::context()->specialFields.internalName(factory.rideMetric(symbol)->name());
            name.replace(" ","_");

            if (name == metric) {
                metric = symbol;
                break;
            }
        }

        // create the data
        PMCData pmcData(context, Specification(), metric);

        // how many entries ?
        unsigned int size = all ? pmcData.days() : range.from.daysTo(range.to) + 1;
        // returning a dict with
        // date, stress, lts, sts, sb, rr
        PyObject* ans = PyDict_New();

        // DATE - 1 a day from start
        PyObject* datelist = PyList_New(size);
        QDate start = all ? pmcData.start() : range.from;
        for(unsigned int k=0; k<size; k++) {
            QDate d = start.addDays(k);
            PyList_SET_ITEM(datelist, k, PyDate_FromDate(d.year(), d.month(), d.day()));
        }

        // add to the dict
        PyDict_SetItemString_Steal(ans, "date", datelist);

        // PMC DATA

        PyObject* stress = PyList_New(size);
        PyObject* lts = PyList_New(size);
        PyObject* sts = PyList_New(size);
        PyObject* sb = PyList_New(size);
        PyObject* rr = PyList_New(size);

        if (all) {

            // just copy
            for(unsigned int k=0; k<size; k++) {

                PyList_SET_ITEM(stress, k, PyFloat_FromDouble(pmcData.stress()[k]));
                PyList_SET_ITEM(lts, k, PyFloat_FromDouble(pmcData.lts()[k]));
                PyList_SET_ITEM(sts, k, PyFloat_FromDouble(pmcData.sts()[k]));
                PyList_SET_ITEM(sb, k, PyFloat_FromDouble(pmcData.sb()[k]));
                PyList_SET_ITEM(rr, k, PyFloat_FromDouble(pmcData.rr()[k]));
            }

        } else {

            unsigned int index=0;
            for(int k=0; k < pmcData.days(); k++) {

                // day today
                if (start.addDays(k) >= range.from && start.addDays(k) <= range.to) {

                    PyList_SET_ITEM(stress, index, PyFloat_FromDouble(pmcData.stress()[k]));
                    PyList_SET_ITEM(lts, index, PyFloat_FromDouble(pmcData.lts()[k]));
                    PyList_SET_ITEM(sts, index, PyFloat_FromDouble(pmcData.sts()[k]));
                    PyList_SET_ITEM(sb, index, PyFloat_FromDouble(pmcData.sb()[k]));
                    PyList_SET_ITEM(rr, index, PyFloat_FromDouble(pmcData.rr()[k]));
                    index++;
                }
            }
        }

        // add to the dict
        PyDict_SetItemString_Steal(ans, "stress", stress);
        PyDict_SetItemString_Steal(ans, "lts", lts);
        PyDict_SetItemString_Steal(ans, "sts", sts);
        PyDict_SetItemString_Steal(ans, "sb", sb);
        PyDict_SetItemString_Steal(ans, "rr", rr);

        // return it
        return ans;
    }

    // nothing to return
    return NULL;
}

PyObject*
Bindings::seasonMeasures(bool all, QString group) const
{
    Context *context = python->contexts.value(threadid()).context;

    // return a dict with Measures data for all or the current season
    if (context && context->athlete && context->athlete->measures) {

        // import datetime if necessary
        if (PyDateTimeAPI == NULL) PyDateTime_IMPORT;

        // get the currently selected date range
        DateRange range(context->currentDateRange());

        // convert the group symbol to an index, default to Body=0
        int groupIdx = context->athlete->measures->getGroupSymbols().indexOf(group);
        if (groupIdx < 0) groupIdx = 0;

        // Update range for all
        if (all) {
            range.from = context->athlete->measures->getStartDate(groupIdx);
            range.to = context->athlete->measures->getEndDate(groupIdx);
        }

        // how many entries ?
        unsigned int size = range.from.daysTo(range.to) + 1;

        // returning a dict with
        // date, field1, field2, ...
        PyObject* ans = PyDict_New();

        // DATE - 1 a day from start
        PyObject* datelist = PyList_New(size);
        QDate start = range.from;
        for(unsigned int k=0; k<size; k++) {
            QDate d = start.addDays(k);
            PyList_SET_ITEM(datelist, k, PyDate_FromDate(d.year(), d.month(), d.day()));
        }

        // add to the dict
        PyDict_SetItemString_Steal(ans, "date", datelist);

        // MEASURES DATA
        QStringList fieldSymbols = context->athlete->measures->getFieldSymbols(groupIdx);
        QVector<PyObject*> fields(fieldSymbols.count());
        for (int i=0; i<fieldSymbols.count(); i++)
            fields[i] = PyList_New(size);

        unsigned int index = 0;
        for(unsigned int k=0; k < size; k++) {

            // day today
            if (start.addDays(k) >= range.from && start.addDays(k) <= range.to) {

                for (int fieldIdx=0; fieldIdx<fields.count(); fieldIdx++)
                    PyList_SET_ITEM(fields[fieldIdx], index, PyFloat_FromDouble(context->athlete->measures->getFieldValue(groupIdx, start.addDays(k), fieldIdx)));

                index++;
            }
        }

        // add to the dict
        for (int fieldIdx=0; fieldIdx<fields.count(); fieldIdx++)
            PyDict_SetItemString_Steal(ans, fieldSymbols[fieldIdx].toUtf8().constData(), fields[fieldIdx]);

        // return it
        return ans;
    }

    // nothing to return
    return NULL;
}

PyObject*
Bindings::season(bool all, bool compare) const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL) return NULL;

    // import datetime if necessary
    if (PyDateTimeAPI == NULL) PyDateTime_IMPORT;

    // dict for season: color, name, start, end
    // XXX TODO type needs adding, but we need to unpick the
    //          phase/season object model first, will do later
    PyObject* ans = PyDict_New();

    // worklist of date ranges to return
    // XXX TODO use a Season worklist one the phase/season
    //          object model is fixed
    QList<DateRange> worklist;

    if (compare) {
        // return a list, even if just one
        if (context->isCompareDateRanges) {
            foreach(CompareDateRange p, context->compareDateRanges)
                worklist << DateRange(p.start, p.end, p.name, p.color);
        } else {
            // if compare not active just return current selection
            worklist << context->currentDateRange();
        }

    } else if (all) {
        // list all seasons
        foreach(Season season, context->athlete->seasons->seasons) {
            worklist << DateRange(season.getStart(), season.getEnd(), season.name, QColor(127,127,127));
        }

    } else {

        // just the currently selected season please
        worklist << context->currentDateRange();
    }

    PyObject* start = PyList_New(worklist.count());
    PyObject* end = PyList_New(worklist.count());
    PyObject* name = PyList_New(worklist.count());
    PyObject* color = PyList_New(worklist.count());

    int index=0;

    foreach(DateRange p, worklist){

        PyList_SET_ITEM(start, index, PyDate_FromDate(p.from.year(), p.from.month(), p.from.day()));
        PyList_SET_ITEM(end, index, PyDate_FromDate(p.to.year(), p.to.month(), p.to.day()));
        PyList_SET_ITEM(name, index, PyUnicode_FromString(p.name.toUtf8().constData()));
        PyList_SET_ITEM(color, index, PyUnicode_FromString(p.color.name().toUtf8().constData()));
        index++;
    }

    // list into a data.frame
    PyDict_SetItemString_Steal(ans, "start", start);
    PyDict_SetItemString_Steal(ans, "end", end);
    PyDict_SetItemString_Steal(ans, "name", name);
    PyDict_SetItemString_Steal(ans, "color", color);

    // return it
    return ans;
}

PyObject*
Bindings::seasonPeaks(QString series, int duration, bool all, QString filter, bool compare) const
{
    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL) return NULL;

    // lets get a Map of names to series
    QMap<QString, RideFile::SeriesType> snames;
    foreach(RideFile::SeriesType s, RideFileCache::meanMaxList()) {
        snames.insert(RideFile::seriesName(s, true), s);
    }

    // extract as QStrings
    QList<RideFile::SeriesType> seriesList;
    RideFile::SeriesType stype;
    if ((stype=snames.value(series, RideFile::none)) == RideFile::none)
        return NULL;
    else
        seriesList << stype;

    // extract as integers
    QList<int>durations;
    if (duration <= 0)
        return NULL;
    else
        durations << duration;

    // want a list of compares not a dataframe
    if (compare) {

        // only return compares if its actually active
        if (context->isCompareDateRanges) {

            // how many to return?
            int count=0;
            foreach(CompareDateRange p, context->compareDateRanges) if (p.isChecked()) count++;

            // cool we can return a list of intervals to compare
            PyObject* list = PyList_New(count);
            int idx = 0;

            // create a dict for each and add to list
            foreach(CompareDateRange p, context->compareDateRanges) {
                if (p.isChecked()) {

                    // create a tuple (peaks, color)
                    PyObject* sp = seasonPeaks(all, DateRange(p.start, p.end), filter, seriesList, durations);
                    PyObject* tuple = Py_BuildValue("(Os)", sp, p.color.name().toUtf8().constData());
                    Py_DECREF(sp);
                    // add to back and move on
                    PyList_SET_ITEM(list, idx++, tuple);
                }
            }

            return list;

        } else { // compare isn't active...

            // otherwise return the current season meanmax in a compare list
            PyObject* list = PyList_New(1);

            // create a tuple (peaks, color)
            DateRange range = context->currentDateRange();
            PyObject* sp = seasonPeaks(all, range, filter, seriesList, durations);
            PyObject* tuple = Py_BuildValue("(Os)", sp, "#FF00FF");
            Py_DECREF(sp);
            // add to back and move on
            PyList_SET_ITEM(list, 0, tuple);

            return list;
        }

    } else if (context->athlete && context->athlete->rideCache) {

        // just a dict of peaks
        DateRange range = context->currentDateRange();

        return seasonPeaks(all, range, filter, seriesList, durations);

    }

    // fail
    return NULL;
}

PyObject*
Bindings::seasonPeaks(bool all, DateRange range, QString filter, QList<RideFile::SeriesType> series, QList<int> durations) const
{
    if (PyDateTimeAPI == NULL) PyDateTime_IMPORT;// import datetime if necessary

    Context *context = python->contexts.value(threadid()).context;
    if (context == NULL) return NULL;

    // we return a dict
    PyObject* ans = PyDict_New();
    if (ans == NULL) return NULL;

    // how many rides ?
    Specification specification;
    FilterSet fs;
    fs.addFilter(context->isfiltered, context->filters);
    fs.addFilter(context->ishomefiltered, context->homeFilters);
    if (python->perspective) fs.addFilter(python->perspective->isFiltered(), python->perspective->filterlist(DateRange(QDate(1,1,1970),QDate(31,12,3000))));
    specification.setFilterSet(fs);

    // did call contain any filters?
    if (filter != "") {

        DataFilter dataFilter(python->chart, context);
        QStringList files;
        dataFilter.parseFilter(context, filter, &files);
        fs.addFilter(true, files);
    }
    specification.setFilterSet(fs);

    // how many pass?
    int size=0;
    foreach(RideItem *item, context->athlete->rideCache->rides()) {

        // apply filters
        if (!specification.pass(item)) continue;

        // do we want this one ?
        if (all || range.pass(item->dateTime.date()))  size++;
    }

    // dates first
    PyObject* datetimelist = PyList_New(size);

    // fill with values for date
    int i=0;
    foreach(RideItem *item, context->athlete->rideCache->rides()) {
        // apply filters
        if (!specification.pass(item)) continue;

        if (all || range.pass(item->dateTime.date())) {
            // add datetime to the list
            QDate d = item->dateTime.date();
            QTime t = item->dateTime.time();
            PyList_SET_ITEM(datetimelist, i++, PyDateTime_FromDateAndTime(d.year(), d.month(), d.day(), t.hour(), t.minute(), t.second(), t.msec()*10));
        }
    }

    // add to the dict
    PyDict_SetItemString_Steal(ans, "datetime", datetimelist);

    foreach(RideFile::SeriesType pseries, series) {

        foreach(int pduration, durations) {

            // set a list
            PyObject* list = PyList_New(size);

            // give it a name
            QString name = QString("peak_%1_%2").arg(RideFile::seriesName(pseries, true)).arg(pduration);

            // fill with values
            // get the value for the series and duration requested, although this is called
            int index=0;
            foreach(RideItem *item, context->athlete->rideCache->rides()) {

                // apply filters
                if (!specification.pass(item)) continue;

                // do we want this one ?
                if (all || range.pass(item->dateTime.date())) {

                    // for each series/duration independently its pretty quick since it lseeks to
                    // the actual value, so /should't/ be too expensive.........
                    PyList_SET_ITEM(list, index++, PyFloat_FromDouble(RideFileCache::best(item->context, item->fileName, pseries, pduration)));
                }
            }

            // add to the dict
            PyDict_SetItemString_Steal(ans, name.toUtf8().constData(), list);
        }
    }

    return ans;
}


int
Bindings::PyDict_SetItemString_Steal
(PyObject *p, const char *key, PyObject *val) const
{
    int ret = PyDict_SetItemString(p, key, val);
    Py_DECREF(val);
    return ret;
}
