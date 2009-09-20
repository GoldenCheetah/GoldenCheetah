/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PLUGIN_H
#define QWT_PLUGIN_H

#include <qglobal.h>

#if QT_VERSION >= 0x040000

#ifdef __GNUC__
#error This code is Qt3 only
#endif

This code is Qt3 only

#endif

#include <qwidgetplugin.h>

class QT_WIDGET_PLUGIN_EXPORT QwtPlugin: public QWidgetPlugin
{
public:
    QwtPlugin();

    QStringList keys() const;
    QWidget* create( const QString &classname, QWidget* parent = 0, const char* name = 0 );
    QString group( const QString& ) const;
    QIconSet iconSet( const QString& ) const;
    QString includeFile( const QString& ) const;
    QString toolTip( const QString& ) const;
    QString whatsThis( const QString& ) const;
    bool isContainer( const QString& ) const;
};

#endif
