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

#include "RideNavigator.h"
#include "RideNavigatorProxy.h"
#include "SearchFilterBox.h"

#include <QtGui>
#include <QString>

RideNavigator::RideNavigator(MainWindow *parent, bool mainwindow) : main(parent), active(false), _groupBy(-1)
{
    // get column headings
    // default column layouts etc
    _columns = QString(tr("*|Workout Code|TSS|Date|"));
    _widths = QString("0|80|50|50|");
    _sortByIndex = 2;
    _sortByOrder = 0;
    currentColumn = -1;
    _groupBy = -1;
    fontHeight = QFontMetrics(QFont()).height();

    init = false;

    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    if (mainwindow) mainLayout->setContentsMargins(0,0,0,0);
    else mainLayout->setContentsMargins(2,2,2,2); // so we can resize!

    sqlModel = new QSqlTableModel(this, main->metricDB->db()->connection());
    sqlModel->setTable("metrics");
    sqlModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    sqlModel->select();
    while (sqlModel->canFetchMore(QModelIndex())) sqlModel->fetchMore(QModelIndex());

    searchFilter = new SearchFilter(this);
    searchFilter->setSourceModel(sqlModel); // filter out/in search results

    groupByModel = new GroupByModel(this);
    groupByModel->setSourceModel(searchFilter);

    sortModel = new BUGFIXQSortFilterProxyModel(this);
    sortModel->setSourceModel(groupByModel);
    sortModel->setDynamicSortFilter(true);

#ifdef GC_HAVE_LUCENE
    if (!mainwindow) {
        searchFilterBox = new SearchFilterBox(this, main);
        mainLayout->addWidget(searchFilterBox);
    }
#endif

    // get setup
    tableView = new QTreeView;
    delegate = new NavigatorCellDelegate(this);
    tableView->setItemDelegate(delegate);
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
#ifdef Q_OS_MAC
    tableView->header()->setSortIndicatorShown(false); // blue looks nasty
    tableView->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    tableView->installEventFilter(this);
    tableView->viewport()->installEventFilter(this);
    tableView->setMouseTracking(true);
    tableView->setFrameStyle(QFrame::NoFrame);

    // good to go
    tableView->show();
    resetView();

    // refresh when database is updated
    connect(main->metricDB, SIGNAL(dataChanged()), this, SLOT(refresh()));

    // refresh when config changes (metric/imperial?)
    connect(main, SIGNAL(configChanged()), this, SLOT(refresh()));
    // refresh when rides added/removed
    connect(main, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh()));
    connect(main, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh()));
    connect(main->rideTreeWidget(), SIGNAL(itemSelectionChanged()), this, SLOT(rideTreeSelectionChanged()));
    // selection of a ride by double clicking it, we need to update the ride list
    connect(tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectRide(QModelIndex)));
    connect(tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(cursorRide()));
    // user selected a ride on the ride list, we should reflect that too..
    // user moved columns
    connect(tableView->header(), SIGNAL(sectionMoved(int,int,int)), this, SLOT(columnsChanged()));
    connect(tableView->header(), SIGNAL(sectionResized(int,int,int)), this, SLOT(columnsChanged()));
    // user sorted by column
    connect(tableView->header(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), this, SLOT(selectRow()));
    // context menu is provided by mainWindow
    connect(tableView,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showTreeContextMenuPopup(const QPoint &)));
    connect(tableView->header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(setSortBy(int,Qt::SortOrder)));

#ifdef GC_HAVE_LUCENE
    if (!mainwindow) {
        connect(searchFilterBox, SIGNAL(searchResults(QStringList)), this, SLOT(searchStrings(QStringList)));
        connect(searchFilterBox, SIGNAL(searchClear()), this, SLOT(clearSearch()));
    }
#endif

    // we accept drag and drop operations
    setAcceptDrops(true);
    columnsChanged(); // set visual headings etc
}

RideNavigator::~RideNavigator()
{
    delete tableView;
    delete groupByModel;
    delete sqlModel;
}

void
RideNavigator::refresh()
{
    fontHeight = QFontMetrics(QFont()).height();

    sqlModel->select();
    while (sqlModel->canFetchMore(QModelIndex()))
        sqlModel->fetchMore(QModelIndex());

    active=false;
    rideTreeSelectionChanged();
}

void
RideNavigator::resizeEvent(QResizeEvent*)
{
    setWidth(geometry().width());
}

