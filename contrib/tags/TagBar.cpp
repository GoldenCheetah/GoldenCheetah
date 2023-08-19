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

#include "TagBar.h"

#include <QDebug>
#include <QStringList>
#include <QStringListModel>
#include <QMessageBox>
#include <QTimer>
#include <QCompleter>
#include <QAbstractItemView>

#include <flowlayout.h>

#include "TagWidget.h"



TagEdit::TagEdit
(QWidget *parent)
: QLineEdit(parent)
{
}


void
TagEdit::keyPressEvent
(QKeyEvent *e)
{
    if (   completer() != nullptr
        && ! completer()->popup()->isVisible()
        && e->key() == Qt::Key_Escape) {
        completer()->setCompletionPrefix(text());
        completer()->complete();
        e->ignore();
    } else {
        QLineEdit::keyPressEvent(e);
    }
}



TagBar::TagBar
(TagStore * const tagStore, QWidget *parent)
: QWidget(parent), tagStore(tagStore), taggable(nullptr)
{
    FlowLayout *layout = new FlowLayout(0, -1, -1);

    QCompleter *completer = new QCompleter(tagStore->getTagLabels(), this);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);

    edit = new TagEdit();
    edit->setStyleSheet("QLineEdit { border: 0px; background-color: rgba(0, 0, 0, 0); }");
    edit->setPlaceholderText(tr("Add Tag..."));
    edit->setCompleter(completer);
    edit->setVisible(false);
    connect(edit, SIGNAL(returnPressed()), this, SLOT(tagNameEntered()));
    layout->addWidget(edit);

    setLayout(layout);
}


TagBar::~TagBar
()
{
}


void
TagBar::setTaggable
(Taggable *taggable)
{
    edit->clear();
    this->taggable = taggable;

    QLayoutItem *child;
    while (layout()->count() > 1 && (child = layout()->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    if (taggable != nullptr) {
        QLayoutItem *editItem = nullptr;
        if (layout()->count() == 1) {
            editItem = layout()->takeAt(0);
        }
        QList<int> tagIds = taggable->getTagIds();
        for (const auto &tagId : tagIds) {
            QString title = tagStore->getTagLabel(tagId);

            TagWidget *tagWidget = new TagWidget(tagId, title, false);
            connect(tagWidget, SIGNAL(deleted(int)), this, SLOT(removeTag(int)));
            layout()->addWidget(tagWidget);
        }
        layout()->addItem(editItem);
        edit->setVisible(true);
        setCompletionList();
    } else {
        edit->setVisible(false);
    }
    layout()->update();
}


QStringList
TagBar::getTagLabels
() const
{
    return tagStore->getTagLabels(taggable->getTagIds());
}


QList<int>
TagBar::getTagIds
() const
{
    return taggable->getTagIds();
}


bool
TagBar::addTag
(int id)
{
    if (taggable == nullptr || ! tagStore->hasTag(id)) {
        return false;
    }

    if (! taggable->hasTag(id)) {
        taggable->addTag(id);

        QString title = tagStore->getTagLabel(id);
        QLayoutItem *editItem = layout()->takeAt(layout()->count() - 1);

        TagWidget *tagWidget = new TagWidget(id, title, true);
        connect(tagWidget, SIGNAL(deleted(int)), this, SLOT(removeTag(int)));
        layout()->addWidget(tagWidget);

        layout()->addItem(editItem);
        setCompletionList();

        return true;
    } else {
        TagWidget *tagWidget = findChild<TagWidget*>(TagWidget::mkObjectName(id));
        if (tagWidget != nullptr) {
            tagWidget->updateAnimation();
        }
        return false;
    }
}


void
TagBar::removeTag
(int id)
{
    if (taggable == nullptr) {
        return;
    }
    if (taggable->hasTag(id)) {
        taggable->removeTag(id);
        QString searchName(TagWidget::mkObjectName(id));
        for (int i = 0; i < layout()->count() - 1; ++i) {
            QWidget *w = layout()->itemAt(i)->widget();
            if (w->objectName() == searchName) {
                QLayoutItem *l = layout()->takeAt(i);
                delete l->widget();
                delete l;
                layout()->update();
                break;
            }
        }
        setCompletionList();
    }
}


void
TagBar::tagNameEntered
()
{
    addTag(edit->text().trimmed().toHtmlEscaped());
    QTimer::singleShot(0, edit, SLOT(clear()));
}


void
TagBar::addTag
(const QString &label)
{
    if (taggable == nullptr) {
        return;
    }
    if (label.size() > 0) {
        bool add = true;
        int tagId = tagStore->getTagId(label);
        if (tagId == TAGSTORE_UNDEFINED_ID) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this,
                                          tr("Create new tag"),
                                          QString(tr("The tag \"<b>%1</b>\" does not exist yet.<br>Should it really be created?")).arg(label),
                                          QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                tagId = tagStore->addTag(label);
            } else {
                add = false;
            }
        }
        if (add) {
            addTag(tagId);
        }
    }
}


void
TagBar::setCompletionList
()
{
    if (taggable == nullptr) {
        return;
    }
    QStringList completionTagLabels = tagStore->getTagLabels();
    QStringList currentTagLabels = getTagLabels();
    for (const auto &i : currentTagLabels) {
        completionTagLabels.removeAll(i);
    }
    completionTagLabels.sort(Qt::CaseInsensitive);
    edit->completer()->setModel(new QStringListModel(completionTagLabels));
}


void
TagBar::tagStoreChanged
(int, int, int)
{
    setTaggable(taggable);
}
