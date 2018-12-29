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
#include "TabView.h"
#include "HelpWhatsThis.h"

#include <QtGui>
#include <QString>
#include <QTreeView>
#include <QStyle>
#include <QStyleFactory>
#include <QScrollBar>

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
    fontHeight = QFontMetrics(QFont()).height();
    ColorEngine ce(context);
    reverseColor = ce.reverseColor;
    currentItem = NULL;

    init = false;

    mainLayout = new QVBoxLayout;
    setChartLayout(mainLayout);
    mainLayout->setSpacing(0);
    if (mainwindow) mainLayout->setContentsMargins(0,0,0,0);
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
    tableView = new RideTreeView(this);
    delegate = new NavigatorCellDelegate(this);
    tableView->setAnimated(true);
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
    tableView->setMouseTracking(true);
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
    ColorEngine ce(context);
    fontHeight = QFontMetrics(QFont()).height();
    reverseColor = ce.reverseColor;

    // hide ride list scroll bar ?
#ifndef Q_OS_MAC
    tableView->setStyleSheet(TabView::ourStyleSheet());
    if (mainwindow) {
        if (appsettings->value(this, GC_RIDESCROLL, true).toBool() == false)
            tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        else 
            tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        //if (appsettings->value(this, GC_RIDEHEAD, true).toBool() == false)
            //tableView->header()->hide();
        //else 
            tableView->header()->show();

        tableView->header()->setStyleSheet(
        QString("QHeaderView { background-color: %1; color: %2; }"
                "QHeaderView::section { background-color: %1; color: %2; "
                " border: 0px ; }")
                .arg(GColor(CPLOTBACKGROUND).name())
                .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name()));
    }

