#include <PythonEmbed.h>
#include "Context.h"
#include "RideItem.h"
#include "Athlete.h"
#include "GcUpgrade.h"
#include "PythonChart.h"
#include "Bindings.h"

#include <QWebEngineView>
#include <QUrl>

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
    return new PythonDataSeries(const_cast<RideItem*>(context->currentRideItem())->ride(), static_cast<RideFile::SeriesType>(type));
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

PythonDataSeries::PythonDataSeries(const RideFile *f, RideFile::SeriesType type)
{
    if (f) {
        count = f->dataPoints().count();
        data = new double[count];
        series = type;
        for(int i=0; i<count; i++) data[i] = f->dataPoints()[i]->value(type);
    }
}

// default constructor and copy constructor
PythonDataSeries::PythonDataSeries() : series(RideFile::none), count(0), data(NULL) {}
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
Bindings::activityMetrics() const
{
    PyObject* dict = PyDict_New();
    if (dict == NULL) return dict;

    Context *context = python->contexts.value(threadid());
    if (context == NULL || context->currentRideItem()==NULL) return dict;

    RideItem *item = const_cast<RideItem*>(context->currentRideItem());
    const RideMetricFactory &factory = RideMetricFactory::instance();

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
