/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning ( disable : 4786 )
#endif

#include <qglobal.h>
#include <qvaluelist.h>
#include <qmime.h>
#include <qdragobject.h>

#include "qwtplugin.h"
#include "qwt_text_label.h"

#ifndef NO_QWT_PLOT
#include "qwt_plot.h"
#include "qwt_scale_widget.h"
#endif

#ifndef NO_QWT_WIDGETS
#include "qwt_counter.h"
#include "qwt_wheel.h"
#include "qwt_thermo.h"
#include "qwt_knob.h"
#include "qwt_slider.h"
#include "qwt_analog_clock.h"
#include "qwt_compass.h"
#endif

namespace
{
    struct Entry
    {
        Entry() {}
        Entry( QString _classname, QString _header, QString  _pixmap,
                QString _tooltip, QString _whatshis):       
                classname(_classname),
                header(_header),
                pixmap(_pixmap),
                tooltip(_tooltip),
                whatshis(_whatshis)
        {}

        QString classname;
        QString header;
        QString pixmap;
        QString tooltip;
        QString whatshis;
    };

    QValueList<Entry> vec;

    const Entry *entry(const QString& str)
    {
        for ( uint i = 0; i < vec.count(); i++ )
        {
            if (str == vec[i].classname)
                return &vec[i];
        }
        return NULL;
    }
}

QwtPlugin::QwtPlugin()
{
#ifndef NO_QWT_PLOT
    vec.append(Entry("QwtPlot", "qwt_plot.h",
        "qwtplot.png", "QwtPlot", "whatsthis"));
    vec.append(Entry("QwtScaleWidget", "qwt_scale_widget.h",
        "qwtscale.png", "QwtScaleWidget", "whatsthis"));
#endif

#ifndef NO_QWT_WIDGETS
    vec.append(Entry("QwtAnalogClock", "qwt_analog_clock.h", 
        "qwtanalogclock.png", "QwtAnalogClock", "whatsthis"));
    vec.append(Entry("QwtCompass", "qwt_compass.h",
        "qwtcompass.png", "QwtCompass", "whatsthis"));
    vec.append(Entry("QwtCounter", "qwt_counter.h", 
        "qwtcounter.png", "QwtCounter", "whatsthis"));
    vec.append(Entry("QwtDial", "qwt_dial.h", 
        "qwtdial.png", "QwtDial", "whatsthis"));
    vec.append(Entry("QwtKnob", "qwt_knob.h",
        "qwtknob.png", "QwtKnob", "whatsthis"));
    vec.append(Entry("QwtSlider", "qwt_slider.h",
        "qwtslider.png", "QwtSlider", "whatsthis"));
    vec.append(Entry("QwtThermo", "qwt_thermo.h",
        "qwtthermo.png", "QwtThermo", "whatsthis"));
    vec.append(Entry("QwtWheel", "qwt_wheel.h",
        "qwtwheel.png", "QwtWheel", "whatsthis"));
#endif

    vec.append(Entry("QwtTextLabel", "qwt_text_label.h", 
        "qwtwidget.png", "QwtTextLabel", "whatsthis"));

}

QWidget* QwtPlugin::create(const QString &key, 
    QWidget* parent, const char* name)
{
    QWidget *w = NULL;

#ifndef NO_QWT_PLOT
    if ( key == "QwtPlot" )
        w = new QwtPlot( parent );
    else if ( key == "QwtScaleWidget" )
        w = new QwtScaleWidget( QwtScaleDraw::LeftScale, parent);
#endif

#ifndef NO_QWT_WIDGETS
    if ( key == "QwtAnalogClock" )
        w = new QwtAnalogClock( parent);
    else if ( key == "QwtCounter" )
        w = new QwtCounter( parent);
    else if ( key == "QwtCompass" )
        w = new QwtCompass( parent);
    else if ( key == "QwtDial" )
        w = new QwtDial( parent);
    else if ( key == "QwtWheel" )
        w = new QwtWheel( parent);
    else if ( key == "QwtThermo" )
        w = new QwtThermo( parent);
    else if ( key == "QwtKnob" )
        w = new QwtKnob( parent);
    else if ( key == "QwtSlider" )
        w = new QwtSlider( parent);
#endif

    if ( key == "QwtTextLabel" )
        w = new QwtTextLabel( parent);

    if ( w )
        w->setName(name);

    return w;
}

QStringList QwtPlugin::keys() const
{
    QStringList list;
    
    for (unsigned i = 0; i < vec.count(); i++)
        list += vec[i].classname;

    return list;
}

QString QwtPlugin::group( const QString& feature ) const
{
    if (entry(feature) != NULL )
        return QString("Qwt"); 
    return QString::null;
}

QIconSet QwtPlugin::iconSet( const QString& pmap) const
{
    QString pixmapKey("qwtwidget.png");
    if (entry(pmap) != NULL )
        pixmapKey = entry(pmap)->pixmap;

    const QMimeSource *ms =
        QMimeSourceFactory::defaultFactory()->data(pixmapKey);

    QPixmap pixmap;
    QImageDrag::decode(ms, pixmap);

    return QIconSet(pixmap);
}

QString QwtPlugin::includeFile( const QString& feature ) const
{
    if (entry(feature) != NULL)
        return entry(feature)->header;        
    return QString::null;
}

QString QwtPlugin::toolTip( const QString& feature ) const
{
    if (entry(feature) != NULL )
        return entry(feature)->tooltip;       
    return QString::null;
}

QString QwtPlugin::whatsThis( const QString& feature ) const
{
    if (entry(feature) != NULL)
        return entry(feature)->whatshis;      
    return QString::null;
}

bool QwtPlugin::isContainer( const QString& ) const
{
    return false;
}

Q_EXPORT_PLUGIN( QwtPlugin )
