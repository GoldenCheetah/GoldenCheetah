#include <PythonEmbed.h>
#include "Context.h"
#include "RideItem.h"
#include "Athlete.h"
#include "GcUpgrade.h"
#include "PythonChart.h"
#include "Colors.h"

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

    if (compare && context->isCompareIntervals) {

        // compare mode, return a list with compared intervals
        int count = 0; // how many to return?
        foreach(CompareInterval p, context->compareIntervals) if (p.isChecked()) count++;
        PyObject* list = PyList_New(count);

        // create a data.frame for each and add to list (metrics, color)
        long idx = 0;
        foreach(CompareInterval p, context->compareIntervals) {
            if (p.isChecked()) {
                PyObject* tuple = Py_BuildValue("(Os)", activityMetrics(p.rideItem), p.color.name().toUtf8().constData());
                PyList_SET_ITEM(list, idx++, tuple);
            }
        }

        return list;
    } else if (compare && !context->isCompareIntervals) {

        // not compare mode, return 1 element list with current activity metrics
        if (context->currentRideItem()==NULL) return NULL;
        RideItem *item = const_cast<RideItem*>(context->currentRideItem());
        PyObject* list = PyList_New(1);

        PyObject* tuple = Py_BuildValue("(Os)", activityMetrics(item), "#FF00FF");
        PyList_SET_ITEM(list, 0, tuple);

        return list;
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
