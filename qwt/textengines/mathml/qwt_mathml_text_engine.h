/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2003   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#ifndef QWT_MATHML_TEXT_ENGINE_H
#define QWT_MATHML_TEXT_ENGINE_H 1

#if QT_VERSION >= 0x040000

#include "qwt_text_engine.h"

/*!
  \brief Text Engine for the MathML renderer of the Qt solutions package.

  The Qt Solution package includes a renderer for MathML 
  http://www.trolltech.com/products/qt/addon/solutions/catalog/4/Widgets/qtmmlwidget 
  that is available for owners of a commercial Qt license.
  You need a version >= 2.1, that is only available for Qt4.

  To enable MathML support the following code needs to be added to the
  application:
  \verbatim
#include <qwt_mathml_text_engine.h>

QwtText::setTextEngine(QwtText::MathMLText, new QwtMathMLTextEngine());
  \endverbatim

  \sa QwtTextEngine, QwtText::setTextEngine
  \warning Unfortunately the MathML renderer doesn't support rotating of texts. 
*/

class QWT_EXPORT QwtMathMLTextEngine: public QwtTextEngine
{
public:
    QwtMathMLTextEngine();
    virtual ~QwtMathMLTextEngine();

    virtual int heightForWidth(const QFont &font, int flags, 
        const QString &text, int width) const;

    virtual QSize textSize(const QFont &font, int flags,
        const QString &text) const;

    virtual void draw(QPainter *painter, const QRect &rect,
        int flags, const QString &text) const;

    virtual bool mightRender(const QString &) const;

    virtual void textMargins(const QFont &, const QString &,
        int &left, int &right, int &top, int &bottom) const;
};

#endif // QT_VERSION >= 0x040000

#endif
