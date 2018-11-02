/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_PythonChart_h
#define _GC_PythonChart_h 1

#include <PythonEmbed.h>

#include <QString>
#include <QDebug>
#include <QColor>
#include <QTextEdit>
#include <QScrollBar>
#include <QCheckBox>
#include <QSplitter>
#include <QByteArray>
#include <string.h>
#include <QWebEngineView>
#include <QUrl>

#include "GoldenCheetah.h"
#include "Context.h"
#include "Athlete.h"
#include "RCanvas.h"

class PythonChart;

class PythonHost {
public:
    virtual PythonChart *chart() = 0;
    virtual bool readOnly() = 0;
};

// a console widget to type commands and display response
class PythonConsole : public QTextEdit {

    Q_OBJECT

signals:
    void getData(const QByteArray &data);

public slots:
    void configChanged(qint32);
    void rMessage(QString);

public:
    explicit PythonConsole(Context *context, PythonHost *pythonHost, QWidget *parent = 0);

    void putData(QString data);
    void putData(QColor color, QString data);
    void setLocalEchoEnabled(bool set);

    // return the current "line" of text
    QString currentLine();
    void setCurrentLine(QString);

    QStringList history;
    int hpos;
    QString chartid;

protected:
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);
    virtual void contextMenuEvent(QContextMenuEvent *e);

private:
    Context *context;
    bool localEchoEnabled;
    PythonHost *pythonHost;
    int promptStartIndex = 4;
};

// the chart
class PythonChart : public GcChartWindow, public PythonHost {

    Q_OBJECT

    Q_PROPERTY(QString script READ getScript WRITE setScript USER true)
    Q_PROPERTY(QString state READ getState WRITE setState USER true)
    Q_PROPERTY(bool showConsole READ showConsole WRITE setConsole USER true)

    public:
        PythonChart(Context *context, bool ridesummary);

        // reveal
        bool hasReveal() { return true; }
        QCheckBox *showCon;

        // receives all the events
        QTextEdit *script;
        PythonConsole *console;
        QWebEngineView *canvas; // not yet!!

        void emitUrl(QUrl x) { emit setUrl(x); }

        bool showConsole() const { return (showCon ? showCon->isChecked() : true); }
        void setConsole(bool);

        QString getScript() const;
        void setScript(QString);

        QString getState() const;
        void setState(QString);

        PythonChart *chart() { return this; }
        bool readOnly() { return true; }

    signals:
        void setUrl(QUrl);

    public slots:
        void configChanged(qint32);
        void showConChanged(int state);
        void runScript();
        void webpage(QUrl);
        static void execScript(PythonChart *);

    protected:
        // enable stopping long running scripts
        bool eventFilter(QObject *, QEvent *e);

        QSplitter *splitter;
        QSplitter *leftsplitter;

    private:
        Context *context;
        QString text; // if Rtool not alive
        bool ridesummary;
};


#endif
