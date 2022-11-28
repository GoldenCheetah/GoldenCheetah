/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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

%Module(name=goldencheetah, keyword_arguments="All")


//
// QString type conversion
//
%MappedType QString /AllowNone,TypeHint="str",TypeHintValue="''"/
{
%TypeHeaderCode
#include <qstring.h>
%End

%ConvertToTypeCode
    if (sipIsErr == NULL) return PyUnicode_Check(sipPy);
    if (sipPy == Py_None) {
        *sipCppPtr = new QString();
        return 1;
    }
    if (PyUnicode_Check(sipPy)) {
        Py_ssize_t size;
        wchar_t *string = PyUnicode_AsWideCharString(sipPy, &size);
        if (string) {
            if (size) *sipCppPtr = new QString(QString::fromWCharArray(string));
            else *sipCppPtr = new QString();
        }
        return 1;
    }
    return 0;
%End
%ConvertFromTypeCode
    return PyUnicode_FromString(sipCpp->toUtf8().data());
%End
};

//
// Return a DataSeries using the Buffer Protocol
//
class PythonDataSeries {

%TypeHeaderCode
#include "Bindings.h"
%End

%BIGetBufferCode
    sipBuffer->obj = sipSelf;
    sipBuffer->buf = (void*)sipCpp->data;
    sipBuffer->len = sipCpp->count * sizeof(double);
    sipBuffer->readonly = 0;
    sipBuffer->itemsize = sizeof(double);
    sipBuffer->format = (char*)"d";  // double
    sipBuffer->ndim = 1;
    sipBuffer->shape = &sipCpp->count;  // length-1 sequence of dimensions
    sipBuffer->strides = &sipBuffer->itemsize;  // for the simple case we can do this
    sipBuffer->suboffsets = NULL;
    sipBuffer->internal = NULL;

    Py_INCREF(sipSelf);  // need to increase the reference count
    sipRes = 0;
%End

%BIReleaseBufferCode
    // we do not require any special release function
%End

public:
    QString __str__();
        %MethodCode
        sipRes = new QString(sipCpp->name);
        %End
    SIP_SSIZE_T __len__();
        %MethodCode
        sipRes = sipCpp->count;
        %End
    double __getitem__(long);
        %MethodCode
        if (a0 < 0) a0 += sipCpp->count;
        if (a0 >= 0 && a0 < sipCpp->count) {
            sipRes = sipCpp->data[a0];
        } else {
            PyErr_SetString(PyExc_IndexError, "Index out of range");
            sipError = sipErrorFail;
        }
        %End
    void __setitem__(long, double);
        %MethodCode
        if (sipCpp->readOnly) {
            PyErr_SetString(PyExc_AttributeError, "Object is read-only");
            sipError = sipErrorFail;
        } else {
            if (a0 < 0) a0 += sipCpp->count;
            if (a0 >= 0 && a0 < sipCpp->count) {
                sipCpp->data[a0] = a1;
                RideFile *rideFile = sipCpp->rideFile;
                if (rideFile) {
                    RideFile::SeriesType seriesType = static_cast<RideFile::SeriesType>(sipCpp->seriesType);
                    rideFile->command->setPointValue(a0, seriesType, a1);
                    if (!rideFile->isDataPresent(seriesType)) rideFile->command->setDataPresent(seriesType, true);
                }
            } else {
                PyErr_SetString(PyExc_IndexError, "Index out of range");
                sipError = sipErrorFail;
            }
        }
        %End
};

//
// QStringList Conversion
//

