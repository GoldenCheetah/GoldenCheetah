/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "LTMChartParser.h"
#include "LTMSettings.h"
#include "LTMTool.h"

#include <QDate>
#include <QDebug>

bool LTMChartParser::startDocument()
{
    buffer.clear();
    return true;
}

// to see the format of the charts.xml file, look at the serialize()
// function at the bottom of this source file.
bool LTMChartParser::endElement( const QString&, const QString&, const QString &qName )
{
    //
    // Single Attribute elements
    //
    if(qName == "chart") {
        QString string = buffer.mid(1,buffer.length()-2);

        QByteArray base64(string.toLatin1());
        QByteArray unmarshall = QByteArray::fromBase64(base64);
        QDataStream s(&unmarshall, QIODevice::ReadOnly);
        LTMSettings x;
        s >> x;
        settings << x;
    } 
    return true;
}

bool LTMChartParser::startElement( const QString&, const QString&, const QString &, const QXmlAttributes &)
{
    buffer.clear();
    return true;
}

bool LTMChartParser::characters( const QString& str )
{
    buffer += str;
    return true;
}

QList<LTMSettings>
LTMChartParser::getSettings()
{
    return settings;
}

bool LTMChartParser::endDocument()
{
    return true;
}

// static helper to protect special xml characters
// ideally we would use XMLwriter to do this but
// the file format is trivial and this implementation
// is easier to follow and modify... for now.
static QString xmlprotect(QString string)
{
    QTextEdit trademark("&#8482;"); // process html encoding of(TM)
    QString tm = trademark.toPlainText();

    QString s = string;
    s.replace( tm, "&#8482;" );
    s.replace( "&", "&amp;" );
    s.replace( ">", "&gt;" );
    s.replace( "<", "&lt;" );
    s.replace( "\"", "&quot;" );
    s.replace( "\'", "&apos;" );
    return s;
}

//
// Write out the charts.xml file
//
// Most of the heavy lifting is done by the LTMSettings
// << and >> operators. We just put them into the character
// data for a chart.
//
void
LTMChartParser::serialize(QString filename, QList<LTMSettings> charts)
{
    // open file - truncate contents
    QFile file(filename);
    file.open(QFile::WriteOnly);
    file.resize(0);
    QTextStream out(&file);
    out.setCodec("UTF-8");

    // begin document
    out << "<charts>\n";

    // write out to file
    foreach (LTMSettings chart, charts) {

        out <<"\t<chart name=\"" << xmlprotect(chart.name) <<"\">\"";
        // chart name
        QByteArray marshall;
        QDataStream s(&marshall, QIODevice::WriteOnly);
        s << chart;
        out<<marshall.toBase64();
        out<<"\"</chart>\n";
    }

    // end document
    out << "</charts>\n";

    // close file
    file.close();
}
