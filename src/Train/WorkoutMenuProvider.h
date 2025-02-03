/*
 * Copyright (c) 2025, Joachim Kohlhammer <joachim.kohlhammer@gmx.de>
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

#ifndef WORKOUTMENUPROVIDER_H
#define WORKOUTMENUPROVIDER_H

#include <QObject>
#include <QList>
#include <QXmlStreamReader>
#include <QTreeWidget>
#include <QSortFilterProxyModel>

#include "WorkoutFilterBox.h"


class WorkoutFilterStore
{
public:
    struct StorableWorkoutFilter
    {
        StorableWorkoutFilter(const QString &name, const QString &filter) : name(name), filter(filter) {}
        QString name;
        QString filter;
    };

    WorkoutFilterStore(const QString &fileName);

    QList<StorableWorkoutFilter> getWorkoutFilters() const;
    void append(const QString &name, const QString &filter);
    void insert(int index, const QString &name, const QString &filter);
    void update(int index, const QString &name, const QString &filter);
    void remove(int index);
    void moveUp(int index);
    void moveDown(int index);

    void load();
    void save();

private:
    QList<StorableWorkoutFilter> workoutFilters;
    QString fileName;

    bool parseWorkoutFilters(QXmlStreamReader &reader);
    bool parseWorkoutFilter(QXmlStreamReader &reader);
};


class TopLevelFilterProxyModel : public QSortFilterProxyModel
{
public:
    TopLevelFilterProxyModel(QObject *parent = nullptr);

    void setContains(const QString &contains);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString contains;
};


class WorkoutMenuProvider : public QObject, public MenuProvider
{
    Q_OBJECT

public:
    WorkoutMenuProvider(WorkoutFilterBox *editor, const QString &filterFile);
    virtual ~WorkoutMenuProvider() {}
    void addActions(QMenu *menu) const override;

signals:
    void itemsRecreated();

private slots:
    void addTagDialog();
    void manageFiltersDialog();
    void addFilterDialog();

    void manageItemChanged(QTreeWidgetItem *item);
    void manageAdd(QTreeWidget *tree);
    void manageMoveUp(QTreeWidgetItem *item);
    void manageMoveDown(QTreeWidgetItem *item);
    void manageDelete(QTreeWidgetItem *item);

private:
    WorkoutFilterBox *editor;
    WorkoutFilterStore store;

    void manageFillItems(QTreeWidget *tree, int selected = 0);
};

#endif
