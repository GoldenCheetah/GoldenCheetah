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

/*******************************************************
 *
 * This is a copy of the RideNavigatorProxy
 * but with less functionalities :
 * User can't change the group by
 *
 *******************************************************/

#ifndef _GC_IntervalNavigatorProxy_h
#define _GC_IntervalavigatorProxy_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include "IntervalNavigator.h"
#include "RideItem.h"
#include "RideFile.h"

// Proxy model for doing groupBy
class IntervalGroupByModel : public QAbstractProxyModel
{
    Q_OBJECT
    G_OBJECT


private:
    // used to produce a ranked list
    struct rankx {
        double row;
        double value;

        bool operator< (rankx right) const {
            return (value > right.value);  // sort ascending! (.gt not .lt)
        }
        static bool sortByRow(const rankx &left, const rankx &right) {
            return (left.row < right.row); // sort ascending by row
        }
    };

    IntervalNavigator *intervalNavigator;
    int groupBy;
    int calendarText;
    int colorColumn;
    int fileIndex;
    int isRunIndex;
    int tempIndex;
    int dateColumn;

    QString starttimeHeader;

    QList<QString> groups;
    QList<QModelIndex> groupIndexes;

    QMap<QString, QVector<int>*> groupToSourceRow;
    QVector<int> sourceRowToGroupRow;
    QList<rankx> rankedRows;

    void clearGroups() {
        // Wipe current
        QMapIterator<QString, QVector<int>*> i(groupToSourceRow);
        while (i.hasNext()) {
            i.next();
            delete i.value();
        }
        groups.clear();
        groupIndexes.clear();
        groupToSourceRow.clear();
        sourceRowToGroupRow.clear();
        rankedRows.clear();
    }

public:

    IntervalGroupByModel(IntervalNavigator *parent) : QAbstractProxyModel(parent), intervalNavigator(parent), groupBy(-1) {
        setParent(parent);
    }
    ~IntervalGroupByModel() {}

