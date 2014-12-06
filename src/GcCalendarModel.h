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

/********************* WARNING !!! *********************
 *
 * PLEASE DO NOT EDIT THIS SOURCE FILE IF YOU WANT TO
 * CHANGE THE WAY ROWS ARE GROUPED. THERE IS A FUNCTION
 * IN QxtScheduleView.cpp CALLED groupFromValue() WHICH
 * IS CALLED TO GET THE GROUP NAME FOR A COLUMN HEADING
 * VALUE COMBINATION. THIS IS CALLED FROM whichGroup()
 * BELOW.
 *
 * Of course, if there is a bug in this ProxyModel you
 * are welcome to fix it!
 * But do take care. Specifically with the index()
 * parent() mapToSource() and mapFromSource() functions.
 *
 *******************************************************/

#ifndef _GC_GcCalendarModel_h
#define _GC_GcCalendarModel_h 1
#include "GoldenCheetah.h"
#include "MainWindow.h"

#include <QtGui>
#include <QTextStream>
#include <QTableView>
#include <QItemDelegate>
#include <QDebug>
#include "qxtscheduleview.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"
#include "RideMetadata.h"
#ifdef GC_HAVE_ICAL
#include "ICalendar.h"
#endif
#include "Colors.h"
#include "Settings.h"

Q_DECLARE_METATYPE(QList<QColor>) // for passing back colors for each workout

// Proxy model for doing groupBy
class GcCalendarModel : public QAbstractProxyModel
{
    Q_OBJECT
    G_OBJECT


private:
    int month, year; // current month and year, default to this month
    bool stale;
    QVector<QDate> dates; // dates for each cell from zero onwards
    int rows;

    QMap <QDate, QVector<int>* > dateToRows; // map a date to SQL rows

    QList<QString> columns; // what columns in the sql model
    Context *context;
    int filenameIndex, durationIndex, dateIndex, textIndex, colorIndex;

public slots:

    void setStale() { stale = true; }
    void rideChange(RideItem *rideItem) {
        if (rideItem) {
            QDate date = rideItem->dateTime.date();
            if (month && year && date.month() == month && date.year() == year) stale = true;
        }
    }

    void refresh() {

        if (!sourceModel()) return; // no model yet!

        // ok, let em know we're about to reset!
        beginResetModel();

        QDate first = QDate(year, month, 1);
        // Date array
        int monthDays = first.daysTo(first.addMonths(1)); // how many days in this month?
        QDate firstDate = first.addDays((first.dayOfWeek()-1)*-1); // date in cell 0,0
        int ndays = firstDate.daysTo(QDate(year, month, monthDays));
        ndays += 7 - ndays % 7;

        dates.clear();
        dates.resize(ndays);
        for(int i=0; i<ndays; i++) dates[i] = firstDate.addDays(i);

        rows = ndays / 7;

        textIndex = colorIndex = filenameIndex = dateIndex = durationIndex = -1;
        columns.clear();
        for (int i=0; i<sourceModel()->columnCount(); i++) {
            QString column = sourceModel()->headerData (i, Qt::Horizontal, Qt::DisplayRole).toString();
            columns << column;
            if (durationIndex == -1 && column == tr("Duration")) durationIndex = i;
            if (dateIndex == -1 && column == tr("Date")) dateIndex = i;
            if (filenameIndex == -1 && column == tr("File")) filenameIndex = i;
            if (textIndex == -1 && column == tr("Calendar Text")) textIndex = i;
            if (colorIndex == -1 && column == "color") colorIndex = i;
        }

        // we need to build a list of all the rides
        // in the source model for the dates we have
        // first we clear previous
        QMapIterator<QDate, QVector<int>* > fm(dateToRows);
        while (fm.hasNext()) {
            fm.next();
            delete fm.value();
        }
        dateToRows.clear();

        // then we create new
        for (int j=0; j<sourceModel()->rowCount(); j++) {
            QVector<int> *arr;

            // get ride date
            QDateTime dateTime = sourceModel()->data(sourceModel()->index(j, dateIndex), Qt::DisplayRole).toDateTime();
            if ((arr = dateToRows.value(dateTime.date(), NULL)) == NULL)
                arr = new QVector<int>();

            arr->append(j);
            dateToRows.insert(dateTime.date(), arr);
        }

        // reset completed
	endResetModel();
    }

public:

