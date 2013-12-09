/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_AXIS_ID_H
#define QWT_AXIS_ID_H

#include "qwt_global.h"

#ifndef QT_NO_DEBUG_STREAM
#include <qdebug.h>
#endif

namespace QwtAxis
{
    //! \brief Axis position
    enum Position
    {
        //! Y axis left of the canvas
        yLeft,

        //! Y axis right of the canvas
        yRight,

        //! X axis below the canvas
        xBottom,

        //! X axis above the canvas
        xTop
    };

    //! \brief Number of axis positions
    enum { PosCount = xTop + 1 };

    bool isValid( int axisPos );
    bool isYAxis( int axisPos );
    bool isXAxis( int axisPos );
};

inline bool QwtAxis::isValid( int axisPos )
{
    return ( axisPos >= 0 && axisPos < PosCount );
}

inline bool QwtAxis::isXAxis( int axisPos )
{
    return ( axisPos == xBottom ) || ( axisPos == xTop );
}

inline bool QwtAxis::isYAxis( int axisPos )
{
    return ( axisPos == yLeft ) || ( axisPos == yRight );
}

class QWT_EXPORT QwtAxisId
{
public:
    QwtAxisId( int position, int index = 0 );

    bool operator==( const QwtAxisId & ) const;
    bool operator!=( const QwtAxisId & ) const;

    bool isXAxis() const;
    bool isYAxis() const;

public:
    int pos;
    int id;
};

inline QwtAxisId::QwtAxisId( int position, int index ):
    pos( position ),
    id( index )
{
}

inline bool QwtAxisId::operator==( const QwtAxisId &other ) const
{
    return ( pos == other.pos ) && ( id == other.id );
}

inline bool QwtAxisId::operator!=( const QwtAxisId &other ) const
{
    return !operator==( other );
}

inline bool QwtAxisId::isXAxis() const
{
    return QwtAxis::isXAxis( pos );
}

inline bool QwtAxisId::isYAxis() const
{
    return QwtAxis::isYAxis( pos );
}

#ifndef QT_NO_DEBUG_STREAM
QWT_EXPORT QDebug operator<<( QDebug, const QwtAxisId & );
#endif

#endif
