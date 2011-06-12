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
#include <assert.h>

// local helper functions to convert Qwt enums to ints and back
static int curveToInt(QwtPlotCurve::CurveStyle x)
{
    switch (x) {
    case QwtPlotCurve::NoCurve : return 0;
    case QwtPlotCurve::Lines : return 1;
    case QwtPlotCurve::Sticks : return 2;
    case QwtPlotCurve::Steps : return 3;
    case QwtPlotCurve::Dots : return 4;
    default : return 100;
    }
}
static QwtPlotCurve::CurveStyle intToCurve(int x)
{
    switch (x) {
    default:
    case 0 : return QwtPlotCurve::NoCurve;
    case 1 : return QwtPlotCurve::Lines;
    case 2 : return QwtPlotCurve::Sticks;
    case 3 : return QwtPlotCurve::Steps;
    case 4 : return QwtPlotCurve::Dots;
    case 100: return QwtPlotCurve::UserCurve;
    }
}
static int symbolToInt(QwtSymbol::Style x)
{
    switch (x) {
    default:
    case QwtSymbol::NoSymbol: return -1;
    case QwtSymbol::Ellipse: return 0;
    case QwtSymbol::Rect: return 1;
    case QwtSymbol::Diamond: return 2;
    case QwtSymbol::Triangle: return 3;
    case QwtSymbol::DTriangle: return 4;
    case QwtSymbol::UTriangle: return 5;
    case QwtSymbol::LTriangle: return 6;
    case QwtSymbol::RTriangle: return 7;
    case QwtSymbol::Cross: return 8;
    case QwtSymbol::XCross: return 9;
    case QwtSymbol::HLine: return 10;
    case QwtSymbol::VLine: return 11;
    case QwtSymbol::Star1: return 12;
    case QwtSymbol::Star2: return 13;
    case QwtSymbol::Hexagon: return 14;
    case QwtSymbol::StyleCnt: return 15;

    }
}
static QwtSymbol::Style intToSymbol(int x)
{
    switch (x) {
    default:
    case -1: return QwtSymbol::NoSymbol;
    case 0 : return QwtSymbol::Ellipse;
    case 1 : return QwtSymbol::Rect;
    case 2 : return QwtSymbol::Diamond;
    case 3 : return QwtSymbol::Triangle;
    case 4 : return QwtSymbol::DTriangle;
    case 5 : return QwtSymbol::UTriangle;
    case 6 : return QwtSymbol::LTriangle;
    case 7 : return QwtSymbol::RTriangle;
    case 8 : return QwtSymbol::Cross;
    case 9 : return QwtSymbol::XCross;
    case 10 : return QwtSymbol::HLine;
    case 11 : return QwtSymbol::VLine;
    case 12 : return QwtSymbol::Star1;
    case 13 : return QwtSymbol::Star2;
    case 14 : return QwtSymbol::Hexagon;
    case 15 : return QwtSymbol::StyleCnt;

    }
}
bool LTMChartParser::startDocument()
{
    buffer.clear();
    return TRUE;
}

static QString unprotect(QString buffer)
{
    // get local TM character code
    QTextEdit trademark("&#8482;"); // process html encoding of(TM)
    QString tm = trademark.toPlainText();

    // remove quotes
    QString t = buffer.trimmed();
    QString s = t.mid(1,t.length()-2);

    // replace html (TM) with local TM character
    s.replace( "&#8482;", tm );

    // html special chars are automatically handled
    // XXX other special characters will not work
    // cross-platform but will work locally, so not a biggie
    // i.e. if thedefault charts.xml has a special character
    // in it it should be added here
    return s;
}

// to see the format of the charts.xml file, look at the serialize()
// function at the bottom of this source file.
bool LTMChartParser::endElement( const QString&, const QString&, const QString &qName )
{
    //
    // Single Attribute elements
    //
    if(qName == "chartname") setting.name = unprotect(buffer);
    else if(qName == "metricname") metric.symbol = buffer.trimmed();
    else if(qName == "metricdesc") metric.name = unprotect(buffer);
    else if(qName == "metricuname") metric.uname = unprotect(buffer);
    else if(qName == "metricuunits") metric.uunits = unprotect(buffer);
    else if(qName == "metricbaseline") metric.baseline = buffer.trimmed().toDouble();
    else if(qName == "metricsmooth") metric.smooth = buffer.trimmed().toInt();
    else if(qName == "metrictrend") metric.trend = buffer.trimmed().toInt();
    else if(qName == "metrictopn") metric.topN = buffer.trimmed().toInt();
    else if(qName == "metriccurve") metric.curveStyle = intToCurve(buffer.trimmed().toInt());
    else if(qName == "metricsymbol") metric.symbolStyle = intToSymbol(buffer.trimmed().toInt());
    else if(qName == "metricpencolor") {
            // the r,g,b values are in red="xx",green="xx" and blue="xx" attributes
            // of this element and captured in startelement below
            metric.penColor = QColor(red,green,blue);
    }
    else if(qName == "metricpenalpha") metric.penAlpha = buffer.trimmed().toInt();
    else if(qName == "metricpenwidth") metric.penWidth = buffer.trimmed().toInt();
    else if(qName == "metricpenstyle") metric.penStyle = buffer.trimmed().toInt();
    else if(qName == "metricbrushcolor") {
            // the r,g,b values are in red="xx",green="xx" and blue="xx" attributes
            // of this element and captured in startelement below
            metric.brushColor = QColor(red,green,blue);

    } else if(qName == "metricbrushalpha") metric.penAlpha = buffer.trimmed().toInt();

    //
    // Complex Elements
    //
    else if(qName == "metric") // <metric></metric> block
        setting.metrics.append(metric);
    else if (qName == "LTM-chart") // <LTM-chart></LTM-chart> block
        settings.append(setting);
    else if (qName == "charts") { // <charts></charts> block top-level
    } // do nothing for now
    return TRUE;
}

