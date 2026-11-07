/*****************************************************************************
 * Qwt Examples
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#pragma once

#include <QwtPlot>
#include <QElapsedTimer>

class Plot : public QwtPlot
{
  public:
    Plot( QWidget* = nullptr );

  protected:
    void timerEvent( QTimerEvent* ) override;

  private:
    void updateCurves();

    QElapsedTimer m_timer;
};
