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

#include "Athlete.h"
#include "Context.h"
#include "Colors.h"
#include "RideCache.h"
#include "RideCacheModel.h"
#include "RideItem.h"
#include "RideNavigator.h"
#include "RideNavigatorProxy.h"
#include "SearchFilterBox.h"
#include "AbstractView.h"
#include "SpecialFields.h"
#include "HelpWhatsThis.h"
#include "IconManager.h"

#include <QtGui>
#include <QString>
#include <QTreeView>
#include <QStyle>
#include <QStyleFactory>
#include <QScrollBar>
#include <QToolTip>


//////////////////////////////////////////////////////////////////////////////
// static declarations

class groupRange;

static QList<groupRange> groupRanges;
static bool _initGroupRanges = false;

static constexpr int kRowPaddingLeft = 2;
static constexpr int kRowPaddingRight = 8;
static constexpr int kFieldsTextIndent = 8;
static constexpr int kFieldsTextPaddingTop = 6;
static constexpr int kSummaryMarginTop = 8;
static constexpr int kSummaryMarginBottom = 12;
static constexpr int kSummaryIconTextGap = 8;
static constexpr int kHeaderSectionPadding = 5;
static constexpr int kIconSpacing = 0;

static bool insensitiveLessThan(const QString &a, const QString &b);


//////////////////////////////////////////////////////////////////////////////
// groupRange

//
// This function is called for every row in the ridecache
// and wants to know what group string or 'name' you want
// to put this row into. It is passed the heading value
// as a string, and the row value for this column.
//
// It should return a string that will be used in the first
// column tree to group rows together.
//
// It is intended to allow us to do clever groupings, such
// as grouping dates as 'This week', 'Last week' etc or
// Power values into zones etc.
//
class groupRange {
    public:
    class range {
        public:
        double low, high;
        QString name;
        range(double low, double high, QString name) : low(low), high(high), name(name) {}
        range() : low(0), high(0), name("") {}
    };

    QString column;         // column name
    QList<range> ranges;    // list of ranges we can put them in
};


//////////////////////////////////////////////////////////////////////////////
// GroupByModel

bool
GroupByModel::initGroupRanges()
{
    groupRange::range add;
    groupRange addColumn;

    // BikeStress
    addColumn.column = "BikeStress";
    add = groupRange::range( 0.0,  0.0, tr("Zero or not present")); addColumn.ranges << add;
    add = groupRange::range( 0,  150, tr("Low Stress")); addColumn.ranges << add;
    add = groupRange::range( 150,  300, tr("Medium Stress")); addColumn.ranges << add;
    add = groupRange::range( 300,  450, tr("High Stress")); addColumn.ranges << add;
    add = groupRange::range( 450,  0.00, tr("Very High Stress")); addColumn.ranges << add;

    groupRanges << addColumn;
    addColumn.ranges.clear();

    // Intensity Factor
    addColumn.column = "BikeIntensity";
    add = groupRange::range( 0.0,  0.0, tr("Zero or not present")); addColumn.ranges << add;
    add = groupRange::range( 0.0,  0.55, tr("Active Recovery")); addColumn.ranges << add;
    add = groupRange::range( 0.55,  0.75, tr("Endurance")); addColumn.ranges << add;
    add = groupRange::range( 0.75,  0.90, tr("Tempo")); addColumn.ranges << add;
    add = groupRange::range( 0.90, 1.05, tr("Threshold")); addColumn.ranges << add;
    add = groupRange::range( 1.05, 1.2, tr("VO2Max")); addColumn.ranges << add;
    add = groupRange::range( 1.2, 1.5, tr("Anaerobic Capacity")); addColumn.ranges << add;
    add = groupRange::range( 1.5,  0.0, tr("Maximal")); addColumn.ranges << add;

    groupRanges << addColumn;
    addColumn.ranges.clear();

    // Variability Index
    addColumn.column = "VI";
    add = groupRange::range( 0.0,  1.0, tr("Zero or not present")); addColumn.ranges << add;
    add = groupRange::range( 1.0,  1.05, tr("Isopower")); addColumn.ranges << add;
    add = groupRange::range( 1.05, 1.1, tr("Steady")); addColumn.ranges << add;
    add = groupRange::range( 1.1,  1.2, tr("Variable")); addColumn.ranges << add;
    add = groupRange::range( 1.2,  0.0, tr("Highly Variable")); addColumn.ranges << add;

    groupRanges << addColumn;
    addColumn.ranges.clear();

    // Duration (seconds)
    addColumn.column = "Duration";
    add = groupRange::range( 0.0,  3600.0, tr("Less than an hour")); addColumn.ranges << add;
    add = groupRange::range( 3600,  5400, tr("Less than 90 minutes")); addColumn.ranges << add;
    add = groupRange::range( 5400, 10800, tr("Less than 3 hours")); addColumn.ranges << add;
    add = groupRange::range( 10800,  18000, tr("Less than 5 hours")); addColumn.ranges << add;
    add = groupRange::range( 18000,  0.0, tr("More than 5 hours")); addColumn.ranges << add;

    groupRanges << addColumn;
    addColumn.ranges.clear();

    // Distance (km)
    addColumn.column = "Distance";
    add = groupRange::range( 0.0,  0.0, tr("Zero or not present")); addColumn.ranges << add;
    add = groupRange::range( 0.0,  40.0, tr("Short")); addColumn.ranges << add;
    add = groupRange::range( 40,  80.00, tr("Medium")); addColumn.ranges << add;
    add = groupRange::range( 80,  140, tr("Long")); addColumn.ranges << add;
    add = groupRange::range( 140,  0, tr("Very Long")); addColumn.ranges << add;

    groupRanges << addColumn;
    addColumn.ranges.clear();

    return true;
}