bool LTMChartParser::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs )
{
    buffer.clear();
    if(name == "charts")
        ; // do nothing for now
    else if (name == "LTM-chart")
        setting = LTMSettings();
    else if (name == "metric")
        metric = MetricDetail();
    else if (name == "metricpencolor" || name == "metricbrushcolor") {

        // red="x" green="x" blue="x" attributes for pen/brush color
        for(int i=0; i<attrs.count(); i++) {
            if (attrs.qName(i) == "red") red=attrs.value(i).toInt();
            if (attrs.qName(i) == "green") green=attrs.value(i).toInt();
            if (attrs.qName(i) == "blue") blue=attrs.value(i).toInt();
        }
    }

    return TRUE;
}

bool LTMChartParser::characters( const QString& str )
{
    buffer += str;
    return TRUE;
}

QList<LTMSettings>
LTMChartParser::getSettings()
{
    return settings;
}

bool LTMChartParser::endDocument()
{
    return TRUE;
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
void
LTMChartParser::serialize(QString filename, QList<LTMSettings> charts)
{
    // open file - truncate contents
    QFile file(filename);
    file.open(QFile::WriteOnly);
    file.resize(0);
    QTextStream out(&file);

    // Character set encoding added to support international characters in names
    out.setCodec("UTF-8");
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

    // begin document
    out << "<charts>\n";

    // write out to file
    foreach (LTMSettings chart, charts) {
        // chart name
        out<<QString("\t<LTM-chart>\n\t\t<chartname>\"%1\"</chartname>\n").arg(xmlprotect(chart.name));

        // all the metrics
        foreach (MetricDetail metric, chart.metrics) {
            out<<QString("\t\t<metric>\n");
            out<<QString("\t\t\t<metricdesc>\"%1\"</metricdesc>\n").arg(xmlprotect(metric.name));
            out<<QString("\t\t\t<metricname>%1</metricname>\n").arg(metric.symbol);
            out<<QString("\t\t\t<metricuname>\"%1\"</metricuname>\n").arg(xmlprotect(metric.uname));
            out<<QString("\t\t\t<metricuunits>\"%1\"</metricuunits>\n").arg(xmlprotect(metric.uunits));

            // SMOOTH, TREND, TOPN
            out<<QString("\t\t\t<metricsmooth>%1</metricsmooth>\n").arg(metric.smooth);
            out<<QString("\t\t\t<metrictrend>%1</metrictrend>\n").arg(metric.trend);
            out<<QString("\t\t\t<metrictopn>%1</metrictopn>\n").arg(metric.topN);
            out<<QString("\t\t\t<metricbaseline>%1</metricbaseline>\n").arg(metric.baseline);

            // CURVE, SYMBOL
            out<<QString("\t\t\t<metriccurve>%1</metriccurve>\n").arg(curveToInt(metric.curveStyle));
            out<<QString("\t\t\t<metricsymbol>%1</metricsymbol>\n").arg(symbolToInt(metric.symbolStyle));

            // PEN
            out<<QString("\t\t\t<metricpencolor red=\"%1\" green=\"%3\" blue=\"%4\"></metricpencolor>\n")
                        .arg(metric.penColor.red())
                        .arg(metric.penColor.green())
                        .arg(metric.penColor.blue());
            out<<QString("\t\t\t<metricpenalpha>%1</metricpenalpha>\n").arg(metric.penAlpha);
            out<<QString("\t\t\t<metricpenwidth>%1</metricpenwidth>\n").arg(metric.penWidth);
            out<<QString("\t\t\t<metricpenstyle>%1</metricpenstyle>\n").arg(metric.penStyle);

            // BRUSH
            out<<QString("\t\t\t<metricbrushcolor red=\"%1\" green=\"%3\" blue=\"%4\"></metricbrushcolor>\n")
                        .arg(metric.brushColor.red())
                        .arg(metric.brushColor.green())
                        .arg(metric.brushColor.blue());
            out<<QString("\t\t\t<metricbrushalpha>%1</metricbrushalpha>\n").arg(metric.brushAlpha);

            out<<QString("\t\t</metric>\n");
        }
        out<<QString("\t</LTM-chart>\n");
    }

    // end document
    out << "</charts>\n";

    // close file
    file.close();
}
