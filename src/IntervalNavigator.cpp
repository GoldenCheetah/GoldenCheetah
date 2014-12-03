/*
 * Copyright (c) 2014 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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
#include "RideItem.h"
#include "IntervalNavigator.h"
#include "IntervalNavigatorProxy.h"
#include "SearchFilterBox.h"
#include "TabView.h"

#include <QtGui>
#include <QString>
#include <QTreeView>
#include <QStyle>
#include <QStyleFactory>
#include <QScrollBar>

IntervalNavigator::IntervalNavigator(Context *context, QString type, bool mainwindow) : context(context), type(type), active(false), _groupBy(-1)
{
    // get column headings
    // default column layouts etc
    _columns = QString(tr("*|Date|Duration|"));
    _widths = QString("0|100|100|");
    _sortByIndex = 2;
    _sortByOrder = 0;
    currentColumn = -1;
    this->mainwindow = mainwindow;
    _groupBy = 8; // GroupName
    fontHeight = QFontMetrics(QFont()).height();
    ColorEngine ce(context);
    reverseColor = ce.reverseColor;

    init = false;

    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    if (mainwindow) mainLayout->setContentsMargins(0,0,0,0);
    else mainLayout->setContentsMargins(2,2,2,2); // so we can resize!

    if (type == "Best")
        sqlModel = context->athlete->sqlBestIntervalsModel;
    else
        sqlModel= context->athlete->sqlRouteIntervalsModel;

    //QString filter = QString("type='%1'").arg(type);
    //context->athlete->sqlIntervalsModel->setFilter(filter);
    sqlModel->select();


    while (sqlModel->canFetchMore(QModelIndex())) sqlModel->fetchMore(QModelIndex());

    searchFilter = new IntervalSearchFilter(this);
    searchFilter->setSourceModel(sqlModel); // filter out/in search results


    groupByModel = new IntervalGroupByModel(this);
    groupByModel->setSourceModel(searchFilter);

    sortModel = new BUGFIXQSortFilterProxyModel(this);
    sortModel->setSourceModel(groupByModel);
    sortModel->setDynamicSortFilter(true);

#ifdef GC_HAVE_LUCENE
    if (!mainwindow) {
        searchFilterBox = new SearchFilterBox(this, context, false);
        mainLayout->addWidget(searchFilterBox);
    }
#endif

    // get setup
    tableView = new IntervalGlobalTreeView;
    delegate = new IntervalNavigatorCellDelegate(this);
    tableView->setAnimated(true);
    tableView->setItemDelegate(delegate);
    tableView->setModel(sortModel);
    tableView->setSortingEnabled(true);
    tableView->setAlternatingRowColors(false);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers); // read-only
    mainLayout->addWidget(tableView);
    tableView->expandAll();
    tableView->header()->setCascadingSectionResizes(true); // easier to resize this way
    //tableView->setContextMenuPolicy(Qt::CustomContextMenu);
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
    tableView->setColumnWidth(1, 100);

    // good to go
    resetView();
    tableView->show();


    // refresh when database is updated
    connect(context->athlete->metricDB, SIGNAL(dataChanged()), this, SLOT(refresh()));

    // refresh when config changes (metric/imperial?)
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));
    // refresh when rides added/removed
    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh()));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh()));
    // selection of a ride by double clicking it, we need to update the ride list
    connect(tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(selectRide(QModelIndex)));
    connect(tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(cursorRide()));
    // user selected a ride on the ride list, we should reflect that too..
    // user moved columns
    connect(tableView->header(), SIGNAL(sectionMoved(int,int,int)), this, SLOT(columnsChanged()));
    connect(tableView->header(), SIGNAL(sectionResized(int,int,int)), this, SLOT(columnsResized(int, int, int)));
    // user sorted by column
    connect(tableView->header(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), this, SLOT(selectRow()));
    //connect(tableView,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showTreeContextMenuPopup(const QPoint &)));
    connect(tableView->header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(setSortBy(int,Qt::SortOrder)));

#ifdef GC_HAVE_LUCENE
    if (!mainwindow) {
        connect(searchFilterBox, SIGNAL(searchResults(QStringList)), this, SLOT(searchStrings(QStringList)));
        connect(searchFilterBox, SIGNAL(searchClear()), this, SLOT(clearSearch()));
    }
#endif

    // we accept drag and drop operations
    setAcceptDrops(true);

    // lets go
    configChanged();
}

IntervalColumnChooser::IntervalColumnChooser(QList<QString>&logicalHeadings)
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
    small.setPointSize(8);

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
        if (x > 5) {
            y++;
            x = 0;
        }
    }
}

void
IntervalColumnChooser::buttonClicked(QString name)
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

IntervalNavigator::~IntervalNavigator()
{
    delete tableView;
    delete groupByModel;
}

void
IntervalNavigator::configChanged()
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
        if (appsettings->value(this, GC_RIDEHEAD, true).toBool() == false)
            tableView->header()->hide();
        else 
            tableView->header()->show();

        tableView->header()->setStyleSheet(
        QString("QHeaderView { background-color: %1; color: %2; }"
                "QHeaderView::section { background-color: %1; color: %2; "
                " border: 0px ; }")
                .arg(GColor(CPLOTBACKGROUND).name())
                .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name()));
    }

#endif

    refresh();
}

void
IntervalNavigator::refresh()
{
    //QString filter = QString("type='%1'").arg(type);
    //context->athlete->sqlIntervalsModel->setFilter(filter);
    sqlModel->select();
    while (sqlModel->canFetchMore(QModelIndex()))
        sqlModel->fetchMore(QModelIndex());

    active=false;

    setWidth(geometry().width());
}

void
IntervalNavigator::resizeEvent(QResizeEvent*)
{
    // ignore if main window .. we get told to resize
    // by the splitter mover
    if (mainwindow) return;

    setWidth(geometry().width());
}

void
IntervalNavigator::resetView()
{
    active = true;

    QList<QString> cols = _columns.split("|", QString::SkipEmptyParts);
    int widco = _widths.split("|", QString::SkipEmptyParts).count();

    // something is wrong with the config ? reset 
    if (widco != cols.count() || widco <= 1) {
        _columns = QString(tr("*|Date|Duration|"));
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
    nameMap.insert("groupName", tr("Group"));
    internalNameMap.insert("Group", tr("Group"));

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
    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
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

    columnsChanged();
}

void
IntervalNavigator::searchStrings(QStringList list)
{
    searchFilter->setStrings(list);
    setWidth(geometry().width());
}

void
IntervalNavigator::clearSearch()
{
    searchFilter->clearStrings();
    QApplication::processEvents(); // repaint/resize list view - scrollbar..
    setWidth(geometry().width());  // before we update column sizes!
}

void
IntervalNavigator::setWidth(int x)
{
    // use helper function
    setColumnWidth(x, false);

}

// make sure the columns are all neat and tidy when the ride navigator is shown
void
IntervalNavigator::showEvent(QShowEvent *)
{
    init = true;

    //setWidth(geometry().width());
}

// routines called by the sidebar to let the user
// update the columns/grouping without using right-click
QStringList
IntervalNavigator::columnNames() const
{
    return visualHeadings;
}

void
IntervalNavigator::setGroupByColumnName(QString name)
{
    name = "Group";

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
IntervalNavigator::columnsChanged()
{
    // do the work - (column changed, but no "inWidget" column resize)
    calcColumnsChanged(false);
}

void
IntervalNavigator::columnsResized(int logicalIndex, int oldSize, int newSize)
{

    // do the work - resize only
    calcColumnsChanged(true, logicalIndex, oldSize, newSize);
}

bool
IntervalNavigator::eventFilter(QObject *object, QEvent *e)
{
    // not for the table?
    if (object != (QObject *)tableView) return false;

    // what happened?
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
IntervalNavigator::borderMenu(const QPoint &pos)
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

    // set current column...
    currentColumn = column;
    menu.exec(tableView->mapToGlobal(QPoint(pos.x(), pos.y())));
}

void
IntervalNavigator::setGroupByColumn()
{
    // toggle
    //setGroupBy(_groupBy >= 0 ? -1 : currentColumn);

    // set proxy model
    groupByModel->setGroupBy(_groupBy);

    // lets expand column 0 for the groupBy heading
    for (int i=0; i < groupByModel->groupCount(); i++) {
        tableView->setFirstColumnSpanned (i, QModelIndex(), true);
    }

    // now show em
    tableView->expandAll();

}

void
IntervalNavigator::setSortBy(int index, Qt::SortOrder order)
{
    _sortByIndex = index;
    _sortByOrder = static_cast<int>(order);
}


void
IntervalNavigator::calcColumnsChanged(bool resized, int logicalIndex, int oldSize, int newSize ) {

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
IntervalNavigator::setColumnWidth(int x, bool resized, int logicalIndex, int oldWidth, int newWidth) {

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

void
IntervalNavigator::removeColumn()
{
    active = true;
    tableView->setColumnHidden(currentColumn, true);
    active = false;
    
    setWidth(geometry().width()); // calculate width...
    columnsChanged(); // need to do after, just once
    columnsChanged(); // need to do after, and again
}

void
IntervalNavigator::showColumnChooser()
{
    IntervalColumnChooser *selector = new IntervalColumnChooser(logicalHeadings);
    selector->show();
}


// user double clicked a ride, we need to update the main window ride list
void
IntervalNavigator::selectRide(const QModelIndex &index)
{
    QModelIndex fileIndex = tableView->model()->index(index.row(), 3, index.parent()); // column 2 for filename ?

    QString filename = tableView->model()->data(fileIndex, Qt::DisplayRole).toString();

    context->athlete->selectRideFile(filename);

    // interval
    // start : 10
    // stop : 11
    fileIndex = tableView->model()->index(index.row(), 6, index.parent()); // column 10 for start ?
    QDateTime date = tableView->model()->data(fileIndex, Qt::DisplayRole).toDateTime();
    fileIndex = tableView->model()->index(index.row(), 10, index.parent()); // column 10 for start ?
    int startInterval = tableView->model()->data(fileIndex, Qt::DisplayRole).toInt();
    fileIndex = tableView->model()->index(index.row(), 11, index.parent()); // column 11 for stop ?
    int stopInterval = tableView->model()->data(fileIndex, Qt::DisplayRole).toInt();

    const RideFile *ride = context->ride ? context->ride->ride() : NULL;

    RideFile* f = new RideFile(const_cast<RideFile*>(ride));
    int start = ride->timeIndex(startInterval);
    int end = ride->timeIndex(stopInterval);

    for (int i = start; i <= end; ++i) {
        const RideFilePoint *p = ride->dataPoints()[i];
        f->appendPoint(p->secs, p->cad, p->hr, p->km, p->kph, p->nm,
                      p->watts, p->alt, p->lon, p->lat, p->headwind, p->slope, p->temp, p->lrbalance,
                      p->lte, p->rte, p->lps, p->rps, p->smo2, p->thb,
                      p->rvert, p->rcad, p->rcontact, 0);

        // derived data
        RideFilePoint *l = f->dataPoints().last();
        l->np = p->np;
        l->xp = p->xp;
        l->apower = p->apower;
    }

    f->clearIntervals();
    f->addInterval(start, end, "1");

    RideItem* rideItem = new RideItem(f, date, context );

    // emit signal!
    context->notifyRideSelected(rideItem);
}

// user cursor moved to ride
void
IntervalNavigator::cursorRide()
{
    if (active ==  true) return;
    else active = true;
    selectRide(tableView->currentIndex());
    active = false;
}

// fixup selection lost when columns sorted etc
void
IntervalNavigator::selectRow()
{
    // this is fugly and either a bug in QtreeView sorting
    // or a bug in our QAbstractProxyModel.
    // XXX need to work this out for first show XXX
}

// Drag and drop columns from the chooser...
void
IntervalNavigator::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->data("application/x-columnchooser") != "")
        event->acceptProposedAction(); // whatever you wanna drop we will try and process!
    else
        event->ignore();
}

void
IntervalNavigator::dropEvent(QDropEvent *event)
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

IntervalNavigatorCellDelegate::IntervalNavigatorCellDelegate(IntervalNavigator *intervalNavigator, QObject *parent) :
    QItemDelegate(parent), intervalNavigator(intervalNavigator), pwidth(300)
{
}

// Editing functions are null since the model is read-only
QWidget *IntervalNavigatorCellDelegate::createEditor(QWidget *, const QStyleOptionViewItem &, const QModelIndex &) const { return NULL; }
void IntervalNavigatorCellDelegate::commitAndCloseEditor() { }
void IntervalNavigatorCellDelegate::setEditorData(QWidget *, const QModelIndex &) const { }
void IntervalNavigatorCellDelegate::updateEditorGeometry(QWidget *, const QStyleOptionViewItem &, const QModelIndex &) const {}
void IntervalNavigatorCellDelegate::setModelData(QWidget *, QAbstractItemModel *, const QModelIndex &) const { }

QSize IntervalNavigatorCellDelegate::sizeHint(const QStyleOptionViewItem & /*option*/, const QModelIndex &index) const
{
    QSize s;

    if (intervalNavigator->groupByModel->mapToSource(intervalNavigator->sortModel->mapToSource(index)) != QModelIndex() &&
        intervalNavigator->groupByModel->data(intervalNavigator->sortModel->mapToSource(index), Qt::UserRole).toString() != "") {
        s.setHeight((intervalNavigator->fontHeight+2) * 3);
    } else s.setHeight(intervalNavigator->fontHeight + 2);
    return s;
}