// Perhaps a groupName function on the metrics would be useful
QString
GroupByModel::groupFromValue(QString headingName, QString value, double rank, double count) const
{
    if (!_initGroupRanges)
        _initGroupRanges = initGroupRanges();
    // Check for predefined thresholds / zones / bands for this metric/column
    foreach (groupRange orange, groupRanges) {
        if (orange.column == headingName) {

            double number = value.toDouble();
            // use thresholds defined for this column/metric
            foreach(groupRange::range range, orange.ranges) {

                // 0-x is lower, x-0 is upper, 0-0 is no data and x-x is a range
                if (range.low == 0.0 && range.high == 0.0 && number == 0.0) return range.name;
                else if (range.high != 0.0 && range.low == 0.0 && number < range.high) return range.name;
                else if (range.low != 0.0 && range.high == 0.0 && number >= range.low) return range.name;
                else if (number < range.high && number >= range.low) return range.name;
            }
            return tr("Undefined");
        }
    }

    // Use upper quartile for anything left that is a metric
    if (rideNavigator->columnMetrics.value(headingName, NULL) != NULL) {

        double quartile = rank / count;

        if (value.toDouble() == 0) return QString(tr("Zero or not present"));
        else if (rank < 10) return QString(tr("Best 10"));
        else if (quartile <= 0.25) return QString (tr("Quartile 1:  0% -  25%"));
        else if (quartile <= 0.50) return QString (tr("Quartile 2: 25% -  50%"));
        else if (quartile <= 0.75) return QString (tr("Quartile 3: 50% -  75%"));
        else if (quartile <= 1) return QString    (tr("Quartile 4: 75% - 100%"));

    } else {

        if (headingName == tr("Date")) {

            // get the date from value string
            QDateTime dateTime = QDateTime::fromString(value, Qt::ISODate);
            QDateTime today = QDateTime::currentDateTime();

            if (today.date().weekNumber() == dateTime.date().weekNumber()
                && today.date().year() == dateTime.date().year())
                return tr("This week");
            else if (today.date().month() == dateTime.date().month()
                    && today.date().year() == dateTime.date().year())
                return tr("This month");
            else if (today.date().month() == (dateTime.date().month()+1)
                    && today.date().year() == dateTime.date().year())
                return tr("Last month");
            else {
                return dateTime.toString(tr("yyyy-MM (MMMM)"));
            }
        }

        // not a metric, i.e. metadata
        return value;
    }

    // if all else fails just return the value and group by
    // that. Which is a fair approach for text fields for example
    return value;
}


//////////////////////////////////////////////////////////////////////////////
// RideNavigator

RideNavigator::RideNavigator(Context *context, bool mainwindow) : GcChartWindow(context), context(context), active(false), _groupBy(-1)
{
    // get column headings
    // default column layouts etc
    _columns = QString(tr("*|Workout Code|Date|"));
    _widths = QString("0|100|100|");
    _sortByIndex = 2;
    _sortByOrder = 0;
    currentColumn = -1;
    this->mainwindow = mainwindow;
    _groupBy = -1;
    currentItem = NULL;

    init = false;

    mainLayout = new QVBoxLayout;
    setChartLayout(mainLayout);
    mainLayout->setSpacing(0);
    if (mainwindow) mainLayout->setContentsMargins(5*dpiXFactor,0,0,0);
    else mainLayout->setContentsMargins(2,2,2,2); // so we can resize!

    searchFilter = new SearchFilter(this);
    searchFilter->setSourceModel(context->athlete->rideCache->model()); // filter out/in search results


    groupByModel = new GroupByModel(this);
    groupByModel->setSourceModel(searchFilter);

    sortModel = new RideNavigatorSortProxyModel(this);
    sortModel->setSourceModel(groupByModel);
    sortModel->setDynamicSortFilter(true);

    if (!mainwindow) {
        searchFilterBox = new SearchFilterBox(this, context, false);
        mainLayout->addWidget(searchFilterBox);
        HelpWhatsThis *searchHelp = new HelpWhatsThis(searchFilterBox);
        searchFilterBox->setWhatsThis(searchHelp->getWhatsThisText(HelpWhatsThis::SearchFilterBox));
    }

    // get setup
    tableView = new ActivityTreeView(this);
    delegate = new ActivityItemDelegate(this);
    tableView->setItemDelegate(delegate);
    tableView->setAnimated(true);
    tableView->setModel(sortModel);
    tableView->setSortingEnabled(true);
    tableView->setAlternatingRowColors(false);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers); // read-only
    mainLayout->addWidget(tableView);
    tableView->expandAll();
    tableView->header()->setCascadingSectionResizes(true); // easier to resize this way
    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    tableView->header()->setStretchLastSection(false);
    tableView->header()->setMinimumSectionSize(0);
    tableView->header()->setFocusPolicy(Qt::NoFocus);
#ifdef Q_OS_WIN
    QStyle *cde = QStyleFactory::create(OS_STYLE);
    tableView->verticalScrollBar()->setStyle(cde);
#endif
#ifdef Q_OS_MAC
    tableView->header()->setSortIndicatorShown(false); // blue looks nasty
    tableView->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    tableView->installEventFilter(this);
    tableView->viewport()->installEventFilter(this);
    tableView->setFrameStyle(QFrame::NoFrame);
    tableView->setAcceptDrops(true);
    tableView->setColumnWidth(1, 100 *dpiXFactor);

    HelpWhatsThis *helpTableView = new HelpWhatsThis(tableView);
    if (mainwindow)
        tableView->setWhatsThis(helpTableView->getWhatsThisText(HelpWhatsThis::SideBarRidesView_Rides));
    else
        tableView->setWhatsThis(helpTableView->getWhatsThisText(HelpWhatsThis::ChartDiary_Navigator));

    // good to go
    tableView->show();
    resetView();

    // refresh when config changes (metric/imperial?)
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // refresh when rides added/removed
    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh()));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(rideDeleted(RideItem*)));
    connect(context, SIGNAL(rideSelected(RideItem*)), this, SLOT(setRide(RideItem*)));

    // user selected a ride on the ride list, we should reflect that too..
    connect(tableView, SIGNAL(rowSelected(QItemSelection)), this, SLOT(selectionChanged(QItemSelection)));

    // user hit return or double clicked a ride !
    connect(tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectRide(QModelIndex)));

    // user moved columns
    connect(tableView->header(), SIGNAL(sectionMoved(int,int,int)), this, SLOT(columnsChanged()));
    connect(tableView->header(), SIGNAL(sectionResized(int,int,int)), this, SLOT(columnsResized(int, int, int)));

    // user sorted by column
    connect(tableView->header(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), this, SLOT(cursorRide()));
    connect(tableView,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showTreeContextMenuPopup(const QPoint &)));
    connect(tableView->header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(setSortBy(int,Qt::SortOrder)));

    // repaint etc when background refresh is working
    connect(context, SIGNAL(refreshStart()), this, SLOT(backgroundRefresh()));
    connect(context, SIGNAL(refreshEnd()), this, SLOT(backgroundRefresh()));
    connect(context, SIGNAL(refreshUpdate(QDate)), this, SLOT(backgroundRefresh())); // we might miss 1st one

    if (!mainwindow) {
        connect(searchFilterBox, SIGNAL(searchResults(QStringList)), this, SLOT(searchStrings(QStringList)));
        connect(searchFilterBox, SIGNAL(searchClear()), this, SLOT(clearSearch()));
    }

    // we accept drag and drop operations
    setAcceptDrops(true);

    // lets go
    configChanged(CONFIG_APPEARANCE | CONFIG_NOTECOLOR | CONFIG_FIELDS);
}

RideNavigator::~RideNavigator()
{
    delete tableView;
    delete groupByModel;
}

