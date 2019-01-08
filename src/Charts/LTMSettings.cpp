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

#include "LTMSettings.h"
#include "MainWindow.h"
#include "LTMTool.h"
#include "Context.h"
#include "LTMChartParser.h"
#include "Utils.h"

#include <QtGui>
#include <qwt_plot.h>
#include <qwt_symbol.h>
#include <qwt_plot_curve.h>


/*----------------------------------------------------------------------
 * EDIT CHART DIALOG
 *--------------------------------------------------------------------*/
EditChartDialog::EditChartDialog(Context *context, LTMSettings *settings, QList<LTMSettings>presets) :
    QDialog(context->mainWindow, Qt::Dialog), context(context), settings(settings), presets(presets)
{
    setWindowTitle(tr("Enter Chart Name"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Metric Name
    mainLayout->addSpacing(5);

    chartName = new QLineEdit;
    mainLayout->addWidget(chartName);
    mainLayout->addStretch();

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    okButton = new QPushButton(tr("&OK"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);
    mainLayout->addLayout(buttonLayout);

    // make it wide enough
    setMinimumWidth(250);

    // connect up slots
    connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

void
EditChartDialog::okClicked()
{
    // mustn't be blank
    if (chartName->text() == "") {
        QMessageBox::warning( 0, tr("Entry Error"), tr("Name is blank"));
        return;
    }

    // does it already exist?
    foreach (LTMSettings chart, presets) {
        if (chart.name == chartName->text()) {
            QMessageBox::warning( 0, tr("Entry Error"), tr("Chart already exists"));
            return;
        }
    }

    settings->name = chartName->text();
    accept();
}
void
EditChartDialog::cancelClicked()
{
    reject();
}

/*----------------------------------------------------------------------
 * Write to charts.xml
 *--------------------------------------------------------------------*/
void
LTMSettings::writeChartXML(QDir home, QList<LTMSettings> charts)
{
    LTMChartParser::serialize(QString(home.canonicalPath() + "/charts.xml"), charts);
}


/*----------------------------------------------------------------------
 * Read charts.xml
 *--------------------------------------------------------------------*/

void
LTMSettings::readChartXML(QDir home, bool useMetricUnits, QList<LTMSettings> &charts)
{
    QFileInfo chartFile(home.canonicalPath() + "/charts.xml");
    QFile chartsFile;
    bool builtIn;

    // if it doesn't exist use our built-in default version
    if (chartFile.exists()) {
        chartsFile.setFileName(chartFile.filePath());
        builtIn = false;
    }
    else {
        chartsFile.setFileName(":/xml/charts.xml");
        builtIn = true;
    }

    QXmlInputSource source( &chartsFile );
    QXmlSimpleReader xmlReader;
    LTMChartParser handler;
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);
    xmlReader.parse( source );
    charts = handler.getSettings();

    // translate only once and only if the built-in version is imported
    if (builtIn) {

        // now run over all charts to replace name and unit
        for (int i=0; i<charts.count(); i++) {
            charts[i].translateMetrics(useMetricUnits);
        }
    }
}

void
LTMSettings::translateMetrics(bool useMetricUnits)
{
    const RideMetricFactory &factory = RideMetricFactory::instance();
    // run over all chart metrics and replace name and unit
    for (int j=0; j<metrics.count(); j++){
        // get access to the metric using the .symbol() as key,
        // since only CHART.XML and home-layout.xml are mapped
        const RideMetric *m = factory.rideMetric(metrics[j].symbol);
        if (m) {
            // set name, units only if there was a description before
            QString name(Utils::unprotect(m->name()));
            // uname is replaced only if it matches english name
            if (metrics[j].uname == m->internalName())
                metrics[j].uname = name;
            metrics[j].name = name;
            // uunits is replaced only if it matches unit,
            // ir units are not saved it don't translate uunits
            if (metrics[j].uunits == metrics[j].units)
                metrics[j].uunits = m->units(useMetricUnits);
            metrics[j].units = m->units(useMetricUnits);
        }
    }
}

/*----------------------------------------------------------------------
 * Marshall/Unmarshall to DataStream to store as a QVariant
 *----------------------------------------------------------------------*/
QDataStream &operator<<(QDataStream &out, const LTMSettings &settings)
{
    // 4.6 - 4.9 all the same
    out.setVersion(QDataStream::Qt_4_6);

    // all the basic fields first
    out<<settings.name;
    out<<settings.title;
    out<<settings.start;
    out<<settings.end;
    out<<settings.groupBy;
    out<<settings.shadeZones;
    out<<settings.legend;
    out<<settings.field1;
    out<<settings.field2;
    out<<int(-1);
    out<<int(LTM_VERSION_NUMBER); // defined in LTMSettings.h
    out<<settings.metrics.count();
    foreach(MetricDetail metric, settings.metrics) {
        bool discard = false;
        out<<metric.type;
        out<<metric.stack;
        out<<metric.symbol;
        out<<metric.name;
        out<<metric.uname;
        out<<metric.uunits;
        out<<metric.smooth;
        out<<discard; // was metric.trend but that was deprecated
        out<<metric.topN;
        out<<metric.topOut;
        out<<metric.baseline;
        out<<metric.showOnPlot;
        out<<metric.filter;
        out<<metric.from;
        out<<metric.to;
        out<<static_cast<int>(metric.curveStyle-1); // curveStyle change between qwt 5 and 6
        out<<static_cast<int>(metric.symbolStyle);
        out<<metric.penColor;
        out<<metric.penAlpha;
        out<<metric.penWidth;
        out<<metric.penStyle;
        out<<metric.brushColor;
        out<<metric.brushAlpha;
        out<<metric.fillCurve;
        out<<metric.duration;
        out<<metric.duration_units;
        out<<metric.bestSymbol;
        out<<static_cast<int>(metric.series);
        out<<metric.trendtype;
        out<<metric.labels;
        out<<metric.lowestN;
        out<<metric.model;
        out<<metric.estimate;
        out<<metric.estimateDuration;
        out<<metric.estimateDuration_units;
        out<<metric.wpk;
        out<<metric.stressType;
        out<<metric.units;
        out<<metric.formula;
        out<<static_cast<int>(metric.formulaType);
        out<<metric.datafilter;
        out<<metric.measureGroup;
        out<<metric.measureField;
        out<<metric.tests;
        out<<metric.perfs;
        out<<metric.submax;
    }
    out<<settings.showData;
    out<<settings.stack;
    out<<settings.stackWidth;
    return out;
}

QDataStream &operator>>(QDataStream &in, LTMSettings &settings)
{
    // 4.6 - 4.9 all the same
    in.setVersion(QDataStream::Qt_4_6);

    RideMetricFactory &factory = RideMetricFactory::instance();
    int counter=0;
    int version=0;

    // all the basic fields first
    in>>settings.name;
    in>>settings.title;
    in>>settings.start;
    in>>settings.end;
    in>>settings.groupBy;
    in>>settings.shadeZones;
    in>>settings.legend;
    in>>settings.field1;
    in>>settings.field2;
    in>>counter;

    // we now add version number before the counter
    // if counter is -1 -- to make settings extensible
    if (counter == -1) {
        in>>version;
        in>>counter;
    }

if (version <= LTM_VERSION_NUMBER) { // only if we know how to read !
while(counter-- && !in.atEnd()) {
        bool discard;
        MetricDetail m;
        in>>m.type;
        in>>m.stack;
        in>>m.symbol;
        in>>m.name;
        in>>m.uname;
        in>>m.uunits;
        in>>m.smooth;
        in>>discard; // was m.trend but that was deprecated
        if (discard) m.trendtype = 1; // will be overwritten below if not old settings
        in>>m.topN;
        in>>m.topOut;
        in>>m.baseline;
        in>>m.showOnPlot;
        in>>m.filter;
        in>>m.from;
        in>>m.to;
        int x;
        in>> x; m.curveStyle = static_cast<QwtPlotCurve::CurveStyle>(x+1);  // curveStyle change between qwt 5 and 6
        in>> x; m.symbolStyle = static_cast<QwtSymbol::Style>(x);
        in>>m.penColor;
        in>>m.penAlpha;
        in>>m.penWidth;
        in>>m.penStyle;
        in>>m.brushColor;
        in>>m.brushAlpha;

        // added curve filling in v1.0
        if (version >=1) {
            in>>m.fillCurve;
        } else {
            m.fillCurve = false;
        }

        if (version >= 2) { // get bests info
            in>>m.duration;
            in>>m.duration_units;
            in>>m.bestSymbol;
            in>>x;
            m.series = static_cast<RideFile::SeriesType>(x);
        }

        if (version >= 3) { // trendtype added
            in>>m.trendtype;
        } else {
            m.trendtype = 0; // default!
        }

        
        if (version >= 5) {
            in >>m.labels;
        }

        if (version >= 8) {
            in >>m.lowestN;
        }

        if (version >= 9) {
            in >> m.model;
            in >> m.estimate;
        } else {
            m.model = "";
            m.estimate = 0;
        }
        if (version >= 10) {
            in >> m.estimateDuration;
            in >> m.estimateDuration_units;
        }
        if (version >= 11) {
            in >> m.wpk;
        }
        if (version >= 12) {
            in >> m.stressType;
        }
        if (version >= 13) {
            in >> m.units;
        }
        if (version >= 14) {
            in >> m.formula;
        }
        if (version >= 15) {
            int x;
            in>> x; m.formulaType = static_cast<RideMetric::MetricType>(x);  // curveStyle change between qwt 5 and 6
        }
        if (version >= 16) {
            in >> m.datafilter;
        }
        if (version >= 17) {
            in >> m.measureGroup;
            in >> m.measureField;
        }
        if (version >= 18) {
            in >> m.tests;
            in >> m.perfs;
        }
        if (version >= 19) in >> m.submax;

        bool keep=true;
        // check for deprecated things and set keep=false if
        // we don't support this any more !
        if (m.type == METRIC_MEASURE) keep = false;

        // some sanity checking after strange corruption in 3.2
        if (m.type < 0) keep = false;
        if (m.type == METRIC_DB && m.symbol == "") keep = false;
        if (m.type == METRIC_DB && factory.rideMetric(m.symbol)==NULL) keep = false;
        if (m.topN > 100) m.topN = 0;
        if (m.lowestN > 100) m.lowestN = 0;

        if (keep) {
            // get a metric pointer (if it exists)
            m.metric = factory.rideMetric(m.symbol);
            settings.metrics.append(m);
        }
    }
    if (version >= 4) in >> settings.showData;
    if (version >= 6) {
        in >>settings.stack;
    }
    if (version >= 7) {
        in >>settings.stackWidth;
    }
    }

    return in;
}
