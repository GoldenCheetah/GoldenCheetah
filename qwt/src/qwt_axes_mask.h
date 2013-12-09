/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_AXES_MASK_H
#define QWT_AXES_MASK_H 1

#include "qwt_global.h"
#include "qwt_axis_id.h"

class QWT_EXPORT QwtAxesMask
{
public:
    QwtAxesMask();
    ~QwtAxesMask();

    void setEnabled( QwtAxisId, bool );
    bool isEnabled( QwtAxisId ) const;

private:
    class PrivateData;
    PrivateData *d_data;
};

#endif