void
RideNavigator::configChanged(qint32 state)
{
    bool hasCalendarText = GlobalContext::context()->rideMetadata->hasCalendarText();

#ifndef Q_OS_MAC
    tableView->setStyleSheet(AbstractView::ourStyleSheet());
    if (mainwindow) {
        if (appsettings->value(this, GC_RIDESCROLL, true).toBool() == false) {
            tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        } else {
            tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        tableView->header()->show();

        QColor headerBg = GColor(CPLOTBACKGROUND);
        QString headerFormat = QString(
            "QHeaderView {"
            "    background-color: %1;"
            "    color: %2;"
            "}"
            "QHeaderView::section {"
            "    background-color: %1;"
            "    color: %2;"
            "    border: none;"
            "    padding: %3px;"
            "    font-weight: 700;"
            "}")
            .arg(headerBg.name())
            .arg(GCColor::invertColor(headerBg).name())
            .arg(kHeaderSectionPadding * dpiXFactor);
        tableView->header()->setStyleSheet(headerFormat);
    }

#endif
    tableView->setStyleSheet(tableView->styleSheet() + "QTreeView::item:hover { background: transparent; color: inherit; }");
    int summaryLines = appsettings->value(this, GC_SUMMARYROWS, 3).toInt();
    if (! hasCalendarText) {
        summaryLines = 0;
    }
    tableView->setSummaryLines(summaryLines);
    delegate->setSummaryLines(summaryLines);

    // if the fields changed we need to reset indexes etc
    if (state & CONFIG_FIELDS) resetView();

    refresh();
}

void
RideNavigator::rideDeleted(RideItem*item)
{
    if (currentItem == item) currentItem = NULL;
    refresh();
}

void
RideNavigator::refresh()
{
    active=false;

    setWidth(geometry().width());
    cursorRide();
}

void
RideNavigator::backgroundRefresh()
{
    tableView->doItemsLayout();
}

void
RideNavigator::resizeEvent(QResizeEvent*)
{
    // ignore if main window .. we get told to resize
    // by the splitter mover
    if (mainwindow) return;

    setWidth(geometry().width());
}

void
RideNavigator::resetView()
{
    active = true;

    QList<QString> cols = _columns.split("|", Qt::SkipEmptyParts);
    int widco = _widths.split("|", Qt::SkipEmptyParts).count();

    // something is wrong with the config ? reset 
    if (widco != cols.count() || widco <= 1) {
        _columns = QString(tr("*|Workout Code|Date|"));
        _widths = QString("0|100|100|");
        cols = _columns.split("|", Qt::SkipEmptyParts);
    }

    // to account for translations
    QMap <QString, QString> internalNameMap;

    nameMap.clear();
    columnMetrics.clear();

    // add the standard columns to the map
    nameMap.insert("filename", tr("File"));
    internalNameMap.insert("File", tr("File"));
    nameMap.insert("timestamp", tr("Last updated"));
    internalNameMap.insert("Last updated", tr("Last updated"));
    nameMap.insert("ride_date", tr("Date"));
    internalNameMap.insert("Date", tr("Date"));
    nameMap.insert("ride_time", tr("Time")); // virtual columns show time from ride_date
    internalNameMap.insert("Time", tr("Time"));
    nameMap.insert("fingerprint", tr("Config Checksum"));
    internalNameMap.insert("Config Checksum", tr("Config Checksum"));

    // add metrics to the map
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++) {
        QString converted = QTextEdit(factory.rideMetric(factory.metricName(i))->name()).toPlainText();

        // from sql column name to friendly metric name
        nameMap.insert(QString("%1").arg(factory.metricName(i)), converted);

        // from (english) internalName to (translated) Name
        internalNameMap.insert(factory.rideMetric(factory.metricName(i))->internalName(), converted);

        // from friendly metric name to metric pointer
        columnMetrics.insert(converted, factory.rideMetric(factory.metricName(i)));
    }

    // add metadata fields...
    SpecialFields& sp = SpecialFields::getInstance(); // all the special fields are in here...
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {
        if (!sp.isMetric(field.name) && (field.type != GcFieldType::FIELD_DATE && field.type != GcFieldType::FIELD_TIME)) {
            nameMap.insert(QString("%1").arg(sp.makeTechName(field.name)), sp.displayName(field.name));
            internalNameMap.insert(field.name, sp.displayName(field.name));
        }
    }

    // cols list needs to be mapped to match logicalHeadings
    for (int i = 0; i < cols.count(); i++)
        cols[i] = internalNameMap.value(cols[i], cols[i]);

    logicalHeadings.clear();
    tableView->reset();
    tableView->header()->reset();

    // setup the logical heading list
    for (int i=0; i<tableView->header()->count(); i++) {
        QString friendly, techname = sortModel->headerData(i, Qt::Horizontal).toString();

        if ((friendly = nameMap.value(techname, "unknown")) != "unknown") {
            sortModel->setHeaderData(i, Qt::Horizontal, friendly);
            logicalHeadings << friendly;
        } else
            logicalHeadings << techname;
    }

    // hide everything, we show what we want later
    for (int i=0; i<tableView->header()->count(); i++) {
        int index = tableView->header()->logicalIndex(i);
        tableView->setColumnHidden(index, true);
        tableView->setColumnWidth(index, 0);
    }

    // now re-order the columns according to the
    // prevailing preferences. They are listed in
    // the order we want them, column zero is the
    // group by column, so we leave that alone
    for(int i=1; i<cols.count(); i++) {
        tableView->header()->moveSection(tableView->header()->visualIndex(logicalHeadings.indexOf(cols[i])), i);
    }

    // initialise to whatever groupBy we want to start with
    tableView->sortByColumn(sortByIndex(), static_cast<Qt::SortOrder>(sortByOrder()));;

    tableView->setColumnHidden(0, true);
    tableView->setColumnWidth(0,0);

    // set the column widths, column zero is the
    // group by column, so we always set that to zero width.
    int columnnumber=0;
    foreach(QString size, _widths.split("|", Qt::SkipEmptyParts)) {

        if (columnnumber >= cols.count()) break;

        int index = tableView->header()->logicalIndex(columnnumber);
        tableView->setColumnHidden(index, false);
        tableView->setColumnWidth(index, columnnumber ? size.toInt() : 0);
        columnnumber++;
    }

    setGroupByColumn();

    active = false;

    resizeEvent(NULL);

    // Select the current ride
    cursorRide();

    // get height
    tableView->doItemsLayout();

    columnsChanged();
}

void RideNavigator::searchStrings(QStringList list)
{
    searchFilter->setStrings(list);
    setWidth(geometry().width());
}

void RideNavigator::clearSearch()
{
    searchFilter->clearStrings();
    QApplication::processEvents(); // repaint/resize list view - scrollbar..
    setWidth(geometry().width());  // before we update column sizes!
}

