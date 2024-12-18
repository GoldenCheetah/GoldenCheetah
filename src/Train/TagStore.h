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

#ifndef TAGSTORE_H
#define TAGSTORE_H

#include <QString>
#include <QStringList>
#include <QList>

#define TAGSTORE_UNDEFINED_ID -1


class TagStore
{
public:
    struct Tag {
        Tag(int id, const QString &label): id(id), label(label) {}
        int id;
        QString label;
    };

    virtual void deferTagSignals(bool deferred) = 0;
    virtual bool isDeferredTagSignals() const = 0;
    virtual void catchupTagSignals() = 0;

    virtual int addTag(const QString &label) = 0;
    virtual bool updateTag(int id, const QString &label) = 0;
    virtual bool deleteTag(int id) = 0;
    virtual bool deleteTag(const QString &label) = 0;
    virtual int getTagId(const QString &label) const = 0;
    virtual QString getTagLabel(int id) const = 0;
    virtual bool hasTag(int id) const = 0;
    virtual bool hasTag(const QString &label) const = 0;

    virtual QList<Tag> getTags() const = 0;
    virtual QList<int> getTagIds() const = 0;
    virtual QStringList getTagLabels() const = 0;
    virtual QStringList getTagLabels(const QList<int> ids) const = 0;

    virtual int countTagUsage(int id) const = 0;

signals:
    virtual void tagsChanged(int idAdded, int idDeleted, int idUpdated) = 0;
    virtual void deferredTagsChanged(QList<int> idsAdded, QList<int> idsDeleted, QList<int> idsUpdated) = 0;
};

#endif
