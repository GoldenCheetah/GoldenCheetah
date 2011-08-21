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

#include <QtGui>
#include <QString>

#define tr(s) QObject::tr(s)

static const QString defaultconfig = QString("Duration|Distance|TSS|IF|Date");

RideNavigator::RideNavigator(MainWindow *parent) : main(parent), active(false), groupBy(-1)
{
    // get column headings
    QList<QString> defaultColumns = appsettings->cvalue(main->cyclist, GC_NAVHEADINGS, defaultconfig)
                                    .toString().split("|", QString::SkipEmptyParts);

    if (defaultColumns.count() < 2) defaultColumns = defaultconfig.split("|", QString::SkipEmptyParts);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    sqlModel = new QSqlTableModel(this, main->metricDB->db()->connection());
    sqlModel->setTable("metrics");
    //sqlModel->setSort(2, Qt::AscendingOrder); // date backwards
    sqlModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    sqlModel->select();
    while (sqlModel->canFetchMore(QModelIndex())) sqlModel->fetchMore(QModelIndex());

    groupByModel = new GroupByModel(this);
    groupByModel->setSourceModel(sqlModel);

    sortModel = new BUGFIXQSortFilterProxyModel(this);
    sortModel->setSourceModel(groupByModel);
    sortModel->setDynamicSortFilter(true);
    //sortModel->setSort(2, Qt::AscendingOrder); // date backwards

    tableView = new QTreeView;
    delegate = new NavigatorCellDelegate(this);
    tableView->setItemDelegate(delegate);
    tableView->setModel(sortModel);
    tableView->setSortingEnabled(true);
    tableView->setAlternatingRowColors(false);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers); // read-only
    mainLayout->addWidget(tableView);
    tableView->expandAll();
    //XXXtableView->horizontalScrollBar()->setDisabled(true);
    //XXXtableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    tableView->header()->setStretchLastSection(false);
    tableView->header()->setMinimumSectionSize(20);
    //tableView->setUniformRowHeights(true);
    QFont smaller;
    smaller.setPointSize(smaller.pointSize()-2);
    //tableView->setFont(smaller);
    tableView->installEventFilter(this);
    tableView->setFrameStyle(QFrame::NoFrame);

#if 0
    tableView->header()->setStyleSheet( "::section { background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                                        "stop: 0 #CFCFCF, stop: 1.0 #A8A8A8);"
                                        "border: 2px; border-color: #A8A8A8; "
                                        "color: #535353;"
                                        "font-weight: bold; }");
