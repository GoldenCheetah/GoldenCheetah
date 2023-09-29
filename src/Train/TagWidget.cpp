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

#include "TagWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDebug>
#include <QGraphicsOpacityEffect>


TagWidget::TagWidget
(int id, const QString &text, bool fadein, const QColor &color, QWidget *parent)
: QFrame(parent), id(id), deleteScheduled(false)
{
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

    delButton = new QPushButton("✖");
    delButton->setFlat(true);
    connect(delButton, &QPushButton::clicked, [=] { deleteScheduled = true; deleteAnimation(); });

    setColor(color);

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
TagWidget::setColor
(const QColor &color)
{
    if (color.isValid()) {
        QColor alphaColor = color;
        alphaColor.setAlpha(128);
        setStyleSheet(QString("QFrame { border: 1px solid %1; border-radius: 6px; }").arg(alphaColor.name(QColor::HexArgb)));
        label->setStyleSheet(QString("QLabel { border: 0px; color: %1; }").arg(color.name(QColor::HexRgb)));
        delButton->setStyleSheet(QString("QPushButton { padding: 0px; color: %1; }").arg(alphaColor.name(QColor::HexArgb)));
    } else {
        setStyleSheet("QFrame { border: 1px solid; border-radius: 6px; }");
        label->setStyleSheet("QLabel { border: 0px; }");
        delButton->setStyleSheet("QPushButton { padding: 0px; }");
    }
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