    void setSourceModel(QAbstractItemModel *model) {
        QAbstractProxyModel::setSourceModel(model);
        setGroupBy(groupBy);

        // find the Calendar TextColumn
        calendarText = -1;
        colorColumn = -1;
        fileIndex = -1;
        isRunIndex = -1;
        tempIndex = -1;
        for(int i=0; i<model->columnCount(); i++) {
            if (model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() == "Xaverage_temp") {
                tempIndex = i;
            }
            if (model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() == "isRun") {
                isRunIndex = i;
            }
            if (model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() == "filename") {
                fileIndex = i;
            }
            if (model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() == "color") {
                colorColumn = i;
            }
            if (model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() == "ZCalendar_Text") {
                calendarText = i;
            }
            if (model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() == "ride_date") {
                dateColumn = i;
            }
            starttimeHeader = "ride_time"; //initialisation with techname
        }

        connect(model, SIGNAL(modelReset()), this, SLOT(sourceModelChanged()));
        connect(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(sourceModelChanged()));
        connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(sourceModelChanged()));
        connect(model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(sourceModelChanged()));
        connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(sourceModelChanged()));
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const {
        if (parent.isValid()) {
            return createIndex(row,column, (void*)&groupIndexes[parent.row()]);
        } else { // XXX
            if (column == 0)
                return groupIndexes[row];
            else {
                return createIndex(row,column,(void*)NULL);
            }
        }
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

        if (proxyIndex.internalPointer() != NULL) {

            int groupNo = ((QModelIndex*)proxyIndex.internalPointer())->row();
            if (groupNo < 0 || groupNo >= groups.count() || proxyIndex.column() == 0) {
                return QModelIndex();
            }

            return sourceModel()->index(groupToSourceRow.value(groups[groupNo])->at(proxyIndex.row()),
                                        proxyIndex.column()-2, // accommodate virtual columns
                                        QModelIndex());
        }
        return QModelIndex();
    }

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const {

        // which group did we put this row into?
        QString group = whichGroup(sourceIndex.row());
        int groupNo = groups.indexOf(group);

        if (groupNo < 0) {
            return QModelIndex();
        } else {
            QModelIndex *p = new QModelIndex(createIndex(groupNo, 0, (void*)NULL));
            if (sourceIndex.row() > 0 && sourceIndex.row() < sourceRowToGroupRow.size())
                return createIndex(sourceRowToGroupRow[sourceIndex.row()], sourceIndex.column()+2, &p); // accommodate virtual columns
            else
                return QModelIndex();
        }
    }

    // we override the standard version to make our virtual column zero
    // selectable. If we don't do that then the arrow keys don't work
    // since there are no valid rows to cursor up or down to.
    Qt::ItemFlags flags (const QModelIndex &/*index*/) const {

        //if (index.internalPointer() == NULL) {
        //    return Qt::ItemIsEnabled;
        //} else {
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled;
        //}
    }

    QStringList mimeTypes() const {

        QStringList returning;
        returning << "application/x-gc-intervals";

        return returning;
    }

    QMimeData *mimeData (const QModelIndexList & /*indexes*/) const {

        QMimeData *returning = new QMimeData;

        // we need to pack into a byte array
        QByteArray rawData;
        QDataStream stream(&rawData, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_4_6);

        // pack data 
        stream << (quint64)(intervalNavigator->context); // where did this come from?

        RideItem *ride = intervalNavigator->context->ride;  // the currently selected ride

        // if this ride has any kind of content
        if (ride && ride->ride() && ride->ride()->dataPoints().count() > 1) {

            // serialize
            stream << (int)1;
            stream << QString(tr("Entire Ride"));
            stream << (quint64)(ride->ride()); // ridefile
            stream << (quint64)ride->ride()->dataPoints().first()->secs
                   << (quint64)ride->ride()->dataPoints().last()->secs;
            stream << (quint64)ride->ride()->dataPoints().first()->km
                   << (quint64)ride->ride()->dataPoints().last()->km;
            stream << (quint64)1;

        } else {
            stream << (int)0; // nothing
        }

        // and return as mime data
        returning->setData("application/x-gc-intervals", rawData);
        return returning;
    }

    QVariant data(const QModelIndex &proxyIndex, int role = Qt::DisplayRole) const {

        // this is never called ! (QT-BUG)
        if (role == Qt::TextAlignmentRole) {
            return QVariant(Qt::AlignLeft | Qt::AlignTop);
        }

        if (!proxyIndex.isValid()) return QVariant();
        QVariant returning;

        // if we are not at column 0 or we have a parent
        //if (proxyIndex.internalPointer() != NULL || proxyIndex.column() > 0)
        if (proxyIndex.column() > 0) {

            if (role == Qt::UserRole) {

                QString string;

                if (calendarText != -1 && proxyIndex.internalPointer()) {

                    // hideous code, sorry
                    int groupNo = ((QModelIndex*)proxyIndex.internalPointer())->row();
                    if (groupNo < 0 || groupNo >= groups.count() || proxyIndex.column() == 0) returning="";
                    else string = sourceModel()->data(sourceModel()->index(groupToSourceRow.value(groups[groupNo])->at(proxyIndex.row()), calendarText)).toString();

                    // get rid of cr, lf and tab chars
                    string.replace("\n", " ");
                    string.replace("\t", " ");
                    string.replace("\r", " ");

                } else {

                    string = "";
                }

                returning = string;


            } else if (role == Qt::ForegroundRole || role == Qt::BackgroundRole) {


                if (colorColumn != -1 && proxyIndex.internalPointer()) {

                    QString colorstring;

                    // hideous code, sorry
                    int groupNo = ((QModelIndex*)proxyIndex.internalPointer())->row();
                    if (groupNo < 0 || groupNo >= groups.count() || proxyIndex.column() == 0)
                        colorstring= GColor(CPLOTMARKER).name();
                    else colorstring = sourceModel()->data(sourceModel()->index(groupToSourceRow.value(groups[groupNo])->at(proxyIndex.row()), colorColumn)).toString();

                    returning = QColor(colorstring);
                } else {
                    returning = GColor(CPLOTMARKER).name();
                }

            } else if (role == (Qt::UserRole+1)) { // FILENAME ?

                if (colorColumn != -1 && proxyIndex.internalPointer()) {

                    QString filename;

                    // hideous code, sorry
                    int groupNo = ((QModelIndex*)proxyIndex.internalPointer())->row();
                    if (groupNo < 0 || groupNo >= groups.count() || proxyIndex.column() == 0)
                        filename="";
                    else filename = sourceModel()->data(sourceModel()->index(groupToSourceRow.value(groups[groupNo])->at(proxyIndex.row()), fileIndex)).toString();

                    returning = filename;
                } else {
                    returning = "";
                }

            } else if (role == (Qt::UserRole+2)) { // isRUN ?

                if (isRunIndex != -1 && proxyIndex.internalPointer()) {

                    bool isRun = false;

                    // hideous code, sorry
                    int groupNo = ((QModelIndex*)proxyIndex.internalPointer())->row();
                    if (groupNo < 0 || groupNo >= groups.count() || proxyIndex.column() == 0)
                        returning = false;
                    else isRun = sourceModel()->data(sourceModel()->index(groupToSourceRow.value(groups[groupNo])->at(proxyIndex.row()), isRunIndex)).toBool();

                    returning = isRun;
                } else {
                    returning = false;
                }

            } else {

                // column 1 = ride_time we have to use ride_date
                if (proxyIndex.column() == 1)  {
                    QString date;

                    // hideous code, sorry
                    int groupNo = ((QModelIndex*)proxyIndex.internalPointer())->row();
                    if (groupNo < 0 || groupNo >= groups.count() || proxyIndex.column() == 0)
                        date="";
                    else date = sourceModel()->data(sourceModel()->index(groupToSourceRow.value(groups[groupNo])->at(proxyIndex.row()), dateColumn)).toString();

                    returning = date;//sourceModel()->data(sourceModel()->index(proxyIndex.row(),dateColumn)).toString();

                } else {

                    // get the data
                    returning = sourceModel()->data(mapToSource(proxyIndex), role);

                    // -255 temperature means not present
                    if (mapToSource(proxyIndex).column() == tempIndex && returning.toDouble() == RideFile::NoTemp) {
                         returning = "";
                    }
                }
            }

        } else if (proxyIndex.internalPointer() == NULL) {

            // its our group by!
            if (proxyIndex.row() < groups.count()) {

                // blank values become "(blank)"
                QString group = groups[proxyIndex.row()];
                if (group == "") group = QString("(blank)");

                // format the group by with ride count etc
                if (groupBy != -1) {
                    //QString returnString = QString(tr("%1: %2 (%3 rides)"))
                    //                       .arg(sourceModel()->headerData(groupBy, Qt::Horizontal).toString())
                    //                       .arg(group)
                    //                       .arg(groupToSourceRow.value(groups[proxyIndex.row()])->count());
                    QString returnString = QString(tr("%2 (%3)"))
                                           .arg(group)
                                           .arg(groupToSourceRow.value(groups[proxyIndex.row()])->count());
                    returning = QVariant(returnString);
                } else {
                    QString returnString = QString(tr("%1 rides"))
                                           .arg(groupToSourceRow.value(groups[proxyIndex.row()])->count());
                    returning = QVariant(returnString);
                }
            }
        }

        // post process - temperatures of -255 are no available !
        return returning;

    }

    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const {
        if (section>1)
            return sourceModel()->headerData(section-2, orientation, role); // accommodate virtual columns
        else if (section == 1) // return header for virtual column ride_time
            return  QVariant(starttimeHeader);
        else
            return QVariant("*");
    }

    bool setHeaderData (int section, Qt::Orientation orientation, const QVariant & value, int role = Qt::EditRole) {
        if (section>1)
            return sourceModel()->setHeaderData(section-2, orientation, value, role); // accommodate virtual columns
        else if (section == 1) // set header for virtual column ride_time
            starttimeHeader = value.toString();

        return true;
    }

    int columnCount(const QModelIndex &/*parent*/ = QModelIndex()) const {
        return sourceModel()->columnCount(QModelIndex())+2; // accommodate virtual group column and starttime
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const {
        if (parent == QModelIndex()) {

            // top level return count of groups
            return groups.count();

        } else if (parent.column() == 0 && parent.internalPointer() == NULL) {

            // second level return count of rows for group
            return groupToSourceRow.value(groups[parent.row()])->count();

        } else {

            // no children any lower
            return 0;
        }
    }

    // does this index have children?
    bool hasChildren(const QModelIndex &index) const {

        if (index == QModelIndex()) {

            // at top
            return (groups.count() > 0);

        } else if (index.column() == 0 && index.internalPointer() == NULL) {

            // first column - the group bys
            return (groupToSourceRow.value(groups[index.row()])->count() > 0);

        } else {

            return false;

        }
    }

    void sort(int, Qt::SortOrder) {
        qDebug()<<"ASKED TO SORT!";
    }

    //
    // GroupBy features
    //
    void setGroupBy(int column) {

        // shift down
        if (column >= 0) column -= 2; // accommodate virtual column

        groupBy = column < 0 ? -1 : column;
        setGroups();
    }

    QString whichGroup(int row) const {

        if (row < 0 || row >= rankedRows.count()) return ("");
        if (groupBy == -1) return tr("All Rides");
        else return sourceModel()->data(sourceModel()->index(row,groupBy)).toString();

    }

    int groupCount() {
        return groups.count();
    }

    void setGroups() {

        // let the views know we're doing this
        beginResetModel();

        // wipe whatever is there first
        clearGroups();

        if (groupBy >= 0) {

            // rank all the values
            for (int i=0; i<sourceModel()->rowCount(QModelIndex()); i++) {
                rankx rank;
                rank.value = sourceModel()->data(sourceModel()->index(i,groupBy)).toDouble();
                rank.row = i;
                rankedRows << rank;
            }

            // rank the entries
            qSort(rankedRows); // sort by value
            for (int i=0; i<rankedRows.count(); i++) {
                rankedRows[i].value = i;
            }

            // sort by row again
            qStableSort(rankedRows.begin(), rankedRows.end(), rankx::sortByRow);


            // create a QMap from 'group' string to list of rows in that group
            for (int i=0; i<sourceModel()->rowCount(QModelIndex()); i++) {

                // which group are we in?
                QString value = whichGroup(i); // uses rankedRows

                QVector<int> *rows;
                if ((rows=groupToSourceRow.value(value,NULL)) == NULL) {
                    // add to list of groups
                    rows = new QVector<int>;
                    groupToSourceRow.insert(value, rows);
                }

                // rowmap is an array corresponding to each row in the
                // source model, and maps to its row # within the group
                sourceRowToGroupRow.append(rows->count());

                // add to this groups rows
                rows->append(i);
            }

        } else {

            // Just one group by 'All Rides'
            QVector<int> *rows = new QVector<int>;
            for (int i=0; i<sourceModel()->rowCount(QModelIndex()); i++) {
                rows->append(i);
                sourceRowToGroupRow.append(i);
            }
            groupToSourceRow.insert("All Rides", rows);

        }

        // Update list of groups
        int group=0;
        QMapIterator<QString, QVector<int>*> j(groupToSourceRow);
        while (j.hasNext()) {
            j.next();
            groups << j.key();
            groupIndexes << createIndex(group++,0,(void*)NULL);
        }

        // all done. let the views know everything changed
        endResetModel();
    }

public slots:
    void sourceModelChanged() {

        // notify everyone we're changing
        beginResetModel();

        clearGroups();
        setGroupBy(groupBy+2); // accommodate virtual columns

        endResetModel();// we're clean

        // lets expand column 0 for the groupBy heading
        for (int i=0; i < groupCount(); i++)
            intervalNavigator->tableView->setFirstColumnSpanned(i, QModelIndex(), true);
        // now show em
        intervalNavigator->tableView->expandAll();
    }
};

