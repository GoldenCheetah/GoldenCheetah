/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#include "UserMetricParser.h"
#include "UserMetricSettings.h"
#include "Context.h"
#include "Utils.h"

#include <QDate>
#include <QDebug>
#include <QTextEdit>
#include <QMessageBox>
#include <QXmlInputSource>
#include <QXmlSimpleReader>

bool UserMetricParser::startDocument()
{
    buffer.clear();
    return true;
}

// to see the format of the charts.xml file, look at the serialize()
// function at the bottom of this source file.
bool UserMetricParser::endElement( const QString&, const QString&, const QString &qName )
{
    //
    // Single Attribute elements
    //
    if(qName == "usermetric") {

        // get the contents
        add.program = Utils::unprotect(buffer.mid(1,buffer.length()-2));

        // add to the list
        settings << add;

    }
    return true;
}

bool UserMetricParser::startElement( const QString&, const QString&, const QString &, const QXmlAttributes &attrs)
{
    // reset
    add = UserMetricSettings();
    buffer.clear();

    // basic settings for the metric are in the element attributes
    for(int i=0; i<attrs.count(); i++) {
        if (attrs.qName(i) == "symbol") add.symbol=Utils::unprotect(attrs.value(i));
        if (attrs.qName(i) == "name") add.name=Utils::unprotect(attrs.value(i));
        if (attrs.qName(i) == "description") add.description=Utils::unprotect(attrs.value(i));
        if (attrs.qName(i) == "precision") add.precision=Utils::unprotect(attrs.value(i)).toInt();
        if (attrs.qName(i) == "aggzero") add.aggzero=Utils::unprotect(attrs.value(i)).toInt();
        if (attrs.qName(i) == "istime") add.istime=Utils::unprotect(attrs.value(i)).toInt();
        if (attrs.qName(i) == "type") add.type=Utils::unprotect(attrs.value(i)).toInt();
        if (attrs.qName(i) == "unitsMetric") add.unitsMetric=Utils::unprotect(attrs.value(i));
        if (attrs.qName(i) == "unitsImperial") add.unitsImperial=Utils::unprotect(attrs.value(i));
        if (attrs.qName(i) == "conversion") add.conversion=Utils::unprotect(attrs.value(i)).toDouble();
        if (attrs.qName(i) == "conversionSum") add.conversionSum=Utils::unprotect(attrs.value(i)).toDouble();
        if (attrs.qName(i) == "fingerprint") add.fingerprint=Utils::unprotect(attrs.value(i));
    }

    return true;
}

bool UserMetricParser::characters( const QString& str )
{
    buffer += str;
    return true;
}

bool UserMetricParser::endDocument()
{
    return true;
}

//
// Write out the charts.xml file
//
// Most of the heavy lifting is done by the LTMSettings
// << and >> operators. We just put them into the character
// data for a chart.
//
void
UserMetricParser::serialize(QString filename, QList<UserMetricSettings> metrics)
{
    // open file - truncate contents
    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("Problem Saving User Metric Configuration"));
        msgBox.setInformativeText(tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(filename));
        msgBox.exec();
        return;
    };
    file.resize(0);

    QTextStream out(&file);
    serializeToQTextStream(out, metrics);
    file.close();
}

void
UserMetricParser::serializeToQTextStream(QTextStream& out, QList<UserMetricSettings> metrics)
{
#if QT_VERSION < 0x060000
    out.setCodec("UTF-8");
#endif

    // begin document
    out << QString("<usermetrics version=\"%1\">\n").arg(USER_METRICS_VERSION_NUMBER);

    // write out to file
    foreach (UserMetricSettings metric, metrics) {

        // never write them to file, they may be obsoleted by the user.
        if (metric.symbol.startsWith("compatibility_")) continue;

        // symbol
        out <<"\t<usermetric symbol=\"" << Utils::xmlprotect(metric.symbol) <<"\" ";

        out <<"name=\"" << Utils::xmlprotect(metric.name) << "\" ";
        out <<"description=\"" << Utils::xmlprotect(metric.description) << "\" ";
        out <<"unitsMetric=\"" << Utils::xmlprotect(metric.unitsMetric) << "\" ";
        out <<"unitsImperial=\"" << Utils::xmlprotect(metric.unitsImperial) << "\" ";
        out <<"aggzero=\"" << (metric.aggzero ? 1 : 0) << "\" ";
        out <<"istime=\"" << (metric.istime ? 1 : 0) << "\" ";
        out <<"precision=\"" << metric.precision << "\" ";
        out <<"type=\"" << metric.type << "\" ";
        out <<"conversion=\"" << metric.conversion << "\" ";
        out <<"conversionSum=\"" << metric.conversionSum << "\" ";
        out <<"fingerprint=\"" << Utils::xmlprotect(metric.fingerprint) << "\" ";

        out << ">\n";
        out << Utils::xmlprotect(metric.program);
        out << "\n</usermetric>\n";
    }

    // end document
    out << "</usermetrics>\n";

    // flush stream
    out.flush();
}
