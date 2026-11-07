/*****************************************************************************
 * Qwt Examples
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#pragma once

#include <QWidget>

class QLabel;
class QwtThermo;
class QwtWheel;

class WheelBox : public QWidget
{
  public:
    WheelBox( Qt::Orientation,
        int type, QWidget* parent = nullptr );

  private:
    void showValue( double );
    QWidget* createBox( Qt::Orientation, int type );

    QwtWheel* m_wheel;
    QwtThermo* m_thermo;
    QLabel* m_label;
};
