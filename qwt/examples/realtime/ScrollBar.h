/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#pragma once

#include <QScrollBar>

class ScrollBar : public QScrollBar
{
    Q_OBJECT

  public:
    ScrollBar( QWidget* parent = nullptr );
    ScrollBar( Qt::Orientation, QWidget* parent = nullptr );
    ScrollBar( double minBase, double maxBase,
        Qt::Orientation o, QWidget* parent = nullptr );

    void setInverted( bool );
    bool isInverted() const;

    double minBaseValue() const;
    double maxBaseValue() const;

    double minSliderValue() const;
    double maxSliderValue() const;

    int extent() const;

  Q_SIGNALS:
    void sliderMoved( Qt::Orientation, double, double );
    void valueChanged( Qt::Orientation, double, double );

  public Q_SLOTS:
    virtual void setBase( double min, double max );
    virtual void moveSlider( double min, double max );

  protected:
    void sliderRange( int value, double& min, double& max ) const;
    int mapToTick( double ) const;
    double mapFromTick( int ) const;

  private:
    void init();

    void catchValueChanged( int value );
    void catchSliderMoved( int value );

    bool m_inverted;

    double m_minBase = 0.0;
    double m_maxBase = 1.0;
    int m_baseTicks = 1000000;
};
