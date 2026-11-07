/*****************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#pragma once

#include <QFrame>

class QwtWheel;
class QwtSlider;
class TuningThermo;

class TunerBox : public QFrame
{
    Q_OBJECT

  public:
    TunerBox( QWidget* = nullptr );

  Q_SIGNALS:
    void fieldChanged( double f );

  public Q_SLOTS:
    void setFrequency( double frq );

  private:
    void adjustFrequency( double frq );

    QwtWheel* m_wheelFrequency;
    TuningThermo* m_thermoTune;
    QwtSlider* m_sliderFrequency;
};
