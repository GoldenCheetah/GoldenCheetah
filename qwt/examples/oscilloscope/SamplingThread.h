/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#pragma once

#include <QwtSamplingThread>

class SamplingThread : public QwtSamplingThread
{
    Q_OBJECT

  public:
    SamplingThread( QObject* parent = nullptr );

    double frequency() const;
    double amplitude() const;

  public Q_SLOTS:
    void setAmplitude( double );
    void setFrequency( double );

  protected:
    void sample( double elapsed ) override;

  private:
    virtual double value( double timeStamp ) const;

    double m_frequency;
    double m_amplitude;
};