void
RideNavigator::setDisplayFilter(RideNavFilter filter)
{
    searchFilter->setDisplayFilter(filter);
    QApplication::processEvents(); // repaint/resize list view - scrollbar..
}

void RideNavigator::setWidth(int x)
{
    // use helper function
    setColumnWidth(x, false);

}

// make sure the columns are all neat and tidy when the ride navigator is shown
void
RideNavigator::showEvent(QShowEvent *)
{
    init = true;

    //setWidth(geometry().width());
}

// routines called by the sidebar to let the user
// update the columns/grouping without using right-click
QStringList
RideNavigator::columnNames() const
{
    return visualHeadings;
}

void
RideNavigator::setGroupByColumnName(QString name)
{
    if (name == "") {

        noGroups();

    } else {

        int logical = logicalHeadings.indexOf(name);
        if (logical >= 0) {
            currentColumn = logical;
            setGroupByColumn();
        }
    }
}

void
RideNavigator::columnsChanged()
{
    // do the work - (column changed, but no "inWidget" column resize)
    calcColumnsChanged(false);
}

void
RideNavigator::columnsResized(int logicalIndex, int oldSize, int newSize)
{

    // do the work - resize only
    calcColumnsChanged(true, logicalIndex, oldSize, newSize);
}

bool
RideNavigator::eventFilter(QObject *object, QEvent *e)
{
    // not for the table?
    if (object != (QObject *)tableView) return false;

    // what happnned?
    switch(e->type())
    {
        case QEvent::ContextMenu:
        {
            //borderMenu(((QMouseEvent *)e)->pos());
            borderMenu(tableView->mapFromGlobal(QCursor::pos()));
            return true; // I'll take that thanks
            break;
        }
        case QEvent::KeyPress:
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
            if (keyEvent->modifiers() & Qt::ControlModifier) {

                // Ctrl+Key
                switch (keyEvent->key()) {

                    case Qt::Key_C: // defacto standard for copy
                        return true;

                    case Qt::Key_V: // defacto standard for paste
                        return true;

                    case Qt::Key_X: // defacto standard for cut
                        return true;

                    case Qt::Key_Y: // emerging standard for redo
                         return true;

                    case Qt::Key_Z: // common standard for undo
                         return true;

                    case Qt::Key_0:
                        return true;

                    default:
                        return false;
                }
            } else {

                // Not Ctrl ...
                switch (keyEvent->key()) {

                    case Qt::Key_Return:
                    case Qt::Key_Enter:
                        selectRide(tableView->currentIndex());
                        return true;
                    default:
                        return false;
                }
            }
            break;
        }

        case QEvent::LayoutRequest:
        {
            active=true;
            // set the column widths, column zero is the
            // group by column, so we always set that to zero width.
            int columnnumber=0;
            foreach(QString size, _widths.split("|", Qt::SkipEmptyParts)) {
                tableView->setColumnWidth(columnnumber, columnnumber ? size.toInt() : 0);
                columnnumber++;
            }
            active=false;
            setWidth(geometry().width()); // calculate width...
        }
        break;

        default:
            break;
    }
    return false;
}

void
RideNavigator::borderMenu(const QPoint &pos)
{
    // Which column did we right click on?
    //
    // if not in the border then do nothing, this
    // context menu should only be shown when
    // the user right clicks on a column heading.
    int column=0;
    if (pos.y() <= tableView->header()->height())
        column = tableView->header()->logicalIndexAt(pos);
    else return; // not in the border

    QMenu menu(tableView);

    // reset viaual headings first
    columnsChanged();

    // don't allow user to delete last column!
    // need to also include '*' column 0 wide in count hence 2 not 1
    if (visualHeadings.count() > 2) {
        QAction *delCol = new QAction(tr("Remove Column"), tableView);
        delCol->setEnabled(true);
        menu.addAction(delCol);
        connect(delCol, SIGNAL(triggered()), this, SLOT(removeColumn()));
    }

    QAction *insCol = new QAction(tr("Column Chooser"), tableView);
    insCol->setEnabled(true);
    menu.addAction(insCol);
    connect(insCol, SIGNAL(triggered()), this, SLOT(showColumnChooser()));

    QAction *toggleGroupBy = new QAction(_groupBy >= 0 ? tr("Do Not Show in Groups") : tr("Show In Groups"), tableView);
    toggleGroupBy->setEnabled(column!=1?true:false); // No group for Ride Time
    menu.addAction(toggleGroupBy);
    connect(toggleGroupBy, SIGNAL(triggered()), this, SLOT(setGroupByColumn()));

    // set current column...
    currentColumn = column;
    menu.exec(tableView->mapToGlobal(QPoint(pos.x(), pos.y())));
}

void
RideNavigator::setGroupByColumn()
{
    // toggle
    setGroupBy(_groupBy >= 0 ? -1 : currentColumn);

    // set proxy model
    groupByModel->setGroupBy(_groupBy);

    // lets expand column 0 for the groupBy heading
    for (int i=0; i < groupByModel->groupCount(); i++) {
        tableView->setFirstColumnSpanned (i, QModelIndex(), true);
    }

    // now show em
    tableView->expandAll();

    // reselect current ride - since selectionmodel
    // is changed by setGroupBy()
    cursorRide();
}

void
RideNavigator::setSortBy(int index, Qt::SortOrder order)
{
    _sortByIndex = index;
    _sortByOrder = static_cast<int>(order);
}

void
RideNavigator::calcColumnsChanged(bool resized, int logicalIndex, int oldSize, int newSize ) {

    // double use - for "changing" and "only resizing" of the columns

    if (active == true) return;
    active = true;

    visualHeadings.clear(); // they have moved

    // get the names used
    for (int i=0; i<tableView->header()->count(); i++) {
        if (tableView->header()->isSectionHidden(tableView->header()->logicalIndex(i)) != true) {
            int index = tableView->header()->logicalIndex(i);
            visualHeadings << logicalHeadings[index];
        }
    }
    // write to config
    QString headings;
    foreach(QString name, visualHeadings)
        headings += name + "|";

    _columns = headings;

    // correct width and store result
    setColumnWidth(geometry().width(), resized, logicalIndex, oldSize, newSize); // calculate width...

    // get column widths, column zero is the
    // group by column, so we always set that to zero width.
    QString widths("0|");
    for (int i=1; i<tableView->header()->count(); i++) {
        int index = tableView->header()->logicalIndex(i);
        if (tableView->header()->isSectionHidden(index) != true) {
           widths += QString("%1|").arg(tableView->columnWidth(index));
        }
    }
    _widths = widths;

    active = false;
}

