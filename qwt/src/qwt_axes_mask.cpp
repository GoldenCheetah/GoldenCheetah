/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_plot.h"
#include "qwt_axes_mask.h"
#include <qlist.h>
#include <qalgorithms.h>

class QwtAxesMask::PrivateData
{
public:
    QList<int> disabledAxes[ QwtAxis::PosCount ];
};

QwtAxesMask::QwtAxesMask()
{
    d_data = new PrivateData();
}

QwtAxesMask::~QwtAxesMask()
{
    delete d_data;
}

/*!
   \brief En/Disable an axis

   Only Axes that are enabled will be zoomed.
   All other axes will remain unchanged.

   \param axisId Axis id
   \param on On/Off

   \sa isAxisEnabled()
*/
void QwtAxesMask::setEnabled( QwtAxisId axisId, bool on )
{
    if ( !QwtAxis::isValid( axisId.pos ) )
        return;

    QList<int> &axes = d_data->disabledAxes[ axisId.pos ];
    
    QList<int>::iterator it = std::lower_bound( axes.begin(), axes.end(), axisId.id );

    const bool isEnabled = ( it != axes.end() ) && ( *it != axisId.id );

    if ( on )
    {
        if ( !isEnabled )
            axes.insert( it, axisId.id );
    }
    else
    {
        if ( isEnabled )
            axes.erase( it );
    }
}

/*!
   Test if an axis is enabled

   \param axisId Axis id
   \return True, if the axis is enabled

   \sa setAxisEnabled()
*/
bool QwtAxesMask::isEnabled( QwtAxisId axisId ) const
{
    if ( QwtAxis::isValid( axisId.pos ) )
    {
        const QList<int> &axes = d_data->disabledAxes[ axisId.pos ];
        return std::find( axes.begin(), axes.end(), axisId.id ) != axes.end();
    }

    return true;
}
