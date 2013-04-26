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

#include "TreeMapWindow.h"
#include "LTMTool.h"
#include "TreeMapPlot.h"
#include "LTMSettings.h"
#include "MainWindow.h"
#include "SummaryMetrics.h"
#include "Settings.h"
#include "math.h"
#include "Units.h" // for MILES_PER_KM

#include <QtGui>
#include <QString>

#include <qwt_plot_panner.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_marker.h>

TreeMapWindow::TreeMapWindow(MainWindow *parent, bool useMetricUnits, const QDir &home) :
            GcWindow(parent), main(parent), home(home),
            useMetricUnits(useMetricUnits), active(false), dirty(true), useCustom(false), useToToday(false)
{
    setInstanceName("Treemap Window");

    // the plot
    mainLayout = new QVBoxLayout;
    ltmPlot = new TreeMapPlot(this, main, home);
    mainLayout->addWidget(ltmPlot);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
    setLayout(mainLayout);

    // the controls
    QWidget *c = new QWidget;
    QFormLayout *cl = new QFormLayout(c);
    setControls(c);

    // read metadata.xml
    QString filename = main->home.absolutePath()+"/metadata.xml";
    QString colorfield;
    if (!QFile(filename).exists()) filename = ":/xml/metadata.xml";
    RideMetadata::readXML(filename, keywordDefinitions, fieldDefinitions, colorfield);

    //title = new QLabel(this);
    //QFont font;
    //font.setPointSize(14);
    //font.setWeight(QFont::Bold);
    //title->setFont(font);
    //title->setFixedHeight(20);
    //title->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    // widgets

    // setup the popup widget
    popup = new GcPane();
    ltmPopup = new LTMPopup(main);
    QVBoxLayout *popupLayout = new QVBoxLayout();
    popupLayout->addWidget(ltmPopup);
    popup->setLayout(popupLayout);

    // controls
    field1 = new QComboBox(this);
    addTextFields(field1);
    field2 = new QComboBox(this);
    addTextFields(field2);

    cl->addRow(new QLabel("First"), field1);
    cl->addRow(new QLabel("Second"), field2);
    cl->addRow(new QLabel(tr(""))); // spacing

    // metric selector .. just ride metrics
    metricTree = new QTreeWidget;
#ifdef Q_OS_MAC
    metricTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    metricTree->setColumnCount(1);
    metricTree->setSelectionMode(QAbstractItemView::SingleSelection);
    metricTree->header()->hide();
    //metricTree->setFrameStyle(QFrame::NoFrame);
    //metricTree->setAlternatingRowColors (true);
    metricTree->setIndentation(5);
    allMetrics = new QTreeWidgetItem(metricTree, ROOT_TYPE);
    allMetrics->setText(0, tr("Metric"));
    metricTree->setContextMenuPolicy(Qt::CustomContextMenu);

    // initialise the metrics catalogue and user selector
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i = 0; i < factory.metricCount(); ++i) {

        QTreeWidgetItem *add;
        add = new QTreeWidgetItem(allMetrics, METRIC_TYPE);

        // I didn't define this API with name referring to a symbol in the factory
        // I know it is confusing, but changing it will mean changing it absolutely
        // everywhere. Just remember - in the factory "name" refers to symbol and
        // if you want the user friendly metric description you get it via the metric
        QString title = factory.rideMetric(factory.metricName(i))->name();
        add->setText(0, title); // long name
        add->setText(1, factory.metricName(i)); // symbol (hidden)

        // by default use workout_time
        if (factory.metricName(i) == "workout_time") allMetrics->child(i)->setSelected(true);
    }
    metricTree->expandItem(allMetrics);
    cl->addRow(new QLabel("Metric"), metricTree);
    dateSetting = new DateSettingsEdit(this);
    cl->addRow(new QLabel(tr(""))); // spacing
    cl->addRow(new QLabel("Date range"), dateSetting);
    cl->addRow(new QLabel(tr(""))); // spacing

    // chart settings changed
    connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
    connect(metricTree,SIGNAL(itemSelectionChanged()), this, SLOT(metricTreeWidgetSelectionChanged()));
    connect(field1, SIGNAL(currentIndexChanged(int)), this, SLOT(fieldSelected(int)));
    connect(field2, SIGNAL(currentIndexChanged(int)), this, SLOT(fieldSelected(int)));

    // config changes or ride file activities cause a redraw/refresh (but only if active)
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(main, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh(void)));
    connect(main, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh(void)));
    connect(main, SIGNAL(filterChanged(QStringList&)), this, SLOT(refresh(void)));

    connect(main, SIGNAL(configChanged()), this, SLOT(refresh()));

    // user clicked on a cell in the plot
    connect(ltmPlot, SIGNAL(clicked(QString,QString)), this, SLOT(cellClicked(QString,QString)));

    // date settings
    connect(dateSetting, SIGNAL(useCustomRange(DateRange)), this, SLOT(useCustomRange(DateRange)));
    connect(dateSetting, SIGNAL(useThruToday()), this, SLOT(useThruToday()));
    connect(dateSetting, SIGNAL(useStandardRange()), this, SLOT(useStandardRange()));

    // lets refresh / setup state
    refresh();
}

