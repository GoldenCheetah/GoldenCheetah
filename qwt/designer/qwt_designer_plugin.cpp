/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qglobal.h>
#include <QtPlugin>

#include <QtUiPlugin/QDesignerCustomWidgetInterface>

#ifndef NO_QWT_PLOT
#include "qwt_plot.h"
#include "qwt_scale_widget.h"
#endif

#ifndef NO_QWT_WIDGETS
#include "qwt_counter.h"
#include "qwt_wheel.h"
#include "qwt_thermo.h"
#include "qwt_knob.h"
#include "qwt_slider.h"
#include "qwt_dial.h"
#include "qwt_dial_needle.h"
#include "qwt_analog_clock.h"
#include "qwt_compass.h"
#endif

#ifndef NO_QWT_POLAR
#include "qwt_polar_plot.h"
#endif

#include "qwt_text_label.h"

namespace
{
    class CustomWidgetInterface : public QDesignerCustomWidgetInterface
    {
      public:
        QString group() const override { return "Qwt Widgets"; }
        bool isContainer() const override { return false; }
        bool isInitialized() const override { return true; }

        QIcon icon() const override { return m_icon; }
        QString codeTemplate() const override { return m_codeTemplate; }
        QString domXml() const override { return m_domXml; }
        QString includeFile() const override { return m_include; }
        QString name() const override { return m_name; }
        QString toolTip() const override { return m_toolTip; }
        QString whatsThis() const override { return m_whatsThis; }

      protected:
        QString m_name;
        QString m_include;
        QString m_toolTip;
        QString m_whatsThis;
        QString m_domXml;
        QString m_codeTemplate;
        QIcon m_icon;
    };
}

#ifndef NO_QWT_PLOT

namespace
{
    class PlotInterface : public CustomWidgetInterface
    {
      public:
        PlotInterface()
        {
            m_name = "QwtPlot";
            m_include = "qwt_plot.h";
            m_icon = QPixmap( ":/pixmaps/qwtplot.png" );
            m_domXml =
                "<widget class=\"QwtPlot\" name=\"qwtPlot\">\n"
                " <property name=\"geometry\">\n"
                "  <rect>\n"
                "   <x>0</x>\n"
                "   <y>0</y>\n"
                "   <width>400</width>\n"
                "   <height>200</height>\n"
                "  </rect>\n"
                " </property>\n"
                "</widget>\n";
        }

        QWidget* createWidget( QWidget* parent ) override
        {
            return new QwtPlot( parent );
        }
    };
}

#endif

#ifndef NO_QWT_POLAR

namespace
{
    class PolarPlotInterface : public CustomWidgetInterface
    {
      public:
        PolarPlotInterface()
        {
            m_name = "QwtPolarPlot";
            m_include = "qwt_polar_plot.h";
            m_icon = QPixmap( ":/pixmaps/qwt_polar_plot.png" );
            m_domXml =
                "<widget class=\"QwtPolarPlot\" name=\"qwtPolarPlot\">\n"
                " <property name=\"geometry\">\n"
                "  <rect>\n"
                "   <x>0</x>\n"
                "   <y>0</y>\n"
                "   <width>400</width>\n"
                "   <height>400</height>\n"
                "  </rect>\n"
                " </property>\n"
                "</widget>\n";
        }

        QWidget* createWidget( QWidget* parent ) override
        {
            return new QwtPolarPlot( parent );
        }
    };
}

#endif

#ifndef NO_QWT_WIDGETS

namespace
{
    class AnalogClockInterface : public CustomWidgetInterface
    {
      public:
        AnalogClockInterface()
        {
            m_name = "QwtAnalogClock";
            m_include = "qwt_analog_clock.h";
            m_icon = QPixmap( ":/pixmaps/qwtanalogclock.png" );
            m_domXml =
                "<widget class=\"QwtAnalogClock\" name=\"AnalogClock\">\n"
                " <property name=\"geometry\">\n"
                "  <rect>\n"
                "   <x>0</x>\n"
                "   <y>0</y>\n"
                "   <width>200</width>\n"
                "   <height>200</height>\n"
                "  </rect>\n"
                " </property>\n"
                " <property name=\"lineWidth\">\n"
                "  <number>4</number>\n"
                " </property>\n"
                "</widget>\n";
        }

        QWidget* createWidget( QWidget* parent ) override
        {
            return new QwtAnalogClock( parent );
        }
    };
}

#endif

#ifndef NO_QWT_WIDGETS