#endif

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

    QList<QString> cols = _columns.split("|", QString::SkipEmptyParts);
    int widco = _widths.split("|", QString::SkipEmptyParts).count();

    // something is wrong with the config ? reset 
    if (widco != cols.count() || widco <= 1) {
        _columns = QString(tr("*|Workout Code|Date|"));
        _widths = QString("0|100|100|");
        cols = _columns.split("|", QString::SkipEmptyParts);
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
    SpecialFields sp; // all the special fields are in here...
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
        if (!sp.isMetric(field.name) && (field.type < 5 || field.type == 7)) {
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

    // get column widths
    QString widths;
    for (int i=0; i<tableView->header()->count(); i++) {
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

#if !defined (Q_OS_MAC) || (defined (Q_OS_MAC) && (QT_VERSION < 0x050000)) // on QT5 the scrollbars have no width
    if (tableView->verticalScrollBar()->isVisible())
        x -= tableView->verticalScrollBar()->width()
                + 0 ; // !! no longer account for content margins of 3,3,3,3 was + 6
#else // we're on a mac with QT5 .. so dodgy way of spotting preferences for scrollbars...
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
    int n=0;
    for (int i=1; i<tableView->header()->count(); i++)
        if (tableView->header()->isSectionHidden(i) == false) {
            headwidth += tableView->columnWidth(i);
            n++;
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

        if (setwidth < x)
            delegate->setWidth(pwidth=setwidth);
        else
            delegate->setWidth(pwidth=x);

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

static QList<groupRange> groupRanges;
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
            QString fileName = tableView->model()->data(tableView->model()->index(j,2, group), Qt::UserRole+1).toString();
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
bool NavigatorCellDelegate::helpEvent(QHelpEvent*, QAbstractItemView*, const QStyleOptionViewItem&, const QModelIndex&) { return true; }

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

    // paint background for user defined color ?
    bool rideBG = appsettings->value(this,GC_RIDEBG,false).toBool();

    // state of item
    bool hover = false; //disable this, its annoying option.state & QStyle::State_MouseOver;
    bool selected = option.state & QStyle::State_Selected;
    bool focus = option.state & QStyle::State_HasFocus;
    //bool isRun = rideNavigator->tableView->model()->data(index, Qt::UserRole+2).toBool();

    // format the cell depending upon what it is...
    QString columnName = rideNavigator->tableView->model()->headerData(index.column(), Qt::Horizontal).toString();
    const RideMetric *m;
    QString value;

    // are we a selected cell ? need to paint accordingly
    //bool selected = false;
    //if (rideNavigator->tableView->selectionModel()->selectedIndexes().count()) { // zero if no rides in list
        //if (rideNavigator->tableView->selectionModel()->selectedIndexes().value(0).row() == index.row())
            //selected = true;
    //}

    if ((m=rideNavigator->columnMetrics.value(columnName, NULL)) != NULL) {

        // get double from sqlmodel
        value = index.model()->data(index, Qt::DisplayRole).toString();

        // get rid of 0 its ugly
        if (value =="nan" || value == "0" || value == "0.0" || value == "0.00") value="";

    } else {
        // is this the ride date/time ?
        value = index.model()->data(index, Qt::DisplayRole).toString();
        if (columnName == tr("Date")) {
            QDateTime dateTime = QDateTime::fromString(value, Qt::ISODate);
            value = dateTime.toString(tr("MMM d, yyyy")); // same format as ride list
        } else if (columnName == tr("Time")) {
            QDateTime dateTime = QDateTime::fromString(value, Qt::ISODate);
            value = dateTime.toString("hh:mm:ss"); // same format as ride list
        } else if (columnName == tr("Last updated")) {
            QDateTime dateTime;
            dateTime.setTime_t(index.model()->data(index, Qt::DisplayRole).toInt());
            value = dateTime.toString(tr("ddd MMM d, yyyy hh:mm")); // same format as ride list
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
    bool isnormal=false;
    QString calendarText = rideNavigator->tableView->model()->data(index, Qt::UserRole).toString();
    QColor userColor = rideNavigator->tableView->model()->data(index, Qt::BackgroundRole).value<QBrush>().color();
    if (userColor == QColor(1,1,1)) {
        rideBG = false; // default so don't swap round...
        isnormal = true; // just default so no bg or box
        userColor = GColor(CPLOTMARKER);
    }

    // basic background
    QBrush background = QBrush(GColor(CPLOTBACKGROUND));

    // runs are darker
    //if (isRun) {
        //background.setColor(background.color().darker(150));
        //userColor = userColor.darker(150);
    //}

    if (columnName != "*") {

        myOption.displayAlignment = Qt::AlignLeft | Qt::AlignTop;
        QRectF bigger(myOption.rect.x(), myOption.rect.y(), myOption.rect.width()+1, myOption.rect.height()+1);

        if (hover) painter->fillRect(myOption.rect, QColor(Qt::lightGray));
        else painter->fillRect(bigger, rideBG ? userColor : background);

        // clear first
        drawDisplay(painter, myOption, myOption.rect, ""); //added

        // draw border of each cell
        QPen rpen;
        rpen.setWidth(1);
        rpen.setColor(GColor(CPLOTBACKGROUND));
        QPen isColor = painter->pen();
        QFont isFont = painter->font();
        painter->setPen(rpen);
        painter->drawLine(0,myOption.rect.y(),rideNavigator->pwidth-1,myOption.rect.y());
        painter->drawLine(0,myOption.rect.y()+myOption.rect.height(),rideNavigator->pwidth-1,myOption.rect.y()+myOption.rect.height());
        painter->drawLine(0,myOption.rect.y()+myOption.rect.height(),0,myOption.rect.y()+myOption.rect.height());
        painter->drawLine(rideNavigator->pwidth-1, myOption.rect.y(), rideNavigator->pwidth-1, myOption.rect.y()+myOption.rect.height());

        // indent first column and draw all in plotmarker color
        myOption.rect.setHeight(rideNavigator->fontHeight + 2); //added
        myOption.font.setWeight(QFont::Bold);

        QFont boldened = painter->font();
        boldened.setWeight(QFont::Bold);
        painter->setFont(boldened);
        if (!selected) {
            // not selected, so invert ride plot color
            if (hover) painter->setPen(QColor(Qt::black));
            else painter->setPen(rideBG ? rideNavigator->reverseColor : userColor);
        } else if (!focus) { // selected but out of focus //
            painter->setPen(QColor(Qt::black));
        }

        QRect normal(myOption.rect.x(), myOption.rect.y()+1, myOption.rect.width(), myOption.rect.height());
        if (myOption.rect.x() == 0) {
            // first line ?
            QRect indented(myOption.rect.x()+5, myOption.rect.y()+1, myOption.rect.width()-5, myOption.rect.height());
            painter->drawText(indented, value); //added
        } else {
            painter->drawText(normal, value); //added
        }
        painter->setPen(isColor);
        painter->setFont(isFont);

        // now get the calendar text to appear ...
        if (calendarText != "") {
            QRect high(myOption.rect.x()+myOption.rect.width() - (7*dpiXFactor), myOption.rect.y(), (7*dpiXFactor), (rideNavigator->fontHeight+2) * 3);

            myOption.rect.setX(0);
            myOption.rect.setY(myOption.rect.y() + rideNavigator->fontHeight + 2);//was +23
            myOption.rect.setWidth(rideNavigator->pwidth);
            myOption.rect.setHeight(rideNavigator->fontHeight * 2); //was 36
            //myOption.font.setPointSize(myOption.font.pointSize());
            myOption.font.setWeight(QFont::Normal);

            if (hover) painter->fillRect(myOption.rect, QColor(Qt::lightGray)); 
            else painter->fillRect(myOption.rect, rideBG ? userColor : background.color());

            drawDisplay(painter, myOption, myOption.rect, "");
            myOption.rect.setX(10); // wider notes display
            myOption.rect.setWidth(pwidth-20);// wider notes display
            painter->setFont(myOption.font);
            QPen isColor = painter->pen();
            if (!selected) {
                // not selected, so invert ride plot color
                if (hover) painter->setPen(QPen(Qt::black));
                else painter->setPen(rideBG ? rideNavigator->reverseColor : GCColor::invertColor(GColor(CPLOTBACKGROUND)));
            }
            painter->drawText(myOption.rect, Qt::AlignLeft | Qt::TextWordWrap, calendarText);
            painter->setPen(isColor);

#if (defined (Q_OS_MAC) && (QT_VERSION >= 0x050000)) // on QT5 the scrollbars have no width
            if (!selected && !rideBG && high.x()+12 > rideNavigator->geometry().width() && !isnormal) {
#else
            if (!selected && !rideBG && high.x()+32 > rideNavigator->geometry().width() && !isnormal) {
#endif
                painter->fillRect(high, userColor);
            } else {

                // border
                QPen rpen;
                rpen.setWidth(1);
                rpen.setColor(GColor(CPLOTBACKGROUND));
                QPen isColor = painter->pen();
                QFont isFont = painter->font();
                painter->setPen(rpen);
                painter->drawLine(rideNavigator->pwidth-1, myOption.rect.y(), rideNavigator->pwidth-1, myOption.rect.y()+myOption.rect.height());
                painter->setPen(isColor);
            }
        }

    } else {

        if (value != "") {
            myOption.displayAlignment = Qt::AlignLeft | Qt::AlignBottom;
            myOption.rect.setX(0);
            myOption.rect.setHeight(rideNavigator->fontHeight + 2);
            myOption.rect.setWidth(rideNavigator->pwidth);
            painter->fillRect(myOption.rect, GColor(CPLOTBACKGROUND));
        }
        QPen isColor = painter->pen();
        painter->setPen(QPen(GColor(CPLOTMARKER)));
        myOption.palette.setColor(QPalette::WindowText, QColor(GColor(CPLOTMARKER))); //XXX
        painter->drawText(myOption.rect, value);
        painter->setPen(isColor);
    }
}

static bool insensitiveLessThan(const QString &a, const QString &b)
{
    return a.toLower() < b.toLower();
}

ColumnChooser::ColumnChooser(QList<QString>&logicalHeadings)
{
    // wipe away everything when you close please...
    setWindowTitle(tr("Column Chooser"));
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::Tool);

    clicked = new QSignalMapper(this); // maps each button click event
    connect(clicked, SIGNAL(mapped(const QString &)), this, SLOT(buttonClicked(const QString &)));

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
    qSort(buttonNames.begin(), buttonNames.end(), insensitiveLessThan);

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

RideTreeView::RideTreeView(QWidget *parent) : QTreeView(parent)
{
#if (defined WIN32) && (QT_VERSION > 0x050000) && (QT_VERSION < 0x050301) 
    // don't allow ride drop on Windows with QT5 until 5.3.1 when they fixed the bug
#else
    setDragDropMode(QAbstractItemView::InternalMove);
    setDragEnabled(true);
    setDragDropOverwriteMode(false);
    setDropIndicatorShown(true);
#endif
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
}


RideNavigatorSortProxyModel::RideNavigatorSortProxyModel(QObject *parent) : QSortFilterProxyModel (parent)
{
}

bool RideNavigatorSortProxyModel::lessThan(const QModelIndex &left,
                                           const QModelIndex &right) const
{
    QVariant leftData = sourceModel()->data(left);
    QVariant rightData = sourceModel()->data(right);

    if (leftData.type() == QVariant::DateTime) {
        return leftData.toDateTime() < rightData.toDateTime();
    }
    QString leftString = leftData.toString();
    QString rightString = rightData.toString();

    if (leftString.contains(QRegExp("[^0-9.,]")) ||
            rightString.contains(QRegExp("[^0-9.,]"))) { // alpha
        return QString::localeAwareCompare(leftString, rightString) < 0;
    }
    // assume numeric
    return leftString.toDouble() < rightString.toDouble();

}

