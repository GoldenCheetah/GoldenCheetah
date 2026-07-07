/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_SCALE_MAP_TABLE_H
#define QWT_SCALE_MAP_TABLE_H

#include "qwt_global.h"
#include "qwt_scale_map.h"
#include "qwt_plot.h"
#include "qwt_axis_id.h"
#include <qlist.h>

class QWT_EXPORT QwtScaleMapTable
{
  public:
    bool isValid( QwtAxisId ) const;
    const QwtScaleMap& map( QwtAxisId ) const;

    QList< QwtScaleMap > maps[ QwtAxis::AxisPositions ];
};

inline bool QwtScaleMapTable::isValid( QwtAxisId axisId ) const
{
    if ( axisId.pos >= 0 && axisId.pos < QwtAxis::AxisPositions && axisId.id >= 0 )
        return maps[ axisId.pos ].size() > axisId.id;

    return false;
}

inline const QwtScaleMap& QwtScaleMapTable::map( QwtAxisId axisId ) const
{
    return maps[ axisId.pos ].at( axisId.id );
}

#endif
