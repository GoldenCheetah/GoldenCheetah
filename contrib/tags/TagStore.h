/*
 * Copyright (c) 2023, Joachim Kohlhammer <joachim.kohlhammer@gmx.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
    virtual bool isDeferredTagSignals() = 0;
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