void
RideNavigator::setColumnWidth(int x, bool resized, int logicalIndex, int oldWidth, int newWidth) {

    // double use - for use after any change (e.g. widget size,..) "changing" and "only resizing" of the columns

    if (init == false) return;

    active = true;

#if !defined (Q_OS_MAC)
    if (tableView->verticalScrollBar()->isVisible())
        x -= tableView->verticalScrollBar()->width()
                + 0 ; // !! no longer account for content margins of 3,3,3,3 was + 6
#else // we're on a mac.. so dodgy way of spotting preferences for scrollbars...
    // this is a nasty hack, to see if the 'always on' preference for scrollbars is set we
    // look at the scrollbar width which is 15 in this case (it is 16 when they 'appear' when
    // needed. No doubt this will change over time and need to be fixed by referencing the
    // Mac system preferences via an NSScroller - but that will be a massive hassle.
    if (tableView->verticalScrollBar()->isVisible() && tableView->verticalScrollBar()->width() == 15)
        x -= tableView->verticalScrollBar()->width() + 0 ;
#endif

    // take the margins into account top
    x -= mainLayout->contentsMargins().left() + mainLayout->contentsMargins().right();

    // ** NOTE **
    // When iterating over the section headings we
    // always use the tableview not the sortmodel
    // so we can skip over the virtual column 0
    // which is used to group by, is visible but
    // must have a width of 0. This is why all
    // the for loops start with i=1
    tableView->setColumnWidth(0,0); // in case use grabbed it

    // is it narrower than the headings?
    int headwidth=0;
    for (int i=1; i<tableView->header()->count(); i++)
        if (tableView->header()->isSectionHidden(i) == false) {
            headwidth += tableView->columnWidth(i);
        }

    if (!resized) {

        // headwidth is no, x is to-be width
        // we need to 'stretch' the sections
        // proportionally to fit into new
        // layout
        int setwidth=0;
        int newwidth=0;
        for (int i=1; i<tableView->header()->count(); i++) {
            if (tableView->header()->isSectionHidden(i) == false) {
                newwidth = (double)((((double)tableView->columnWidth(i)/(double)headwidth)) * (double)x);
                if (newwidth < 20) newwidth = 20;
                QString columnName = tableView->model()->headerData(i, Qt::Horizontal).toString();
                if (columnName == "*") newwidth = 0;
                tableView->setColumnWidth(i, newwidth);
                setwidth += newwidth;
            }
        }

        // UGH. Now account for the fact that the smaller columns
        //      didn't take their fair share of a negative resize
        //      so we need to snip off from the larger columns.
        if (setwidth != x) {
            // how many columns we got to snip from?
            int colsleft = 0;
            for (int i=1; i<tableView->header()->count(); i++)
                if (tableView->header()->isSectionHidden(i) == false && tableView->columnWidth(i)>20)
                    colsleft++;

            // run through ... again.. snipping off some pixels
            if (colsleft) {
                int snip = (setwidth-x) / colsleft; //could be negative too
                for (int i=1; i<tableView->header()->count(); i++) {
                    if (tableView->header()->isSectionHidden(i) == false && tableView->columnWidth(i)>20) {
                        tableView->setColumnWidth(i, tableView->columnWidth(i)-snip);
                        setwidth -= snip;
                    }
                }
            }
        }
    } else {

        // columns are resized - for each affected column this function is called
        // and makes sure that
        // a) nothing gets smaller than 20 and
        // b) last section is not moved over the right border / does not fill the widget to the right border

        // first step: make sure that the current column got smaller than 20 by resizing
        if (newWidth < 20) {
            tableView->setColumnWidth(logicalIndex, oldWidth);
            // correct the headwidth by the added space
            headwidth += (oldWidth - newWidth);
        }

        // get the index of the most right column (since here all further resizing will start)
        int visIndex = 0;
        // find the most right visible column
        for (int i=1; i<tableView->header()->count(); i++) {
            if (tableView->header()->isSectionHidden(i) == false &&
                    tableView->header()->visualIndex(i) > visIndex)
                visIndex = tableView->header()->visualIndex(i);
        }

        if (headwidth > x) {
            // now make sure that no column disappears right border of the table view
            // by taking the overlapping part "cut" from last column(s)
            int cut = headwidth - x;
             // now resize, but not smaller than 20 (from right to left of Visible Columns)
            while (cut >0 && visIndex > 0) {
                int logIndex = tableView->header()->logicalIndex(visIndex);
                int k = tableView->columnWidth(logIndex);
                if (k - cut >= 20) {
                    tableView->setColumnWidth(logIndex, k-cut);
                    cut = 0;
                } else {
                    tableView->setColumnWidth(logIndex, 20);
                    cut -= (k-20);
                }
                visIndex--;
            }
        } else {
            // since QT on fast mouse moves resizes more columns then expected
            // give all available space to the last visible column
            int logIndex = tableView->header()->logicalIndex(visIndex);
            int k = tableView->columnWidth(logIndex);
            tableView->setColumnWidth(logIndex, (k+x-headwidth));
        }
    }

    // make the scrollbars go away
    tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    active = false;

}

void
RideNavigator::removeColumn()
{
    active = true;
    tableView->setColumnHidden(currentColumn, true);
    active = false;
    
    setWidth(geometry().width()); // calculate width...
    columnsChanged(); // need to do after, just once
    columnsChanged(); // need to do after, and again
}

void
RideNavigator::showColumnChooser()
{
    ColumnChooser *selector = new ColumnChooser(logicalHeadings);
    selector->show();
}