void
RideNavigator::resetView()
{
    active = true;

    QList<QString> cols = _columns.split("|", QString::SkipEmptyParts);

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
        nameMap.insert(QString("X%1").arg(factory.metricName(i)), converted);

        // from (english) internalName to (translated) Name
        internalNameMap.insert(factory.rideMetric(factory.metricName(i))->internalName(), converted);

        // from friendly metric name to metric pointer
        columnMetrics.insert(converted, factory.rideMetric(factory.metricName(i)));
    }

    // add metadata fields...
    SpecialFields sp; // all the special fields are in here...
    foreach(FieldDefinition field, main->rideMetadata()->getFields()) {
        if (!sp.isMetric(field.name) && (field.type < 5 || field.type == 7)) {
            nameMap.insert(QString("Z%1").arg(sp.makeTechName(field.name)), sp.displayName(field.name));
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

    //tableView->setColumnHidden(0, true);
    tableView->setColumnWidth(0,0);

    // set the column widths
    int columnnumber=0;
    foreach(QString size, _widths.split("|", QString::SkipEmptyParts)) {

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
    rideTreeSelectionChanged();
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

void RideNavigator::setWidth(int x)
{
    if (init == false) return;

    active = true;

    if (tableView->verticalScrollBar()->isVisible())
        x -= tableView->verticalScrollBar()->width()
             + 0 ; // !! no longer account for content margins of 3,3,3,3 was + 6

    // take the margins into accopunt top
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
    int n=0;
    for (int i=1; i<tableView->header()->count(); i++)
        if (tableView->header()->isSectionHidden(i) == false) {
            headwidth += tableView->columnWidth(i);
            n++;
        }

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

    if (setwidth < x)
        delegate->setWidth(pwidth=setwidth);
    else
        delegate->setWidth(pwidth=x);

    active = false;
}

// make sure the columns are all neat and tidy when the ride navigator is shown
void
RideNavigator::showEvent(QShowEvent *)
{
    init = true;
    setWidth(geometry().width());
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

    // get column widths
    QString widths;
    for (int i=0; i<tableView->header()->count(); i++) {
        int index = tableView->header()->logicalIndex(i);
        if (tableView->header()->isSectionHidden(index) != true) {
           widths += QString("%1|").arg(tableView->columnWidth(index));
        }
    }

    // clean up
    active = false;
    setWidth(geometry().width()); // calculate width...
    _widths = widths;
}

bool
RideNavigator::eventFilter(QObject *object, QEvent *e)
{
    // if in doubt hide the tooltip, but paint events are
    // always generated in the viewport when the popup is shown
    // so we ignore those
    if (e->type() != QEvent::ToolTip && e->type() != QEvent::Paint &&
        e->type() != QEvent::WinIdChange && e->type() != QEvent::Destroy) {
        main->setBubble("");
    }

    // not for the table?
    if (object != (QObject *)tableView) return false;

    // what happenned?
    switch(e->type())
    {
        case QEvent::ContextMenu:
        {
            borderMenu(((QMouseEvent *)e)->pos());
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

        case QEvent::WindowActivate:
        {
            active=true;
            // set the column widths
            int columnnumber=0;
            foreach(QString size, _widths.split("|", QString::SkipEmptyParts)) {
                tableView->setColumnWidth(columnnumber, size.toInt());
            }
            active=false;
            setWidth(geometry().width()); // calculate width...
        }
        break;

        case QEvent::ToolTip:
        case QEvent::ToolTipChange:
        {
            QPoint global = dynamic_cast<QHelpEvent*>(e)->globalPos();
            QPoint local = tableView->viewport()->mapFromGlobal(global);

            // off view port!
            if (local.y() <= 0 || local.x() <= 0) {
                main->setBubble("");
                return false;
            }

            QModelIndex index = tableView->indexAt(local);
            if (index.isValid()) {
                QString hoverFileName = tableView->model()->data(index, Qt::UserRole+1).toString();
                e->accept();
                QPoint p = local;
                p.setX(width()-20);
                main->setBubble(hoverFileName, tableView->viewport()->mapToGlobal(p));
            }
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
    rideTreeSelectionChanged();
}

void
RideNavigator::setSortBy(int index, Qt::SortOrder order)
{
    _sortByIndex = index;
    _sortByOrder = static_cast<int>(order);
}

//
// This function is called for every row in the metricDB
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

static QList<groupRange> groupRanges;
bool
GroupByModel::initGroupRanges()
{
    groupRange::range add;
    groupRange addColumn;

    // TSS
    addColumn.column = "TSS";
    add = groupRange::range( 0.0,  0.0, tr("Zero or not present")); addColumn.ranges << add;
    add = groupRange::range( 0,  150, tr("Low Stress")); addColumn.ranges << add;
    add = groupRange::range( 150,  300, tr("Medium Stress")); addColumn.ranges << add;
    add = groupRange::range( 300,  450, tr("High Stress")); addColumn.ranges << add;
    add = groupRange::range( 450,  0.00, tr("Very High Stress")); addColumn.ranges << add;

    groupRanges << addColumn;
    addColumn.ranges.clear();

    // Intensity Factor
    addColumn.column = "IF";
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
static bool _initGroupRanges = false;

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
            return "Undefined";
        }
    }

    // Use upper quartile for anything left that is a metric
    if (rideNavigator->columnMetrics.value(headingName, NULL) != NULL) {

        double quartile = rank / count;

        if (value.toDouble() == 0) return QString("Zero or not present");
        else if (rank < 10) return QString("Best 10");
        else if (quartile <= 0.25) return QString ("Quartile 1:  0% -  25%");
        else if (quartile <= 0.50) return QString ("Quartile 2: 25% -  50%");
        else if (quartile <= 0.75) return QString ("Quartile 3: 50% -  75%");
        else if (quartile <= 1) return QString    ("Quartile 4: 75% - 100%");

    } else {

        if (headingName == tr("Date")) {

            // get the date from value string
            QDateTime dateTime = QDateTime::fromString(value, Qt::ISODate);
            QDateTime today = QDateTime::currentDateTime();

            if (today.date().weekNumber() == dateTime.date().weekNumber()
                && today.date().year() == dateTime.date().year())
                return "This week";
            else if (today.date().month() == dateTime.date().month()
                    && today.date().year() == dateTime.date().year())
                return "This month";
            else if (today.date().month() == (dateTime.date().month()+1)
                    && today.date().year() == dateTime.date().year())
                return "Last month";
            else {
                return dateTime.toString("yyyy-MM (MMMM)");
            }
        }

        // not a metric, i.e. metadata
        return value;
    }

    // if all else fails just return the value and group by
    // that. Which is a fair approach for text fields for example
    return value;
}

void
RideNavigator::removeColumn()
{
    active = true;
    tableView->setColumnHidden(currentColumn, true);
    active = false;
    
    setWidth(geometry().width()); // calculate width...
    columnsChanged(); // need to do after, just once
}

void
RideNavigator::showColumnChooser()
{
    ColumnChooser *selector = new ColumnChooser(logicalHeadings);
    selector->show();
}


// user double clicked a ride, we need to update the main window ride list
void
RideNavigator::selectRide(const QModelIndex &index)
{
    QModelIndex fileIndex = tableView->model()->index(index.row(), 2, index.parent()); // column 2 for filename ?

    QString filename = tableView->model()->data(fileIndex, Qt::DisplayRole).toString();
    main->selectRideFile(filename);
}

// user cursor moved to ride
void
RideNavigator::cursorRide()
{
    if (active ==  true) return;
    else active = true;
    selectRide(tableView->currentIndex());
    active = false;
}

// fixup selection lost when columns sorted etc
void
RideNavigator::selectRow()
{
    // this is fugly and either a bug in QtreeView sorting
    // or a bug in our QAbstractProxyModel.
    rideTreeSelectionChanged(); // reset from ridelist
}

// the main window ride list changed, we need to reflect it
void
RideNavigator::rideTreeSelectionChanged()
{
    if (active == true) return;
    else active = true;

    QTreeWidgetItem *which;
    if (main->rideTreeWidget()->selectedItems().count())
        which = main->rideTreeWidget()->selectedItems().first();
    else // no rides slected
        which = NULL;

    if (which && which->type() == RIDE_TYPE) {
        RideItem *rideItem = static_cast<RideItem *>(which);

        // now find rideItem->fileName in the model
        // and then select it in the view!
        for (int i=0; i<tableView->model()->rowCount(); i++) {

            QModelIndex group = tableView->model()->index(i,0,QModelIndex());
            for (int j=0; j<tableView->model()->rowCount(group); j++) {

                QString fileName = tableView->model()->data(tableView->model()->index(j,2, group), Qt::DisplayRole).toString();
                if (fileName == rideItem->fileName) {
                    // we set current index to column 2 (date/time) since we can be guaranteed it is always show (all others are removable)
                    QItemSelection row(tableView->model()->index(j,0,group),
                                       tableView->model()->index(j,tableView->model()->columnCount()-1, group));
                    tableView->selectionModel()->select(row, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
                    tableView->selectionModel()->setCurrentIndex(tableView->model()->index(j,0,group), QItemSelectionModel::NoUpdate);
                    tableView->scrollTo(tableView->model()->index(j,3,group), QAbstractItemView::PositionAtCenter);
                    active = false;
                    return;
                }
            }
        }
    }
    active = false;
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
    QString name = event->mimeData()->data("application/x-columnchooser");
    // fugly, but it works for BikeScore with the (TM) in it...
    if (name == "BikeScore?") name = QTextEdit("BikeScore&#8482;").toPlainText();
    tableView->setColumnHidden(logicalHeadings.indexOf(name), false);
    tableView->setColumnWidth(logicalHeadings.indexOf(name), 50);
    tableView->header()->moveSection(tableView->header()->visualIndex(logicalHeadings.indexOf(name)), 1);
    columnsChanged();
}

NavigatorCellDelegate::NavigatorCellDelegate(RideNavigator *rideNavigator, QObject *parent) :
    QItemDelegate(parent), rideNavigator(rideNavigator), pwidth(300)
{
}

// Editing functions are null since the model is read-only
QWidget *NavigatorCellDelegate::createEditor(QWidget *, const QStyleOptionViewItem &, const QModelIndex &) const { return NULL; }
void NavigatorCellDelegate::commitAndCloseEditor() { }
void NavigatorCellDelegate::setEditorData(QWidget *, const QModelIndex &) const { }
void NavigatorCellDelegate::updateEditorGeometry(QWidget *, const QStyleOptionViewItem &, const QModelIndex &) const {}
void NavigatorCellDelegate::setModelData(QWidget *, QAbstractItemModel *, const QModelIndex &) const { }

QSize NavigatorCellDelegate::sizeHint(const QStyleOptionViewItem & /*option*/, const QModelIndex &index) const 
{
    QSize s;

    if (rideNavigator->groupByModel->mapToSource(rideNavigator->sortModel->mapToSource(index)) != QModelIndex() &&
        rideNavigator->groupByModel->data(rideNavigator->sortModel->mapToSource(index), Qt::UserRole).toString() != "") {
        s.setHeight((rideNavigator->fontHeight+2) * 3);
    } else s.setHeight(rideNavigator->fontHeight + 2);
    return s;
}

// anomalies are underlined in red, otherwise straight paintjob
void NavigatorCellDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    // format the cell depending upon what it is...
    QString columnName = rideNavigator->tableView->model()->headerData(index.column(), Qt::Horizontal).toString();
    const RideMetric *m;
    QString value;

    if ((m=rideNavigator->columnMetrics.value(columnName, NULL)) != NULL) {
        // format as a metric

        // get double from sqlmodel
        double metricValue = index.model()->data(index, Qt::DisplayRole).toDouble();

        if (metricValue) {
            // metric / imperial converstion
            metricValue *= (rideNavigator->main->useMetricUnits) ? 1 : m->conversion();
            metricValue += (rideNavigator->main->useMetricUnits) ? 0 : m->conversionSum();

            // format with the right precision
            if (m->units(true) == "seconds" || m->units(true) == tr("seconds")) {
                value = QTime(0,0,0,0).addSecs(metricValue).toString("hh:mm:ss");
            } else {
                value = QString("%1").arg(metricValue, 0, 'f', m->precision());
            }

        } else {

            // blank out zero values, they look ugly and are distracting
            value = "";
        }

    } else {
        // is this the ride date/time ?
        value = index.model()->data(index, Qt::DisplayRole).toString();
        if (columnName == tr("Date")) {
            QDateTime dateTime = QDateTime::fromString(value, Qt::ISODate);
            value = dateTime.toString("MMM d, yyyy"); // same format as ride list
        } else if (columnName == tr("Time")) {
            QDateTime dateTime = QDateTime::fromString(value, Qt::ISODate);
            value = dateTime.toString("hh:mm:ss"); // same format as ride list
        } else if (columnName == tr("Last updated")) {
            QDateTime dateTime;
            dateTime.setTime_t(index.model()->data(index, Qt::DisplayRole).toInt());
            value = dateTime.toString("ddd MMM d, yyyy h:mm AP"); // same format as ride list
        }
    }

    QStyleOptionViewItem myOption = option;

    // groupBy in bold please
    if (columnName == "*") {
        QFont enbolden = option.font;
        enbolden.setWeight(QFont::Bold);
        myOption.font = enbolden;
    }

    // normal render
    QString calendarText = rideNavigator->tableView->model()->data(index, Qt::UserRole).toString();
    QBrush background = rideNavigator->tableView->model()->data(index, Qt::BackgroundRole).value<QBrush>();

    if (columnName != "*") {

        myOption.displayAlignment = Qt::AlignLeft | Qt::AlignTop;
        painter->fillRect(myOption.rect, background);

        // clear first
        drawDisplay(painter, myOption, myOption.rect, ""); //added

        // draw border of each cell
        QPen rpen;
        rpen.setWidth(0.5);
        rpen.setColor(Qt::lightGray);
        painter->setPen(rpen);
        painter->drawLine(0,myOption.rect.y(),rideNavigator->pwidth,myOption.rect.y());
        painter->drawLine(0,myOption.rect.y()+myOption.rect.height(),rideNavigator->pwidth,myOption.rect.y()+myOption.rect.height());
        painter->drawLine(0,myOption.rect.y()+myOption.rect.height(),0,myOption.rect.y()+myOption.rect.height());

        // indent first column and draw all in bold
        myOption.rect.setHeight(rideNavigator->fontHeight + 2); //added
        myOption.font.setWeight(QFont::Bold);
        if (myOption.rect.x() == 0) {
            QRect indented(myOption.rect.x()+5, myOption.rect.y(), myOption.rect.width()-5, myOption.rect.height());
            drawDisplay(painter, myOption, indented, value); //added
        } else drawDisplay(painter, myOption, myOption.rect, value); //added

        // now get the calendar text to appear ...
        if (calendarText != "") {
            myOption.rect.setX(0);
            myOption.rect.setY(myOption.rect.y() + rideNavigator->fontHeight + 2);//was +23
            myOption.rect.setWidth(rideNavigator->pwidth);
            myOption.rect.setHeight(rideNavigator->fontHeight * 2); //was 36
            myOption.font.setPointSize(myOption.font.pointSize());
            myOption.font.setWeight(QFont::Normal);
            painter->fillRect(myOption.rect, background);
            drawDisplay(painter, myOption, myOption.rect, "");
            myOption.rect.setX(10); // wider notes display
            myOption.rect.setWidth(pwidth-20);// wider notes display
            painter->setFont(myOption.font);
            QPen isColor = painter->pen();
            if (isColor.color() == Qt::black) painter->setPen(Qt::darkGray);
            painter->drawText(myOption.rect, Qt::AlignLeft | Qt::TextWordWrap, calendarText);
            painter->setPen(isColor);
        }

    } else {

        if (value != "") {
            myOption.displayAlignment = Qt::AlignLeft | Qt::AlignBottom;
            myOption.rect.setX(0);
            myOption.rect.setHeight(rideNavigator->fontHeight + 2);
            myOption.rect.setWidth(rideNavigator->pwidth);
            painter->fillRect(myOption.rect, GColor(CRIDEGROUP));
        }
        drawDisplay(painter, myOption, myOption.rect, value);
    }
}

ColumnChooser::ColumnChooser(QList<QString>&logicalHeadings)
{
    // wipe away everything when you close please...
    setWindowTitle(tr("Column Chooser"));
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::Tool);

    clicked = new QSignalMapper(this); // maps each button click event
    connect(clicked, SIGNAL(mapped(const QString &)), this, SLOT(buttonClicked(const QString &)));

    buttons = new QGridLayout(this);
    buttons->setSpacing(0);
    buttons->setContentsMargins(0,0,0,0);

    QFont small;
    small.setPointSize(10);

    QList<QString> buttonNames = logicalHeadings;
    qSort(buttonNames);

    int x = 0;
    int y = 0;
    foreach (QString column, buttonNames) {

        if (column == "*") continue;

        // setup button
        QPushButton *add = new QPushButton(column, this);
        add->setFont(small);
        add->setContentsMargins(0,0,0,0);
        buttons->addWidget(add, y, x);

        connect(add, SIGNAL(pressed()), clicked, SLOT(map()));
        clicked->setMapping(add, column);

        // update layout
        x++;
        if (x > 3) {
            y++;
            x = 0;
        }
    }
}

void
ColumnChooser::buttonClicked(QString name)
{
    // setup the drag data
    QMimeData *mimeData = new QMimeData;
    QByteArray empty;
    mimeData->setData("application/x-columnchooser", name.toLatin1());

    // create a drag event
    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->exec(Qt::MoveAction);
}

void
RideNavigator::showTreeContextMenuPopup(const QPoint &pos)
{
   main->showTreeContextMenuPopup(mapToGlobal(pos));
}
