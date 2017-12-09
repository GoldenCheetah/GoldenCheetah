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

%Module goldencheetah


//
// QString type conversion
//
%MappedType QString /AllowNone,TypeHint="str",TypeHintValue="''"/
{
%TypeHeaderCode
#include <qstring.h>
%End

%ConvertToTypeCode
    //*sipCppPtr = new QString(qpycore_PyObject_AsQString(sipPy));
    //return sipGetState(sipTransferObj);
    Q_UNUSED(sipTransferObj);
    return 0;
%End
%ConvertFromTypeCode
    return PyUnicode_FromString(sipCpp->toLatin1().data());
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
        sipRes = new QString(RideFile::seriesName(sipCpp->series));
        %End
    SIP_SSIZE_T __len__();
        %MethodCode
        sipRes = sipCpp->count;
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
    QString athlete() const;
    int build() const;
    QString version() const;

    // working with series
    bool seriesPresent(int type=10) const;
    QString seriesName(int type=10) const;
    int seriesLast() const;
    PythonDataSeries series(int type=10) /TransferBack/;
};