#endif
    tableView->show();

    // this maps friendly names to metric names
    QMap <QString, QString> nameMap;

    // add the standard columns to the map
    nameMap.insert("filename", "File");
    nameMap.insert("timestamp", "Last updated");
    nameMap.insert("ride_date", "Date");
    nameMap.insert("fingerprint", "Config Checksum");

    // add metrics to the map
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++) {
        QString converted = QTextEdit(factory.rideMetric(factory.metricName(i))->name()).toPlainText();

        // from sql column name to friendly metric name
        nameMap.insert(QString("X%1").arg(factory.metricName(i)), converted);

        // from friendly metric name to metric pointer
        columnMetrics.insert(converted, factory.rideMetric(factory.metricName(i)));
    }

    // add metadata fields...
    SpecialFields sp; // all the special fields are in here...
    foreach(FieldDefinition field, main->rideMetadata()->getFields()) {
        if (!sp.isMetric(field.name) && field.type < 5) {
            nameMap.insert(QString("Z%1").arg(sp.makeTechName(field.name)), field.name);
        }
    }

    // rename and hide columns in the view to be more user friendly
    // and maintain a list of all the column headings so we can
    // reference the section for a given column and unhide later
    // if the user specifically wants to see it
    for (int i=0; i<tableView->header()->count(); i++) {
        QString friendly, techname = sortModel->headerData(i, Qt::Horizontal).toString();
        if ((friendly = nameMap.value(techname, "unknown")) != "unknown") {
            sortModel->setHeaderData(i, Qt::Horizontal, friendly);
            logicalHeadings << friendly;
        } else
            logicalHeadings << techname;

        if (i && !defaultColumns.contains(friendly)) {
            tableView->setColumnHidden(i, true);
        }
    }

    // now re-order the columns according to the
    // prevailing preferences. They are listed in
    // the order we want them, column zero is the
    // group by column, so we leave that alone
    for(int i=1; i<defaultColumns.count(); i++) {
        tableView->header()->moveSection(tableView->header()->visualIndex(logicalHeadings.indexOf(defaultColumns[i])), i);
    }

    // initialise to whatever groupBy we want to start with
    tableView->sortByColumn(appsettings->cvalue(main->cyclist, GC_SORTBY, "2").toInt(),
                              static_cast<Qt::SortOrder>(appsettings->value(this, GC_SORTBYORDER, static_cast<int>(Qt::AscendingOrder)).toInt()));
    //tableView->setColumnHidden(0, true);
    tableView->setColumnWidth(0,0);

    // set the column widths
    int columnnumber=0;
    foreach(QString size, appsettings->cvalue(main->cyclist, GC_NAVHEADINGWIDTHS, "50|50|50|50|50").toString().split("|", QString::SkipEmptyParts))
        tableView->setColumnWidth(columnnumber++,size.toInt());

    groupBy = -1;
    currentColumn = appsettings->cvalue(main->cyclist, GC_NAVGROUPBY, "-1").toInt();
    setGroupBy();

    // refresh when database is updated
    connect(main->metricDB, SIGNAL(dataChanged()), this, SLOT(refresh()));

    // refresh when config changes (metric/imperial?)
    connect(main, SIGNAL(configChanged()), this, SLOT(refresh()));
    // refresh when rides added/removed
    connect(main, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh()));
    connect(main, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh()));
    // selection of a ride by double clicking it, we need to update the ride list
    connect(tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectRide(QModelIndex)));
    connect(tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(cursorRide()));
    // user selected a ride on the ride list, we should reflect that too..
    connect(main->rideTreeWidget(), SIGNAL(itemSelectionChanged()), this, SLOT(rideTreeSelectionChanged()));
    // user moved columns
    connect(tableView->header(), SIGNAL(sectionMoved(int,int,int)), this, SLOT(columnsChanged()));
    connect(tableView->header(), SIGNAL(sectionResized(int,int,int)), this, SLOT(columnsChanged()));
    // user sorted by column
    connect(tableView->header(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), this, SLOT(selectRow()));
    // context menu is provided by mainWindow
    connect(tableView,SIGNAL(customContextMenuRequested(const QPoint &)), main, SLOT(showTreeContextMenuPopup(const QPoint &)));
    connect(tableView->header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(setSortBy(int,Qt::SortOrder)));
    // we accept drag and drop operations
    setAcceptDrops(true);

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
RideNavigator::setWidth(int x)
{
    active = true;

    if (tableView->verticalScrollBar()->isVisible())
        x -= tableView->verticalScrollBar()->width();

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
    int last=0;
    int newwidth=0;
    for (int i=1; i<tableView->header()->count(); i++) {
        if (tableView->header()->isSectionHidden(i) == false) {
            newwidth = (double)((((double)tableView->columnWidth(i)/(double)headwidth)) * (double)x); 
            if (newwidth < 20) newwidth = 20;
            QString columnName = tableView->model()->headerData(i, Qt::Horizontal).toString();
            if (columnName == "*") newwidth = 0;
            tableView->setColumnWidth(i, newwidth);
            setwidth += newwidth;
            last = i;
        }
    }

    // UGH. Now account for the fact that the smaller columns
    //      didn't take their fair share of a negative resize
    //      so we need to snip off from the larger columns.
    // XXX this is a hack, we should use stretch factors...
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

    //tableView->setColumnWidth(last, newwidth + (x-setwidth)); // account for rounding errors

    if (setwidth < x)
        delegate->setWidth(pwidth=setwidth);
    else
        delegate->setWidth(pwidth=x);

    active = false;
}

// make sure the columns are all neat and tidy when the ride navigator is shown
bool
RideNavigator::event(QEvent *e)
{
    if (e->type() == QEvent::WindowActivate) {
        active=false;
        setWidth(geometry().width()); // calculate width...
    }
    return QWidget::event(e);
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

    appsettings->setCValue(main->cyclist, GC_NAVHEADINGS, headings);

    // get column widths
    QString widths;
    for (int i=0; i<tableView->header()->count(); i++)
        widths += QString("%1|").arg(tableView->columnWidth(i));

    // clean up
    active = false;
    setWidth(geometry().width()); // calculate width...
    appsettings->setCValue(main->cyclist, GC_NAVHEADINGWIDTHS, widths);
}

