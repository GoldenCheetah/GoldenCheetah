/*
 * Copyright (c) 2015 Vianney Boyer (vlcvboyer@gmail.com) copied from
 * GPXParser.cpp
 * Copyright (c) 2010 Greg Lonnon (greg.lonnon@gmail.com)
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net),
 *		      J.T Conklin (jtc@acorntoolworks.com)
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

#include "VideoLayoutParser.h"
#include "MeterWidget.h"

// use stc strtod to bypass Qt toDouble() issues
#include <stdlib.h>

VideoLayoutParser::VideoLayoutParser (QObject* layoutFile, QList<MeterWidget*>& metersWidget, QWidget* VideoContainer)
    : layoutFile(layoutFile), metersWidget(metersWidget), VideoContainer(VideoContainer)
{
    nonameindex = 0;
    meterWidget = NULL;
}

void VideoLayoutParser::SetDefaultValues()
{
    if (meterWidget)
    {
        meterWidget->m_RelativeWidth = 20.0;
        meterWidget->m_RelativeHeight = 20.0;
        meterWidget->m_RelativePosX = 50.0;
        meterWidget->m_RelativePosY = 80.0;

        meterWidget->RangeMin = 0.0;
        meterWidget->RangeMax = 80.0;
        meterWidget->MainColor = QColor(255,0,0,200);
        meterWidget->ScaleColor = QColor(255,255,255,200);
        meterWidget->OutlineColor = QColor(100,100,100,200);
        meterWidget->BackgroundColor = QColor(100,100,100,200);
        meterWidget->MainFont = QFont(meterWidget->font().family(), 64);
        meterWidget->AltFont = QFont(meterWidget->font().family(), 48);
    }
}

bool VideoLayoutParser::startElement( const QString&, const QString&,
    const QString& qName,
    const QXmlAttributes& qAttributes)
{
    buffer.clear();

    if(qName == "meter")
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

        //TODO: allows creation of meter when container will be created later
        QWidget* containerWidget = NULL;
        if (container == QString("Video"))
        {
            containerWidget = VideoContainer;
        }
        else
        {
            foreach(MeterWidget* p_meterWidget , metersWidget)
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
        else
        {
            qDebug() << QObject::tr("Error creating meter");
        }
        
        SetDefaultValues();
    }

    return true;
}

bool VideoLayoutParser::endElement( const QString&, const QString&, const QString& qName)
{
    if (meterWidget)
    {
        if (qName == "RelativeWidth")
        {
            meterWidget->m_RelativeWidth = buffer.toFloat();
        }
        else if (qName == "RelativeHeight")
        {
            meterWidget->m_RelativeHeight = buffer.toFloat();
        }
        else if (qName == "RelativePosX")
        {
            meterWidget->m_RelativePosX = buffer.toFloat();
        }
        else if (qName == "RelativePosY")
        {
            meterWidget->m_RelativePosY = buffer.toFloat();
        }
        else if (qName == "RangeMin")
        {
            meterWidget->m_RangeMin = buffer.toFloat();
        }
        else if (qName == "RangeMax")
        {
            meterWidget->m_RangeMax = buffer.toFloat();
        }
        
    /*
    //TODO:    
        MainColor
        ScaleColor
        OutlineColor
        BackgroundColor
        MainFont
        AltFont
        speedmeterwidget->Angle = 220.0;
        speedmeterwidget->SubRange = 6;
        
    */

        else if (qName == "meter")
        {
            metersWidget.append(meterWidget);
            meterWidget = NULL;
        }

        return true;

    }
    else
        return false;
}

bool VideoLayoutParser::characters( const QString& str )
{
    buffer += str;
    return true;
}