namespace
{
    class CompassInterface : public CustomWidgetInterface
    {
      public:
        CompassInterface()
        {
            m_name = "QwtCompass";
            m_include = "qwt_compass.h";
            m_icon = QPixmap( ":/pixmaps/qwtcompass.png" );
            m_domXml =
                "<widget class=\"QwtCompass\" name=\"Compass\">\n"
                " <property name=\"geometry\">\n"
                "  <rect>\n"
                "   <x>0</x>\n"
                "   <y>0</y>\n"
                "   <width>200</width>\n"
                "   <height>200</height>\n"
                "  </rect>\n"
                " </property>\n"
                " <property name=\"lineWidth\">\n"
                "  <number>4</number>\n"
                " </property>\n"
                "</widget>\n";
        }

        QWidget* createWidget( QWidget* parent ) override
        {
            QwtCompass* compass = new QwtCompass( parent );

            compass->setNeedle( new QwtCompassMagnetNeedle(
                QwtCompassMagnetNeedle::TriangleStyle,
                compass->palette().color( QPalette::Mid ),
                compass->palette().color( QPalette::Dark ) ) );

            return compass;
        }
    };
}

#endif

#ifndef NO_QWT_WIDGETS

namespace
{
    class CounterInterface : public CustomWidgetInterface
    {
      public:
        CounterInterface()
        {
            m_name = "QwtCounter";
            m_include = "qwt_counter.h";
            m_icon = QPixmap( ":/pixmaps/qwtcounter.png" );
            m_domXml =
                "<widget class=\"QwtCounter\" name=\"Counter\">\n"
                "</widget>\n";
        }

        QWidget* createWidget( QWidget* parent ) override
        {
            return new QwtCounter( parent );
        }
    };
}

#endif

#ifndef NO_QWT_WIDGETS

namespace
{
    class DialInterface : public CustomWidgetInterface
    {
      public:
        DialInterface()
        {
            m_name = "QwtDial";
            m_include = "qwt_dial.h";
            m_icon = QPixmap( ":/pixmaps/qwtdial.png" );
            m_domXml =
                "<widget class=\"QwtDial\" name=\"Dial\">\n"
                " <property name=\"geometry\">\n"
                "  <rect>\n"
                "   <x>0</x>\n"
                "   <y>0</y>\n"
                "   <width>200</width>\n"
                "   <height>200</height>\n"
                "  </rect>\n"
                " </property>\n"
                " <property name=\"lineWidth\">\n"
                "  <number>4</number>\n"
                " </property>\n"
                "</widget>\n";
        }

        QWidget* createWidget( QWidget* parent ) override
        {
            QwtDial* dial = new QwtDial( parent );
            dial->setNeedle( new QwtDialSimpleNeedle(
                QwtDialSimpleNeedle::Arrow, true,
                dial->palette().color( QPalette::Dark ),
                dial->palette().color( QPalette::Mid ) ) );

            return dial;
        }
    };
}

#endif

#ifndef NO_QWT_WIDGETS

namespace
{
    class KnobInterface : public CustomWidgetInterface
    {
      public:
        KnobInterface()
        {
            m_name = "QwtKnob";
            m_include = "qwt_knob.h";
            m_icon = QPixmap( ":/pixmaps/qwtknob.png" );
            m_domXml =
                "<widget class=\"QwtKnob\" name=\"Knob\">\n"
                " <property name=\"geometry\">\n"
                "  <rect>\n"
                "   <x>0</x>\n"
                "   <y>0</y>\n"
                "   <width>150</width>\n"
                "   <height>150</height>\n"
                "  </rect>\n"
                " </property>\n"
                "</widget>\n";
        }

        QWidget* createWidget( QWidget* parent ) override
        {
            return new QwtKnob( parent );
        }
    };
}

#endif

#ifndef NO_QWT_PLOT

namespace
{
    class ScaleWidgetInterface : public CustomWidgetInterface
    {
      public:
        ScaleWidgetInterface()
        {
            m_name = "QwtScaleWidget";
            m_include = "qwt_scale_widget.h";
            m_icon = QPixmap( ":/pixmaps/qwtscale.png" );
            m_domXml =
                "<widget class=\"QwtScaleWidget\" name=\"ScaleWidget\">\n"
                " <property name=\"geometry\">\n"
                "  <rect>\n"
                "   <x>0</x>\n"
                "   <y>0</y>\n"
                "   <width>60</width>\n"
                "   <height>250</height>\n"
                "  </rect>\n"
                " </property>\n"
                "</widget>\n";
        }

        QWidget* createWidget( QWidget* parent ) override
        {
            return new QwtScaleWidget( QwtScaleDraw::LeftScale, parent );
        }
    };
}

