#include <PythonEmbed.h>
#include "Context.h"
#include "RideItem.h"
#include "Athlete.h"
#include "Bindings.h"
#include "GcUpgrade.h"
#include "PythonChart.h"
#include <QWebEngineView>
#include <QUrl>

#undef slots
#include <Python.h>

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

int
Bindings::webpage(QString url) const
{
    //fprintf(stderr, "URL=%s", url.toStdString().c_str());
    //python->chart->canvas->setUrl(QUrl(url));
    return 0;
}
