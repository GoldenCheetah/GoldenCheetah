/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PLOT_ITEM_H
#define QWT_PLOT_ITEM_H

#include "qwt_global.h"
#include "qwt_legend_itemmanager.h"
#include "qwt_text.h"
#include "qwt_double_rect.h"

class QString;
class QRect;
class QPainter;
class QWidget;
class QwtPlot;
class QwtLegend;
class QwtScaleMap;
class QwtScaleDiv;

/*!
  \brief Base class for items on the plot canvas

  A plot item is "something", that can be painted on the plot canvas,
  or only affects the scales of the plot widget. They can be categorized as:

  - Representator\n
    A "Representator" is an item that represents some sort of data
    on the plot canvas. The different representator classes are organized
    according to the characteristics of the data:
    - QwtPlotMarker
      Represents a point or a horizontal/vertical coordinate
    - QwtPlotCurve
      Represents a series of points
    - QwtPlotSpectrogram ( QwtPlotRasterItem )
      Represents raster data
    - ...

  - Decorators\n
    A "Decorator" is an item, that displays additional information, that
    is not related to any data:
    - QwtPlotGrid
    - QwtPlotScaleItem
    - QwtPlotSvgItem
    - ...

  Depending on the QwtPlotItem::ItemAttribute flags, an item is included
  into autoscaling or has an entry on the legnd.

  Before misusing the existing item classes it might be better to
  implement a new type of plot item 
  ( don't implement a watermark as spectrogram ).
  Deriving a new type of QwtPlotItem primarily means to implement 
  the YourPlotItem::draw() method. 

  \sa The cpuplot example shows the implementation of additional plot items.
*/

class QWT_EXPORT QwtPlotItem: public QwtLegendItemManager
{
public:
    /*!
        \brief Runtime type information
  
        RttiValues is used to cast plot items, without
        having to enable runtime type information of the compiler.
     */
    enum RttiValues
    { 
        Rtti_PlotItem = 0,

        Rtti_PlotGrid,
        Rtti_PlotScale,
        Rtti_PlotMarker,
        Rtti_PlotCurve,
        Rtti_PlotHistogram,
        Rtti_PlotSpectrogram,
        Rtti_PlotSVG,

        Rtti_PlotUserItem = 1000
    };

    /*!
       Plot Item Attributes

       - Legend\n
         The item is represented on the legend.
       - AutoScale \n
         The boundingRect() of the item is included in the 
         autoscaling calculation.

       \sa setItemAttribute(), testItemAttribute()
     */
    enum ItemAttribute
    {
        Legend = 1,
        AutoScale = 2
    };

#if QT_VERSION >= 0x040000
    //! Render hints
    enum RenderHint
    {
        RenderAntialiased = 1
    };
#endif

    explicit QwtPlotItem(const QwtText &title = QwtText());
    virtual ~QwtPlotItem();

    void attach(QwtPlot *plot);

    /*!
       \brief This method detaches a QwtPlotItem from any QwtPlot it has been
              associated with.

       detach() is equivalent to calling attach( NULL )
       \sa attach( QwtPlot* plot )
    */
    void detach() { attach(NULL); }

    QwtPlot *plot() const;
    
    void setTitle(const QString &title);
    void setTitle(const QwtText &title);
    const QwtText &title() const;

    virtual int rtti() const;

    void setItemAttribute(ItemAttribute, bool on = true);
    bool testItemAttribute(ItemAttribute) const;

#if QT_VERSION >= 0x040000
    void setRenderHint(RenderHint, bool on = true);
    bool testRenderHint(RenderHint) const;
#endif

    double z() const; 
    void setZ(double z);

    void show();
    void hide();
    virtual void setVisible(bool);
    bool isVisible () const;

    void setAxis(int xAxis, int yAxis);

    void setXAxis(int axis);
    int xAxis() const;

    void setYAxis(int axis);
    int yAxis() const;

    virtual void itemChanged();

    /*!
      \brief Draw the item

      \param painter Painter
      \param xMap Maps x-values into pixel coordinates.
      \param yMap Maps y-values into pixel coordinates.
      \param canvasRect Contents rect of the canvas in painter coordinates
    */
    virtual void draw(QPainter *painter, 
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRect &canvasRect) const = 0;

    virtual QwtDoubleRect boundingRect() const;

    virtual void updateLegend(QwtLegend *) const;
    virtual void updateScaleDiv(const QwtScaleDiv&,
        const QwtScaleDiv&);

    virtual QWidget *legendItem() const;

    QwtDoubleRect scaleRect(const QwtScaleMap &, const QwtScaleMap &) const;
    QRect paintRect(const QwtScaleMap &, const QwtScaleMap &) const;
    
    QRect transform(const QwtScaleMap &, const QwtScaleMap &, 
        const QwtDoubleRect&) const; 
    QwtDoubleRect invTransform(const QwtScaleMap &, const QwtScaleMap &,
        const QRect&) const; 

private:
    // Disabled copy constructor and operator=
    QwtPlotItem( const QwtPlotItem & );
    QwtPlotItem &operator=( const QwtPlotItem & );

    class PrivateData;
    PrivateData *d_data;
};
            
#endif