// SEE QT-BUG #14831 - when it is fixed this can be removed
class BUGFIXQSortFilterProxyModel : public QSortFilterProxyModel
{

    public:

    BUGFIXQSortFilterProxyModel(QWidget *p) : QSortFilterProxyModel(p) {}

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const {
        QVariant l = (left.model() ? left.model()->data(left, sortRole()) : QVariant());
        QVariant r = (right.model() ? right.model()->data(right, sortRole()) : QVariant());
        switch (l.userType()) {
            case QVariant::Invalid: return (r.type() == QVariant::Invalid);
            default: return QSortFilterProxyModel::lessThan(left, right);
        }
        return false;
    }
};

class IntervalSearchFilter : public QSortFilterProxyModel
{

    Q_OBJECT

    public:

    IntervalSearchFilter(QWidget *p) : QSortFilterProxyModel(p), searchActive(false) {}

    void setSourceModel(QAbstractItemModel *model) {
        QAbstractProxyModel::setSourceModel(model);
        this->model = model;

        // find the filename column
        fileIndex = -1;
        for(int i=0; i<model->columnCount(); i++) {
            if (model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() == "filename"
                || model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() == tr("File")) {
                fileIndex = i;
            }
        }

	// make sure changes are propogated upstream
        connect(model, SIGNAL(modelReset()), this, SIGNAL(modelReset()));
        connect(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SIGNAL(modelReset()));
        connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SIGNAL(modelReset()));
        connect(model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SIGNAL(modelReset()));
        connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SIGNAL(modelReset()));
    }

    bool filterAcceptsRow (int source_row, const QModelIndex &source_parent) const {

        if (fileIndex == -1 || searchActive == false) return true; // nothing to do

        // lets get the filename
        QModelIndex source_index = model->index(source_row, fileIndex, source_parent);
        if (!source_index.isValid()) return true;

        QString key = model->data(source_index, Qt::DisplayRole).toString();
        return strings.contains(key);
    }

    public slots:

    void setStrings(QStringList list) {
        beginResetModel();
        strings = list;
        searchActive = true;
        endResetModel();
    }

    void clearStrings() {
        beginResetModel();
        strings.clear();
        searchActive = false;
        endResetModel();
    }

    private:
        QAbstractItemModel *model;
        QStringList strings;
        int fileIndex;
        bool searchActive;
};
#endif
