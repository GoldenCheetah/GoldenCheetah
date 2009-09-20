/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#ifndef QWT_LEGEND_ITEM_MANAGER_H
#define QWT_LEGEND_ITEM_MANAGER_H

#include "qwt_global.h"

class QwtLegend;
class QWidget;

/*!
  \brief Abstract API to bind plot items to the legend
*/

class QWT_EXPORT QwtLegendItemManager
{
public:
    //! Constructor
    QwtLegendItemManager() 
    {
    }

    //! Destructor
    virtual ~QwtLegendItemManager() 
    {
    }

    /*!
      Update the widget that represents the item on the legend
      \param legend Legend
      \sa legendItem()
     */
    virtual void updateLegend(QwtLegend *legend) const = 0;

    /*!
      Allocate the widget that represents the item on the legend
      \return Allocated widget
      \sa updateLegend() QwtLegend()
     */

    virtual QWidget *legendItem() const = 0;
};

#endif