TreeMapWindow::~TreeMapWindow()
{
    delete popup;
}

void
TreeMapWindow::rideSelected()
{
}

void
TreeMapWindow::refreshPlot()
{
    ltmPlot->setData(&settings);
}

void
TreeMapWindow::useCustomRange(DateRange range)
{
    // plot using the supplied range
    useCustom = true;
    useToToday = false;
    custom = range;
    dateRangeChanged(custom);
}

void
TreeMapWindow::useStandardRange()
{
    useToToday = useCustom = false;
    dateRangeChanged(myDateRange);
}

void
TreeMapWindow::useThruToday()
{
    // plot using the supplied range
    useCustom = false;
    useToToday = true;
    custom = myDateRange;
    if (custom.to > QDate::currentDate()) custom.to = QDate::currentDate();
    dateRangeChanged(custom);
}

// total redraw, reread data etc
void
TreeMapWindow::refresh()
{
    if (!amVisible()) return;

    // refresh for changes to ridefiles / zones
    if (active == false) {
        // if config has changed get new useMetricUnits
        useMetricUnits = main->useMetricUnits;

        // setup settings to current user selection
        foreach(QTreeWidgetItem *metric, metricTree->selectedItems()) {
            if (metric->type() != ROOT_TYPE) {
                QString symbol = metric->text(1);
                settings.symbol = symbol;
            }
        }

        if (useCustom) {
            settings.from = custom.from;
            settings.to = custom.to;
        } else if (useToToday) {
            QDate today = QDate::currentDate();
            settings.from = myDateRange.from;
            settings.to = myDateRange.to > today ? today : myDateRange.to;
        } else {
            settings.from = myDateRange.from;
            settings.to = myDateRange.to;
        }
        settings.field1 = field1->currentText();
        settings.field2 = field2->currentText();
        settings.data = &results;

        // get the data
        results.clear(); // clear any old data
        results = main->metricDB->getAllMetricsFor(QDateTime(settings.from, QTime(0,0,0)),
                                                   QDateTime(settings.to, QTime(0,0,0)));

        refreshPlot();
    }
    repaint(); // get title repainted
}

void
TreeMapWindow::metricTreeWidgetSelectionChanged()
{
    refresh();
}

void
TreeMapWindow::dateRangeChanged(DateRange)
{
    refresh();
}

void
TreeMapWindow::fieldSelected(int)
{
    refresh();
}

void
TreeMapWindow::cellClicked(QString f1, QString f2)
{
    QList<SummaryMetrics> cell;

    // create a list of activities in this cell
    int count = 0;
    foreach(SummaryMetrics x, results) {
        if (x.getText(settings.field1, "(unknown)") == f1 &&
            x.getText(settings.field2, "(unknown)") == f2) {
            cell.append(x);
            count++;
        }
    }

    const RideMetricFactory &factory = RideMetricFactory::instance();
    const RideMetric *metric = factory.rideMetric(settings.symbol);

    ltmPopup->setData(cell, metric, QString("%1 ride%2").arg(count).arg(count == 1 ? "" : "s"));
    popup->show();

}

void
TreeMapWindow::addTextFields(QComboBox *combo)
{
    combo->addItem("None");
    foreach (FieldDefinition x, fieldDefinitions) {
        if (x.type < 4) combo->addItem(x.name);
    }
}
