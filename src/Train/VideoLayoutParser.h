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
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301     USA
 */

#ifndef _VideoLayoutParser_h
#define _VideoLayoutParser_h
#include "GoldenCheetah.h"

#include <QString>
#include <QXmlDefaultHandler>

#include "MeterWidget.h"

class VideoLayoutParser : public QXmlDefaultHandler
{
public:
    VideoLayoutParser(QList<MeterWidget*>* metersWidget, QList<QString>* layoutNames, QWidget* VideoContainer);

    bool startElement( const QString&, const QString&, const QString&, const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );

    bool characters( const QString& );
    void SetDefaultValues();
    int  layoutPositionSelected;

private:
    QList<MeterWidget*>* metersWidget;
    QList<QString>* layoutNames;
    QWidget*    VideoContainer;

    MeterWidget* meterWidget;
    QString     buffer;
    int         nonameindex;
    int         layoutPosition;
    bool        skipLayout;
};

#endif // _VideoLayoutParser_h

