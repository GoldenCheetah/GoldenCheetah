/*
 * Copyright (c) 2023, Joachim Kohlhammer <joachim.kohlhammer@gmx.de>
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

#ifndef TAGBAR_H
#define TAGBAR_H

#include <QWidget>
#include <QLineEdit>
#include <QString>
#include <QStringList>
#include <QList>
#include <QKeyEvent>

#include "TagStore.h"
#include "Taggable.h"


class TagEdit : public QLineEdit
{
public:
    TagEdit(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *e) override;
};


class TagBar : public QWidget
{
    Q_OBJECT

public:
    TagBar(TagStore * const tagStore, QWidget *parent = nullptr);
    ~TagBar();

    void setTaggable(Taggable *taggable);

    QStringList getTagLabels() const;
    QList<int> getTagIds() const;

    bool addTag(int id);

public slots:
    void tagStoreChanged(int idAdded, int idRemoved, int idUpdated);

private slots:
    void removeTag(int id);
    void tagNameEntered();
    void addTag(const QString &label);

private:
    TagStore * const tagStore;
    Taggable *taggable;
    TagEdit *edit;

    void setCompletionList();
};

#endif
