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
#include "Context.h"
#include "Athlete.h"
#include "RideCache.h"
#include "Settings.h"
#include "cmath"
#include "Units.h" // for MILES_PER_KM
#include "HelpWhatsThis.h"
#include "GoldenCheetah.h"
#include "Perspective.h"
#include "SpecialFields.h"

#include <QtGui>
#include <QString>

#include <qwt_plot_panner.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_marker.h>

TreeMapWindow::TreeMapWindow(Context *context) :
            GcChartWindow(context), context(context), active(false), dirty(true), useCustom(false), useToToday(false)
{
    // the controls
    QWidget *c = new QWidget;
    HelpWhatsThis *helpConfig = new HelpWhatsThis(c);
    c->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::ChartTrends_CollectionTreeMap));
    QFormLayout *cl = new QFormLayout(c);
    setControls(c);

    // the plot
    mainLayout = new QVBoxLayout;

    treemapPlot = new TreeMapPlot(this, context);
    treemapPlot->setVisible(true);
    mainLayout->addWidget(treemapPlot);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
    setChartLayout(mainLayout);

    setIsBlank(false);

    HelpWhatsThis *helpLTMPlot = new HelpWhatsThis(treemapPlot);
    treemapPlot->setWhatsThis(helpLTMPlot->getWhatsThisText(HelpWhatsThis::ChartTrends_CollectionTreeMap));

    // read metadata.xml
    QString filename = QDir(gcroot).canonicalPath()+"/metadata.xml";
    QString colorfield;
    if (!QFile(filename).exists()) filename = ":/xml/metadata.xml";
    RideMetadata::readXML(filename, keywordDefinitions, fieldDefinitions, colorfield, defaultDefinitions);

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
    ltmPopup = new LTMPopup(context);
    QVBoxLayout *popupLayout = new QVBoxLayout();
    popupLayout->addWidget(ltmPopup);
    popup->setLayout(popupLayout);

    // controls
    field1 = new QComboBox(this);
    addTextFields(field1);
    field2 = new QComboBox(this);
    addTextFields(field2);

    cl->addRow(new QLabel(tr("First")), field1);
    cl->addRow(new QLabel(tr("Second")), field2);
    cl->addRow(new QLabel("")); // spacing

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
    QTextEdit title;
    for (int i = 0; i < factory.metricCount(); ++i) {

        // don't let user configure using compatibility metrics - they
        // are deprecated, but here to support old charts.
        if (factory.metricName(i).startsWith("compatibility_")) continue;

        QTreeWidgetItem *add;
        add = new QTreeWidgetItem(allMetrics, METRIC_TYPE);

        // I didn't define this API with name referring to a symbol in the factory
        // I know it is confusing, but changing it will mean changing it absolutely
        // everywhere. Just remember - in the factory "name" refers to symbol and
        // if you want the user friendly metric description you get it via the metric
        title.setText(factory.rideMetric(factory.metricName(i))->name()); // to handle HTML
        add->setText(0, title.toPlainText()); // long name
        add->setText(1, factory.metricName(i)); // symbol (hidden)

        // by default use workout_time
        if (factory.metricName(i) == "workout_time") allMetrics->child(i)->setSelected(true);
    }
    metricTree->expandItem(allMetrics);
    cl->addRow(new QLabel(tr("Metric")), metricTree);
    dateSetting = new DateSettingsEdit(this);
    cl->addRow(new QLabel("")); // spacing
    cl->addRow(new QLabel(tr("Date range")), dateSetting);
    cl->addRow(new QLabel("")); // spacing

    // chart settings changed
    connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
    connect(metricTree,SIGNAL(itemSelectionChanged()), this, SLOT(metricTreeWidgetSelectionChanged()));
    connect(field1, SIGNAL(currentIndexChanged(int)), this, SLOT(fieldSelected(int)));
    connect(field2, SIGNAL(currentIndexChanged(int)), this, SLOT(fieldSelected(int)));

    // config changes or ride file activities cause a redraw/refresh (but only if active)
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh(void)));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh(void)));
    connect(context, SIGNAL(filterChanged()), this, SLOT(refresh(void)));
    connect(context, SIGNAL(homeFilterChanged()), this, SLOT(refresh(void)));
    connect(this, SIGNAL(perspectiveFilterChanged(QString)), this, SLOT(refresh()));
    connect(this, SIGNAL(perspectiveChanged(Perspective*)), this, SLOT(refresh()));

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(refresh()));

    // user clicked on a cell in the plot
    connect(treemapPlot, SIGNAL(clicked(QString,QString)), this, SLOT(cellClicked(QString,QString)));

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
    treemapPlot->setData(&settings);
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

    setProperty("color", GColor(CTRENDPLOTBACKGROUND));

    // refresh for changes to ridefiles / zones
    if (active == false) {
        // setup settings to current user selection
        foreach(QTreeWidgetItem *metric, metricTree->selectedItems()) {
            if (metric->type() != ROOT_TYPE) {
                QString symbol = metric->text(1);
                settings.symbol = symbol;
            }
        }

        DateRange dr;

        if (useCustom) {
            dr.from = custom.from;
            dr.to = custom.to;
        } else if (useToToday) {
            QDate today = QDate::currentDate();
            dr.from = myDateRange.from;
            dr.to = myDateRange.to > today ? today : myDateRange.to;
        } else {
            dr.from = myDateRange.from;
            dr.to = myDateRange.to;
        }

        // set the specification
        FilterSet fs;
        fs.addFilter(context->isfiltered, context->filters);
        fs.addFilter(context->ishomefiltered, context->homeFilters);
        if (myPerspective) fs.addFilter(myPerspective->isFiltered(), myPerspective->filterlist(dr));
        settings.specification.setFilterSet(fs);
        settings.specification.setDateRange(dr);

        // and the fields to use
        SpecialFields& sp = SpecialFields::getInstance();;
        settings.field1 = sp.internalName(field1->currentText());
        settings.field2 = sp.internalName(field2->currentText());

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
    QStringList match;

    // create a list of activities in this cell
    int count = 0;
    foreach(RideItem *item, context->athlete->rideCache->rides()) {

        // honour the settings
        if (!settings.specification.pass(item)) continue;

        // text may either not exists, then "unknown" or just be "" but f1, f2 don't know ""
        QString x1 = item->getText(settings.field1, tr("(unknown)"));
        QString x2 = item->getText(settings.field2, tr("(unknown)"));
        if (x1 == "") x1 = tr("(unknown)");
        if (x2 == "") x2 = tr("(unknown)");

        // match !
        if (x1 == f1 && x2 == f2) {
            match << item->fileName;
            count++;
        }
    }

    // create a specification for ours
    Specification spec;
    spec.setDateRange(settings.specification.dateRange());
    FilterSet fs = settings.specification.filterSet();
    fs.addFilter(true, match);
    spec.setFilterSet(fs);

    // and the metric to display
    const RideMetricFactory &factory = RideMetricFactory::instance();
    const RideMetric *metric = factory.rideMetric(settings.symbol);

    ltmPopup->setData(spec, metric, QString(tr("%1 activities")).arg(count));
    popup->show();

}

void
TreeMapWindow::addTextFields(QComboBox *combo)
{
    combo->addItem(tr("None")); // if "None" is changed to not being first any more, adjust public methods f1,f2,setf1,setf2
    SpecialFields& sp = SpecialFields::getInstance();
    foreach (FieldDefinition x, fieldDefinitions) {
        switch (x.type) {
            case GcFieldType::FIELD_TEXT:
            case GcFieldType::FIELD_TEXTBOX:
            case GcFieldType::FIELD_SHORTTEXT:
            case GcFieldType::FIELD_INTEGER:
                combo->addItem(sp.displayName(x.name));
                break;
            default:
                break;
        }
    }
}