// user selected a different ride somewhere else, we need to align with that
void
RideNavigator::setRide(RideItem*rideItem)
{
    // if we have a selection and its this one just ignore.
    if (rideItem == NULL || (currentItem == rideItem && tableView->selectionModel()->selectedRows().count() != 0)) return;

    for (int i=0; i<tableView->model()->rowCount(); i++) {

        QModelIndex group = tableView->model()->index(i,0,QModelIndex());
        for (int j=0; j<tableView->model()->rowCount(group); j++) {

            QString fileName = tableView->model()->data(tableView->model()->index(j,3, group), Qt::DisplayRole).toString();
            if (fileName == rideItem->fileName) {
                // we set current index to column 2 (date/time) since we can be guaranteed it is always show (all others are removable)
                QItemSelection row(tableView->model()->index(j,0,group),
                tableView->model()->index(j,tableView->model()->columnCount()-1, group));
                tableView->selectionModel()->select(row, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
                tableView->selectionModel()->setCurrentIndex(tableView->model()->index(j,0,group), QItemSelectionModel::NoUpdate);
                tableView->scrollTo(tableView->model()->index(j,3,group), QAbstractItemView::PositionAtCenter);

                currentItem = rideItem;
                repaint();
                active = false;
                return;
            }
        }
    }
}

void
RideNavigator::selectionChanged(QItemSelection selected)
{
    if (selected.isEmpty()) {
        return;
    }

    QModelIndex ref = selected.indexes().first();
    QModelIndex fileIndex = tableView->model()->index(ref.row(), 3, ref.parent());
    QString filename = tableView->model()->data(fileIndex, Qt::DisplayRole).toString();

    // lets make sure we know what we've selected, so we don't
    // select it twice
    foreach(RideItem *item, context->athlete->rideCache->rides()) {
       if (item->fileName == filename) {
           currentItem = item;
           break;
       }
    }

    // lets notify others
    context->athlete->selectRideFile(filename);

}

void
RideNavigator::selectRide(const QModelIndex &index)
{
    // we don't use this at present, but hitting return
    // or double clicking a ride will cause this to get called....
    QModelIndex fileIndex = tableView->model()->index(index.row(), 3, index.parent()); // column 2 for filename ?
    QString filename = tableView->model()->data(fileIndex, Qt::DisplayRole).toString();

    // do nothing .. but maybe later do something ?
    //context->athlete->selectRideFile(filename);
}

void
RideNavigator::cursorRide()
{
    if (currentItem == NULL) return;

    // find our ride and scroll to it
    for (int i=0; i<tableView->model()->rowCount(); i++) {

        QModelIndex group = tableView->model()->index(i,0,QModelIndex());
        for (int j=0; j<tableView->model()->rowCount(group); j++) {
            QString fileName = tableView->model()->data(tableView->model()->index(j,2, group), GroupByModel::FilenameRole).toString();
            if (fileName == currentItem->fileName) {
                // we set current index to column 2 (date/time) since we can be guaranteed it is always show (all others are removable)
                tableView->scrollTo(tableView->model()->index(j,3,group));
                return;
            }
        }
    }
}

// Drag and drop columns from the chooser...
void
RideNavigator::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->data("application/x-columnchooser") != "")
        event->acceptProposedAction(); // whatever you wanna drop we will try and process!
    else
        event->ignore();
}

void
RideNavigator::dropEvent(QDropEvent *event)
{
    QByteArray rawData = event->mimeData()->data("application/x-columnchooser");
    QDataStream stream(&rawData, QIODevice::ReadOnly);
    stream.setVersion(QDataStream::Qt_4_6);
    QString name;
    stream >> name;

    tableView->setColumnHidden(logicalHeadings.indexOf(name), false);
    tableView->setColumnWidth(logicalHeadings.indexOf(name), 50);
    tableView->header()->moveSection(tableView->header()->visualIndex(logicalHeadings.indexOf(name)), 1);
    columnsChanged();
}

void
RideNavigator::showTreeContextMenuPopup(const QPoint &pos)
{
    // map to global does not take into account the height of the header (??)
    // so we take it off the result of map to global

    // in the past this called mainwindow routinesfor the menu -- that was
    // a bad design since it coupled the ride navigator with the gui
    // we emit signals now, which only the sidebar is interested in trapping
    // so the activity log for example doesn't have a context menu now
    emit customContextMenuRequested(tableView->mapToGlobal(pos+QPoint(0,tableView->header()->geometry().height())));
}


//////////////////////////////////////////////////////////////////////////////
// ColumnChooser

ColumnChooser::ColumnChooser(QList<QString>&logicalHeadings)
{
    // wipe away everything when you close please...
    setWindowTitle(tr("Column Chooser"));
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::Tool);

    clicked = new QSignalMapper(this); // maps each button click event
    connect(clicked, &QSignalMapper::mappedString, this, &ColumnChooser::buttonClicked);

    QVBoxLayout *us = new QVBoxLayout(this);
    us->setSpacing(0);
    us->setContentsMargins(0,0,0,0);
    
    scrollarea = new QScrollArea(this);
    us->addWidget(scrollarea);

    QWidget *but = new QWidget(this);
    buttons = new QVBoxLayout(but);
    buttons->setSpacing(0);
    buttons->setContentsMargins(0,0,0,0);

    QFont smallFont;
    smallFont.setPointSizeF(baseFont.pointSizeF() *0.8f);

    QList<QString> buttonNames = logicalHeadings;
    std::sort(buttonNames.begin(), buttonNames.end(), insensitiveLessThan);

    QString last;
    foreach (QString column, buttonNames) {

        // hide compatibility metrics
        if (RideMetricFactory::instance().compatibilitymetrics.contains(column)) continue;

        // ignore groupby
        if (column == "*") continue;

        // ignore meta fields that are metrics or duplicates
        if (column == last || column.contains("_")) continue;

        // setup button
        QPushButton *add = new QPushButton(column, this);
        add->setFont(smallFont);
        add->setContentsMargins(0,0,0,0);
        buttons->addWidget(add);

        connect(add, SIGNAL(pressed()), clicked, SLOT(map()));
        clicked->setMapping(add, column);

        // for spotting duplicates
        last = column;

    }
    scrollarea->setWidget(but);

    but->setFixedWidth((but->width()+10)* dpiXFactor);
    scrollarea->setFixedWidth((but->width()+20)* dpiXFactor);
    setFixedWidth((but->width()+20)* dpiXFactor);
}

void
ColumnChooser::buttonClicked(QString name)
{
    // setup the drag data
    QMimeData *mimeData = new QMimeData;

    // we need to pack into a byte array (since UTF8() conversion is not reliable in QT4.8 vs QT 5.3)
    QByteArray rawData;
    QDataStream stream(&rawData, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_6);
    stream << name;
    // send raw
    mimeData->setData("application/x-columnchooser", rawData);

    // create a drag event
    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->exec(Qt::MoveAction);
}


//////////////////////////////////////////////////////////////////////////////
// ActivityItemDelegate

ActivityItemDelegate::ActivityItemDelegate
(RideNavigator *rideNavigator, QObject *parent)
: QStyledItemDelegate(parent), rideNavigator(rideNavigator)
{
}


QString
ActivityItemDelegate::formatValue
(const QModelIndex &index) const
{
    QString columnName = rideNavigator->tableView->model()->headerData(index.column(), Qt::Horizontal).toString();
    const RideMetric *m;
    QString value;
    if ((m = rideNavigator->columnMetrics.value(columnName, nullptr)) != nullptr) {
        if (index.model()->data(index, Qt::DisplayRole).metaType().id() == QMetaType::QTime) {
            value = index.model()->data(index, Qt::DisplayRole).toTime().toString("hh:mm:ss");
        } else {
            value = index.model()->data(index, Qt::DisplayRole).toString();
        }
        if (value =="nan" || value == "0" || value == "0.0" || value == "0.00" || value == "00:00:00") {
            value="";
        }
    } else {
        // is this the activity date/time ?
        value = index.model()->data(index, Qt::DisplayRole).toString();
        QLocale locale;
        if (columnName == tr("Date")) {
            QDateTime dateTime = QDateTime::fromString(value, Qt::ISODate);
            value = locale.toString(dateTime.date(), QLocale::NarrowFormat);
        } else if (columnName == tr("Time")) {
            QDateTime dateTime = QDateTime::fromString(value, Qt::ISODate);
            value = locale.toString(dateTime.time(), QLocale::NarrowFormat);
        } else if (columnName == tr("Last updated")) {
            QDateTime dateTime;
            dateTime.setSecsSinceEpoch(index.model()->data(index, Qt::DisplayRole).toInt());
            value = locale.toString(dateTime, QLocale::NarrowFormat);
        }
    }
    return value;
}


