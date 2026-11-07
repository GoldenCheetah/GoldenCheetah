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
class QwtDial;

class DialBox : public QWidget
{
  public:
    DialBox( QWidget* parent, int type );

  private:
    QwtDial* createDial( int type ) const;
    void showValue( double v );

    QwtDial* m_dial;
    QLabel* m_label;
};
