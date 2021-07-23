/******************************************************************************
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
#include "qwt_axis.h"
#include <qhash.h>

class QWT_EXPORT QwtAxisId
{
  public:
    QwtAxisId( int position, int index = 0 );

    bool operator==( const QwtAxisId& ) const;
    bool operator!=( const QwtAxisId& ) const;

    bool isXAxis() const;
    bool isYAxis() const;

  public:
    int pos;
    int id;
};

inline QwtAxisId::QwtAxisId( int position, int index )
    : pos( position )
    , id( index )
{
}

inline bool QwtAxisId::operator==( const QwtAxisId& other ) const
{
    return ( pos == other.pos ) && ( id == other.id );
}

inline bool QwtAxisId::operator!=( const QwtAxisId& other ) const
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

#if QT_VERSION >= 0x060000
inline size_t qHash( const QwtAxisId& axisId, size_t seed = 0 ) noexcept
#else
inline uint qHash( const QwtAxisId& axisId, uint seed = 0 ) noexcept
#endif
{
    return qHash( axisId.pos, seed ) ^ qHash( axisId.id, seed );
}

namespace QwtAxis
{
    // compatibility APIs
    inline bool isValid( QwtAxisId axisId )
        { return isValid( axisId.pos ) && ( axisId.id >= 0 ); }
    inline bool isXAxis( QwtAxisId axisId ) { return isXAxis( axisId.pos ); }
    inline bool isYAxis( QwtAxisId axisId ) { return isYAxis( axisId.pos ); }
}

#ifndef QT_NO_DEBUG_STREAM
QWT_EXPORT QDebug operator<<( QDebug, const QwtAxisId& );
#endif

#endif
