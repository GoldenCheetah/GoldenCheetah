/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2003   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <qglobal.h>
#if QT_VERSION >= 0x040000

#include <qstring.h>
#include <qpainter.h>
#include "qwt_mathml_text_engine.h"

#include <qtmmlwidget.h>

//! Constructor
QwtMathMLTextEngine::QwtMathMLTextEngine()
{
}

//! Destructor
QwtMathMLTextEngine::~QwtMathMLTextEngine()
{
}

/*!
   Find the height for a given width

   \param font Font of the text
   \param flags Bitwise OR of the flags used like in QPainter::drawText
   \param text Text to be rendered
   \param width Width

   \return Calculated height
*/
int QwtMathMLTextEngine::heightForWidth(const QFont& font, int flags,
        const QString& text, int) const
{
    return textSize(font, flags, text).height();
}

/*!
  Returns the size, that is needed to render text

  \param font Font of the text
  \param flags Bitwise OR of the flags used like in QPainter::drawText
  \param text Text to be rendered

  \return Caluclated size
*/
QSize QwtMathMLTextEngine::textSize(const QFont &font,
    int, const QString& text) const
{
    static QString t;
    static QSize sz;

    if ( text != t )
    {
        QtMmlDocument doc;
        doc.setContent(text);
        doc.setBaseFontPointSize(font.pointSize());

        sz = doc.size();
        t = text;
    }

    return sz;
}

/*!
  Return margins around the texts

  \param left Return 0
  \param right Return 0
  \param top Return 0
  \param bottom Return 0
*/
void QwtMathMLTextEngine::textMargins(const QFont &, const QString &,
    int &left, int &right, int &top, int &bottom) const
{
    left = right = top = bottom = 0;
}

/*!
   Draw the text in a clipping rectangle

   \param painter Painter
   \param rect Clipping rectangle 
   \param flags Bitwise OR of the flags like in for QPainter::drawText
   \param text Text to be rendered
*/ 
void QwtMathMLTextEngine::draw(QPainter *painter, const QRect &rect,
    int flags, const QString& text) const
{
    QtMmlDocument doc;
    doc.setContent(text);
    doc.setBaseFontPointSize(painter->font().pointSize());

    const QSize docSize = doc.size();

    QPoint pos = rect.topLeft();
    if ( rect.width() > docSize.width() )
    {
        if ( flags & Qt::AlignRight )
            pos.setX(rect.right() - docSize.width());
        if ( flags & Qt::AlignHCenter )
            pos.setX(rect.center().x() - docSize.width() / 2);
    }
    if ( rect.height() > docSize.height() )
    {
        if ( flags & Qt::AlignBottom )
            pos.setY(rect.bottom() - docSize.height());
        if ( flags & Qt::AlignVCenter )
            pos.setY(rect.center().y() - docSize.height() / 2);
    }

    doc.paint(painter, pos);
}

/*!
  Test if a string can be rendered by QwtMathMLTextEngine

  \param text Text to be tested
  \return true, if text begins with "<math>".
*/
bool QwtMathMLTextEngine::mightRender(const QString &text) const
{
    return text.trimmed().startsWith("<math");
}

#endif // QT_VERSION < 0x040000
