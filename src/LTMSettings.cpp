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
#include "LTMTool.h"
#include "MainWindow.h"
#include "LTMChartParser.h"

#include <QtGui>
#include <qwt_plot.h>
#include <qwt_symbol.h>
#include <qwt_plot_curve.h>


/*----------------------------------------------------------------------
 * EDIT CHART DIALOG
 *--------------------------------------------------------------------*/
EditChartDialog::EditChartDialog(MainWindow *mainWindow, LTMSettings *settings, QList<LTMSettings>presets) :
    QDialog(mainWindow, Qt::Dialog), mainWindow(mainWindow), settings(settings), presets(presets)
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
        QMessageBox::warning( 0, "Entry Error", "Name is blank");
        return;
    }

    // does it already exist?
    foreach (LTMSettings chart, presets) {
        if (chart.name == chartName->text()) {
            QMessageBox::warning( 0, "Entry Error", "Chart already exists");
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
 * CHART MANAGER DIALOG
 *--------------------------------------------------------------------*/
ChartManagerDialog::ChartManagerDialog(MainWindow *mainWindow, QList<LTMSettings>*presets) :
    QDialog(mainWindow, Qt::Dialog), mainWindow(mainWindow), presets(presets)
{
    setWindowTitle(tr("Manage Charts"));

    QGridLayout *mainLayout = new QGridLayout(this);

    importButton = new QPushButton(tr("Import..."));
    exportButton = new QPushButton(tr("Export..."));
    upButton = new QPushButton(tr("Move up"));
    downButton = new QPushButton(tr("Move down"));
    renameButton = new QPushButton(tr("Rename"));
    deleteButton = new QPushButton(tr("Delete"));

    QVBoxLayout *actionButtons = new QVBoxLayout;
    actionButtons->addWidget(renameButton);
    actionButtons->addWidget(deleteButton);
    actionButtons->addWidget(upButton);
    actionButtons->addWidget(downButton);
    actionButtons->addStretch();
    actionButtons->addWidget(importButton);
    actionButtons->addWidget(exportButton);

    charts = new QTreeWidget;
    charts->headerItem()->setText(0, "Charts");
    charts->setColumnCount(1);
    charts->setSelectionMode(QAbstractItemView::SingleSelection);
    charts->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    charts->setIndentation(0);
    foreach(LTMSettings chart, *presets) {
        QTreeWidgetItem *add;
        add = new QTreeWidgetItem(charts->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);
        add->setText(0, chart.name);
    }
    charts->setCurrentItem(charts->invisibleRootItem()->child(0));

    // Cancel/ OK Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    okButton = new QPushButton(tr("&OK"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);

    mainLayout->addWidget(charts, 0,0);
    mainLayout->addLayout(actionButtons, 0,1);
    mainLayout->addLayout(buttonLayout,1,0);

    // seems reasonable...
    setMinimumHeight(350);

    // connect up slots
    connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
    connect(renameButton, SIGNAL(clicked()), this, SLOT(renameClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(importButton, SIGNAL(clicked()), this, SLOT(importClicked()));
    connect(exportButton, SIGNAL(clicked()), this, SLOT(exportClicked()));
}

void
ChartManagerDialog::okClicked()
{
    // take the edited versions of the name first
    for(int i=0; i<charts->invisibleRootItem()->childCount(); i++)
        (*presets)[i].name = charts->invisibleRootItem()->child(i)->text(0);

    accept();
}

void
ChartManagerDialog::cancelClicked()
{
    reject();
}

void
ChartManagerDialog::importClicked()
{
    QFileDialog existing(this);
    existing.setFileMode(QFileDialog::ExistingFile);
    existing.setNameFilter(tr("Chart File (*.xml)"));
    if (existing.exec()){
        // we will only get one (ExistingFile not ExistingFiles)
        QStringList filenames = existing.selectedFiles();

        if (QFileInfo(filenames[0]).exists()) {

            QList<LTMSettings> imported;
            QFile chartsFile(filenames[0]);

            // setup XML processor
            QXmlInputSource source( &chartsFile );
            QXmlSimpleReader xmlReader;
            LTMChartParser (handler);
            xmlReader.setContentHandler(&handler);
            xmlReader.setErrorHandler(&handler);

            // parse and get return values
            xmlReader.parse(source);
            imported = handler.getSettings();

            // now append to the QList and QTreeWidget
            *presets += imported;
            foreach (LTMSettings chart, imported) {
                QTreeWidgetItem *add;
                add = new QTreeWidgetItem(charts->invisibleRootItem());
                add->setFlags(add->flags() | Qt::ItemIsEditable);
                add->setText(0, chart.name);
            }

        } else {
            // oops non existant - does this ever happen?
            QMessageBox::warning( 0, "Entry Error", QString("Selected file (%1) does not exist").arg(filenames[0]));
        }
    }
}

void
ChartManagerDialog::exportClicked()
{
    QFileDialog newone(this);
    newone.setFileMode(QFileDialog::AnyFile);
    newone.setNameFilter(tr("Chart File (*.xml)"));
    if (newone.exec()){
        // we will only get one (ExistingFile not ExistingFiles)
        QStringList filenames = newone.selectedFiles();

        // if exists confirm overwrite
        if (QFileInfo(filenames[0]).exists()) {
            QMessageBox msgBox;
            msgBox.setText(QString("The selected file (%1) exists.").arg(filenames[0]));
            msgBox.setInformativeText("Do you want to overwrite it?");
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            msgBox.setIcon(QMessageBox::Warning);
            if (msgBox.exec() != QMessageBox::Ok)
                return;
        }
        LTMChartParser::serialize(filenames[0], *presets);
    }
}

void
ChartManagerDialog::upClicked()
{
    if (charts->currentItem()) {
        int index = charts->invisibleRootItem()->indexOfChild(charts->currentItem());
        if (index == 0) return; // its at the top already

        // movin on up!
        QTreeWidgetItem *moved;
        charts->invisibleRootItem()->insertChild(index-1, moved=charts->invisibleRootItem()->takeChild(index));
        charts->setCurrentItem(moved);
        LTMSettings save = (*presets)[index];
        presets->removeAt(index);
        presets->insert(index-1, save);
    }
}

void
ChartManagerDialog::downClicked()
{
    if (charts->currentItem()) {
        int index = charts->invisibleRootItem()->indexOfChild(charts->currentItem());
        if (index == (charts->invisibleRootItem()->childCount()-1)) return; // its at the bottom already

        // movin on up!
        QTreeWidgetItem *moved;
        charts->invisibleRootItem()->insertChild(index+1, moved=charts->invisibleRootItem()->takeChild(index));
        charts->setCurrentItem(moved);
        LTMSettings save = (*presets)[index];
        presets->removeAt(index);
        presets->insert(index+1, save);
    }
}

void
ChartManagerDialog::renameClicked()
{
    // which one is selected?
    if (charts->currentItem()) charts->editItem(charts->currentItem(), 0);
}

void
ChartManagerDialog::deleteClicked()
{
    // must have at least 1 child
    if (charts->invisibleRootItem()->childCount() == 1) {
        QMessageBox::warning(0, "Error", "You must have at least one chart");
        return;

    } else if (charts->currentItem()) {
        int index = charts->invisibleRootItem()->indexOfChild(charts->currentItem());

        // zap!
        presets->removeAt(index);
        delete charts->invisibleRootItem()->takeChild(index);
    }
}

/*----------------------------------------------------------------------
 * Write to charts.xml
 *--------------------------------------------------------------------*/
void
LTMSettings::writeChartXML(QDir home, QList<LTMSettings> charts)
{
    LTMChartParser::serialize(QString(home.path() + "/charts.xml"), charts);
}


/*----------------------------------------------------------------------
 * Read charts.xml
 *--------------------------------------------------------------------*/

void
LTMSettings::readChartXML(QDir home, QList<LTMSettings> &charts)
{
    QFileInfo chartFile(home.absolutePath() + "/charts.xml");
    QFile chartsFile;

    // if it doesn't exist use our built-in default version
    if (chartFile.exists())
        chartsFile.setFileName(chartFile.filePath());
    else
        chartsFile.setFileName(":/xml/charts.xml");

    QXmlInputSource source( &chartsFile );
    QXmlSimpleReader xmlReader;
    LTMChartParser( handler );
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);
    xmlReader.parse( source );
    charts = handler.getSettings();
}

/*----------------------------------------------------------------------
 * Marshall/Unmarshall to DataStream to store as a QVariant
 *----------------------------------------------------------------------*/
QDataStream &operator<<(QDataStream &out, const LTMSettings &settings)
{
    // all the baisc fields first
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
    out<<int(1);
    out<<settings.metrics.count();
    foreach(MetricDetail metric, settings.metrics) {
        out<<metric.type;
        out<<metric.stack;
        out<<metric.symbol;
        out<<metric.name;
        out<<metric.uname;
        out<<metric.uunits;
        out<<metric.smooth;
        out<<metric.trend;
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
    }
    return out;
}

QDataStream &operator>>(QDataStream &in, LTMSettings &settings)
{
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

    while(counter--) {
        MetricDetail m;
        in>>m.type;
        in>>m.stack;
        in>>m.symbol;
        in>>m.name;
        in>>m.uname;
        in>>m.uunits;
        in>>m.smooth;
        in>>m.trend;
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
        // get a metric pointer (if it exists)
        m.metric = factory.rideMetric(m.symbol);
        settings.metrics.append(m);
    }
    return in;
}