    GcCalendarModel(QWidget *parent, QList<FieldDefinition> *, Context *context) : QAbstractProxyModel(parent), context(context) {
        setParent(parent);

        stale = true;
        QDate today = QDateTime::currentDateTime().date();
        setMonth(today.month(), today.year());

        // invalidate model if rides added or deleted or filter applier
        connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(rideChange(RideItem*)));
        connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(rideChange(RideItem*)));
        connect(context, SIGNAL(filterChanged()), this, SLOT(setStale()));

    }
    ~GcCalendarModel() {}

    void setMonth(int month, int year) {

        if (stale || this->month != month || this->year != year) {
            stale = false;
            this->month = month;
            this->year  = year;
            refresh();
        }
    }

    int getMonth() { return month; }
    int getYear() { return year; }

    QDate date(QModelIndex index) const {
        if (index.row()*7+index.column() < dates.count())
            return dates[index.row()*7+index.column()];
        else
            return QDate();
    }

    void setSourceModel(QAbstractItemModel *model) {
        QAbstractProxyModel::setSourceModel(model);
        connect(model, SIGNAL(modelReset()), this, SLOT(refresh()));
        connect(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(refresh()));
        connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(refresh()));
        connect(model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(refresh()));
        connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(refresh()));
#ifdef GC_HAVE_ICAL
        connect(context->athlete->rideCalendar, SIGNAL(dataChanged()), this, SLOT(refresh()));
