#include <PythonEmbed.h>
#include "Context.h"
#include "RideItem.h"
#include "Athlete.h"
#include "GcUpgrade.h"
#include "PythonChart.h"
#include "Colors.h"
#include "RideCache.h"
#include "DataFilter.h"
#include "PMCData.h"
#include "Season.h"
#include "WPrime.h"

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

QString Bindings::athlete() const
{
    Context *context = python->contexts.value(threadid());
    return context->athlete->cyclist;
}

long Bindings::build() const
{
    return VERSION_LATEST;
}

QString Bindings::version() const
{
    return VERSION_STRING;
}

// get a list of activities (Date&Time)
PyObject*
Bindings::activities(QString filter) const
{
    Context *context = python->contexts.value(threadid());

    if (context && context->athlete && context->athlete->rideCache) {

        // import datetime if necessary
        if (PyDateTimeAPI == NULL) PyDateTime_IMPORT;

        // filters
        // apply any global filters
        Specification specification;
        FilterSet fs;
        fs.addFilter(context->isfiltered, context->filters);
        fs.addFilter(context->ishomefiltered, context->homeFilters);

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
    Context *context = python->contexts.value(threadid());

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

// get the data series for the currently selected ride
PythonDataSeries*
Bindings::series(int type, PyObject* activity) const
{
    Context *context = python->contexts.value(threadid());
    if (context == NULL) return NULL;

    RideItem* item = fromDateTime(activity);
    if (item == NULL) item = const_cast<RideItem*>(context->currentRideItem());
    if (item == NULL) return NULL;

    RideFile* f = item->ride();
    PythonDataSeries* ds = new PythonDataSeries(seriesName(type), f->dataPoints().count());
    for(int i=0; i<ds->count; i++) ds->data[i] = f->dataPoints()[i]->value(static_cast<RideFile::SeriesType>(type));

    return ds;
}

// get the wbal series for the currently selected ride
PythonDataSeries*
Bindings::wbal(PyObject* activity) const
{
    Context *context = python->contexts.value(threadid());
    if (context == NULL) return NULL;

    RideItem* item = fromDateTime(activity);
    if (item == NULL) item = const_cast<RideItem*>(context->currentRideItem());
    if (item == NULL) return NULL;

    RideFile* f = item->ride();
    if (f == NULL) return NULL;

    f->recalculateDerivedSeries();
    WPrime *w = f->wprimeData();
    if (w == NULL) return NULL;

    PythonDataSeries* ds = new PythonDataSeries("WBal", w->ydata().count());
    for(int i=0; i<ds->count; i++) ds->data[i] = w->ydata()[i];

    return ds;
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

    Context *context = python->contexts.value(threadid());
    if (context == NULL) return false;

    RideItem* item = fromDateTime(activity);
    if (item == NULL) item = const_cast<RideItem*>(context->currentRideItem());
    if (item == NULL) return NULL;

    return item->ride()->isDataPresent(static_cast<RideFile::SeriesType>(type));
}

PythonDataSeries::PythonDataSeries(QString name, Py_ssize_t count) : name(name), count(count), data(NULL)
{
    if (count > 0) data = new double[count];
}

// default constructor and copy constructor
PythonDataSeries::PythonDataSeries() : name(QString()), count(0), data(NULL) {}
PythonDataSeries::PythonDataSeries(PythonDataSeries *clone)
{
    *this = *clone;
}

PythonDataSeries::~PythonDataSeries()
{
    if (data) delete[] data;
    data=NULL;
}

PyObject*
Bindings::activityMetrics(bool compare) const
{
    Context *context = python->contexts.value(threadid());
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
                    PyObject* tuple = Py_BuildValue("(Os)", activityMetrics(p.rideItem), p.color.name().toUtf8().constData());
                    PyList_SET_ITEM(list, idx++, tuple);
                }
            }

            return list;
        } else { // compare isn't active...

            // otherwise return the current metrics in a compare list
            if (context->currentRideItem()==NULL) return NULL;
            RideItem *item = const_cast<RideItem*>(context->currentRideItem());
            PyObject* list = PyList_New(1);

            PyObject* tuple = Py_BuildValue("(Os)", activityMetrics(item), "#FF00FF");
            PyList_SET_ITEM(list, 0, tuple);

            return list;
        }
    } else {

        // not compare, so just return a dict
        RideItem *item = const_cast<RideItem*>(context->currentRideItem());

        return activityMetrics(item);
    }
}

