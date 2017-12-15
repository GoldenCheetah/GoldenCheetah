#include <PythonEmbed.h>
#include "Context.h"
#include "RideItem.h"
#include "Athlete.h"
#include "GcUpgrade.h"
#include "PythonChart.h"
#include "Colors.h"
#include "RideCache.h"
#include "DataFilter.h"

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

// get the data series for the currently selected ride
PythonDataSeries*
Bindings::series(int type) const
{
    Context *context = python->contexts.value(threadid());
    if (context == NULL || context->currentRideItem()==NULL) return NULL;

    RideFile* f = const_cast<RideItem*>(context->currentRideItem())->ride();
    PythonDataSeries* ds = new PythonDataSeries(seriesName(type), f->dataPoints().count());
    for(int i=0; i<ds->count; i++) ds->data[i] = f->dataPoints()[i]->value(static_cast<RideFile::SeriesType>(type));

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
Bindings::seriesPresent(int type) const
{

    Context *context = python->contexts.value(threadid());
    if (context == NULL || context->currentRideItem()==NULL) return false;
    return const_cast<RideItem*>(context->currentRideItem())->ride()->isDataPresent(static_cast<RideFile::SeriesType>(type));
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
    PyDict_SetItemString(dict, "time", PyTime_FromTime(t.hour(), t.minute(), t.second(), t.msec()));

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
            PyList_SET_ITEM(timelist, idx, PyTime_FromTime(t.hour(), t.minute(), t.second(), t.msec()));

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
