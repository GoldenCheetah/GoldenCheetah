/*
 * Copyright (c) 2015 Vianney Boyer (vlcvboyer@gmail.com) copied from
 * GPXParser.cpp
 * Copyright (c) 2010 Greg Lonnon (greg.lonnon@gmail.com)
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net),
 *           J.T Conklin (jtc@acorntoolworks.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <QString>
#include <QDebug>

#include "Context.h"

#include "VideoLayoutParser.h"
#include "MeterWidget.h"

VideoLayoutParser::VideoLayoutParser (QList<MeterWidget*>* metersWidget, QList<QString>* layoutNames, QWidget* VideoContainer, Context* context)
    : context(context), metersWidget(metersWidget), layoutNames(layoutNames), VideoContainer(VideoContainer)
{
    nonameindex = 0;
    skipLayout = false;
    layoutPosition = 0;
    meterWidget = NULL;
}

void VideoLayoutParser::SetDefaultValues()
{
    meterWidget->m_RelativeWidth = 20.0;
    meterWidget->m_RelativeHeight = 20.0;
    meterWidget->m_RelativePosX = 50.0;
    meterWidget->m_RelativePosY = 80.0;
    meterWidget->m_RangeMin = 0.0;
    meterWidget->m_RangeMax = 80.0;
    meterWidget->m_MainColor = QColor(255,0,0,200);
    meterWidget->m_ScaleColor = QColor(255,255,255,200);
    meterWidget->m_OutlineColor = QColor(100,100,100,200);
    meterWidget->m_BackgroundColor = QColor(100,100,100,0);
    meterWidget->m_BoundingRectColor = QColor(150,150,150,100);
    meterWidget->m_MainFont = QFont(meterWidget->font().family(), 64);
    meterWidget->m_AltFont = QFont(meterWidget->font().family(), 48);
    meterWidget->m_VideoContainer = VideoContainer;
    meterWidget->Text = "";
    meterWidget->AltText = "";
    meterWidget->AltTextSuffix = "";
}

QColor GetColorFromFields(const QXmlAttributes& qAttributes)
{
        int i;
        int R,G,B,A;
        // reset all
        R=G=B=A=0;
        if((i = qAttributes.index("R"))>= 0)
            R = qAttributes.value(i).toInt();
        if((i = qAttributes.index("G"))>= 0)
            G = qAttributes.value(i).toInt();
        if((i = qAttributes.index("B"))>= 0)
            B = qAttributes.value(i).toInt();
        if((i = qAttributes.index("A"))>= 0)
            A = qAttributes.value(i).toInt();
        return QColor(R,G,B,A);
}

bool VideoLayoutParser::startElement( const QString&, const QString&,
    const QString& qName,
    const QXmlAttributes& qAttributes)
{
    QString source;
    QString meterName;
    QString meterType;
    QString container; // will be "Video" when not defined otherwise another meter name (allows positioning of one meter inside another one)
    int textWidth;
    Qt::Alignment alignment;
    bool boundingRectVisibility;
    bool backgroundVisibility;

    if(skipLayout) return true;

    buffer.clear();

    if(qName == "layout")
    {
        int i = qAttributes.index("name");
        layoutNames->append(i >= 0 ? qAttributes.value(i) : QString("noname_") + QString::number(layoutPosition));

        if(layoutPositionSelected != layoutPosition) skipLayout = true;
    }
    else if(qName == "meter")
    {
        int i = qAttributes.index("name"); // To be used to enclose another sub meter (typ. text within NeedleMeter)
        if(i >= 0)
            meterName = qAttributes.value(i);
        else
            meterName = QString("noname_") + QString::number(nonameindex++);

        i = qAttributes.index("container"); // Video, ... or other meter name
        if(i >= 0)
            container = qAttributes.value(i);
        else
            container = QString("Video");

        i = qAttributes.index("source"); // Watt, Speed...
        if(i >= 0)
            source = qAttributes.value(i);
        else
            source = QString("None");

        i = qAttributes.index("type"); // Text, NeedleMeter, CircularMeter...
        if(i >= 0)
            meterType = qAttributes.value(i);
        else
            meterType = QString("Text");

        i = qAttributes.index("alignment");
        if(i >= 0)
            if(qAttributes.value(i) == QString("AlignLeft")) alignment = Qt::AlignLeft;
            else if(qAttributes.value(i) == QString("AlignRight")) alignment = Qt::AlignRight;
            else alignment = Qt::AlignHCenter;
        else
            alignment = Qt::AlignHCenter;

        i = qAttributes.index("textWidth");
        if(i >= 0)
            textWidth = qAttributes.value(i).toInt();
        else
            textWidth = 0;

        i = qAttributes.index("BoundingRect");
        if(i >= 0 && qAttributes.value(i) == QString("true"))
            boundingRectVisibility = true;
        else
            boundingRectVisibility = false;

        i = qAttributes.index("Background");
        if(i >= 0 && qAttributes.value(i) == QString("true"))
            backgroundVisibility = true;
        else
            backgroundVisibility = false;

        //TODO: allow creation of meter when container will be created later
        QWidget* containerWidget = NULL;
        if (container == QString("Video"))
        {
            containerWidget = VideoContainer;
        }
        else
        {
            foreach(MeterWidget* p_meterWidget , *metersWidget)
            {
                if (p_meterWidget->m_Name == container)
                    containerWidget = p_meterWidget;
            }
        }

        //create meter object
        if (meterType == QString("Text"))
        {
            meterWidget = new TextMeterWidget(meterName, containerWidget, source);
        }
        else if (meterType == QString("NeedleMeter"))
        {
            meterWidget = new NeedleMeterWidget(meterName, containerWidget, source);
        }
        else if (meterType == QString("CircularMeter"))
        {
            meterWidget = new CircularIndicatorMeterWidget(meterName, containerWidget, source);
        }
        else if (meterType == QString("CircularBargraph"))
        {
            meterWidget = new CircularBargraphMeterWidget(meterName, containerWidget, source);
        }
        else if (meterType == QString("Elevation"))
        {
            meterWidget = new ElevationMeterWidget(meterName, containerWidget, source);
        }
        else if (meterType == QString("ElevationZoomed"))
        {
            meterWidget = new ElevationZoomedMeterWidget(meterName, containerWidget, source);
        }
        else if (meterType == QString("LiveMap"))
        {
            meterWidget = new LiveMapWidget(meterName, containerWidget, source, context);
        }
        else
        {
            qDebug() << QObject::tr("Error creating meter");
        }

        if (meterWidget)
        {
            SetDefaultValues();
            meterWidget->textWidth = textWidth;
            meterWidget->alignment = alignment;
            meterWidget->boundingRectVisibility = boundingRectVisibility;
            meterWidget->backgroundVisibility = backgroundVisibility;
        }
    }
    else if ((qName == "MainColor") && meterWidget)
        meterWidget->m_MainColor =  GetColorFromFields(qAttributes);
    else if ((qName == "OutlineColor") && meterWidget)
        meterWidget->m_OutlineColor =  GetColorFromFields(qAttributes);
    else if ((qName == "ScaleColor") && meterWidget)
        meterWidget->m_ScaleColor =  GetColorFromFields(qAttributes);
    else if ((qName == "BackgroundColor") && meterWidget)
        meterWidget->m_BackgroundColor =  GetColorFromFields(qAttributes);
    else if ((qName == "BoundingRectColor") && meterWidget)
        meterWidget->m_BoundingRectColor =  GetColorFromFields(qAttributes);

    else if ((qName == "RelativeSize") && meterWidget)
    {
        int i;
        if ((i = qAttributes.index("Width")) >= 0)
            meterWidget->m_RelativeWidth = qAttributes.value(i).toFloat()/100.0;
        if ((i = qAttributes.index("Height")) >= 0)
            meterWidget->m_RelativeHeight = qAttributes.value(i).toFloat()/100.0;
    }
    else if ((qName == "RelativePosition") && meterWidget)
    {
        int i;
        if ((i = qAttributes.index("X")) >= 0)
            meterWidget->m_RelativePosX = qAttributes.value(i).toFloat()/100.0;
        if ((i = qAttributes.index("Y")) >= 0)
            meterWidget->m_RelativePosY = qAttributes.value(i).toFloat()/100.0;
    }
    else if ((qName == "Range") && meterWidget)
    {
        int i;
        if ((i = qAttributes.index("Min")) >= 0)
            meterWidget->m_RangeMin = qAttributes.value(i).toFloat();
        if ((i = qAttributes.index("Max")) >= 0)
            meterWidget->m_RangeMax = qAttributes.value(i).toFloat();
    }
    else if ((qName == "MainFont") && meterWidget)
    {
        int i;
        int FontSize = 64;
        QString FontName = "Arial";
        if ((i = qAttributes.index("Name")) >= 0)
            FontName = qAttributes.value(i);
        if ((i = qAttributes.index("Size")) >= 0)
            FontSize = qAttributes.value(i).toInt();
        meterWidget->m_MainFont = QFont(FontName, FontSize);
        // Set fixed width, otherwise the meter digits keep moving sideways from a thin "1" to a fat "5" 
        meterWidget->m_MainFont.setFixedPitch(true);
    }
    else if ((qName == "AltFont") && meterWidget)
    {
        int i;
        int FontSize = 48;
        QString FontName = "Arial";
        if ((i = qAttributes.index("Name")) >= 0)
            FontName = qAttributes.value(i);
        if ((i = qAttributes.index("Size")) >= 0)
            FontSize = qAttributes.value(i).toInt();
        meterWidget->m_AltFont = QFont(FontName, FontSize);
        meterWidget->m_AltFont.setFixedPitch(true);
    }
    else if(qName != "layouts" && qName != "Text" && qName != "AltText" && qName != "Angle" && qName != "SubRange" && qName != "Zoom" && qName != "osmURL")
    {
        qDebug() << QObject::tr("Unknown start element ") << qName;
    }

    return true;
}

bool VideoLayoutParser::endElement( const QString&, const QString&, const QString& qName)
{
    if(qName == "layout")
    {
        layoutPosition++;
        skipLayout = false;
        return true;
    }

    if(skipLayout) return true;

    if (meterWidget)
    {
        if (qName == "Angle")
            meterWidget->m_Angle = buffer.toFloat();
        else if (qName == "SubRange")
            meterWidget->m_SubRange = buffer.toInt();
        else if (qName == "Text")
            meterWidget->Text = QString(buffer);
        else if (qName == "AltText")
            meterWidget->AltTextSuffix = QString(buffer);

        else if (qName == "meter")
        {
            if (metersWidget)
                metersWidget->append(meterWidget);
            meterWidget = NULL;
        }

        LiveMapWidget* liveMapWidget = dynamic_cast<LiveMapWidget*>(meterWidget);
        if (liveMapWidget != NULL)
        {
            if (qName == "Zoom")
                liveMapWidget->m_Zoom = buffer.toInt();
            else if (qName == "osmURL")
                liveMapWidget->m_osmURL = QString(buffer);
        }

        return true;
    }
    else
        return false;
}

bool VideoLayoutParser::characters( const QString& str )
{
    if(skipLayout) return true;

    buffer += str;
    return true;
}