PyObject*
Bindings::activityMetrics(RideItem* item) const
{
    Context *context = python->contexts.value(threadid());
    if (context == NULL) return NULL;

    PyObject* dict = PyDict_New();
    if (dict == NULL) return dict;

    const RideMetricFactory &factory = RideMetricFactory::instance();

    //
    // Date and Time
    //
    if (PyDateTimeAPI == NULL) PyDateTime_IMPORT;// import datetime if necessary
    QDate d = item->dateTime.date();
    PyDict_SetItemString(dict, "date", PyDate_FromDate(d.year(), d.month(), d.day()));
    QTime t = item->dateTime.time();
    PyDict_SetItemString(dict, "time", PyTime_FromTime(t.hour(), t.minute(), t.second(), t.msec()*10));

    //
    // METRICS
    //
    for(int i=0; i<factory.metricCount();i++) {

        QString symbol = factory.metricName(i);
        const RideMetric *metric = factory.rideMetric(symbol);
        QString name = context->specialFields.internalName(factory.rideMetric(symbol)->name());
        name = name.replace(" ","_");
        name = name.replace("'","_");

        bool useMetricUnits = context->athlete->useMetricUnits;
        double value = item->metrics()[i] * (useMetricUnits ? 1.0f : metric->conversion()) + (useMetricUnits ? 0.0f : metric->conversionSum());

        // add to the dict
        PyDict_SetItemString(dict, name.toUtf8().constData(), PyFloat_FromDouble(value));
    }

    //
    // META
    //
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {

        // don't add incomplete meta definitions or metric override fields
        if (field.name == "" || field.tab == "" ||
            context->specialFields.isMetric(field.name)) continue;

        // add to the dict
        PyDict_SetItemString(dict, field.name.replace(" ","_").toUtf8().constData(), PyUnicode_FromString(item->getText(field.name, "").toUtf8().constData()));
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
    PyDict_SetItemString(dict, "color", PyUnicode_FromString(color.toUtf8().constData()));

    return dict;
}

PyObject*
Bindings::seasonMetrics(bool all, QString filter, bool compare) const
{
    Context *context = python->contexts.value(threadid());
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
                    PyObject* tuple = Py_BuildValue("(Os)", seasonMetrics(all, DateRange(p.start, p.end), filter), p.color.name().toUtf8().constData());
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
            PyObject* tuple = Py_BuildValue("(Os)", seasonMetrics(all, range, filter), "#FF00FF");
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
    Context *context = python->contexts.value(threadid());
    if (context == NULL || context->athlete == NULL || context->athlete->rideCache == NULL) return NULL;

    // how many rides to return if we're limiting to the
    // currently selected date range ?

    // apply any global filters
    Specification specification;
    FilterSet fs;
    fs.addFilter(context->isfiltered, context->filters);
    fs.addFilter(context->ishomefiltered, context->homeFilters);

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

    PyDict_SetItemString(dict, "date", datelist);
    PyDict_SetItemString(dict, "time", timelist);
    PyDict_SetItemString(dict, "color", colorlist);

    //
    // METRICS
    //
    const RideMetricFactory &factory = RideMetricFactory::instance();
    bool useMetricUnits = context->athlete->useMetricUnits;
    for(int i=0; i<factory.metricCount();i++) {

        QString symbol = factory.metricName(i);
        const RideMetric *metric = factory.rideMetric(symbol);
        QString name = context->specialFields.internalName(factory.rideMetric(symbol)->name());
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
        PyDict_SetItemString(dict, name.toUtf8().constData(), metriclist);
    }

    //
    // META
    //
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {

        // don't add incomplete meta definitions or metric override fields
        if (field.name == "" || field.tab == "" ||
            context->specialFields.isMetric(field.name)) continue;

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
        PyDict_SetItemString(dict, field.name.replace(" ","_").toUtf8().constData(), metalist);
    }

    return dict;
}

PythonDataSeries*
Bindings::metrics(QString metric, bool all, QString filter) const
{
    Context *context = python->contexts.value(threadid());
    if (context == NULL || context->athlete == NULL || context->athlete->rideCache == NULL) return NULL;

    // how many rides to return if we're limiting to the
    // currently selected date range ?
    DateRange range = context->currentDateRange();

    // apply any global filters
    Specification specification;
    FilterSet fs;
    fs.addFilter(context->isfiltered, context->filters);
    fs.addFilter(context->ishomefiltered, context->homeFilters);

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
    bool useMetricUnits = context->athlete->useMetricUnits;
    for(int i=0; i<factory.metricCount();i++) {

        QString symbol = factory.metricName(i);
        const RideMetric *m = factory.rideMetric(symbol);
        QString name = context->specialFields.internalName(factory.rideMetric(symbol)->name());
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
    Context *context = python->contexts.value(threadid());
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
                    PyObject* tuple = Py_BuildValue("(Os)", activityMeanmax(p.rideItem), p.color.name().toUtf8().constData());
                    PyList_SET_ITEM(list, idx++, tuple);
                }
            }

            return list;
        } else { // compare isn't active...

            // otherwise return the current meanmax in a compare list
            if (context->currentRideItem()==NULL) return NULL;
            PyObject* list = PyList_New(1);

            PyObject* tuple = Py_BuildValue("(Os)", activityMeanmax(context->currentRideItem()), "#FF00FF");
            PyList_SET_ITEM(list, 0, tuple);

            return list;
        }
    } else {

        // not compare, so just return a dict
        return activityMeanmax(context->currentRideItem());
    }
}

