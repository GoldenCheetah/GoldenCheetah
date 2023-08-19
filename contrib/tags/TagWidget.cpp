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

#include "TagWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDebug>
#include <QGraphicsOpacityEffect>


TagWidget::TagWidget
(int id, const QString &text, bool fadein, QWidget *parent)
: QFrame(parent), id(id), deleteScheduled(false)
{
    setStyleSheet("QFrame { border: 1px solid; border-radius: 6px; }");
    setContentsMargins(0, 0, 0, 0);

    setObjectName(mkObjectName(id));

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(5, 3, 5, 3);

    QString labelText = text.trimmed();
    label = new QLabel();
    if (labelText.size() > 15) {
        label->setText(labelText.left(6) + "..." + labelText.right(6));
        setToolTip(labelText);
    } else {
        label->setText(labelText);
    }
    label->setStyleSheet("QLabel { border: 0px; }");

    QPushButton *delButton = new QPushButton("✖");
    delButton->setFlat(true);
    delButton->setStyleSheet("QPushButton { padding: 0px; }");
    connect(delButton, &QPushButton::clicked, [=] { deleteScheduled = true; deleteAnimation(); });

    layout->addWidget(delButton);
    layout->addWidget(label);

    setLayout(layout);

    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(this);
    effect->setEnabled(false);
    setGraphicsEffect(effect);

    animation = new QPropertyAnimation(effect, "opacity", this);
    connect(animation, SIGNAL(finished()), this, SLOT(animationFinished()));
    if (fadein) {
        updateAnimation();
    }
}


TagWidget::~TagWidget
()
{
}


int
TagWidget::getId
() const
{
    return id;
}


QString
TagWidget::getLabel
() const
{
    return label->text();
}


void
TagWidget::updateAnimation
()
{
    graphicsEffect()->setEnabled(true);
    animation->setDuration(500);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->start();
}


void
TagWidget::animationFinished
()
{
    graphicsEffect()->setEnabled(false);
    if (deleteScheduled) {
        emit(deleted(id));
    }
}


void
TagWidget::deleteAnimation
()
{
    graphicsEffect()->setEnabled(true);
    animation->setDuration(300);
    animation->setStartValue(1.0);
    animation->setEndValue(0.0);
    animation->start();
}


QString
TagWidget::mkObjectName
(int id)
{
    return QString("TAGWIDGET%1").arg(id);
}
