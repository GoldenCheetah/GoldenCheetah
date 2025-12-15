/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#pragma once

#include <QtGlobal>

class QwtOHLCSample;
class QString;

namespace QuoteFactory
{
    enum Stock
    {
        BMW,
        Daimler,
        Porsche,

        NumStocks
    };

    QVector< QwtOHLCSample > samples2010( Stock );
    QString title( Stock );
}