%MappedType QStringList
        /TypeHintIn="Iterable[QString]", TypeHintOut="List[QString]",
        TypeHintValue="[]"/
{
%TypeHeaderCode
#include <qstringlist.h>
%End

%ConvertFromTypeCode
    PyObject *l = PyList_New(sipCpp->size());

    if (!l)
        return 0;

    for (int i = 0; i < sipCpp->size(); ++i)
    {
        QString *t = new QString(sipCpp->at(i));
        PyObject *tobj = sipConvertFromNewType(t, sipType_QString,
                sipTransferObj);

        if (!tobj)
        {
            delete t;
            Py_DECREF(l);

            return 0;
        }

        PyList_SetItem(l, i, tobj);
    }

    return l;
%End

%ConvertToTypeCode
    PyObject *iter = PyObject_GetIter(sipPy);

    if (!sipIsErr)
    {
        PyErr_Clear();
        Py_XDECREF(iter);

        return (iter
#if PY_MAJOR_VERSION < 3
                && !PyString_Check(sipPy)
#endif
                && !PyUnicode_Check(sipPy));
    }

    if (!iter)
    {
        *sipIsErr = 1;

        return 0;
    }

    QStringList *ql = new QStringList;
 
    for (Py_ssize_t i = 0; ; ++i)
    {
        PyErr_Clear();
        PyObject *itm = PyIter_Next(iter);

        if (!itm)
        {
            if (PyErr_Occurred())
            {
                delete ql;
                Py_DECREF(iter);
                *sipIsErr = 1;

                return 0;
            }

            break;
        }

        int state;
        QString *t = reinterpret_cast<QString *>(
                sipForceConvertToType(itm, sipType_QString, sipTransferObj,
                        SIP_NOT_NONE, &state, sipIsErr));

        if (*sipIsErr)
        {
            PyErr_Format(PyExc_TypeError,
                    "index %zd has type '%s' but 'str' is expected", i,
                    sipPyTypeName(Py_TYPE(itm)));

            Py_DECREF(itm);
            delete ql;
            Py_DECREF(iter);

            return 0;
        }

        ql->append(*t);

        sipReleaseType(t, sipType_QString, state);
        Py_DECREF(itm);
    }
 
    Py_DECREF(iter);

    *sipCppPtr = ql;
 
    return sipGetState(sipTransferObj);
%End
};

//
// Return an XDataSeries using the Buffer Protocol
//
class PythonXDataSeries {

%TypeHeaderCode
#include "Bindings.h"
%End

%BIGetBufferCode
    sipBuffer->obj = sipSelf;
    sipBuffer->buf = sipCpp->rawDataPtr();
    sipBuffer->len = sipCpp->count() * sizeof(double);
    sipBuffer->readonly = 0;
    sipBuffer->itemsize = sizeof(double);
    sipBuffer->format = (char*)"d";  // double
    sipBuffer->ndim = 1;
    sipBuffer->shape = sipCpp->shape.data();  // length-1 sequence of dimensions
    sipBuffer->strides = &sipBuffer->itemsize;  // for the simple case we can do this
    sipBuffer->suboffsets = NULL;
    sipBuffer->internal = NULL;

    Py_INCREF(sipSelf);  // need to increase the reference count
    sipRes = 0;
%End

%BIReleaseBufferCode
    // we do not require any special release function
%End

public:
    QString __str__();
        %MethodCode
        sipRes = new QString(sipCpp->name());
        %End
    SIP_SSIZE_T __len__();
        %MethodCode
        sipRes = sipCpp->count();
        %End
    double __getitem__(long);
        %MethodCode
        if (a0 < 0) a0 += sipCpp->count();
        if (a0 >= 0 && a0 < sipCpp->count()) {
            sipRes = sipCpp->get(a0);
        } else {
            PyErr_SetString(PyExc_IndexError, "Index out of range");
            sipError = sipErrorFail;
        }
        %End
    void __setitem__(long, double);
        %MethodCode
        if (sipCpp->readOnly) {
            PyErr_SetString(PyExc_AttributeError, "Object is read-only");
            sipError = sipErrorFail;
        } else {
            if (a0 < 0) a0 += sipCpp->count();
            if (a0 >= 0 && a0 < sipCpp->count()) {
                if (!sipCpp->set(a0, a1)) {
                    PyErr_SetString(PyExc_RuntimeError, "XData series does not exist");
                }
            } else {
                PyErr_SetString(PyExc_IndexError, "Index out of range");
                sipError = sipErrorFail;
            }
        }
        %End
    void append(double);
        %MethodCode
        if (sipCpp->readOnly) {
            PyErr_SetString(PyExc_AttributeError, "Object is read-only");
            sipError = sipErrorFail;
        } else {
            if (!sipCpp->add(a0)) {
                PyErr_SetString(PyExc_RuntimeError, "XData series does not exist");
            }
        }
        %End
    void remove(long);
        %MethodCode
        if (sipCpp->readOnly) {
            PyErr_SetString(PyExc_AttributeError, "Object is read-only");
            sipError = sipErrorFail;
        } else {
            if (!sipCpp->remove(a0)) {
                PyErr_SetString(PyExc_RuntimeError, "XData series does not exist");
            }
        }
        %End
};

