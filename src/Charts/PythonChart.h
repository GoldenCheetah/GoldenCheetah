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
#include <QtCharts>

#include "GoldenCheetah.h"
#include "Context.h"
#include "Athlete.h"
#include "RCanvas.h"

// keep aligned to library.py
#define GC_CHART_LINE      1
#define GC_CHART_SCATTER   2
#define GC_CHART_BAR       3
#define GC_CHART_PIE       4

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
    Q_PROPERTY(bool asWeb READ asWeb WRITE setWeb USER true)

    public:
        PythonChart(Context *context, bool ridesummary);

        // reveal
        bool hasReveal() { return true; }
        QCheckBox *showCon, *web;

        // receives all the events
        QTextEdit *script;
        PythonConsole *console;
        QWidget *render;
        QVBoxLayout *renderlayout;

        // rendering via...
        QWebEngineView *canvas; // not yet!!
        QChartView *chartview;
        QChart *qchart;

        void emitUrl(QUrl x) { emit setUrl(x); }

        bool asWeb() const { return (web ? web->isChecked() : true); }
        void setWeb(bool);

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
        bool emitChart(QString title, int type, bool animate);
        bool emitCurve(QString name, QVector<double> xseries, QVector<double> yseries, QString xname, QString yname,
                      int line, int symbol, int size, QString color, int opacity, bool opengl);


    public slots:
        void configChanged(qint32);
        void showConChanged(int state);
        void showWebChanged(int state);
        void runScript();
        void webpage(QUrl);
        static void execScript(PythonChart *);

        // set chart settings
        bool configChart(QString title, int type, bool animate) {

            if (chartview) {
                // if we changed the type, all series must go
                if (charttype != type) {
                    qchart->removeAllSeries();
                    curves.clear();
                }
                charttype=type;

                // title is allowed to be blank
                qchart->setTitle(title);

                // would avoid animations as they get very tiresome and are not
                // generally transition animations, so add very little value
                // by default they are disabled anyway
                qchart->setAnimationOptions(animate ? QChart::SeriesAnimations : QChart::NoAnimation);

                return true;
            }
        }
        bool setCurve(QString name, QVector<double> xseries, QVector<double> yseries, QString xname, QString yname,
                      int line, int symbol, int size, QString color, int opacity, bool opengl);

    protected:
        // enable stopping long running scripts
        bool eventFilter(QObject *, QEvent *e);

        QSplitter *splitter;
        QSplitter *leftsplitter;

    private:
        Context *context;
        QString text; // if Rtool not alive
        bool ridesummary;
        int charttype;

        // curves
        QMap<QString, QAbstractSeries *>curves;
};


#endif