bool
RideNavigator::eventFilter(QObject *object, QEvent *e)
{
    // not for the table?
    if (object != (QObject *)tableView) return false;

    // what happenned?
    switch(e->type())
    {
        case QEvent::ContextMenu:
        {
            //ColumnChooser *selector = new ColumnChooser(logicalHeadings);
            //selector->show();
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

    //stdContextMenu(&menu, pos);
    //menu.addSeparator();

    QAction *delCol = new QAction(tr("Remove Column"), tableView);
    delCol->setEnabled(true);
    menu.addAction(delCol);
    connect(delCol, SIGNAL(triggered()), this, SLOT(removeColumn()));

    QAction *insCol = new QAction(tr("Column Chooser"), tableView);
    insCol->setEnabled(true);
    menu.addAction(insCol);
    connect(insCol, SIGNAL(triggered()), this, SLOT(showColumnChooser()));

    QAction *toggleGroupBy = new QAction(groupBy >= 0 ? tr("Do Not Show in Groups") : tr("Show In Groups"), tableView);
    toggleGroupBy->setEnabled(true);
    menu.addAction(toggleGroupBy);
    connect(toggleGroupBy, SIGNAL(triggered()), this, SLOT(setGroupBy()));

    // set current column...
    currentColumn = column;
    menu.exec(tableView->mapToGlobal(QPoint(pos.x(), pos.y())));
}

void
RideNavigator::setGroupBy()
{
    // toggle
    groupBy = groupBy >= 0 ? -1 : currentColumn;

    // set proxy model
    groupByModel->setGroupBy(groupBy);

    // lets expand column 0 for the groupBy heading
    for (int i=0; i < groupByModel->groupCount(); i++) {
        tableView->setFirstColumnSpanned (i, QModelIndex(), true);
    }

    // now show em
    tableView->expandAll();

    // save away
    appsettings->setCValue(main->cyclist, GC_NAVGROUPBY, groupBy);

    // reselect current ride - since selectionmodel
    // is changed by setGroupBy()
    rideTreeSelectionChanged();
}

void
RideNavigator::setSortBy(int index, Qt::SortOrder order)
{
    appsettings->setCValue(main->cyclist, GC_SORTBY, index);
    appsettings->setCValue(main->cyclist, GC_SORTBYORDER, order);
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
static bool initGroupRanges()
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
static bool _initGroupRanges = initGroupRanges();


// Perhaps a groupName function on the metrics would be useful
// or maybe mix zoning into them... XXX todo
//
QString
GroupByModel::groupFromValue(QString headingName, QString value, double rank, double count) const
{
    // Check for predefined thresholds / zones / bands for this metric/column
    foreach (groupRange range, groupRanges) {
        if (range.column == headingName) {

            double number = value.toDouble();
            // use thresholds defined for this column/metric
            foreach(groupRange::range range, range.ranges) {

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

        if (headingName == "Date") {

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
    QModelIndex fileIndex = tableView->model()->index(index.row(), 1, index.parent());

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
    // or a bug in our QAbstractProxyModel. I suspect the
    // latter. XXX fixme
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

                QString fileName = tableView->model()->data(tableView->model()->index(j,1, group), Qt::DisplayRole).toString();
                if (fileName == rideItem->fileName) {
                    // we set current index to column 2 (date/time) since we can be guaranteed it is always show (all others are removable)
                    QItemSelection row(tableView->model()->index(j,0,group),
                                       tableView->model()->index(j,tableView->model()->columnCount()-1, group));
                    tableView->selectionModel()->select(row, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
                    //tableView->selectionModel()->setCurrentIndex(tableView->model()->index(j,0,group), QItemSelectionModel::NoUpdate);
                    tableView->selectionModel()->setCurrentIndex(tableView->model()->index(j,0,group), QItemSelectionModel::NoUpdate);
                    tableView->scrollTo(tableView->model()->index(j,2,group), QAbstractItemView::PositionAtCenter);
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
        s.setHeight(64);
    } else s.setHeight(18);
#if 0
    if (rideNavigator->tableView->model()->data(index, Qt::UserRole).toString() != "") s.setHeight(18);
    else s.setHeight(36);
#endif
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
            metricValue *= (appsettings->value(this, GC_UNIT).toString() == "Metric") ? 1 : m->conversion();

            // format with the right precision
            if (m->units(true) == "seconds") {
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
        //myOption.rect.setHeight(18);
        painter->fillRect(myOption.rect, background);
        //drawFocus(painter, myOption, myOption.rect);
        drawDisplay(painter, myOption, myOption.rect, ""); //added
        //QPen rpen(Qt::DotLine);
        QPen rpen;
        rpen.setWidth(0.5);
        rpen.setColor(Qt::lightGray);
        painter->setPen(rpen);
        painter->drawLine(0,myOption.rect.y(),rideNavigator->pwidth,myOption.rect.y());
        //painter->drawLine(0,myOption.rect.y()+myOption.rect.height()-1,rideNavigator->pwidth,myOption.rect.y()+myOption.rect.height()-1);
        painter->drawLine(0,myOption.rect.y()+myOption.rect.height(),rideNavigator->pwidth,myOption.rect.y()+myOption.rect.height());
        //painter->drawLine(0,myOption.rect.y()+myOption.rect.height()-1,0,myOption.rect.y()+myOption.rect.height()-1);
        painter->drawLine(0,myOption.rect.y()+myOption.rect.height(),0,myOption.rect.y()+myOption.rect.height());
        //painter->drawLine(rideNavigator->pwidth,myOption.rect.y(),rideNavigator->pwidth, myOption.rect.y()+myOption.rect.height()-1);
        painter->drawLine(rideNavigator->pwidth,myOption.rect.y(),rideNavigator->pwidth, myOption.rect.y()+myOption.rect.height());
        myOption.rect.setHeight(18); //added
        myOption.font.setWeight(QFont::Bold);
        drawDisplay(painter, myOption, myOption.rect, value);

        if (calendarText != "") {
            myOption.rect.setX(0);
            myOption.rect.setY(myOption.rect.y() + 23);//was +18
            myOption.rect.setWidth(rideNavigator->pwidth);
            myOption.rect.setHeight(36);
            myOption.font.setPointSize(myOption.font.pointSize()-2);
            myOption.font.setWeight(QFont::Normal);
            //myOption.font.setStyle(QFont::StyleItalic);
            painter->fillRect(myOption.rect, background);
            //drawFocus(painter, myOption, myOption.rect);
            drawDisplay(painter, myOption, myOption.rect, "");
            myOption.rect.setX(50);
            myOption.rect.setWidth(pwidth-100);
            //drawDisplay(painter, myOption, myOption.rect, calendarText);
            painter->setFont(myOption.font);
            painter->drawText(myOption.rect, Qt::AlignLeft | Qt::TextWordWrap, calendarText);
        }

    } else {

        if (value != "") {
#if 0
            QPen blueLine(Qt::darkBlue);
            blueLine.setWidth(3);
            painter->setPen(blueLine);
#endif
            myOption.displayAlignment = Qt::AlignLeft | Qt::AlignBottom;
            myOption.rect.setX(0);
            myOption.rect.setHeight(18);
            myOption.rect.setWidth(rideNavigator->pwidth);
            painter->fillRect(myOption.rect, GColor(CRIDEGROUP));
#if 0
            painter->drawLine(0,myOption.rect.y()+myOption.rect.height()-1,rideNavigator->pwidth,myOption.rect.y()+myOption.rect.height()-1);
#endif
            //drawFocus(painter, myOption, myOption.rect);
            //myOption.font.setPointSize(10);
            //myOption.font.setWeight(QFont::Bold);
            //painter->drawText(myOption.rect, Qt::AlignLeft | Qt::AlignVCenter, value);
        }
        drawDisplay(painter, myOption, myOption.rect, value);
    }


#if 0
    QStyledItemDelegate::paint(painter, option, index);
#endif
}

ColumnChooser::ColumnChooser(QList<QString>&logicalHeadings)
{
    // wipe away everything when you close please...
    setWindowTitle("Column Chooser");
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
    //QSignalMapper *clicked;
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
