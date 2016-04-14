/*
 * Copyright (c) 2016 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_RChart_h
#define _GC_RChart_h 1

#include <RInside.h>
#include <QString>
#include <QDebug>
#include <QColor>
#include <QTextEdit>
#include <QScrollBar>
#include <QSplitter>
#include <QSvgWidget>
#include <string.h>

#include "GoldenCheetah.h"
#include "Context.h"
#include "Athlete.h"


// a console widget to type commands and display response
class RConsole : public QTextEdit {

    Q_OBJECT

signals:
    void getData(const QByteArray &data);

public slots:
    void configChanged(qint32);
    void rMessage(QString);

public:
    explicit RConsole(Context *context, QWidget *parent = 0);

    void putData(QString data);
    void putData(QColor color, QString data);
    void setLocalEchoEnabled(bool set);

    // return the current "line" of text
    QString currentLine();
    void setCurrentLine(QString);

    QStringList history;
    int hpos;

protected:
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);
    virtual void contextMenuEvent(QContextMenuEvent *e);

private:
    Context *context;
    bool localEchoEnabled;
};

// the chart
class RChart : public GcChartWindow {

    Q_OBJECT

    public:
        RChart(Context *context);

    protected:
        QSplitter *splitter;
        RConsole *console;
        QSvgWidget *surface;

    private:
        Context *context;
};


#endif