#endif

#ifndef NO_QWT_WIDGETS

namespace
{
    class SliderInterface : public CustomWidgetInterface
    {
      public:
        SliderInterface()
        {
            m_name = "QwtSlider";
            m_include = "qwt_slider.h";
            m_icon = QPixmap( ":/pixmaps/qwtslider.png" );
            m_domXml =
                "<widget class=\"QwtSlider\" name=\"Slider\">\n"
                " <property name=\"geometry\">\n"
                "  <rect>\n"
                "   <x>0</x>\n"
                "   <y>0</y>\n"
                "   <width>60</width>\n"
                "   <height>250</height>\n"
                "  </rect>\n"
                " </property>\n"
                "</widget>\n";
        }

        QWidget* createWidget( QWidget* parent ) override
        {
            return new QwtSlider( parent );
        }
    };
}

#endif

namespace
{
    class TextLabelInterface : public CustomWidgetInterface
    {
      public:
        TextLabelInterface()
        {
            m_name = "QwtTextLabel";
            m_include = "qwt_text_label.h";

            m_icon = QPixmap( ":/pixmaps/qwtwidget.png" );
            m_domXml =
                "<widget class=\"QwtTextLabel\" name=\"TextLabel\">\n"
                " <property name=\"geometry\">\n"
                "  <rect>\n"
                "   <x>0</x>\n"
                "   <y>0</y>\n"
                "   <width>100</width>\n"
                "   <height>20</height>\n"
                "  </rect>\n"
                " </property>\n"
                "</widget>\n";
        }

        QWidget* createWidget( QWidget* parent ) override
        {
            return new QwtTextLabel( QwtText( "Label" ), parent );
        }
    };
}

#ifndef NO_QWT_WIDGETS

namespace
{
    class ThermoInterface : public CustomWidgetInterface
    {
      public:
        ThermoInterface()
        {
            m_name = "QwtThermo";
            m_include = "qwt_thermo.h";
            m_icon = QPixmap( ":/pixmaps/qwtthermo.png" );
            m_domXml =
                "<widget class=\"QwtThermo\" name=\"Thermo\">\n"
                " <property name=\"geometry\">\n"
                "  <rect>\n"
                "   <x>0</x>\n"
                "   <y>0</y>\n"
                "   <width>60</width>\n"
                "   <height>250</height>\n"
                "  </rect>\n"
                " </property>\n"
                "</widget>\n";
        }

        QWidget* createWidget( QWidget* parent ) override
        {
            return new QwtThermo( parent );
        }
    };
}

#endif

#ifndef NO_QWT_WIDGETS

namespace
{
    class WheelInterface : public CustomWidgetInterface
    {
      public:
        WheelInterface()
        {
            m_name = "QwtWheel";
            m_include = "qwt_wheel.h";
            m_icon = QPixmap( ":/pixmaps/qwtwheel.png" );
            m_domXml =
                "<widget class=\"QwtWheel\" name=\"Wheel\">\n"
                "</widget>\n";
        }

        QWidget* createWidget( QWidget* parent ) override
        {
            return new QwtWheel( parent );
        }
    };
}

#endif

namespace
{
    class WidgetCollectionInterface
        : public QObject
        , public QDesignerCustomWidgetCollectionInterface
    {
        Q_OBJECT
        Q_INTERFACES( QDesignerCustomWidgetCollectionInterface )

        Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QDesignerCustomWidgetCollectionInterface" )

      public:
        WidgetCollectionInterface()
        {
#ifndef NO_QWT_PLOT
            m_plugins += new PlotInterface();
            m_plugins += new ScaleWidgetInterface();
#endif

#ifndef NO_QWT_POLAR
            m_plugins += new PolarPlotInterface();
#endif

#ifndef NO_QWT_WIDGETS
            m_plugins += new AnalogClockInterface();
            m_plugins += new CompassInterface();
            m_plugins += new CounterInterface();
            m_plugins += new DialInterface();
            m_plugins += new KnobInterface();
            m_plugins += new SliderInterface();
            m_plugins += new ThermoInterface();
            m_plugins += new WheelInterface();
#endif
            m_plugins += new TextLabelInterface();
        }

        ~WidgetCollectionInterface() override
        {
            qDeleteAll( m_plugins );
        }

        QList< QDesignerCustomWidgetInterface* > customWidgets() const override
        {
            return m_plugins;
        }

      private:
        QList< QDesignerCustomWidgetInterface* > m_plugins;
    };
}

#include "qwt_designer_plugin.moc"
