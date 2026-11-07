/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_LEGEND_LABEL_H
#define QWT_LEGEND_LABEL_H

#include "qwt_global.h"
#include "qwt_text_label.h"
#include "qwt_legend_data.h"

#include <memory>

class QwtText;

/*!
   \brief A widget representing something on a QwtLegend.
 */
class QWT_EXPORT QwtLegendLabel : public QwtTextLabel
{
    Q_OBJECT
  public:
    explicit QwtLegendLabel( QWidget* parent = 0 );
    ~QwtLegendLabel() override;

    void setData( const QwtLegendData& );
    const QwtLegendData& data() const;

    void setItemMode( QwtLegendData::Mode );
    QwtLegendData::Mode itemMode() const;

    void setSpacing( int spacing );
    int spacing() const;

    void setText( const QwtText& ) override;

    void setIcon( const QPixmap& );
    QPixmap icon() const;

    QSize sizeHint() const override;

    bool isChecked() const;

  public Q_SLOTS:
    void setChecked( bool on );

  Q_SIGNALS:
    //! Signal, when the legend item has been clicked
    void clicked();

    //! Signal, when the legend item has been pressed
    void pressed();

    //! Signal, when the legend item has been released
    void released();

    //! Signal, when the legend item has been toggled
    void checked( bool );

  protected:
    void setDown( bool );
    bool isDown() const;

    void paintEvent( QPaintEvent* ) override;
    void mousePressEvent( QMouseEvent* ) override;
    void mouseReleaseEvent( QMouseEvent* ) override;
    void keyPressEvent( QKeyEvent* ) override;
    void keyReleaseEvent( QKeyEvent* ) override;

  private:
    class PrivateData;
    std::unique_ptr< PrivateData > m_data;
};

#endif
