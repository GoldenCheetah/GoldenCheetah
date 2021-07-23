/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_axis_id.h"

#ifndef QT_NO_DEBUG_STREAM

#include <qdebug.h>

QDebug operator<<( QDebug debug, const QwtAxisId& axisId )
{
    static const char* posNames[] = { "YLeft", "yRight", "XBottom", "xTop" };

    debug.nospace();

    debug << "QwtAxisId(";

    if ( axisId.pos >= 0 && axisId.pos < 4 )
        debug << posNames[axisId.pos];
    else
        debug << axisId.pos;

    debug << "," << axisId.id << ")";

    return debug.space();
}

#endif
