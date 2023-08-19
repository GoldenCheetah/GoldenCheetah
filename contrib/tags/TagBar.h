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
