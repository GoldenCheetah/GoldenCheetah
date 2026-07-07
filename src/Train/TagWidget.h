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

#ifndef TAGWIDGET_H
#define TAGWIDGET_H

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QColor>
#include <QString>
#include <QPropertyAnimation>


class TagWidget : public QFrame
{
    Q_OBJECT

public:
    TagWidget(int id, const QString &text, bool fadein = false, const QColor &color = QColor(), QWidget *parent = nullptr);
    ~TagWidget();

    int getId() const;
    QString getLabel() const;

    void setColor(const QColor &color);

    void updateAnimation();

    static QString mkObjectName(int id);

signals:
    void deleted(int id);

private slots:
    void animationFinished();

private:
    const int id;
    bool deleteScheduled;
    QPushButton *delButton;
    QLabel *label;
    QPropertyAnimation *animation;

    void deleteAnimation();
};

#endif