#endif
        refresh();
    }

    QModelIndex index(int row, int column, const QModelIndex &/*parent*/ = QModelIndex()) const {
        return createIndex(row,column,(void *)NULL);
    }

    QModelIndex parent(const QModelIndex &index) const {
        // parent should be encoded in the index if we supplied it, if
        // we didn't then return a duffer
        if (index == QModelIndex() || index.internalPointer() == NULL) {
            return QModelIndex();
        } else if (index.column()) {
            return QModelIndex();
        }  else {
            return *static_cast<QModelIndex*>(index.internalPointer());
        }
    }

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const {
        return sourceModel()->index(proxyIndex.row(), proxyIndex.column(), QModelIndex());
    }

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const {

        return createIndex(sourceIndex.row(), sourceIndex.column(), (void *)NULL); // accommodate virtual column
    }

    // we override the standard version to make our virtual column zero
    // selectable. If we don't do that then the arrow keys don't work
    // since there are no valid rows to cursor up or down to.
    Qt::ItemFlags flags (const QModelIndex &/*index*/) const {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }

    enum UserRoles {
        DateStringRole = Qt::UserRole + 1,
        HeaderColorRole = DateStringRole + 1,
        CellColorRole = HeaderColorRole + 1,
        EventCountRole = CellColorRole + 1,
        FilenamesRole = EventCountRole + 1,
        DayRole = FilenamesRole + 1
    };

    QVariant data(const QModelIndex &proxyIndex, int role = Qt::DisplayRole) const {

        if (!proxyIndex.isValid()) return QVariant();

        QVariant returning;

        switch (role) {

        case Qt::BackgroundRole:
            {
            QList<QColor> colors;
            QVector<int> *arr = dateToRows.value(date(proxyIndex), NULL);
            if (arr) {
                foreach (int i, *arr) {
                    if (context->rideItem() && sourceModel()->data(index(i, dateIndex, QModelIndex())).toDateTime() == context->rideItem()->dateTime) {
                        colors << GColor(CCALCURRENT); // its the current ride!
                    } else {
                        QColor user = QColor(sourceModel()->data(index(i, colorIndex, QModelIndex())).toString());
                        if (user == QColor(1,1,1)) colors << GColor(CPLOTMARKER);
                        else colors << user;
                    }
                }
            }

#ifdef GC_HAVE_ICAL
            // added planned workouts
            for (int k= context->athlete->rideCalendar->data(date(proxyIndex), EventCountRole).toInt(); k>0; k--)
                colors.append(GColor(CCALPLANNED));
#endif

            return QVariant::fromValue<QList<QColor> >(colors);
            }
            break;

        case Qt::ForegroundRole:
            {
            QList<QColor> colors;
            QVector<int> *arr = dateToRows.value(date(proxyIndex), NULL);
            if (arr) {
                foreach (int i, *arr) {
                    QString filename = sourceModel()->data(index(i, filenameIndex, QModelIndex())).toString();
                    if (context->isfiltered && context->filters.contains(filename))
                        colors << GColor(CCALCURRENT);
                    else
                        colors << QColor(Qt::black);
                }
            }

#ifdef GC_HAVE_ICAL
            // added planned workouts
            for (int k= context->athlete->rideCalendar->data(date(proxyIndex), EventCountRole).toInt(); k>0; k--)
                colors.append(Qt::black);
#endif

            return QVariant::fromValue<QList<QColor> >(colors);
            }
            break;

        case Qt::FontRole:       // XXX Should return a list for each ride
                                 // XXX within the cell
            {
                QFont font;
                font.fromString(appsettings->value(this, GC_FONT_CALENDAR, QFont().toString()).toString());
                font.setPointSize(appsettings->value(this, GC_FONT_CALENDAR_SIZE, 12).toInt());
                return font;
            }
            break;

        case CellColorRole:   // what color for the cell?
            if (date(proxyIndex) == QDate::currentDate())
                return GColor(CCALTODAY);
            if (date(proxyIndex).month() == month)
                return GColor(CPLOTBACKGROUND);
            else
                return GColor(CPLOTBACKGROUND).darker(200);
            break;

        case HeaderColorRole: // what color for the cell heading
            if (date(proxyIndex).month() == month)
                return GColor(CPLOTBACKGROUND);
            else
                return GColor(CPLOTBACKGROUND).darker(200);
            break;

        case FilenamesRole:
            {
                QStringList filenames;
            // is there an entry?
            QVector<int> *arr = dateToRows.value(date(proxyIndex), NULL);
            QStringList strings;

            if (arr)
                foreach (int i, *arr)
                    filenames << sourceModel()->data(index(i, filenameIndex, QModelIndex())).toString();

#ifdef GC_HAVE_ICAL
            // fold in planned workouts
            if (context->athlete->rideCalendar->data(date(proxyIndex), EventCountRole).toInt()) {
                foreach(QString x, context->athlete->rideCalendar->data(date(proxyIndex), Qt::DisplayRole).toStringList())
                    filenames << "calendar";
            }
#endif

            return filenames;
            }

        case Qt::EditRole:
        case Qt::DisplayRole:   // returns the string to display
            {
            // is there an entry?
            QVector<int> *arr = dateToRows.value(date(proxyIndex), NULL);
            QStringList strings;

            if (arr)
                foreach (int i, *arr)
                    strings << sourceModel()->data(index(i, textIndex, QModelIndex())).toString();

#ifdef GC_HAVE_ICAL
            // fold in planned workouts
            if (context->athlete->rideCalendar->data(date(proxyIndex), EventCountRole).toInt()) {
                QStringList planned;
                planned = context->athlete->rideCalendar->data(date(proxyIndex), Qt::DisplayRole).toStringList();
                strings << planned;
            }
#endif
            return strings;
            }
            break;

        case DateStringRole: // how should this date be described in the
                             // cell heading
            {
            QDate today = date(proxyIndex);
            if (today.month() != month &&
                (today.addDays(1).month() == month || today.addDays(-1).month() == month))
                return QString("%1 %2").arg(today.day()).arg(QDate::shortMonthName(today.month()));
            else
                return QString("%1").arg(today.day());
            }
            break;
        case DayRole:
            {
            QDate today = date(proxyIndex);
            if (today.month() != month &&
                (today.addDays(1).month() == month || today.addDays(-1).month() == month))
                return "";
            else
                return QString("%1").arg(today.day());
            }
            break;
        default:
            return QVariant();
            break;
        }
        return QVariant();
    }

    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const {

        if (role == Qt::DisplayRole) {
            if (orientation == Qt::Horizontal) {
                return QDate::shortDayName(section+1);
            } else {
                return QString("%1").arg(date(index(section,0)).weekNumber());
            }
        }
        return QVariant();
    }

    bool setHeaderData (int , Qt::Orientation , const QVariant & , int = Qt::EditRole) {
        return true;
    }

    int columnCount(const QModelIndex & = QModelIndex()) const {
        return 7;
    }

    int rowCount(const QModelIndex & = QModelIndex()) const {
        return rows;
    }

    // does this index have children?
    bool hasChildren(const QModelIndex &) const {
        return false;
    }
};