PyObject*
Bindings::seasonMeanmax(bool all, QString filter, bool compare) const
{
    Context *context = python->contexts.value(threadid());
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
                    PyObject* tuple = Py_BuildValue("(Os)", seasonMeanmax(all, DateRange(p.start, p.end), filter), p.color.name().toUtf8().constData());
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
            PyObject* tuple = Py_BuildValue("(Os)", seasonMeanmax(all, range, filter), "#FF00FF");
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
    Context *context = python->contexts.value(threadid());
    if (context == NULL) return NULL;

    // construct the date range and then get a ridefilecache
    if (all) range = DateRange(QDate(1900,01,01), QDate(2100,01,01));

    // did call contain any filters?
    QStringList filelist;
    bool filt=false;

    // if not empty write a filter
    if (filter != "") {

        DataFilter dataFilter(python->chart, context);
        dataFilter.parseFilter(context, filter, &filelist);
        filt=true;
    }

    // RideFileCache for a date range with our filters (if any)
    RideFileCache cache(context, range.from, range.to, filt, filelist, false, NULL);

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
        PyDict_SetItemString(ans, RideFile::seriesName(series, true).toUtf8().constData(), list);

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
            PyDict_SetItemString(ans, "power_date", datelist);
        }
    }

    // return a valid result
    return ans;
}

PyObject*
Bindings::pmc(bool all, QString metric) const
{
    Context *context = python->contexts.value(threadid());

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
            QString name = context->specialFields.internalName(factory.rideMetric(symbol)->name());
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
        PyDict_SetItemString(ans, "date", datelist);

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
        PyDict_SetItemString(ans, "stress", stress);
        PyDict_SetItemString(ans, "lts", lts);
        PyDict_SetItemString(ans, "sts", sts);
        PyDict_SetItemString(ans, "sb", sb);
        PyDict_SetItemString(ans, "rr", rr);

        // return it
        return ans;
    }

    // nothing to return
    return NULL;
}

PyObject*
Bindings::measures(bool all, QString group) const
{
    Context *context = python->contexts.value(threadid());

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
        PyDict_SetItemString(ans, "date", datelist);

        // MEASURES DATA
        QStringList fieldSymbols = context->athlete->measures->getFieldSymbols(groupIdx);
        QVector<PyObject*> fields(fieldSymbols.count());
        for (int i=0; i<fieldSymbols.count(); i++)
            fields[i] = PyList_New(size);

        unsigned int index = 0;
        for(int k=0; k < size; k++) {

            // day today
            if (start.addDays(k) >= range.from && start.addDays(k) <= range.to) {

                for (int fieldIdx=0; fieldIdx<fields.count(); fieldIdx++)
                    PyList_SET_ITEM(fields[fieldIdx], index, PyFloat_FromDouble(context->athlete->measures->getFieldValue(groupIdx, start.addDays(k), fieldIdx)));

                index++;
            }
        }

        // add to the dict
        for (int fieldIdx=0; fieldIdx<fields.count(); fieldIdx++)
            PyDict_SetItemString(ans, fieldSymbols[fieldIdx].toUtf8().constData(), fields[fieldIdx]);

        // return it
        return ans;
    }

    // nothing to return
    return NULL;
}

PyObject*
Bindings::season(bool all, bool compare) const
{
    Context *context = python->contexts.value(threadid());
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
            worklist << DateRange(season.start, season.end, season.name, QColor(127,127,127));
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
    PyDict_SetItemString(ans, "start", start);
    PyDict_SetItemString(ans, "end", end);
    PyDict_SetItemString(ans, "name", name);
    PyDict_SetItemString(ans, "color", color);

    // return it
    return ans;
}

int
Bindings::webpage(QString url) const
{
#ifdef Q_OS_WIN
    url = url.replace("://C:", ":///C:"); // plotly fails to use enough slashes
    url = url.replace("\\", "/");
#endif

    QUrl p(url);
    python->chart->emitUrl(p);
    return 0;
}