QSize
ActivityItemDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const int lineHeight = QFontMetrics(option.font).lineSpacing() + 1;
    int height;
    if (isHeaderRow(index)) {
        height = 2 * kFieldsTextPaddingTop * dpiYFactor + lineHeight;
    } else {
        height =   2 * kFieldsTextPaddingTop * dpiYFactor
                 + lineHeight
                 + std::max(0, std::min(1, summaryLines)) * (kSummaryMarginTop + kSummaryMarginBottom) * dpiYFactor
                 + lineHeight * summaryLines;
    }
    return { option.rect.width(), height };
}


void
ActivityItemDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    QFont font = option.font;
    if (isHeaderRow(index)) {
        font.setWeight(QFont::Bold);
    } else {
        font.setWeight(QFont::DemiBold);
        const bool dirty = index.data(static_cast<int>(GroupByModel::DirtyRole)).toBool();
        font.setItalic(dirty);
    }
    painter->setFont(font);
    painter->setPen(option.palette.text().color());

    const int leftPad = ((option.rect.x() == 0) ? kRowPaddingLeft + kFieldsTextIndent : kRowPaddingLeft) * dpiXFactor;
    QRect titleRect = option.rect.adjusted(leftPad, kFieldsTextPaddingTop * dpiYFactor, -(kRowPaddingRight * dpiXFactor), 0);
    titleRect.setHeight(QFontMetrics(font).height());

    painter->drawText(titleRect, isHeaderRow(index) ? (Qt::AlignLeft | Qt::AlignBottom) : (Qt::AlignLeft | Qt::AlignVCenter), formatValue(index));

    painter->restore();
}


bool
ActivityItemDelegate::helpEvent
(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    Q_UNUSED(option)

    ActivityTreeView *tree = qobject_cast<ActivityTreeView*>(view);
    if (! tree) {
        return false;
    }
    QList<QModelIndex> indexes = tree->toolTipIndexes(index);
    if (! indexes.isEmpty()) {
        QString toolTipText = "<table>";
        for (const QModelIndex &colIndex : indexes) {
            QString columnName = colIndex.model()->headerData(colIndex.column(), Qt::Horizontal).toString();
            toolTipText += QString("<tr><td><b>%1:</b></td><td>%2</td></tr>")
                                  .arg(columnName.toHtmlEscaped())
                                  .arg(formatValue(colIndex).toHtmlEscaped());
        }
        toolTipText += "</table>";
        QModelIndex useIndex = index.siblingAtColumn(1);
        QString calendarText = useIndex.data(GroupByModel::CalendarTextRole).toString();
        if (! calendarText.isEmpty()) {
            toolTipText += QString("<p>%1</p>").arg(calendarText.toHtmlEscaped());
        }
        QStringList flags;
        if (useIndex.data(GroupByModel::PlannedRole).toBool()) {
            flags << QString("<i>%1</i>").arg(tr("Planned").toHtmlEscaped());
        }
        if (useIndex.data(GroupByModel::DirtyRole).toBool()) {
            flags << QString("<i>%1</i>").arg(tr("Unsaved changes").toHtmlEscaped());
        }
        if (! flags.isEmpty()) {
            toolTipText += QString("<p>%1</p>").arg(flags.join(", "));
        }
        QToolTip::showText(event->globalPos(), toolTipText, view);
    } else {
        QToolTip::hideText();
    }
    return true;
}


void
ActivityItemDelegate::setSummaryLines
(int summaryLines)
{
    this->summaryLines = summaryLines;
}


bool
ActivityItemDelegate::isHeaderRow
(const QModelIndex &index) const
{
    return index.data(GroupByModel::HeaderRole).toBool();
}


//////////////////////////////////////////////////////////////////////////////
// ActivityTreeView

ActivityTreeView::ActivityTreeView
(RideNavigator *rideNavigator, QWidget* parent)
: QTreeView(parent), rideNavigator(rideNavigator)
{
    setUniformRowHeights(false);

#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
}


QList<QModelIndex>
ActivityTreeView::toolTipIndexes
(const QModelIndex &index) const
{
    QList<QModelIndex> ret;
    if (index.data(GroupByModel::HeaderRole).toBool()) {
        return ret;
    }
    QModelIndex useIndex = index.siblingAtColumn(1);
    for (int visual = 0; visual < header()->count(); ++visual) {
        int logical = header()->logicalIndex(visual);
        QString headerText = model()->headerData(logical, Qt::Horizontal, Qt::DisplayRole).toString();
        if (isColumnHidden(logical) || headerText == "*") {
            continue;
        }
        QModelIndex sibling = useIndex.siblingAtColumn(logical);
        if (! sibling.isValid()) {
            continue;
        }
        ret << sibling;
    }
    return ret;
}


void
ActivityTreeView::selectionChanged
(const QItemSelection &selected, const QItemSelection &deselected)
{
    emit rowSelected(selected);
    QTreeView::selectionChanged(selected, deselected);

    // Ugly hack to prevent visual artefacts remaining after selecting a new row
    // Qt doesn't repaint the area beyond the last column on selection change
    // Alternative fix: tableView->header()->setStretchLastSection(true) - but this
    // gives a less predictable resizing UX
    const int lastColumnRight = header()->sectionViewportPosition(header()->count() - 1) + header()->sectionSize(header()->count() - 1);
    if (lastColumnRight >= viewport()->width()) {
        return;
    }
    const int gapLeft  = lastColumnRight;
    const int gapWidth = viewport()->width() - lastColumnRight;
    for (const QModelIndex &idx : selected.indexes()) {
        const QRect r = visualRect(idx);
        viewport()->update(gapLeft, r.top(), gapWidth, r.height());
    }
    for (const QModelIndex &idx : deselected.indexes()) {
        const QRect r = visualRect(idx);
        viewport()->update(gapLeft, r.top(), gapWidth, r.height());
    }
}


void
ActivityTreeView::setSummaryLines
(int summaryLines)
{
    this->summaryLines = summaryLines;
}


void
ActivityTreeView::dragEnterEvent
(QDragEnterEvent *event)
{
    event->accept();
}


