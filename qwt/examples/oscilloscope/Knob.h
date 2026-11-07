/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#pragma once

#include <QwtGlobal>
#include <QWidget>

class QwtKnob;
class QLabel;

class Knob : public QWidget
{
    Q_OBJECT

    Q_PROPERTY( QColor theme READ theme WRITE setTheme )

  public:
    Knob( const QString& title,
        double min, double max, QWidget* parent = nullptr );

    QSize sizeHint() const override;

    void setValue( double value );
    double value() const;

    void setTheme( const QColor& );
    QColor theme() const;

  Q_SIGNALS:
    double valueChanged( double );

  protected:
    void resizeEvent( QResizeEvent* ) override;

  private:
    QwtKnob* m_knob;
    QLabel* m_label;
};