//
// The public bindings
//
class Bindings {

%TypeHeaderCode
//#include "Bindings.h"
%End

public:
    long threadid() const;
    int build() const;
    QString version() const;

    // working with the web view
    int webpage(QString url) const;

    // sending results back (typically when working in user metric code)
    void result(double value);

    // working with athlete data
    PyObject* athlete() /TransferBack/;
    PyObject* athleteZones(PyObject* date=NULL, QString sport="") /TransferBack/;

    // working with activities
    PyObject* activities(QString filter=QString()) /TransferBack/;

    // working with seasons
    PyObject* season(bool all=false, bool compare=false) /TransferBack/;

    // working with series
    bool seriesPresent(int type=10, PyObject* activity=NULL) const;
    QString seriesName(int type=10) const;
    int seriesLast() const;
    PythonDataSeries series(int type=10, PyObject* activity=NULL) /TransferBack/;
    PythonDataSeries activityWbal(PyObject* activity=NULL) /TransferBack/;
    PythonDataSeries xdata(QString name, QString series, QString join="repeat", PyObject* activity=NULL) /TransferBack/;
    PythonXDataSeries xdataSeries(QString name, QString series, PyObject* activity=NULL) /TransferBack/;
    PyObject* xdataNames(QString name=QString(), PyObject* activity=NULL) /TransferBack/;

    // working with metrics
    PyObject* activityMetrics(bool compare=false) /TransferBack/;
    PyObject* seasonMetrics(bool all=false, QString filter=QString(), bool compare=false) /TransferBack/;
    PythonDataSeries *metrics(QString metric, bool all=false, QString filter=QString()) /TransferBack/;
    PyObject* seasonPmc(bool all=false, QString metric=QString("BikeStress")) /TransferBack/;
    PyObject* seasonMeasures(bool all=false, QString group=QString("Body")) /TransferBack/;

    // working with meanmax data
    PyObject* activityMeanmax(bool compare=false) /TransferBack/;
    PyObject* seasonMeanmax(bool all=false, QString filter=QString(), bool compare=false) /TransferBack/;
    PyObject* seasonPeaks(QString series, int duration, bool all=false, QString filter=QString(), bool compare=false) /TransferBack/;

    // working with intervals
    QString intervalType(int type=1) const;
    PyObject* seasonIntervals(QString type=QString(), bool compare=false) /TransferBack/;
    PyObject* activityIntervals(QString type=QString(), PyObject* activity=NULL) /TransferBack/;

    // editing data
    bool createXDataSeries(QString name, QString series, QString seriesUnit, PyObject *activity = NULL) const;
    bool deleteActivitySample(int index = -1, PyObject *activity = NULL) const;
    bool deleteSeries(int type, PyObject *activity = NULL) const;
    bool postProcess(QString processor, PyObject *activity = NULL) const;
    bool setTag(QString name, QString value, PyObject *activity = NULL) const;
    bool delTag(QString name, PyObject *activity = NULL) const;
    bool hasTag(QString name, PyObject *activity = NULL) const;

    // working with qt charts
    bool configChart(QString title, int type, bool animate, int legpos, bool stack, int orientation) const;
    bool setCurve(QString name, PyObject *xseries, PyObject *yseries, QStringList fseries, QString xname, QString yname,
                      QStringList labels, QStringList colors,
                      int line, int symbol, int symbolsize, QString color, int opacity, bool opengl, bool legend, bool datalabels, bool fill) const;
    bool configAxis(QString name, bool visible, int align, double min, double max,
                      int type, QString labelcolor, QString color, bool log, QStringList categories);

    bool addAnnotation(QString type, QString s1, QString s2, double value);
};