void
ActivityTreeView::dragMoveEvent
(QDragMoveEvent *event)
{
    event->accept();
}


void
ActivityTreeView::drawRow
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QModelIndex useIndex = index.siblingAtColumn(1);
    Q_ASSERT(useIndex.isValid());
    if (! useIndex.isValid()) {
        QTreeView::drawRow(painter, option, index);
        return;
    }

    const QRect rowRect = option.rect;
    const bool selected = selectionModel()->isSelected(index);
    const bool isHeader = useIndex.data(GroupByModel::HeaderRole).toBool();
    const bool planned = useIndex.data(GroupByModel::PlannedRole).toBool();
    const QString sport = useIndex.data(GroupByModel::SportRole).toString();
    const QString subSport = useIndex.data(GroupByModel::SubSportRole).toString();
    const int lineHeight = QFontMetrics(option.font).lineSpacing() + 1;
    const bool dark = GCColor::isPaletteDark(palette());

    QColor activityColor = useIndex.data(Qt::BackgroundRole).value<QBrush>().color();
    if (! activityColor.isValid() || activityColor == QColor(1,1,1)) {
        activityColor = palette().mid().color();
    }

    painter->save();
    QColor bg = resolveBackgroundColor(selected);
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->fillRect(rowRect, bg);

    QColor titleFg;
    QColor summaryFg;
    if (! isHeader && ! selected) {
        QColor titleBg;
        if (planned) {
            titleBg = GColor(CCALPLANNED);
            titleBg.setAlphaF(dark ? 0.25 : 0.12);
        } else if (summaryLines > 0) {
            titleBg = palette().mid().color();
            titleBg.setAlphaF(dark ? 0.2 : 0.2);
        } else {
            titleBg = bg;
        }
        titleBg = GCColor::blendedColor(titleBg, bg);
        QRect headerRect(rowRect.x(), rowRect.y(), rowRect.width(), lineHeight + 2 * kFieldsTextPaddingTop * dpiYFactor);
        painter->fillRect(headerRect, titleBg);
        titleFg = GCColor::invertColor(titleBg);
        summaryFg = titleFg;
        summaryFg.setAlphaF(0.9);
    } else {
        titleFg = GCColor::invertColor(bg);
        summaryFg = titleFg;
    }
    painter->restore();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QStyleOptionViewItem opt;
    initViewItemOption(&opt);
    opt.rect = rowRect;
    opt.state &= ~QStyle::State_MouseOver;
    opt.state &= ~QStyle::State_Selected;
    opt.palette.setBrush(QPalette::All, QPalette::Text, titleFg);
    QTreeView::drawRow(painter, opt, index);

    if (! isHeader) {
        if (summaryLines > 0) {
            const int iconWidth = std::min(summaryLines, 2) * lineHeight;
            QSize pixmapSize(iconWidth, iconWidth);
            QPixmap pixmap;

            const int summaryTop = rowRect.top() + 2 * kFieldsTextPaddingTop * dpiYFactor + lineHeight + kSummaryMarginTop * dpiYFactor;
            const int summaryLeft = rowRect.left() + (kRowPaddingLeft + kFieldsTextIndent) * dpiXFactor;
            const int summaryRight = rowRect.right() - kRowPaddingRight * dpiXFactor;
            const QFontMetrics fm(option.font);
            const int summaryHeight = (summaryLines - 1) * lineHeight + fm.height();
            const QRect summaryRect(summaryLeft, summaryTop, summaryRight - summaryLeft, summaryHeight);

            const QString iconFile = IconManager::instance().getFilepath(sport, subSport);
            QColor iconColor(activityColor);
            if (selected) {
                iconColor = GCColor::invertColor(bg);
            } else if (planned) {
                iconColor = GColor(CCALPLANNED);
            }
            pixmap = svgAsColoredPixmap(iconFile, pixmapSize, kIconSpacing * dpiXFactor, iconColor);
            painter->drawPixmap(summaryRect.left(), summaryRect.top(), pixmap);

            const QString detailText = useIndex.data(GroupByModel::CalendarTextRole).toString();
            if (! detailText.isEmpty()) {
                QRect summaryTextRect = summaryRect.adjusted(iconWidth + kSummaryIconTextGap * dpiXFactor, 0, 0, 0);

                if (summaryTextRect.isValid()) {
                    painter->save();

                    QFont summaryFont = option.font;
                    summaryFont.setWeight(QFont::Light);
                    const bool dirty = useIndex.data(GroupByModel::DirtyRole).toBool();
                    summaryFont.setItalic(dirty);
                    painter->setFont(summaryFont);
                    painter->setPen(summaryFg);
                    painter->drawText(summaryTextRect, Qt::AlignLeft | Qt::TextWordWrap, detailText);

                    painter->restore();
                }
            }
        }
        if (selected || summaryLines == 0) {
            const int size = lineHeight * 0.75;
            const int x = rowRect.left();
            const int y = rowRect.top();
            QPolygon triangle;
            triangle << QPoint(x, y)
                     << QPoint(x + size, y)
                     << QPoint(x, y + size);
            painter->setBrush(planned ? GColor(CCALPLANNED) : activityColor);
            painter->setPen(Qt::NoPen);
            painter->drawPolygon(triangle);
        }
    }
    painter->restore();
}


void
ActivityTreeView::resizeEvent
(QResizeEvent *event)
{
    QTreeView::resizeEvent(event);
    scheduleDelayedItemsLayout();
}


QColor
ActivityTreeView::resolveBackgroundColor
(bool selected) const
{
    if (selected) {
        return GColor(CCALCURRENT);
    }
    return palette().base().color();
}


//////////////////////////////////////////////////////////////////////////////
// RideNavigatorSortProxyModel

RideNavigatorSortProxyModel::RideNavigatorSortProxyModel
(QObject *parent)
: QSortFilterProxyModel(parent)
{
}


bool
RideNavigatorSortProxyModel::lessThan
(const QModelIndex &left, const QModelIndex &right) const
{
    QVariant leftData = sourceModel()->data(left);
    QVariant rightData = sourceModel()->data(right);

    if (leftData.metaType().id() == QMetaType::QDateTime) {
        return leftData.toDateTime() < rightData.toDateTime();
    }
    QString leftString = leftData.toString();
    QString rightString = rightData.toString();

    if (   leftString.contains(QRegularExpression("[^0-9.,]"))
        || rightString.contains(QRegularExpression("[^0-9.,]"))) { // alpha
        return QString::localeAwareCompare(leftString, rightString) < 0;
    }
    // assume numeric
    return leftString.toDouble() < rightString.toDouble();
}


//////////////////////////////////////////////////////////////////////////////
// static functions

static bool insensitiveLessThan(const QString &a, const QString &b)
{
    return a.toLower() < b.toLower();
}
