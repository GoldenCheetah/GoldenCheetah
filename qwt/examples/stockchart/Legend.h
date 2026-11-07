/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#pragma once

#include <QwtAbstractLegend>

class LegendTreeView;
class QStandardItem;
class QModelIndex;
class QwtPlotItem;

class Legend : public QwtAbstractLegend
{
    Q_OBJECT

  public:
    explicit Legend( QWidget* parent = nullptr );
    ~Legend() override;

    void renderLegend( QPainter*,
        const QRectF&, bool fillBackground ) const override;

    bool isEmpty() const override;

    int scrollExtent( Qt::Orientation ) const override;

  Q_SIGNALS:
    void checked( QwtPlotItem* plotItem, bool on, int index );

  public Q_SLOTS:
    void updateLegend( const QVariant&,
        const QList< QwtLegendData >& ) override;

  private:
    void handleClick( const QModelIndex& );
    void updateItem( QStandardItem*, const QwtLegendData& );

    LegendTreeView* m_treeView;
};