// anomalies are underlined in red, otherwise straight paintjob
void IntervalNavigatorCellDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{

    // paint background for user defined color ?
    bool rideBG = appsettings->value(this,GC_RIDEBG,false).toBool();

    // state of item
    bool hover = option.state & QStyle::State_MouseOver;
    bool selected = option.state & QStyle::State_Selected;
    bool focus = option.state & QStyle::State_HasFocus;
    bool isRun = intervalNavigator->tableView->model()->data(index, Qt::UserRole+2).toBool();

    // format the cell depending upon what it is...
    QString columnName = intervalNavigator->tableView->model()->headerData(index.column(), Qt::Horizontal).toString();
    const RideMetric *m;
    QString value;

    // are we a selected cell ? need to paint accordingly
    //bool selected = false;
    //if (IntervalNavigator->tableView->selectionModel()->selectedIndexes().count()) { // zero if no rides in list
        //if (IntervalNavigator->tableView->selectionModel()->selectedIndexes().value(0).row() == index.row())
            //selected = true;
    //}

    if ((m=intervalNavigator->columnMetrics.value(columnName, NULL)) != NULL) {
        // format as a metric

        // get double from sqlIntervalsModel
        double metricValue = index.model()->data(index, Qt::DisplayRole).toDouble();

        if (metricValue) {
            // metric / imperial conversion
            metricValue *= (intervalNavigator->context->athlete->useMetricUnits) ? 1 : m->conversion();
            metricValue += (intervalNavigator->context->athlete->useMetricUnits) ? 0 : m->conversionSum();

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
    QString calendarText = intervalNavigator->tableView->model()->data(index, Qt::UserRole).toString();
    QColor userColor = intervalNavigator->tableView->model()->data(index, Qt::BackgroundRole).value<QBrush>().color();
    if (userColor == QColor(1,1,1)) {
        rideBG = false; // default so don't swap round...
        userColor = GColor(CPLOTMARKER);
    }

    // basic background
    QBrush background = QBrush(GColor(CPLOTBACKGROUND));

    // runs are darker
    if (isRun) {
        background.setColor(background.color().darker(150));
        userColor = userColor.darker(150);
    }

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
        rpen.setColor(GColor(CPLOTGRID));
        QPen isColor = painter->pen();
        QFont isFont = painter->font();
        painter->setPen(rpen);
        painter->drawLine(0,myOption.rect.y(),intervalNavigator->pwidth-1,myOption.rect.y());
        painter->drawLine(0,myOption.rect.y()+myOption.rect.height(),intervalNavigator->pwidth-1,myOption.rect.y()+myOption.rect.height());
        painter->drawLine(0,myOption.rect.y()+myOption.rect.height(),0,myOption.rect.y()+myOption.rect.height());
        painter->drawLine(intervalNavigator->pwidth-1, myOption.rect.y(), intervalNavigator->pwidth-1, myOption.rect.y()+myOption.rect.height());

        // indent first column and draw all in plotmarker color
        myOption.rect.setHeight(intervalNavigator->fontHeight + 2); //added
        myOption.font.setWeight(QFont::Bold);

        QFont boldened = painter->font();
        boldened.setWeight(QFont::Bold);
        painter->setFont(boldened);
        if (!selected) {
            // not selected, so invert ride plot color
            if (hover) painter->setPen(QColor(Qt::black));
            else painter->setPen(rideBG ? intervalNavigator->reverseColor : userColor);
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
            QRect high(myOption.rect.x()+myOption.rect.width() - 7, myOption.rect.y(), 7, (intervalNavigator->fontHeight+2) * 3);

            myOption.rect.setX(0);
            myOption.rect.setY(myOption.rect.y() + intervalNavigator->fontHeight + 2);//was +23
            myOption.rect.setWidth(intervalNavigator->pwidth);
            myOption.rect.setHeight(intervalNavigator->fontHeight * 2); //was 36
            myOption.font.setPointSize(myOption.font.pointSize());
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
                else painter->setPen(rideBG ? intervalNavigator->reverseColor : GCColor::invertColor(GColor(CPLOTBACKGROUND)));
            }
            painter->drawText(myOption.rect, Qt::AlignLeft | Qt::TextWordWrap, calendarText);
            painter->setPen(isColor);

#if (defined (Q_OS_MAC) && (QT_VERSION >= 0x050000)) // on QT5 the scrollbars have no width
            if (!selected && !rideBG && high.x()+12 > intervalNavigator->geometry().width() && userColor != GColor(CPLOTMARKER)) {
#else
            if (!selected && !rideBG && high.x()+32 > intervalNavigator->geometry().width() && userColor != GColor(CPLOTMARKER)) {
#endif
                painter->fillRect(high, userColor);
            } else {

                // border
                QPen rpen;
                rpen.setWidth(1);
                rpen.setColor(GColor(CPLOTGRID));
                QPen isColor = painter->pen();
                QFont isFont = painter->font();
                painter->setPen(rpen);
                painter->drawLine(intervalNavigator->pwidth-1, myOption.rect.y(), intervalNavigator->pwidth-1, myOption.rect.y()+myOption.rect.height());
                painter->setPen(isColor);
            }
        }

    } else {

        if (value != "") {
            myOption.displayAlignment = Qt::AlignLeft | Qt::AlignBottom;
            myOption.rect.setX(0);
            myOption.rect.setHeight(intervalNavigator->fontHeight + 2);
            myOption.rect.setWidth(intervalNavigator->pwidth);
            painter->fillRect(myOption.rect, GColor(CPLOTBACKGROUND));
        }
        QPen isColor = painter->pen();
        painter->setPen(QPen(GColor(CPLOTMARKER)));
        myOption.palette.setColor(QPalette::WindowText, QColor(GColor(CPLOTMARKER))); //XXX
        painter->drawText(myOption.rect, value);
        painter->setPen(isColor);
    }
}

void
IntervalNavigator::showTreeContextMenuPopup(const QPoint &pos)
{
    // map to global does not take into account the height of the header (??)
    // so we take it off the result of map to global

    // in the past this called mainwindow routinesfor the menu -- that was
    // a bad design since it coupled the ride navigator with the gui
    // we emit signals now, which only the sidebar is interested in trapping
    // so the activity log for example doesn't have a context menu now
    emit customContextMenuRequested(tableView->mapToGlobal(pos+QPoint(0,tableView->header()->geometry().height())));
}

IntervalGlobalTreeView::IntervalGlobalTreeView()
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