// lets do the calendar delegate too since whilst we're at it...
class GcCalendarDelegate : public QItemDelegate
{
    Q_OBJECT
    G_OBJECT


    public:
    GcCalendarDelegate(QTableView *parent = 0) : QItemDelegate(parent) {}

    void createPainterPath(QPainterPath & emptyPath, const QRect & fullItemRect, const int iRoundTop, const int iRoundBottom) const {
        emptyPath = QPainterPath();
        bool bRoundTop = iRoundTop > 0;
        bool bRountBottom = iRoundBottom > 0;

        if (bRoundTop) {
            emptyPath.moveTo(fullItemRect.topLeft() + QPoint(0, iRoundTop));
            emptyPath.quadTo(fullItemRect.topLeft(), fullItemRect.topLeft() + QPoint(iRoundTop, 0));

        } else emptyPath.moveTo(fullItemRect.topLeft());

        emptyPath.lineTo(fullItemRect.topRight() - QPoint(iRoundTop, 0));

        if (bRoundTop)
            emptyPath.quadTo(fullItemRect.topRight(), fullItemRect.topRight() + QPoint(0, iRoundTop));

        emptyPath.lineTo(fullItemRect.bottomRight() - QPoint(0, iRoundBottom));

        if (bRountBottom)
            emptyPath.quadTo(fullItemRect.bottomRight(), fullItemRect.bottomRight() - QPoint(iRoundBottom, 0));

        emptyPath.lineTo(fullItemRect.bottomLeft() + QPoint(iRoundBottom, 0));

        if (bRountBottom)
            emptyPath.quadTo(fullItemRect.bottomLeft(), fullItemRect.bottomLeft() - QPoint(0, iRoundBottom));

        emptyPath.closeSubpath();
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const {

        painter->save();

        // font
        QVariant vfont = index.data(Qt::FontRole);
        painter->setFont(vfont.value<QFont>());

        // cell decoration
        QColor bg = index.data(GcCalendarModel::CellColorRole).value<QColor>();
        QColor hg = index.data(GcCalendarModel::HeaderColorRole).value<QColor>();


        // If selected then use selection color anyway...
        if (option.state & QStyle::State_Selected) bg = option.palette.highlight().color();
        painter->fillRect(option.rect, bg);

        // still paint header
        QRect hd(option.rect.x(), option.rect.y(), option.rect.width(), 15);
        painter->fillRect(hd, hg);

        // date...
        QString datestring = index.data(GcCalendarModel::DateStringRole).toString();
        QTextOption textOption(Qt::AlignRight);
        painter->setPen(GCColor::invertColor(hg));
        painter->drawText(hd, datestring, textOption);

        // text
        QStringList texts = index.data(Qt::DisplayRole).toStringList();
        QList<QColor> colors = index.data(Qt::BackgroundRole).value<QList<QColor> >();
        QList<QColor> pens =  index.data(Qt::ForegroundRole).value<QList<QColor> >();
        if (texts.count()) {

            int height = (option.rect.height()-21) / texts.count();
            int y = option.rect.y()+17;
            int i=0;
            foreach (QString text, texts) {

                QRect bd(option.rect.x()+2, y, option.rect.width()-4, height);
                y += height+2;

                QPainterPath cachePath;
                createPainterPath(cachePath, bd, 4, 4);

                QPen pen;
                QColor color = pens[i];
                pen.setColor(color);

                // if not black use it for border
                if (color != QColor(Qt::black)) painter->setPen(pen);
                else painter->setPen(Qt::NoPen);
                QColor fillColor = colors[i++];
                fillColor.setAlpha(200);
                painter->setBrush(fillColor);
                painter->setRenderHint(QPainter::Antialiasing);
                painter->drawPath(cachePath);

                // text always needs a pen
                painter->setPen(pen);
                painter->drawText(bd, text);
            }
        }
        painter->restore();
    }

};

#endif
