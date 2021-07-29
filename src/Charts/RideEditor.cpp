/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "RideEditor.h"
#include "LTMOutliers.h"
#include "IntervalItem.h"
#include "Athlete.h"
#include "MainWindow.h"
#include "Context.h"
#include "Settings.h"
#include "Colors.h"
#include "TabView.h"
#include "HelpWhatsThis.h"
#include "HrZones.h"
#include "XDataDialog.h"
#include "XDataTableModel.h"

#include <QtGui>
#include <QString>
#include <QStyle>
#include <QStyleFactory>
#include <QScrollBar>

// for strtod
#include <stdlib.h>

// used to make a lookup string for row/col anomalies
static QString xsstring(int x, RideFile::SeriesType series)
{
    return QString("%1:%2").arg((int)x).arg(static_cast<int>(series));
}
static void unxsstring(QString val, int &x, RideFile::SeriesType &series)
{
    QRegExp it("^([^:]*):([^:]*)$");
    it.exactMatch(val);
    x = it.cap(1).toDouble();
    series = static_cast<RideFile::SeriesType>(it.cap(2).toInt());
}

static void secsMsecs(double value, int &secs, int &msecs)
{
    // split into secs and msecs from a double
    // tried modf, floor, round and a host of others but
    // they all had difference problems. In the end
    // I've resorted to rounding to 100ths of a second.
    // I acknowledge that this is horrid, but its ok
    // for Powertaps but maybe more precise devices will
    // come along?
    secs = floor(value); // assume it is positive!! .. it is a time field!
    msecs = round((value - secs) * 100) * 10;
}

// get clipboard into a 2-dim array of doubles
void
getPaste(QVector<QVector<double> >&cells, QStringList &seps, QStringList &head, bool hasHeader)
{
    QString text = QApplication::clipboard()->text();

    int row = 0;
    int col = 0;
    bool first = true;

    QString regexpStr;
    regexpStr = "[";
    foreach (QString sep, seps) regexpStr += sep;
    regexpStr += "]";
    QRegExp sep(regexpStr); // RegExp for separators

    QRegExp ELine(("\n|\r|\r\n")); //RegExp for line endings

    foreach(QString line, text.split(ELine)) {
        if (line == "") continue;

        if (hasHeader && first == true) {
            foreach (QString token, line.split(sep)) {
                head << token;
            }
        } else {
            cells.resize(row+1);
            foreach (QString token, line.split(sep)) {
                cells[row].resize(col+1);

                // use strtod to get better precision
                char *p;
                cells[row][col] = strtod(token.toLatin1(), &p);

                col++;

                // if there are more cols than in the
                // heading row then set to unknown
                while (hasHeader && (col+1) > head.count())
                    head << "unknown";
            }
            row++;
            col = 0;
        }
        first = false;
    }
}

RideEditor::RideEditor(Context *context) : QWidget(context->mainWindow), data(NULL), ride(NULL), context(context), inLUW(false), colMapper(NULL)
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(2,0,2,2);
    setLayout(mainLayout);

    //Left in the code to display a title, but
    //its a waste of screen estate, maybe uncomment
    //it if someone finds it useful

    //title = new QLabel(tr("No ride selected"));
    //QFont font;
    //font.setWeight(Qt::black);
    //title->setFont(font);
    //title->setAlignment(Qt::AlignHCenter);

    // setup the toolbar
    toolbar = new QToolBar(this);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolbar->setFloatable(true);
    toolbar->setIconSize(QSize(18 *dpiXFactor,18 *dpiYFactor));

    QIcon saveIcon(":images/toolbar/save.png");
    saveAct = new QAction(saveIcon, tr("Save"), this);
    connect(saveAct, SIGNAL(triggered()), this, SLOT(saveFile()));
    toolbar->addAction(saveAct);

    toolbar->addSeparator();
    HelpWhatsThis *helpToolbar = new HelpWhatsThis(toolbar);
    toolbar->setWhatsThis(helpToolbar->getWhatsThisText(HelpWhatsThis::ChartRides_Editor));

    // undo and redo deliberately at a distance from the
    // save icon, since accidentally hitting the wrong
    // icon in that instance would be horrible
    QIcon undoIcon(":images/toolbar/undo.png");
    undoAct = new QAction(undoIcon, tr("Undo"), this);
    connect(undoAct, SIGNAL(triggered()), this, SLOT(undo()));
    toolbar->addAction(undoAct);

    QIcon redoIcon(":images/toolbar/redo.png");
    redoAct = new QAction(redoIcon, tr("Redo"), this);
    connect(redoAct, SIGNAL(triggered()), this, SLOT(redo()));
    toolbar->addAction(redoAct);
    
    toolbar->addSeparator();

    QIcon findIcon(":images/toolbar/search.png");
    searchAct = new QAction(findIcon, tr("Find"), this);
    connect(searchAct, SIGNAL(triggered()), this, SLOT(find()));
    toolbar->addAction(searchAct);

    QIcon checkIcon(":images/toolbar/splash green.png");
    checkAct = new QAction(checkIcon, tr("Anomalies"), this);
    connect(checkAct, SIGNAL(triggered()), this, SLOT(anomalies()));
    toolbar->addAction(checkAct);

    QIcon xdataIcon(":images/toolbar/properties.png");
    xdataAct = new QAction(xdataIcon, tr("XData"), this);
    connect(xdataAct, SIGNAL(triggered()), this, SLOT(xdata()));
    toolbar->addAction(xdataAct);

    // add a tabbar, with no tabs, hide it and only show
    // if there are more than one tabs (i.e. we have XDATA)
    tabbar = new EditorTabBar(this);
    tabbar->setShape(QTabBar::RoundedSouth);
    tabbar->setCurrentIndex(0);
    tabbar->setTabsClosable(true);
    tabbar->hide();

    // stack of standard + other editors
    stack = new QStackedWidget(this);

    // empty model
    model = new RideFileTableModel(NULL);

    // set up the table
    table = new QTableView(this);

    stack->addWidget(table);
    stack->setCurrentIndex(0);

#ifdef Q_OS_WIN
    QStyle *cde = QStyleFactory::create(OS_STYLE);
    table->verticalScrollBar()->setStyle(cde);
    table->horizontalScrollBar()->setStyle(cde);
#endif
    table->setItemDelegate(new CellDelegate(this));
    table->verticalHeader()->setDefaultSectionSize(20 *dpiYFactor);
    table->setModel(model);
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    table->setSelectionMode(QAbstractItemView::ContiguousSelection);
    table->installEventFilter(this);
    //table->setFrameStyle(QFrame::NoFrame);

    HelpWhatsThis *helpTable = new HelpWhatsThis(table);
    table->setWhatsThis(helpTable->getWhatsThisText(HelpWhatsThis::ChartRides_Editor));

    // prettify (and make anomalies more visible)
    table->setGridStyle(Qt::NoPen);

    connect(table, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(cellMenu(const QPoint&)));

    // layout the widget
    //mainLayout->addWidget(title);
    mainLayout->addSpacing(20*dpiXFactor); // bit of white space
    mainLayout->addWidget(toolbar);
    mainLayout->addSpacing(20*dpiXFactor);
    mainLayout->addWidget(stack);
    mainLayout->addWidget(tabbar);

    // trap GC signals
    connect(context, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
    //connect(main, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
    //connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(context, SIGNAL(rideDirty(RideItem*)), this, SLOT(rideDirty()));
    connect(context, SIGNAL(rideClean(RideItem*)), this, SLOT(rideClean()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(tabbar, SIGNAL(currentChanged(int)), this, SLOT(tabbarSelected(int)));
    connect(tabbar, SIGNAL(tabCloseRequested(int)), this, SLOT(removeTabRequested(int)));

    // put find tool and anomaly list in the controls
    findTool = new FindDialog(this);
    findTool->hide();
    anomalyTool = new AnomalyDialog(this);
    anomalyTool->hide();

    // xdatatool
    xdataTool = new XDataDialog(this);
    xdataTool->hide();

    // allow us to jump to an anomaly
    connect(anomalyTool->anomalyList, SIGNAL(itemSelectionChanged()), this, SLOT(anomalySelected()));

    // set initial color config
    configChanged(CONFIG_APPEARANCE);
}

void
RideEditor::configChanged(qint32) 
{
    setProperty("color", GColor(CPLOTBACKGROUND));

    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setBrush(QPalette::Background, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setBrush(QPalette::Base, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::Normal, QPalette::Window, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    setPalette(palette);
    tabbar->setPalette(palette);
    QColor faded = GCColor::invertColor(GColor(CPLOTBACKGROUND));
    tabbar->setStyleSheet(QString("QTabBar::tab { background-color: %1; border: 0.5px solid %1; color: rgba(%3,%4,%5,50%) }"
                                  "QTabBar::tab:selected { background-color: %1; color: %2; border-bottom: %7px solid %1; border-bottom-color: %6 }"
                                  "QTabBar::close-button:!selected { background-color: %1; }")
                    .arg(GColor(CPLOTBACKGROUND).name())
                    .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name())
                    .arg(faded.red()).arg(faded.green()).arg(faded.blue())
                    .arg(GColor(CPLOTMARKER).name())
                    .arg(4 * dpiXFactor));
    table->setPalette(palette);
    table->setStyleSheet(QString("QTableView { background-color: %1; color: %2; border: %1 }"
                                 "QTableView QTableCornerButton::section { background-color: %1; color: %2; border: %1 }"
                                 "QHeaderView { background-color: %1; color: %2; border: %1 }")
                    .arg(GColor(CPLOTBACKGROUND).name())
                    .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name()));
    table->horizontalHeader()->setStyleSheet(QString("QHeaderView::section { background-color: %1; color: %2; border: 0px; border-bottom: %3px solid %2; }")
                    .arg(GColor(CPLOTBACKGROUND).name())
                    .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name())
                    .arg(2 * dpiYFactor));
    table->verticalHeader()->setStyleSheet(QString("QHeaderView::section { background-color: %1; color: %2; border: 0px }")
                    .arg(GColor(CPLOTBACKGROUND).name())
                    .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name()));
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
#ifndef Q_OS_MAC
    table->verticalScrollBar()->setStyleSheet(TabView::ourStyleSheet());
    table->horizontalScrollBar()->setStyleSheet(TabView::ourStyleSheet());
#endif
    toolbar->setStyleSheet(QString("::enabled { background: %1; color: %2; border: 0px; } ").arg(GColor(CPLOTBACKGROUND).name())
                    .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name()));

    // the xdata editors
    QMapIterator<QString, XDataEditor *>it(xdataEditors);
    it.toFront();
    while(it.hasNext()) {
        it.next();
        XDataEditor *edit = it.value();
        edit->configChanged();
    }
}

//----------------------------------------------------------------------
// RideEditor table/model/rideFile utility functions
//----------------------------------------------------------------------

// what are the available columns? (used by insert column context menu)
QList<QString>
RideEditor::whatColumns()
{
    QList<QString> what;

    what << tr("Time")
         << tr("Distance")
         << tr("Power")
         << tr("Heartrate")
         << tr("Cadence")
         << tr("Speed")
         << tr("Torque")
         << tr("Altitude")
         << tr("Latitude")
         << tr("Longitude")
         << tr("Headwind")
         << tr("Slope")
         << tr("Temperature")
         << tr("Left/Right Balance")
         << tr("Left TE")
         << tr("Right TE")
         << tr("Left PS")
         << tr("Right PS")
         << tr("Left Platform Center Offset")
         << tr("Right Platform Center Offset")
         << tr("Left Power Phase Start")
         << tr("Left Power Phase End")
         << tr("Right Power Phase Start")
         << tr("Right Power Phase End")
         << tr("Left Peak Power Phase Start")
         << tr("Left Peak Power Phase End")
         << tr("Right Peak Power Phase Start")
         << tr("Right Peak Power Phase End")
         << tr("SmO2")
         << tr("tHb")
         << tr("Vertical Oscillation")
         << tr("Run Cadence")
         << tr("GCT")
         << tr("Interval");

    return what;
}

double
RideEditor::getValue(int row, int column)
{
    if (row < 0 || column < 0) return 0.0;

    return ride->ride()->getPointValue(row, model->columnType(column));
}

void
RideEditor::setModelValue(int row, int col, double value)
{
    model->setValue(row, col, value);
}

bool
RideEditor::isAnomaly(int row, int col)
{
    if (row < 0 || col < 0) return false;

    if (data->anomalies.value(xsstring(row,model->columnType(col)), "") != "")
        return true;

    return false;
}

bool
RideEditor::isFound(int row, int col)
{
    if (row < 0 || col < 0) return false;

    if (data->found.value(xsstring(row,model->columnType(col)), "") != "")
        return true;

    return false;
}

bool
RideEditor::isTooPrecise(int row, int column)
{
    if (row < 0 || column < 0) return false;

    RideFile::SeriesType what = model->columnType(column);
    int dp;
    QString value = QString("%1").arg(getValue(row, column), 0, 'g', 10);

    if ((dp = value.indexOf(".")) >= 0)
        if (value.length()-(dp+1) > RideFile::decimalsFor(what)) return true;

    return false;
}

bool
RideEditor::isRowSelected()
{
    QList<QModelIndex> selection = table->selectionModel()->selection().indexes();

    if (selection.count() > 0 &&
        selection[0].column() == 0 &&
        selection[selection.count()-1].column() == (model->columnCount()-1))
        return true;

    return false;
}

bool
RideEditor::isColumnSelected()
{
    QList<QModelIndex> selection = table->selectionModel()->selection().indexes();

    if (selection.count() > 0 &&
        selection[0].row() == 0 &&
        selection[selection.count()-1].row() == (ride->ride()->dataPoints().count()-1))
        return true;

    return false;
}


//----------------------------------------------------------------------
// Toolbar functions
//----------------------------------------------------------------------
void
RideEditor::saveFile()
{
    if (ride && ride->isDirty()) {
        context->mainWindow->saveRideSingleDialog(context, ride);
    }
}

void
RideEditor::undo()
{
    if (ride && ride->ride() && ride->ride()->command)
        ride->ride()->command->undoCommand();
}

void
RideEditor::redo()
{
    if (ride && ride->ride() && ride->ride()->command)
        ride->ride()->command->redoCommand();
}

void
RideEditor::hideEvent(QHideEvent *)
{
    findTool->hide();
    anomalyTool->hide();
    xdataTool->hide();
}

void
RideEditor::find()
{
    // find only valid if we have data points to search across
    if (ride && ride->ride() && ride->ride()->dataPoints().count())
        findTool->show();
}

void
RideEditor::anomalies()
{
    // show anomalies list .. hidden when tab changed or ride selected
    if (ride && ride->ride() && ride->ride()->dataPoints().count())
        anomalyTool->show();
    
}

void
RideEditor::xdata()
{
    // work with xdata series (add / remove)
    if (ride && ride->ride()) {
        // show the xdata dialog
        xdataTool->show();
    }
}

void
AnomalyDialog::check()
{
    // run through all the available channels and find anomalies
    rideEditor->data->anomalies.clear();

    // clear the list
    anomalyList->clear();
    //QStringList header;
    //header << "Id" << "Anomalies";
    //anomalyList->setHorizontalHeaderLabels(header);
    anomalyList->horizontalHeader()->hide();

    // use MaxHR if available for suspicious, otherwise 200
    const HrZones *hrZones = rideEditor->context->athlete->hrZones(rideEditor->ride->sport);
    int hrZR = hrZones ? hrZones->whichRange(rideEditor->ride->dateTime.date()) : -1;
    int maxHR = hrZR > 0 ? hrZones->getMaxHr(hrZR) : 200;

    // Speed threshold depends on sport (9kph~20"/50m, 36kph~10"/100m)
    double maxKPH = rideEditor->ride->isSwim ? 9.0 :
                    rideEditor->ride->isRun ? 36.0 : 100.0;

    // Cadence threshold depends on sport
    int maxCad = rideEditor->ride->isSwim ? 80 :
                 rideEditor->ride->isRun ? 120 : 200;

    QVector<double> power;
    QVector<double> cad;
    QVector<double> secs;
    double lastdistance=9;
    double lastpower=0;
    double lastcad=0;
    int count = 0;

    foreach (RideFilePoint *point, rideEditor->ride->ride()->dataPoints()) {
        power.append(point->watts);
        cad.append(point->cad);
        secs.append(point->secs);

        if (count) {

            // whilst we are here we might as well check for gaps in recording
            // anything bigger than a second is of a material concern
            // and we assume time always flows forward ;-)
            double diff = secs[count] - (secs[count-1] + rideEditor->ride->ride()->recIntSecs());
            if (diff > (double)0.0 || diff < (double)0.0 || secs[count] < secs[count-1]) {
                rideEditor->data->anomalies.insert(xsstring(count, RideFile::secs),
                                       tr("Invalid recording gap"));
            }

            // and on the same theme what about distance going backwards?
            if (point->km < lastdistance)
                rideEditor->data->anomalies.insert(xsstring(count, RideFile::km),
                                       tr("Distance goes backwards."));

            // check for non-zeroed cadence/power "triplet"
            if (count >= 3 && point->cad == 0 && point->watts == 0 &&
                              lastcad != 0 && lastpower != 0) {

                // look at previous 3
                if (power[count-1] == power[count-2] && power[count-2] == power[count-3] &&
                    cad[count-1] == cad[count-2] && cad[count-2] == cad[count-3]) {

                    rideEditor->data->anomalies.insert(xsstring(count-1, RideFile::cad),
                                       tr("Cadence/Power duplicated when freewheeling."));
                }
            }
        }

        // so we can look back one quickly
        lastdistance = point->km;
        lastpower = point->watts;
        lastcad = point->cad;

        // suspicious values
        if (point->cad > maxCad) {
            rideEditor->data->anomalies.insert(xsstring(count, RideFile::cad),
                                   tr("Suspiciously high cadence"));
        }
        if (point->hr > maxHR) {
            rideEditor->data->anomalies.insert(xsstring(count, RideFile::hr),
                                   tr("Suspiciously high heartrate"));
        }
        if (point->kph > maxKPH) {
            rideEditor->data->anomalies.insert(xsstring(count, RideFile::kph),
                                   tr("Suspiciously high speed"));
        }
        if (point->lat > 90 || point->lat < -90) {
            rideEditor->data->anomalies.insert(xsstring(count, RideFile::lat),
                                   tr("Out of bounds value"));
        }
        if (point->lon > 180 || point->lon < -180) {
            rideEditor->data->anomalies.insert(xsstring(count, RideFile::lon),
                                   tr("Out of bounds value"));
        }
        // Non-zero torque but zero cadence is not an anomaly for runs or swims
        if (!rideEditor->ride->isRun && !rideEditor->ride->isSwim &&
            rideEditor->ride->ride()->areDataPresent()->cad &&
            point->nm && !point->cad) {

            rideEditor->data->anomalies.insert(xsstring(count, RideFile::nm),
                                   tr("Non-zero torque but zero cadence"));

        }
        count++;
    }

    // lets look at the Power Column if its there and has enough data
    int column = rideEditor->model->headings().indexOf(tr("Power"));
    if (column >= 0 && rideEditor->ride->ride()->dataPoints().count() >= 30) {

        // get spike config
        double max = appsettings->value(this, GC_DPFS_MAX, "200").toDouble();
        double variance = appsettings->value(this, GC_DPFS_VARIANCE, "20").toDouble();

        LTMOutliers outliers(secs.data(), power.data(), power.count(), 30, false);

        // run through the ranked list
        for (int i=0; i<secs.count(); i++) {

            // is this over variance threshold?
            if (outliers.getDeviationForRank(i) < variance) continue;

            // ok, so its highly variant but is it over
            // the max value we are willing to accept?
            if (outliers.getYForRank(i) < max) continue;

            // which one is it
            rideEditor->data->anomalies.insert(xsstring(outliers.getIndexForRank(i), RideFile::watts), tr("Data spike candidate"));
        }
    }

    // now fill in the anomaly list
    anomalyList->setRowCount(0); // <<< fixes crash at ZZZZ
    anomalyList->setRowCount(rideEditor->data->anomalies.count()); // <<< ZZZZ

    int counter = 0;
    QMapIterator<QString,QString> f(rideEditor->data->anomalies);
    while (f.hasNext()) {

        f.next();

        QTableWidgetItem *t = new QTableWidgetItem;
        t->setText(f.key());
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        anomalyList->setItem(counter, 0, t);

        t = new QTableWidgetItem;
        t->setText(f.value());
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        t->setForeground(QBrush(Qt::red));
        anomalyList->setItem(counter, 1, t);

        counter++;
    }

    // enable the toolbar / disable for anomalies found
    if (counter) rideEditor->checkAct->setEnabled(true);
    else rideEditor->checkAct->setEnabled(false);

    // redraw - even if no anomalies were found since
    // some may have been highlighted previously. This is
    // an expensive operation, but then so is the check()
    // function.
    rideEditor->model->forceRedraw();
}

//----------------------------------------------------------------------
// handle TableView signals / events
//----------------------------------------------------------------------
bool
RideEditor::eventFilter(QObject *object, QEvent *e)
{
    // not for the table?
    if (object != (QObject *)table) return false;

    // what happened?
    switch(e->type())
    {
        case QEvent::ContextMenu:
            // borderMenu(((QMouseEvent *)e)->pos());
            borderMenu(table->mapFromGlobal(QCursor::pos()));
            return true; // I'll take that thanks
            break;

        case QEvent::KeyPress:
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
            if (keyEvent->modifiers() & Qt::ControlModifier) {
                switch (keyEvent->key()) {

                    case Qt::Key_C: // defacto standard for copy
                        copy();
                        return true;

                    case Qt::Key_V: // defacto standard for paste
                        paste();
                        return true;

                    case Qt::Key_X: // defacto standard for cut
                        cut();
                        return true;

                    case Qt::Key_Y: // emerging standard for redo
                         redo();
                         return true;

                    case Qt::Key_Z: // common standard for undo
                         undo();
                         return true;

                    case Qt::Key_0:
                        clear();
                        return true;

                    default:
                        return false;
                }
            }
            break;
        }

        default:
            break;
    }
    return false;
}

void
RideEditor::stdContextMenu(QMenu *menu, const QPoint &pos)
{
    int row = table->indexAt(pos).row();
    int column = table->indexAt(pos).column();

    QIcon undoIcon(":images/toolbar/undo.png");
    QIcon redoIcon(":images/toolbar/redo.png");
    QIcon cutIcon(":images/toolbar/cut.png");
    QIcon pasteIcon(":images/toolbar/paste.png");
    QIcon copyIcon(":images/toolbar/copy.png");

    bool pastable = QApplication::clipboard()->text() == "" ? false : true;

    // setup all the actions
    QAction *undoAct = new QAction(undoIcon, tr("Undo"), table);
    undoAct->setShortcut(QKeySequence("Ctrl+Z"));
    undoAct->setEnabled(ride->ride()->command->undoCount() > 0);
    menu->addAction(undoAct);
    connect(undoAct, SIGNAL(triggered()), this, SLOT(undo()));

    QAction *redoAct = new QAction(redoIcon, tr("Redo"), table);
    redoAct->setShortcut(QKeySequence("Ctrl+Y"));
    redoAct->setEnabled(ride->ride()->command->redoCount() > 0);
    menu->addAction(redoAct);
    connect(redoAct, SIGNAL(triggered()), this, SLOT(redo()));

    menu->addSeparator();

    QAction *cutAct = new QAction(cutIcon, tr("Cut"), table);
    cutAct->setShortcut(QKeySequence("Ctrl+X"));
    cutAct->setEnabled(isRowSelected() || isColumnSelected());
    menu->addAction(cutAct);
    connect(cutAct, SIGNAL(triggered()), this, SLOT(cut()));

    QAction *copyAct = new QAction(copyIcon, tr("Copy"), table);
    copyAct->setShortcut(QKeySequence("Ctrl+C"));
    copyAct->setEnabled(true);
    menu->addAction(copyAct);
    connect(copyAct, SIGNAL(triggered()), this, SLOT(copy()));

    QAction *pasteAct = new QAction(pasteIcon, tr("Paste"), table);
    pasteAct->setShortcut(QKeySequence("Ctrl+V"));
    pasteAct->setEnabled(pastable);
    menu->addAction(pasteAct);
    connect(pasteAct, SIGNAL(triggered()), this, SLOT(paste()));

    QAction *specialAct = new QAction(tr("Paste Special..."), table);
    specialAct->setEnabled(pastable);
    menu->addAction(specialAct);
    connect(specialAct, SIGNAL(triggered()), this, SLOT(pasteSpecial()));

    QAction *clearAct = new QAction(tr("Clear Contents"), table);
    clearAct->setShortcut(QKeySequence("Ctrl+0"));
    clearAct->setEnabled(true);
    menu->addAction(clearAct);
    connect(clearAct, SIGNAL(triggered()), this, SLOT(clear()));

    currentCell.row = row;
    currentCell.column = column;
}

void
RideEditor::cellMenu(const QPoint &pos)
{

    int row = table->indexAt(pos).row();
    int column = table->indexAt(pos).column();
    bool anomaly = isAnomaly(row, column);

    QMenu menu(table);
    stdContextMenu(&menu, pos);

    menu.addSeparator();

    QAction *smoothAnomaly = new QAction(tr("Smooth Anomaly"), table);
    smoothAnomaly->setEnabled(anomaly);
    menu.addAction(smoothAnomaly);
    connect(smoothAnomaly, SIGNAL(triggered(void)), this, SLOT(smooth()));

    currentCell.row = row < 0 ? 0 : row;
    currentCell.column = column < 0 ? 0 : column;
    menu.exec(table->mapToGlobal(QPoint(pos.x(), pos.y()+(20*dpiYFactor))));
}


void
RideEditor::borderMenu(const QPoint &pos)
{

    int column=0, row=0;

    // but we need to set the row or column to zero since
    // we are in the border, this seems an easy and quick way
    // to do this (the indexAt function assumes pos starts from
    // 0 for row/col 0 and does not include the vertical
    // or horizontal header width (which is what we go passed)
    if (pos.y() <= table->horizontalHeader()->height()) {
        row = 0;
        QPoint tickle(pos.x() - table->verticalHeader()->width(),
                      pos.y());
        column = table->indexAt(tickle).column();
    }
    if (pos.x() <= table->verticalHeader()->width()) {
        column = 0;
        QPoint tickle(pos.x(), pos.y() - table->horizontalHeader()->height());
        row = table->indexAt(tickle).row();
    }

    // avoid crash when pos translation failed
    // just return with no menu options added
    if (row < 0 && column < 0) return;

    QMenu menu(table);
    stdContextMenu(&menu, pos);

    menu.addSeparator();

    if (column <= 0) {

        QAction *delAct = new QAction(tr("Delete Row"), table);
        delAct->setEnabled(isRowSelected());
        menu.addAction(delAct);
        connect(delAct, SIGNAL(triggered()), this, SLOT(delRow()));

        QAction *insAct = new QAction(tr("Insert Row"), table);
        insAct->setEnabled(true);
        menu.addAction(insAct);
        connect(insAct, SIGNAL(triggered()), this, SLOT(insRow()));

    } else if (row <= 0){

        QAction *delAct = new QAction(tr("Remove Column"), table);
        delAct->setEnabled(isColumnSelected());
        menu.addAction(delAct);
        connect(delAct, SIGNAL(triggered()), this, SLOT(delColumn()));

        QMenu *insCol = new QMenu(tr("Add Column"), table);
        insCol->setEnabled(true);

        // add menu options for each column
        if (colMapper) delete colMapper;
        colMapper = new QSignalMapper(this);
        connect(colMapper, &QSignalMapper::mappedString, this, &RideEditor::insColumn);

        foreach(QString heading, whatColumns()) {
            QAction *insColAct = new QAction(heading, table);
            connect(insColAct, SIGNAL(triggered()), colMapper, SLOT(map()));
            insColAct->setEnabled(!model->headings().contains(heading));
            insCol->addAction(insColAct);

            // map action to column heading
            colMapper->setMapping(insColAct, heading);
        }
        menu.addMenu(insCol);
    }

    currentCell.row = row < 0 ? 0 : row;
    currentCell.column = column < 0 ? 0 : column;
    menu.exec(table->mapToGlobal(QPoint(pos.x(), pos.y())));
}

//----------------------------------------------------------------------
// Context menu actions
//----------------------------------------------------------------------
void
RideEditor::copy()
{
    QString copy;
    QList<QModelIndex> selection = table->selectionModel()->selection().indexes();

    if (selection.count() > 0) {
        QString text;
        for (int row = selection[0].row(); row <= selection[selection.count()-1].row(); row++) {

            for (int column = selection[0].column();  column <= selection[selection.count()-1].column(); column++) {
                if (column == selection[selection.count()-1].column())
                    text += QString("%L1").arg(getValue(row,column), 0, 'g', 11);
                else
                    text += QString("%L1\t").arg(getValue(row,column), 0, 'g', 11);
            }
            text += "\n";
        }
        QApplication::clipboard()->setText(text);

        // remember the headings we copied, so we can
        // default them if we paste special them back
        copyHeadings.clear();
        for (int column = selection[0].column();  column <= selection[selection.count()-1].column(); column++)
            copyHeadings << model->headings()[column];
    }
}

void
RideEditor::cut()
{
    copy();
    if (isRowSelected()) delRow();
    else if (isColumnSelected()) delColumn();
}

void
RideEditor::delRow()
{
    // run through the selected rows and zap them
    QList<QModelIndex> selection = table->selectionModel()->selection().indexes();

    if (selection.count() > 0) {

        // delete from table - we do in one hit since row-by-row is VERY slow
        ride->ride()->command->startLUW("Delete Rows");
        model->removeRows(selection[0].row(),
                          selection[selection.count()-1].row() - selection[0].row() + 1, QModelIndex());
        ride->ride()->command->endLUW();

    }
}

void
RideEditor::delColumn()
{
    // run through the selected columns and "zap" them
    QList<QModelIndex> selection = table->selectionModel()->selection().indexes();

    if (selection.count() > 0) {

        // Delete each column by its SeriesType
        ride->ride()->command->startLUW("Delete Columns");
        for(int column = selection.first().column(),
            count = selection.last().column() - selection.first().column() + 1;
            count > 0 ; count--)
            model->removeColumn(model->columnType(column));
        ride->ride()->command->endLUW();
    }
}

void
RideEditor::insRow()
{
    // add to model
    model->insertRow(currentCell.row, QModelIndex());
}

void
RideEditor::insColumn(QString name)
{
    // update state data
    RideFile::SeriesType series = RideFile::secs;

    if (name == tr("Time")) series = RideFile::secs;
    if (name == tr("Distance")) series = RideFile::km;
    if (name == tr("Power")) series = RideFile::watts;
    if (name == tr("Cadence")) series = RideFile::cad;
    if (name == tr("Speed")) series = RideFile::kph;
    if (name == tr("Torque")) series = RideFile::nm;
    if (name == tr("Latitude")) series = RideFile::lat;
    if (name == tr("Longitude")) series = RideFile::lon;
    if (name == tr("Altitude")) series = RideFile::alt;
    if (name == tr("Headwind")) series = RideFile::headwind;
    if (name == tr("Interval")) series = RideFile::interval;
    if (name == tr("Heartrate")) series = RideFile::hr;
    if (name == tr("Slope")) series = RideFile::slope;
    if (name == tr("Temperature")) series = RideFile::temp;
    if (name == tr("Left/Right Balance")) series = RideFile::lrbalance;
    if (name == tr("Left TE")) series = RideFile::lte;
    if (name == tr("Right TE")) series = RideFile::rte;
    if (name == tr("Left PS")) series = RideFile::lps;
    if (name == tr("Right PS")) series = RideFile::rps;  
    if (name == tr("Left Platform Center Offset")) series = RideFile::lpco;
    if (name == tr("Right Platform Center Offset")) series = RideFile::rpco;
    if (name == tr("Left Power Phase Start")) series = RideFile::lppb;
    if (name == tr("Right Power Phase Start")) series = RideFile::rppb;
    if (name == tr("Left Power Phase End")) series = RideFile::lppe;
    if (name == tr("Right Power Phase End")) series = RideFile::rppe;
    if (name == tr("Left Peak Power Phase Start")) series = RideFile::lpppb;
    if (name == tr("Right Peak Power Phase Start")) series = RideFile::rpppb;
    if (name == tr("Left Peak Power Phase End")) series = RideFile::lpppe;
    if (name == tr("Right Peak Power Phase End")) series = RideFile::rpppe;
    if (name == tr("SmO2")) series = RideFile::smo2;
    if (name == tr("tHb")) series = RideFile::thb;
    if (name == tr("Vertical Oscillation")) series = RideFile::rvert;
    if (name == tr("Run Cadence")) series = RideFile::rcad;
    if (name == tr("GCT")) series = RideFile::rcontact;

    model->insertColumn(series);
}

void
RideEditor::paste()
{
    QVector<QVector<double> > cells;
    QStringList seps, head;
    seps << "\t";

    getPaste(cells, seps, head, false);

    // empty paste buffer
    if (cells.count() == 0 || cells[0].count() == 0) return;

    // if selected range is not the same
    // size as the copy buffer then barf
    // unless just a single cell selected
    QList<QModelIndex> selection = table->selectionModel()->selection().indexes();

    // is anything selected?
    if (selection.count() == 0) {
        // wrong size
        QMessageBox oops(QMessageBox::Critical, tr("Paste error"),
                         tr("Please select target cell or cells to paste values into."));
        oops.exec();
        return;
    }

    int selectedrow = selection[0].row();
    int selectedcol = selection[0].column();
    int selectedrows = selection[selection.count()-1].row() - selectedrow + 1;
    int selectedcols = selection[selection.count()-1].column() - selectedcol + 1;

    if (selection.count() > 1 &&
        (selectedrows != cells.count() || selectedcols != cells[0].count())) {

        // wrong size
        QMessageBox oops(QMessageBox::Critical, tr("Paste error"),
                         tr("Copy buffer and selected area are diffferent sizes."));
        oops.exec();
        return;
    }

    // overrun cols?
    if (selection.count() == 1 &&
        (selectedcol + cells[0].count()) > model->columnCount()) {
        QMessageBox oops(QMessageBox::Critical, tr("Paste error"),
                         tr("Copy buffer has more columns than available."));
        oops.exec();
        return;
    }

    // overrun rows?
    if (selection.count() == 1 &&
        (selectedrow + cells.count()) > ride->ride()->dataPoints().count()) {
        QMessageBox oops(QMessageBox::Critical, tr("Paste error"),
                         tr("Copy buffer has more rows than available."));
        oops.exec();
        return;
    }

    // go paste!
    ride->ride()->command->startLUW("Paste Cells");
    for (int i=0; i<cells.count(); i++) {

        // just in case check boundary (i.e. truncate)
        if (selectedrow+i > ride->ride()->dataPoints().count()-1) break;
        for(int j=0; j<cells[i].count(); j++) {

            // just in case check boundary (i.e. truncate)
            if ((selectedcol+j > model->columnCount()-1)) break;

            // set table
            setModelValue(selectedrow+i, selectedcol+j, cells[i][j]);
        }
    }
    ride->ride()->command->endLUW();
}

void
RideEditor::pasteSpecial()
{
    // paste clipboard but with a dialog to
    // choose field columns and insert rather
    // than overwrite or append rather than insert

    PasteSpecialDialog *paster = new PasteSpecialDialog(this);

    // center the dialog
    QDesktopWidget *desktop = QApplication::desktop();
    int x = (desktop->width() - paster->size().width()) / 2;
    int y = ((desktop->height() - paster->size().height()) / 2) -(50*dpiYFactor);

    // move window to desired coordinates
    paster->move(x,y);
    paster->exec();
}

void
RideEditor::clear()
{
    // Set the selected cells to zero
    ride->ride()->command->startLUW("Clear cells");
    foreach (QModelIndex current, table->selectionModel()->selection().indexes()) {
        setModelValue(current.row(), current.column(), (double)0.0);
    }
    ride->ride()->command->endLUW();
}

void
RideEditor::smooth()
{
    QString xs = xsstring(currentCell.row, model->columnType(currentCell.column));

    // calculate smoothed value
    double left = 0.0;
    double right = 0.0;
    if (currentCell.row > 0) left = getValue(currentCell.row-1, currentCell.column);
    if (currentCell.row < (ride->ride()->dataPoints().count()-1)) right = getValue(currentCell.row+1, currentCell.column);
    double value = (left+right) / 2;

    // update model
    setModelValue(currentCell.row, currentCell.column, value);
}


//----------------------------------------------------------------------
// Cell item delegate
//----------------------------------------------------------------------

// Cell editor - item delegate
CellDelegate::CellDelegate(RideEditor *rideEditor, QObject *parent) : QItemDelegate(parent), rideEditor(rideEditor) {}

// setup editor for edit of field!!
QWidget *CellDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const
{
    // what are we editing?
    RideFile::SeriesType what = rideEditor->model->columnType(index.column());

    if (what == RideFile::secs) {

        QTimeEdit *timeEdit = new QTimeEdit(parent);
        timeEdit->setDisplayFormat ("hh:mm:ss.zzz");
        connect(timeEdit, SIGNAL(editingFinished()), this, SLOT(commitAndCloseEditor()));
        return timeEdit;

    } else {
        QDoubleSpinBox *valueEdit = new QDoubleSpinBox(parent);
        valueEdit->setDecimals(RideFile::decimalsFor(what));
        valueEdit->setMaximum(RideFile::maximumFor(what));
        valueEdit->setMinimum(RideFile::minimumFor(what));
        connect(valueEdit, SIGNAL(editingFinished()), this, SLOT(commitAndCloseEditor()));
        return valueEdit;
    }
}

// user hit tab or return so save away the data to our model
void CellDelegate::commitAndCloseEditor()
{
    QDoubleSpinBox *editor = qobject_cast<QDoubleSpinBox *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

// We don't set anything because the data is saved within the view not the model!
void CellDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    // what are we editing?
    RideFile::SeriesType what = rideEditor->model->columnType(index.column());

    if (what == RideFile::secs) {

        int seconds, msecs;
        secsMsecs(index.model()->data(index, Qt::DisplayRole).toDouble(), seconds, msecs);

        QTime value = QTime(0,0,0,0).addSecs(seconds).addMSecs(msecs);
        QTimeEdit *timeEdit = qobject_cast<QTimeEdit *>(editor);
        timeEdit->setTime(value);

    } else {
        QDoubleSpinBox *valueEdit = qobject_cast<QDoubleSpinBox *>(editor);
        double value = index.model()->data(index, Qt::DisplayRole).toString().toDouble();
        valueEdit->setValue(value);
    }
}

void CellDelegate::updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &/*index*/) const
{
    if (editor) editor->setGeometry(option.rect);
}

// We don't set anything because the data is saved within the view not the model!
void CellDelegate::setModelData(QWidget *editor, QAbstractItemModel *, const QModelIndex &index) const
{
    // what are we editing?
    RideFile::SeriesType what = rideEditor->model->columnType(index.column());

    if (what == RideFile::secs) {

        double seconds;
        QTime midnight(0,0,0,0);
        QTimeEdit *timeEdit = qobject_cast<QTimeEdit *>(editor);
        seconds = (double)midnight.secsTo(timeEdit->time()) + (double)timeEdit->time().msec() / (double)1000.00;
        rideEditor->setModelValue(index.row(), index.column(), seconds);

    } else {
        QDoubleSpinBox *valueEdit = qobject_cast<QDoubleSpinBox *>(editor);
        QString value = QString("%1").arg(valueEdit->value());
        rideEditor->setModelValue(index.row(), index.column(), valueEdit->value());
    }
}

//custom drawContents class
class GTextDocument : public QTextDocument
{
    public:
        GTextDocument(QString string) : QTextDocument(string) {}

        // custom draw
        void drawContents(QPainter *painter, QColor color) {

            painter->save();

            QAbstractTextDocumentLayout::PaintContext ctx;
            ctx.palette.setColor(QPalette::Text, color);
            documentLayout()->draw(painter, ctx);

            painter->restore();
        }
};

// anomalies are underlined in red, otherwise straight paintjob
void CellDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    // what are we editing?
    RideFile::SeriesType what = rideEditor->model->columnType(index.column());
    QString value;

    if (what == RideFile::secs) {
        int seconds, msecs;
        secsMsecs(index.model()->data(index, Qt::DisplayRole).toDouble(), seconds, msecs);
        value = QTime(0,0,0,0).addSecs(seconds).addMSecs(msecs).toString("hh:mm:ss.zzz");
    } else
        value = index.model()->data(index, Qt::DisplayRole).toString();

    // best place to update the tooltip is here, rather than whenever we update the editor
    // data, since this is just before it is used...
    rideEditor->model->setToolTip(index.row(), rideEditor->model->columnType(index.column()),
        rideEditor->data->anomalies.value(xsstring(index.row(), rideEditor->model->columnType(index.column())),""));

    // found items in yellow
    if (rideEditor->isFound(index.row(), index.column()) == true) {
         painter->fillRect(option.rect, QBrush(QColor(255,255,0)));
    }

    if (rideEditor->isAnomaly(index.row(), index.column())) {

        // wavy line is a pain!
        GTextDocument *meh = new GTextDocument(QString(value));
        QTextCharFormat wavy;
        wavy.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        wavy.setUnderlineColor(Qt::red);
        QTextCursor cur = meh->find(value);
        cur.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        cur.selectionStart();
        cur.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        cur.selectionEnd();
        cur.setCharFormat(wavy);

        // only red background if not selected
        //if (rideEditor->table->selectionModel()->isSelected(index) == false)
        //    painter->fillRect(option.rect, QBrush(QColor(255,230,230)));

        painter->save();
        painter->translate(option.rect.x(), option.rect.y());
        meh->drawContents(painter, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
        painter->restore();
        delete meh;

    } else {

        // normal render
        QStyleOptionViewItem myOption = option;
        myOption.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
        drawDisplay(painter, myOption, myOption.rect, value);
        drawFocus(painter, myOption, myOption.rect);
    }

    // warning triangle - for high precision numbers
    if (rideEditor->isTooPrecise(index.row(), index.column())) {
        QPolygon triangle(3);
        triangle.putPoints(0, 3, option.rect.x(), option.rect.y(),
                                option.rect.x()+(4*int(dpiXFactor)), option.rect.y(),
                                option.rect.x(), option.rect.y()+(4*int(dpiXFactor)));
        painter->setBrush(QBrush(QColor(Qt::darkGreen)));
        painter->setPen(QPen(QColor(Qt::darkGreen)));
        painter->drawPolygon(triangle);
    }
}

//----------------------------------------------------------------------
// XData Cell item delegate
//----------------------------------------------------------------------

// Cell editor - item delegate
XDataCellDelegate::XDataCellDelegate(XDataEditor *xdataEditor, QObject *parent) : QItemDelegate(parent), xdataEditor(xdataEditor) {}

// setup editor for edit of field!!
QWidget *XDataCellDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const
{
    // what are we editing?
    switch(index.column()) {

    case 0 :
    {

        QTimeEdit *timeEdit = new QTimeEdit(parent);
        timeEdit->setDisplayFormat ("hh:mm:ss.zzz");
        connect(timeEdit, SIGNAL(editingFinished()), this, SLOT(commitAndCloseEditor()));
        return timeEdit;

    }
    break;

    default:
    {
        QDoubleSpinBox *valueEdit = new QDoubleSpinBox(parent);
        valueEdit->setDecimals(3);
        valueEdit->setMaximum(std::numeric_limits<double>::max());
        valueEdit->setMinimum(-std::numeric_limits<double>::max());
        connect(valueEdit, SIGNAL(editingFinished()), this, SLOT(commitAndCloseEditor()));
        return valueEdit;
    }
    break;
    }
}

// user hit tab or return so save away the data to our model
void XDataCellDelegate::commitAndCloseEditor()
{
    QDoubleSpinBox *editor = qobject_cast<QDoubleSpinBox *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

// We don't set anything because the data is saved within the view not the model!
void XDataCellDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    // what are we editing?
    switch(index.column()) {

        case 0 :
        {

            int seconds, msecs;
            secsMsecs(index.model()->data(index, Qt::DisplayRole).toDouble(), seconds, msecs);

            QTime value = QTime(0,0,0,0).addSecs(seconds).addMSecs(msecs);
            QTimeEdit *timeEdit = qobject_cast<QTimeEdit *>(editor);
            timeEdit->setTime(value);

        }
        break;

        default:
        {
            QDoubleSpinBox *valueEdit = qobject_cast<QDoubleSpinBox *>(editor);
            double value = index.model()->data(index, Qt::DisplayRole).toString().toDouble();
            valueEdit->setValue(value);
        }
    }
}

void XDataCellDelegate::updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &/*index*/) const
{
    if (editor) editor->setGeometry(option.rect);
}

// We don't set anything because the data is saved within the view not the model!
void XDataCellDelegate::setModelData(QWidget *editor, QAbstractItemModel *, const QModelIndex &index) const
{
    switch(index.column()) {

    case 0 :
    {
        double seconds;
        QTime midnight(0,0,0,0);
        QTimeEdit *timeEdit = qobject_cast<QTimeEdit *>(editor);
        seconds = (double)midnight.secsTo(timeEdit->time()) + (double)timeEdit->time().msec() / (double)1000.00;
        xdataEditor->setModelValue(index.row(), index.column(), seconds);

    }
    break;

    default:
    {
        QDoubleSpinBox *valueEdit = qobject_cast<QDoubleSpinBox *>(editor);
        QString value = QString("%1").arg(valueEdit->value());
        xdataEditor->setModelValue(index.row(), index.column(), valueEdit->value());
    }
    break;
    }
}

// anomalies are underlined in red, otherwise straight paintjob
void XDataCellDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    // what are we editing?
    QString value;
    switch(index.column()) {

    case 0:
    {
        int seconds, msecs;
        secsMsecs(index.model()->data(index, Qt::DisplayRole).toDouble(), seconds, msecs);
        value = QTime(0,0,0,0).addSecs(seconds).addMSecs(msecs).toString("hh:mm:ss.zzz");
    }
    break;

    default:
    {
        value = index.model()->data(index, Qt::DisplayRole).toString();
    }
        break;
    }

    // normal render
    QStyleOptionViewItem myOption = option;
    myOption.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    drawDisplay(painter, myOption, myOption.rect, value);
    drawFocus(painter, myOption, myOption.rect);
}

//----------------------------------------------------------------------
// handle GC Signals
//----------------------------------------------------------------------
void
RideEditor::intervalSelected()
{
    // is it for the ride item we are editing?
    if (ride && context->currentRideItem() == ride) {

        // clear all selections
        table->selectionModel()->clear();

        // highlight selection and jump to last
        foreach(IntervalItem *interval, ride->intervalsSelected()) {

            // what is the first dataPoint index for this interval?
            int start = ride->ride()->timeIndex(interval->start);
            int end = ride->ride()->timeIndex(interval->stop);

            // select all the rows
            table->selectionModel()->clearSelection();
            table->selectionModel()->setCurrentIndex(model->index(start,0), QItemSelectionModel::Select);
            table->selectionModel()->select(QItemSelection(model->index(start,0),
                                                               model->index(end,model->columnCount()-1)),
                                                                QItemSelectionModel::Select);
        }

        // propagate interval selection to the xdata editors
        QMapIterator<QString, XDataEditor *>it(xdataEditors);
        it.toFront();
        while(it.hasNext()) {
            it.next();
            XDataEditor *edit = it.value();
            edit->selectIntervals(ride->intervalsSelected());
        }
    }
}

void
RideEditor::rideSelected(RideItem *selected)
{
    findTool->hide(); // hide the dialog!
    anomalyTool->hide();
    xdataTool->hide();

    RideItem *current = selected;
    if (!current || !current->ride() || !current->ride()->dataPoints().count()) {
        model->setRide(NULL);
        findTool->rideSelected();
        return;
    }

    ride = current;

    // get/or setup ridefile state data
    if (ride->ride()->editorData() == NULL) {
        data = new EditorData;
        ride->ride()->setEditorData(data);
    } else {
        data = ride->ride()->editorData();
        data->found.clear(); // search is not active, so clear
    }
    model->setRide(ride->ride());

    // Set for XDATA, including all views
    setTabBar(true);

    // reset the save icon on the toolbar
    if (ride->isDirty()) saveAct->setEnabled(true);
    else saveAct->setEnabled(false);

    // connect the ride command signals so we can
    // set/reset undo/redo
    static QPointer<RideFileCommand> connection = NULL;
    if (connection) {
        disconnect(connection, SIGNAL(endCommand(bool,RideCommand*)), this, SLOT(endCommand(bool,RideCommand*)));
        disconnect(connection, SIGNAL(beginCommand(bool,RideCommand*)), this, SLOT(beginCommand(bool,RideCommand*)));
    }
    connection = ride->ride()->command;
    connect(connection, SIGNAL(endCommand(bool,RideCommand*)), this, SLOT(endCommand(bool,RideCommand*)));
    connect(connection, SIGNAL(beginCommand(bool,RideCommand*)), this, SLOT(beginCommand(bool,RideCommand*)));

    // lets set them anyway
    if (ride->ride()->command->redoCount() == 0) redoAct->setEnabled(false);
    else redoAct->setEnabled(true);
    if (ride->ride()->command->undoCount() == 0) undoAct->setEnabled(false);
    else undoAct->setEnabled(true);

    // look for anomalies
    anomalyTool->check();

    // update finder pane to show available channels
    findTool->rideSelected();

    // xdata
    xdataTool->setRideItem(ride);
}

void
RideEditor::tabbarSelected(int index)
{
    if (index > -1 && index < stack->count()) stack->setCurrentIndex(index);
}

void
RideEditor::removeTabRequested(int index)
{
    // close tab is one way of removing the data from the ride
    QMessageBox confirm(QMessageBox::Warning, tr("Delete XDATA Series"),
                       QString(tr("You are about to permanently remove the XDATA "
                          "series '%1' from the activity\n\n"
                          "Do you want to continue?")).arg(tabbar->tabText(index)),
                       QMessageBox::Ok | QMessageBox::Cancel);

    if ((confirm.exec() & QMessageBox::Cancel) != 0) {
        return;
    }

    // ok then lets remove it !
    ride->ride()->command->removeXData(tabbar->tabText(index));
    return;
}

void
RideEditor::setTabBar(bool force)
{
    // do we need to?
    QStringList xd, tabs;
    for(int i=0; i< tabbar->count()-1; i++) tabs << tabbar->tabText(i);
    QMapIterator<QString, XDataSeries *>ie(ride->ride()->xdata());
    xd<<tr("Basic Data");
    ie.toFront();
    while(ie.hasNext()) {
       ie.next();
       xd << ie.key();
    }

    // no change
    if (!force && xd == tabs) return;

    // OK - if we get here the tabs have changed ....

    // where are we, need to go back if possible.
    QString currentTab = tabbar->currentIndex() >= 0 ? tabbar->tabText(tabbar->currentIndex()) : "";

    foreach(QWidget*stacked, xdataViews) {
        // remove from the stack
        stack->removeWidget(stacked);
        stacked->hide();
    }
    xdataViews.clear();

    while(tabbar->count()) tabbar->removeTab(0);
    tabbar->hide();
    tabbar->addTab(tr("Basic Data"));

    // disable close button on STANDARD tab
    tabbar->setTabButton(0, QTabBar::RightSide, 0);
    tabbar->setTabButton(0, QTabBar::LeftSide, 0);

    if (ride->ride()->xdata().count()) {

        // we need xdata editing too
        QMapIterator<QString, XDataSeries *>it(ride->ride()->xdata());
        it.toFront();
        while(it.hasNext()) {
            it.next();
            QString name = it.key();
            tabbar->addTab(name);

            // add a widget for each view...
            XDataEditor *widget = xdataEditors.value(it.key(), NULL);
            if (widget == NULL) {
                widget = new XDataEditor(this, it.key());
                xdataEditors.insert(it.key(), widget);
            }

            // set ride item
            widget->setRideItem(ride);

            // add to view
            xdataViews << widget;
            stack->addWidget(widget);
            widget->show();
        }

        // add a disabled tab with a "+" button
        tabbar->addTab("");
        QToolButton *tb = new QToolButton(this);
        tb->setText("+");
        tb->setAutoRaise(true);
        tb->setStyleSheet(QString("QToolButton { background: %1; color: %2; border: 0px; } ")
                          .arg(GColor(CPLOTBACKGROUND).name())
                          .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name()));

        tabbar->setTabButton(tabbar->count()-1, QTabBar::LeftSide, tb);
        tabbar->setTabButton(tabbar->count()-1, QTabBar::RightSide, 0);
        tabbar->setTabEnabled(tabbar->count()-1, false);
        connect(tb, SIGNAL(clicked(bool)), xdataTool, SLOT(addXDataClicked()));

        tabbar->show();
    } else {
        tabbar->hide();
    }

    // go back to where we were
    bool found = false;
    if (currentTab != "") {
        for (int i=0; i<tabbar->count(); i++) {
            if (tabbar->tabText(i) == currentTab) {
                tabbar->setCurrentIndex(i);
                stack->setCurrentIndex(i);
                found = true;
            }
        }
    }

    // go to first tab by default.
    if (!found) {
        tabbar->setCurrentIndex(0);
        stack->setCurrentIndex(0);
    }
}

void
RideEditor::anomalySelected()
{
    if (anomalyTool->anomalyList->currentRow() < 0) return;

    // jump to the found item in the main table
    int row;
    RideFile::SeriesType series;
    unxsstring(anomalyTool->anomalyList->item(anomalyTool->anomalyList->currentRow(), 0)->text(), row, series);
    table->setCurrentIndex(model->index(row,model->columnFor(series)));
}

// We update the current selection on the table view
// to reflect the actions performed on the data. This
// is especially relevant when 'redo'ing a command
// since it signposts the user to the change that has
// been applied and makes the UI feel more 'natural'
//
void
RideEditor::beginCommand(bool, RideCommand *cmd)
{
    // when executing a Logical Unit of Work we
    // highlight sells as we go, rather than
    // clearing the current selection and highlighting
    // only those cells updated in the current command
    // we set inLUW to let endCommand (below) know that
    // we are in an LUW
    if (cmd->type == RideCommand::LUW) {
        inLUW = true;

        // redo needs to clear the current selection
        // since we do not clear selections during
        // a LUW in endCommand below. We do not clear
        // for the first 'do' because the current
        // selection is being used to identify which
        // cells to operate upon.
        if (cmd->docount) {
            itemselection.clear();
            table->selectionModel()->clearSelection();
        }
    }
}

// Once the command has been executed we update the
// selection model to highlight the changes to the
// user. We also need to update the EditorData maps
// to reflect changes when rows are added or removed
void
RideEditor::endCommand(bool undo, RideCommand *cmd)
{

    // Update the undo/redo toolbar icons
    if (ride->ride()->command->redoCount() == 0) redoAct->setEnabled(false);
    else redoAct->setEnabled(true);
    if (ride->ride()->command->undoCount() == 0) undoAct->setEnabled(false);
    else undoAct->setEnabled(true);

    // react to xdata changes
    setTabBar(false);

    // update the selection model when a command has been executed
    switch (cmd->type) {

        case RideCommand::SetPointValue:
        {
            SetPointValueCommand *spv = (SetPointValueCommand*)cmd;

            // move cursor to point updated
            QModelIndex cursor = model->index(spv->row, model->columnFor(spv->series));
            // NOTE: This is to circumvent a performance issue with multiple calls to setCurrentIndex 

            if (inLUW) { // remember and do it at the end -- otherwise major performance impact!!
                itemselection << cursor;
            } else {
                table->selectionModel()->select(cursor, QItemSelectionModel::SelectCurrent);
                table->selectionModel()->setCurrentIndex(cursor, inLUW ? QItemSelectionModel::Select :
                                                        QItemSelectionModel::SelectCurrent);
            }

            break;
        }
        case RideCommand::InsertPoint:
        {
            InsertPointCommand *ip = (InsertPointCommand *)cmd;
            if (undo) { // deleted this row...
                data->deleteRows(ip->row, 1);
            } else {
                data->insertRows(ip->row, 1);
            }
            break;
        }
        case RideCommand::DeletePoint:
        {
            DeletePointCommand *dp = (DeletePointCommand *)cmd;
            if (undo) {
                // clear current
                if (!inLUW) table->selectionModel()->clearSelection();

                // undo delete brings in a row to highlight
                QModelIndex topleft = model->index(dp->row, 0);
                QItemSelection highlight(topleft, model->index(dp->row, model->headings().count()-1));

                // highlight the rows brought back in
                table->selectionModel()->setCurrentIndex(topleft, QItemSelectionModel::Select);
                table->selectionModel()->select(highlight, inLUW ? QItemSelectionModel::Select :
                                                            QItemSelectionModel::SelectCurrent);

                // update the EditorData maps
                data->insertRows(dp->row, 1);
            } else {
                // update the EditorData maps
                data->deleteRows(dp->row, 1);
            }
            break;
        }
        case RideCommand::DeletePoints:
        {
            DeletePointsCommand *dp = (DeletePointsCommand *)cmd;
            if (undo) {

                // clear current
                if (!inLUW) table->selectionModel()->clearSelection();

                // highlight the rows brought back in
                // undo delete brings in a row to highlight
                QModelIndex topleft = model->index(dp->row, 0);
                QItemSelection highlight(topleft, model->index(dp->row+dp->count-1, model->headings().count()-1));

                // highlight the rows brought back in
                table->selectionModel()->setCurrentIndex(topleft, QItemSelectionModel::Select);
                table->selectionModel()->select(highlight, inLUW ? QItemSelectionModel::Select :
                                                            QItemSelectionModel::SelectCurrent);
                // update the EditorData maps
                data->insertRows(dp->row, dp->count);
            } else {
                // update the EditorData maps
                data->deleteRows(dp->row, dp->count);
            }
            break;
        }
        case RideCommand::AppendPoints:
            if (!undo) {

                // clear current
                if (!inLUW) table->selectionModel()->clearSelection();

                // show the user where the rows went
                AppendPointsCommand *ap = (AppendPointsCommand*)cmd;
                QModelIndex topleft = model->index(ap->row, 0);
                QItemSelection highlight(topleft, model->index(ap->row+ap->count-1, model->headings().count()-1));

                // move cursor and highligth all the rows
                table->selectionModel()->setCurrentIndex(topleft, QItemSelectionModel::Select);
                table->selectionModel()->select(highlight, inLUW ? QItemSelectionModel::Select :
                                                    QItemSelectionModel::SelectCurrent);
            }
            break;

        case RideCommand::SetDataPresent:
            {
                // clear current
                if (!inLUW) table->selectionModel()->clearSelection();

                // show the user where the rows went
                SetDataPresentCommand *sp = (SetDataPresentCommand*)cmd;

                // highlight row just arrived
                if ((sp->oldvalue == true && undo == true) || (sp->oldvalue == false && undo == false)) {
                    QModelIndex top = model->index(0, model->columnFor(sp->series));
                    QModelIndex bottom = model->index(ride->ride()->dataPoints().count()-1, model->columnFor(sp->series));
                    QItemSelection highlight(top,bottom);

                    // move cursor and highligth all the rows
                    if (!inLUW) {
                        table->selectionModel()->clearSelection();
                        table->selectionModel()->setCurrentIndex(top, QItemSelectionModel::Select);
                    }
                    table->selectionModel()->select(highlight, inLUW ? QItemSelectionModel::Select :
                                                                QItemSelectionModel::SelectCurrent);
                }

            }
            break;

        case RideCommand::AddXDataSeries:
        {
                // show the user where the rows went
                AddXDataSeriesCommand *sp = (AddXDataSeriesCommand*)cmd;
                XDataEditor *editor = xdataEditors.value(sp->xdata);

                if (!inLUW) editor->selectionModel()->clearSelection();

                // highlight column we just arrived
                if (undo == false) {
                    QModelIndex top = editor->model()->index(0, editor->model()->columnCount()-1);
                    QModelIndex bottom = editor->model()->index(editor->model()->rowCount()-1, editor->model()->columnCount()-1);
                    QItemSelection highlight(top,bottom);

                    // move cursor and highligth all the rows
                    if (!inLUW) {
                        editor->selectionModel()->clearSelection();
                        editor->selectionModel()->setCurrentIndex(top, QItemSelectionModel::Select);
                    }
                    editor->selectionModel()->select(highlight, inLUW ? QItemSelectionModel::Select :
                                                                QItemSelectionModel::SelectCurrent);
                }


        }
        break;

        case RideCommand::LUW:
        {
           inLUW = false;

            // kinda crap, but QItemSelection::merge was painfully slow
            // and we cannot guarantee that a LUW will be in a contiguous
            // range since it collects lots of atomic actions
            int top=99999999, left=99999999, right=-9999999, bottom=-999999;
            foreach (QModelIndex index, itemselection) {
                if (index.row() < top) top = index.row();
                if (index.row() > bottom) bottom = index.row();
                if (index.column() < left) left = index.column();
                if (index.column() > right) right = index.column();
            }
            itemselection.clear();
            table->selectionModel()->select(QItemSelection(model->index(top,left),
                        model->index(bottom,right)), QItemSelectionModel::Select);
        }
            break;

        default:
            break;
    }

    // check for anomalies
    if (!inLUW) {
        anomalyTool->check();
        xdataTool->setRideItem(ride);// rebuild tables

        // let everyone know we changed some data
        ride->notifyRideDataChanged();
    }
}

void
RideEditor::rideClean()
{
    // the file was saved / reverted
    saveAct->setEnabled(false);
}

void
RideEditor::rideDirty()
{
    // the file was updated
    saveAct->setEnabled(true);
}

//----------------------------------------------------------------------
// EditorData functions
//----------------------------------------------------------------------
void
EditorData::deleteRows(int row, int count)
{

    // anomalies
    if (anomalies.count()) {
        QMutableMapIterator <QString, QString> a(anomalies);
        QMap<QString,QString> newa; // updated QMap
        while (a.hasNext()) {
            a.next();

            int crow;
            RideFile::SeriesType series;
            unxsstring(a.key(), crow, series);

            if (crow >= row && crow <= (row+count-1)) {
                // do nothing - i.e. don't copy across - it is zapped
                ;
            } else if (crow > (row+count-1)) {
                crow -= count;
                newa.insert(xsstring(crow,series), a.value());
            } else {
                newa.insert(a.key(), a.value());
            }
        }
        anomalies = newa; // replace with resynced values
    }

    // found
    if (found.count()) {
        QMutableMapIterator <QString, QString> r(found);
        QMap<QString,QString> newr; // updated QMap
        while (r.hasNext()) {
            r.next();

            int crow;
            RideFile::SeriesType series;
            unxsstring(r.key(), crow, series);

            if (crow >= row && crow <= (row+count-1)) {
                // do nothing - i.e. don't copy across
                ;
            } else if (crow > (row+count-1)) {
                crow -= count;
                newr.insert(xsstring(crow,series), r.value());
            } else {
                newr.insert(r.key(), r.value());
            }
        }
        found = newr; // replace with resynced values
    }
}

void
EditorData::deleteSeries(RideFile::SeriesType series)
{

    // anomalies
    if (anomalies.count()) {
        QMutableMapIterator <QString, QString> a(anomalies);
        QMap<QString,QString> newa; // updated QMap
        while (a.hasNext()) {
            a.next();

            int crow;
            RideFile::SeriesType cseries;
            unxsstring(a.key(), crow, cseries);

            if (cseries == series) {
                // do nothing - i.e. don't copy across - it is zapped
                ;
            } else {
                newa.insert(a.key(), a.value());
            }
        }
        anomalies = newa; // replace with resynced values
    }

    // found
    if (found.count()) {
        QMutableMapIterator <QString, QString> r(found);
        QMap<QString,QString> newr; // updated QMap
        while (r.hasNext()) {
            r.next();

            int crow;
            RideFile::SeriesType cseries;
            unxsstring(r.key(), crow, cseries);

            if (cseries == series) {
                // do nothing - i.e. don't copy across
                ;
            } else {
                newr.insert(r.key(), r.value());
            }
        }
        found = newr; // replace with resynced values
    }
}

void
EditorData::insertRows(int row, int count)
{

    // anomalies
    if (anomalies.count()) {
        QMutableMapIterator <QString, QString> a(anomalies);
        QMap<QString,QString> newa; // updated QMap
        while (a.hasNext()) {
            a.next();

            int crow;
            RideFile::SeriesType series;
            unxsstring(a.key(), crow, series);

            if (crow > row) {
                crow += count;
                newa.insert(xsstring(crow,series), a.value());
            } else {
                newa.insert(a.key(), a.value());
            }
        }
        anomalies = newa; // replace with resynced values
    }

    // found
    if (found.count()) {
        QMutableMapIterator <QString, QString> r(found);
        QMap<QString,QString> newr; // updated QMap

        while (r.hasNext()) {
            r.next();

            int crow;
            RideFile::SeriesType series;
            unxsstring(r.key(), crow, series);

            if (crow > row) {
                crow += count;
                newr.insert(xsstring(crow,series), r.value());
            } else {
                newr.insert(r.key(), r.value());
            }
        }
        found = newr; // replace with resynced values
    }
}

//----------------------------------------------------------------------
// Toolbar dialogs
//----------------------------------------------------------------------

//
// Find Dialog
//
FindDialog::FindDialog(RideEditor *rideEditor) : QDialog(rideEditor), rideEditor(rideEditor)
{
    // setup the basic window settings; nonmodal, ontop and delete on close
    setWindowTitle("Search");
    //setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::Tool);
    setMinimumSize(QSize(450*dpiXFactor, 600*dpiYFactor));

    // create UI components
    QLabel *look = new QLabel(tr("Find values"), this);
    type = new QComboBox(this);
    type->addItem(tr("Between"));
    type->addItem(tr("Not Between"));
    type->addItem(tr("Greater Than"));
    type->addItem(tr("Less Than"));
    type->addItem(tr("Equal To"));
    type->addItem(tr("Not Equal To"));

    andLabel = new QLabel(tr("and"), this);
    from = new QDoubleSpinBox(this);
    from->setMinimum(-9999999);
    from->setMaximum(9999999);
    from->setDecimals(5);
    from->setSingleStep(0.00001);

    to = new QDoubleSpinBox(this);
    to->setMinimum(-9999999);
    to->setMaximum(9999999);
    to->setDecimals(5);
    to->setSingleStep(0.00001);


    // buttons
    findButton = new QPushButton(tr("Find"));
    clearButton = new QPushButton(tr("Clear"));

    // results
    resultsTable = new QTableWidget(this);
    resultsTable->setColumnCount(4);
    resultsTable->setColumnHidden(3, true);
    resultsTable->setSortingEnabled(true);
    resultsTable->horizontalHeader()->setStretchLastSection(true);

    QStringList header;
    header << tr("Time") << tr("Column") << tr("Value");
    resultsTable->setHorizontalHeaderLabels(header);
    resultsTable->verticalHeader()->hide();
    resultsTable->setShowGrid(false);
    resultsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    // hidden unless we find something
    replaceButton = new QPushButton(tr("Replace"));
    replaceAllButton = new QPushButton(tr("Replace All"));
    replaceSpinBox = new QDoubleSpinBox(this);
    replaceSpinBox->setMinimum(-9999999);
    replaceSpinBox->setMaximum(9999999);
    replaceSpinBox->setDecimals(5);
    replaceSpinBox->setSingleStep(0.00001);

    // layout the widget
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QGridLayout *criteria = new QGridLayout;
    criteria->setColumnStretch(0,1);
    criteria->setColumnStretch(1,2);

    criteria->addWidget(look, 0,0, Qt::AlignLeft|Qt::AlignTop);
    criteria->addWidget(type, 0,1, Qt::AlignLeft|Qt::AlignTop);
    criteria->addWidget(from, 1,1, Qt::AlignLeft|Qt::AlignTop);
    criteria->addWidget(andLabel, 2,0, Qt::AlignRight|Qt::AlignTop);
    criteria->addWidget(to, 2,1, Qt::AlignLeft|Qt::AlignTop);
    mainLayout->addLayout(criteria);

    chans = new QGridLayout;
    mainLayout->addLayout(chans);


    QHBoxLayout *execute = new QHBoxLayout;
    execute->addStretch();
    execute->addWidget(findButton);
    mainLayout->addLayout(execute);

    mainLayout->addWidget(resultsTable);

    // replace stuff
    QGridLayout *replaceLayout = new QGridLayout;
    replaceLayout->addWidget(replaceSpinBox, 0,0);
    replaceLayout->addWidget(replaceButton, 0, 1);
    replaceLayout->addWidget(replaceAllButton, 1,1);
    mainLayout->addLayout(replaceLayout);

    QHBoxLayout *closer = new QHBoxLayout;
    closer->addStretch();
    closer->addWidget(clearButton);
    mainLayout->addLayout(closer);

    setLayout(mainLayout);

    connect(type, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
    connect(findButton, SIGNAL(clicked()), this, SLOT(find()));
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clear()));
    connect(replaceButton, SIGNAL(clicked()), this, SLOT(replace()));
    connect(replaceAllButton, SIGNAL(clicked()), this, SLOT(replaceAll()));
    connect(resultsTable, SIGNAL(itemSelectionChanged()), this, SLOT(selection()));

    // refresh when data changes...
    if (rideEditor->ride && rideEditor->ride->ride())
    connect(rideEditor->ride->ride()->command, SIGNAL(endCommand(bool,RideCommand*)), this, SLOT(dataChanged()));

}

FindDialog::~FindDialog()
{
}

void
FindDialog::replace()
{
    // do we have something to work with?
    if (resultsTable->selectedItems().count() > 0) {

        int row;
        RideFile::SeriesType series;
        unxsstring(resultsTable->item(resultsTable->currentRow(), 3)->text(), row, series);

        // set to be visible regardless of what else happenned
        rideEditor->table->setCurrentIndex(rideEditor->model->index(row,rideEditor->model->columnFor(series)));

        // set the value
        rideEditor->ride->ride()->command->setPointValue(row, series, replaceSpinBox->value());

        // notify it changed
        dataChanged();
    }
}

void
FindDialog::replaceAll()
{

    // set a LUW and make all changes, then a dialog when done
    if(resultsTable->rowCount() > 0) {

        // keep count of changes made
        int count=0;

        // group for undo/redo
        rideEditor->ride->ride()->command->startLUW("Replace All");

        // loop through results setting to the pointvalue
        for(int idx=0; idx < resultsTable->rowCount(); idx++) {

            int row;
            RideFile::SeriesType series;
            unxsstring(resultsTable->item(idx, 3)->text(), row, series);

            // set the value
            rideEditor->ride->ride()->command->setPointValue(row, series, replaceSpinBox->value());

            // counter
            count++;
        }

        // done on LUW
        rideEditor->ride->ride()->command->endLUW();

        // changed
        dataChanged();

        // tell user
        if (count > 0) {
            QMessageBox info(QMessageBox::Information, tr("Replace All"),
                            QString(tr("Replaced %1 values")).arg(count));
            info.exec();
        }
    }
}


void
FindDialog::typeChanged(int index)
{
    // 0 and 1 are range based the rest are single value
    if (index < 2) {
        from->show();
        to->show();
        andLabel->show();
    } else {
        to->hide();
        andLabel->hide();
    }
}

// close doesnt really close, just hides
void
FindDialog::closeEvent(QCloseEvent* event)
{
    event->ignore();
    rideSelected(); // reset the search...
    hide();
}

void
FindDialog::reject()
{
    hide();
}

void
FindDialog::find()
{
    if (rideEditor->data == NULL) return;

    // are we looking anywhere?
    bool search = false;
    foreach (QCheckBox *c, channels) if (c->isChecked()) search = true;
    if (search == false) return;

    // ok something to do then...
    rideEditor->data->found.clear();
    clearResultsTable();

    for (int i=0; i< rideEditor->ride->ride()->dataPoints().count(); i++) {
        // for each selected channel, get the value and
        // see if it matches

        foreach (QCheckBox *c, channels) {

            if (c->isChecked()) {

                // which Column?
                int col = rideEditor->model->headings().indexOf(c->text());
                if (col >= 0) {

                    double value = rideEditor->getValue(i, col);

                    bool match = false;
                    switch(type->currentIndex()) {

                    case 0 : // between
                        if ((value >= from->value() && value <= to->value()) ||
                            (value <= from->value() && value >= to->value())) match = true;
                        break;

                    case 1 : // not between
                        if (!(value >= from->value() && value <= to->value())) match = true;
                        break;

                    case 2 : // greater than
                        if (value > from->value()) match = true;
                        break;

                    case 3 : // less than
                        if (value < from->value()) match = true;
                        break;

                    case 4 : // matches
                        if (value == from->value()) match = true;
                        break;

                    case 5 : // not equal
                        if (value != from->value()) match = true;
                        break;

                    }

                    if (match == true) {

                        // highlight on the table
                        rideEditor->data->found.insert(xsstring(i,rideEditor->model->columnType(col)), QString("%1").arg(value));

                    }
                }
            }
        }
    }

    dataChanged(); // update results table and redraw

    rideEditor->model->forceRedraw();
}

void
FindDialog::dataChanged()
{
    // a column or row was inserted/deleted
    // we need to refresh the results table
    // to reflect the new values
    clearResultsTable();

    // do not be tempted to removed the following two lines!
    // by setting the row count to zero the item selection
    // is cleared properly and fixes a crash in the line
    // marked ZZZZ below. This only occurs when the current item
    // selected is greater than the new row count.
    resultsTable->setRowCount(0); // <<< fixes crash at ZZZZ
    resultsTable->setColumnCount(0);

    resultsTable->setRowCount(rideEditor->data->found.count()); // <<< ZZZZ
    resultsTable->setColumnCount(4);
    resultsTable->setColumnHidden(3, true); // has start xystring
    QMapIterator<QString,QString> f(rideEditor->data->found);

    resultsTable->setSortingEnabled(false);// see QT Bug QTBUG-7483

    int counter =0;
    while (f.hasNext()) {

        f.next();

        int row;
        RideFile::SeriesType series;
        unxsstring(f.key(), row, series);

        // time -- format correctly... held as a double in the model
        int seconds, msecs;
        secsMsecs(rideEditor->model->getValue(row,0), seconds, msecs);
        QString value = QTime(0,0,0,0).addSecs(seconds).addMSecs(msecs).toString("hh:mm:ss.zzz");

        QTableWidgetItem *t = new QTableWidgetItem;
        t->setText(value);
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        resultsTable->setItem(counter, 0, t);

        // channel
        t = new QTableWidgetItem;
        t->setText(RideFile::seriesName(series));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        resultsTable->setItem(counter, 1, t);

        // value
        t = new QTableWidgetItem;
        t->setData(Qt::DisplayRole, QVariant(rideEditor->ride->ride()->getPointValue(row,series)));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        resultsTable->setItem(counter, 2, t);

        // xs for selection
        t = new QTableWidgetItem;
        t->setText(f.key());
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        resultsTable->setItem(counter, 3, t);

        resultsTable->setRowHeight(counter, 20*dpiYFactor);

        counter++;
    }
    resultsTable->setSortingEnabled(true);// see QT Bug QTBUG-7483
    QStringList header;
    header << "Time" << "Column" << "Value";
    resultsTable->setHorizontalHeaderLabels(header);

    // select the first one we found
    if (counter)  resultsTable->selectRow(0);
}

void
FindDialog::clear()
{
    if (rideEditor->data) {
        rideEditor->data->found.clear();
        clearResultsTable();
        rideEditor->model->forceRedraw();
    }
}

void
FindDialog::rideSelected()
{
    // Update the channels we can search
    // wipe old ones
    foreach(QWidget *x, channels) {
        chans->removeWidget(x);
        delete x;
    }
    channels.clear();

    // add new ones
    foreach (QString heading, rideEditor->model->headings()) {
        QCheckBox *add = new QCheckBox(heading);
        if (heading == tr("Power"))
            add->setChecked(true);
        else
            add->setChecked(false);
        channels << add;
    }

    int row =0;
    int col =0;
    foreach (QCheckBox *check, channels) {
        chans->addWidget(check, row,col);
        if (++col > 2) { col =0; row++; }
    }

    // clear old search results etc
    clear();
}

void
FindDialog::selection()
{
    if (resultsTable->currentRow() < 0) return;

    // jump to the found item in the main table
    int row;
    RideFile::SeriesType series;
    unxsstring(resultsTable->item(resultsTable->currentRow(), 3)->text(), row, series);
    rideEditor->table->setCurrentIndex(rideEditor->model->index(row,rideEditor->model->columnFor(series)));
}

void
FindDialog::clearResultsTable()
{
    resultsTable->selectionModel()->clearSelection();
    // zap the 3 main cols and two hidden ones
    for (int i=0; i<resultsTable->rowCount(); i++) {
        for (int j=0; j<resultsTable->columnCount(); j++)
            delete resultsTable->takeItem(i,j);
    }
}

//
// Paste Special Dialog
//
PasteSpecialDialog::PasteSpecialDialog(RideEditor *rideEditor, QWidget *parent) : QDialog(parent), rideEditor(rideEditor)
{
    // setup the basic window settings; nonmodal, ontop and delete on close
    setWindowTitle(tr("Paste Special"));
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::Tool);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // create the widgets
    mode = new QGroupBox(tr("Paste mode"));
    separators = new QGroupBox(tr("Separator options"));
    contents = new QGroupBox(tr("Columns"));

    append = new QRadioButton(tr("Append"));
    overwrite = new QRadioButton(tr("Overwrite"));

    QVBoxLayout *modeLayout = new QVBoxLayout;
    modeLayout->addWidget(append);
    modeLayout->addWidget(overwrite);
    mode->setLayout(modeLayout);
    mode->setFlat(false);
    append->setChecked(true);

    hasHeader = new QCheckBox(tr("First line has headings"));
    tab = new QCheckBox(tr("Tab"));
    tab->setChecked(true);
    comma = new QCheckBox(tr("Comma"));
    semi = new QCheckBox(tr("Semi-colon"));
    space = new QCheckBox(tr("Space"));
    other = new QCheckBox(tr("Other"));
    otherText = new QLineEdit;
    QHBoxLayout *otherLayout = new QHBoxLayout;
    otherLayout->addWidget(other);
    otherLayout->addWidget(otherText);
    otherLayout->addStretch();

    QGridLayout *sepLayout = new QGridLayout;
    sepLayout->addWidget(hasHeader, 0,0);
    sepLayout->addWidget(tab, 1,0);
    sepLayout->addWidget(comma, 1,1);
    sepLayout->addWidget(semi, 1,2);
    sepLayout->addWidget(space, 1,3);
    sepLayout->addLayout(otherLayout, 2,0,1,4);

    separators->setLayout(sepLayout);
    separators->setFlat(false);

    // what we got?
    seps << "\t";
    getPaste(cells, seps, sourceHeadings, false);

    resultsTable = new QTableView(this);
    resultsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    resultsTable->setSelectionBehavior(QAbstractItemView::SelectColumns);
    QFont font;
    font.setPointSize(font.pointSize()-2); // smaller please
    resultsTable->setFont(font);
    resultsTable->verticalHeader()->setDefaultSectionSize(QFontMetrics(font).height()+2);

    model = new QStandardItemModel;
    resultsTable->setModel(model);
    setResultsTable();

    // column selector
    QHBoxLayout *selectorLayout = new QHBoxLayout;
    QLabel *selectLabel = new QLabel(tr("Column Type"));
    columnSelect = new QComboBox;
    columnSelect->addItem(tr("Ignore"));
    foreach (QString name, rideEditor->model->headings())
        columnSelect->addItem(name);
    selectorLayout->addWidget(selectLabel);
    selectorLayout->addWidget(columnSelect);
    selectorLayout->addStretch();

    // contents layout
    QVBoxLayout *contentsLayout = new QVBoxLayout;
    contentsLayout->addLayout(selectorLayout);
    contentsLayout->addWidget(resultsTable);
    contents->setLayout(contentsLayout);
    contents->setFlat(false);

    okButton = new QPushButton(tr("OK"));
    cancelButton = new QPushButton(tr("Cancel"));
    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(cancelButton);
    buttons->addWidget(okButton);

    // layout the widgets
    QGridLayout *widgetLayout = new QGridLayout;
    widgetLayout->addWidget(mode, 0,0);
    widgetLayout->addWidget(separators,0,1);
    widgetLayout->addWidget(contents,1,0,1,2);
    widgetLayout->setRowStretch(0,1);
    widgetLayout->setRowStretch(1,4);

    mainLayout->addLayout(widgetLayout);
    mainLayout->addLayout(buttons);

    // set size hint
    setMinimumHeight(300*dpiYFactor);
    setMinimumWidth(500*dpiXFactor);

    connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(columnSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(columnChanged()));
    connect(resultsTable->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
            this, SLOT(setColumnSelect()));
    connect(hasHeader, SIGNAL(stateChanged(int)), this, SLOT(sepsChanged()));
    connect(tab, SIGNAL(stateChanged(int)), this, SLOT(sepsChanged()));
    connect(comma, SIGNAL(stateChanged(int)), this, SLOT(sepsChanged()));
    connect(semi, SIGNAL(stateChanged(int)), this, SLOT(sepsChanged()));
    connect(space, SIGNAL(stateChanged(int)), this, SLOT(sepsChanged()));
    connect(other, SIGNAL(stateChanged(int)), this, SLOT(sepsChanged()));
}

void
PasteSpecialDialog::clearResultsTable()
{
    model->clear();
}

void
PasteSpecialDialog::setResultsTable()
{
    model->clear();
    headings.clear();

    // is it like what was just copied and no headings line?
    if (hasHeader->isChecked() == false && cells[0].count() == rideEditor->copyHeadings.count()) {
        headings = rideEditor->copyHeadings;
    } else {
        for (int i=0; hasHeader->isChecked() ? (i<sourceHeadings.count()) : (i<cells[0].count()); i++) {
            if (hasHeader->isChecked() == true) {
                // have we mapped this before?
                QString lookup = GC_QSETTINGS_GLOBAL_GENERAL+QString("colmap/") + sourceHeadings[i];
                QString mapto = appsettings->value(this, lookup, "Ignore").toString();
                // is this an available heading tho?
                if (columnSelect->findText(mapto) != -1) {
                    headings << mapto;
                } else {
                    headings << "Ignore";
                }
            } else {
                headings << "Ignore";
            }
        }
    }

    model->setRowCount(cells.count() < 50 ? cells.count() : 50);
    model->setColumnCount(cells[0].count());
    model->setHorizontalHeaderLabels(headings);

    // just setup the first 50 rows
    for (int row=0; row < 50 && row < cells.count(); row++) {
        for (int col=0; col < cells[row].count(); col++) {
            // add value
            model->setItem(row, col, new QStandardItem(QString("%1").arg(cells[row][col])));
        }
    }
}

PasteSpecialDialog::~PasteSpecialDialog()
{
    clearResultsTable();
    rideEditor->model->forceRedraw();
}

void
PasteSpecialDialog::okClicked()
{
    // headings has the headings for each column
    // with "Ignore" set if we are to ignore it
    // cells contains all the actual data from the
    // buffer.
    // We have three modes; (1) insert means add new
    // samples at the top of the current selection
    // which will mean all time/distance offsets for
    // the remaining rows will be increased and the
    // time/distance for the inserted rows will start
    // after the previous row time/distance
    // (2) append will add all samples to the tail of
    // the ride file with time/distance offset from
    // the last sample currently in the ride
    // (3) overwrite will change the current samples to
    // use the non-ignored values, including time/distance
    // which may well create out-of-sync values. Which
    // is why we do an anomaly check at the end.

    // add these to the pasted rows
    double timeOffset=0, distanceOffset=0;
    int where = rideEditor->ride->ride()->dataPoints().count();

    if (append->isChecked() && rideEditor->ride->ride()->dataPoints().count()) {
        timeOffset = rideEditor->ride->ride()->dataPoints().last()->secs;
        distanceOffset = rideEditor->ride->ride()->dataPoints().last()->km;
    }

    // if we are inserting or appending lets create an array of rows to insert
    if (append->isChecked()) {

        QVector <struct RideFilePoint> newRows;

        for (int row=0; row < cells.count(); row++) {

            struct RideFilePoint newrow;
            newrow.secs = timeOffset;   // in case it is being ignore or not available
            newrow.km = distanceOffset; // in case it is being ignored or not available

            for (int col=0; col < cells[row].count(); col++) {
                if (headings[col] == tr("Ignore")) continue;

                double value;
                if (headings[col] == tr("Time")) value = cells[row][col] + timeOffset;
                else if (headings[col] == tr("Distance")) value = cells[row][col] + distanceOffset;
                else value = cells[row][col];

                // update the relevant value in the new dataPoint based upon the heading...
                if (headings[col] == tr("Time")) newrow.secs = value;
                if (headings[col] == tr("Distance")) newrow.km = value;
                if (headings[col] == tr("Speed")) newrow.kph = value;
                if (headings[col] == tr("Cadence")) newrow.cad = value;
                if (headings[col] == tr("Power")) newrow.watts = value;
                if (headings[col] == tr("Heartrate")) newrow.hr = value;
                if (headings[col] == tr("Torque")) newrow.nm = value;
                if (headings[col] == tr("Latitude")) newrow.lat = value;
                if (headings[col] == tr("Longitude")) newrow.lon = value;
                if (headings[col] == tr("Altitude")) newrow.alt = value;
                if (headings[col] == tr("Headwind")) newrow.headwind = value;
                if (headings[col] == tr("Slope")) newrow.slope = value;
                if (headings[col] == tr("Temperature")) newrow.temp = value;
                if (headings[col] == tr("SmO2")) newrow.smo2 = value;
                if (headings[col] == tr("tHb")) newrow.thb = value;
                if (headings[col] == tr("Interval")) newrow.interval = value;
            }

            // add to the list
            newRows << newrow;
        }

        // ok. we are good to go, so overwrite target with source
        rideEditor->ride->ride()->command->startLUW("Paste Special");

        // now we have an array add to dataPoints
        rideEditor->model->appendRows(newRows);

        // highlight the affected cells -- good UI for paste, since it might
        // update offscreen (esp. true for our paste append)
        QItemSelection highlight = QItemSelection(rideEditor->model->index(where, 0),
                                                  rideEditor->model->index(where+newRows.count()-1, rideEditor->model->headings().count()-1));

        // our job is done.
        rideEditor->ride->ride()->command->endLUW();

    } else {

        // *** The paste special overwrite function is somewhat
        // *** "clever", it will only overwrite columns that were
        // *** selected in the dialog box amd will only overwrite
        // *** existing rows. This makes the paste operation quite
        // *** "complicated". We do quite a lot of checks to make
        // *** sure the user really mean't to do this...

        // get the target selection
        QList<QModelIndex> selection = rideEditor->table->selectionModel()->selection().indexes();
        if (selection.count() == 0) {
            // wrong size
            QMessageBox oops(QMessageBox::Critical, tr("Paste error"),
                            tr("Please select target cell or cells to paste values into."));
            oops.exec();
            accept();
            return;
        }

        // to make code more readable, and with an eye to refactoring
        // use these vars to describe the range selected in the table
        // the target we will paste to (i.e. we may truncate) and the
        // range available in the clipboard
        struct range { int row, column, rows, columns; } selected, target, source;
        bool norange = selection.count() == 1 ? true : false;
        bool truncate = false, partial = false;

        // what is selected in the selection model?
        selected.row = selection.first().row();
        selected.column = selection.first().column();
        selected.rows = selection.last().row() - selection.first().row() + 1;
        selected.columns = selection.last().column() - selection.first().column() + 1;
        if (norange) {
            // Single cell selected for paste means 'from here' not
            // 'into here'. So from here to end of the row is selected
            // because we check by series type not column number when
            // the paste is performed we will not go out of bounds
            selected.columns = rideEditor->model->headings().count() - selected.column;
        }

        // what is in the clipboard?
        source.row = 0; // not defined, obviously
        source.column = 0;
        source.rows = cells.count();
        source.columns = 0;
        foreach(QString heading, headings)
            if (heading != tr("Ignore")) source.columns++;

        // so what is the target?
        target.row = selected.row;
        target.column = selected.column;
        if (norange) {
            target.rows = source.rows;
            target.columns = source.columns;
        } else {
            target.rows = selected.rows;
            target.columns = selected.columns;
        }
        // out of bounds for ride?
        if (target.row + target.rows > rideEditor->ride->ride()->dataPoints().count()) {
            truncate = true;
            target.rows = rideEditor->ride->ride()->dataPoints().count() - target.row;
        }
        // selection smaller than clipboard?
        if (source.rows > target.rows) {
            truncate = true;
        }
        // partially fill selected rows ?
        if (source.rows < target.rows) {
            partial = true;
            target.rows = source.rows;
        }
        // out of bounds for columns?
        if (target.column + target.columns - 1 > rideEditor->model->headings().count()) {
            truncate = true;
            target.columns = rideEditor->model->headings().count() - target.column;
        }
        // selection smaller than clipboard?
        if (source.columns > target.columns) {
            truncate = true;
        }
        // partially fill columns ?
        if (source.columns < target.columns) {
            partial = true;
            target.columns = source.columns;
        }

        //
        // Now we have calculated the source and target, lets
        // make sure the user agrees ...
        //
        if (truncate || partial) {

            // we are going to truncate
            QMessageBox confirm(QMessageBox::Question, tr("Copy/Paste Mismatch"),
                                tr("The selected range and available data have "
                                   "different sizes, some data may be lost.\n\n"
                                   "Do you want to continue?"),
                                QMessageBox::Ok | QMessageBox::Cancel);

            if ((confirm.exec() & QMessageBox::Cancel) != 0) {
                accept();
                return; // accept doesn't return.
            }
        }

        // ok. we are good to go, so overwrite target with source
        rideEditor->ride->ride()->command->startLUW("Paste Special");

        for (int i = 0; i < target.rows; i++) {

            for (int j = 0; j < target.columns; j++) {

                // target column type...
                RideFile::SeriesType what = rideEditor->model->columnType(target.column + j);

                // do we have that?
                int sourceSeries = headings.indexOf(RideFile::seriesName(what));
                if (sourceSeries != -1) // YES, we have some
                    rideEditor->ride->ride()->command->setPointValue(target.row+i, what, cells[i][sourceSeries]);
            }
        }

        // highlight what we did
        QItemSelection highlight = QItemSelection(rideEditor->model->index(target.row,target.column),
                                                  rideEditor->model->index(target.row+target.rows-1, target.column+target.columns-1));

        // all done.
        rideEditor->ride->ride()->command->endLUW();
    }
    accept();
}

void
PasteSpecialDialog::cancelClicked()
{
    reject();
}

void
PasteSpecialDialog::sepsChanged()
{
    seps.clear();
    cells.clear();
    sourceHeadings.clear();

    if (tab->isChecked()) seps << "\t";
    if (comma->isChecked()) seps << ",";
    if (semi->isChecked()) seps << ";";
    if (space->isChecked()) seps << " ";
    if (other->isChecked()) seps << otherText->text();

    getPaste(cells, seps, sourceHeadings, hasHeader->isChecked());
    setResultsTable();
}

void
PasteSpecialDialog::setColumnSelect()
{
    QList<QModelIndex> selection = resultsTable->selectionModel()->selection().indexes();
    if (selection.count() == 0) return;
    int column = selection[0].column(); // which column?
    active = true;
    columnSelect->setCurrentIndex(columnSelect->findText(headings[column]));
    active = false;

}

void
PasteSpecialDialog::columnChanged()
{
    if (active) return;

    // is anything selected?
    QList<QModelIndex> selection = resultsTable->selectionModel()->selection().indexes();
    if (selection.count() == 0) return;

    // set column heading
    int column = selection[0].column();
    QString text = columnSelect->itemText(columnSelect->currentIndex());

    // set the headings string
    headings[column] = text;

    // now update the results table
    model->setHorizontalHeaderLabels(headings);

    // lets remember this mapping if its to a source header
    if (hasHeader->isChecked() && headings[column] != "Ignore") {
        QString lookup = GC_QSETTINGS_GLOBAL_GENERAL+QString("colmap/") + sourceHeadings[column];
        appsettings->setValue(lookup, headings[column]);
    }
}

AnomalyDialog::AnomalyDialog(RideEditor *rideEditor) : QDialog(rideEditor), rideEditor(rideEditor)
{
    // setup the basic window settings; nonmodal, ontop and delete on close
    setWindowTitle(tr("Anomalies"));
    //setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::Tool);
    setMinimumSize(QSize(200*dpiXFactor, 300*dpiYFactor));
    QVBoxLayout *main = new QVBoxLayout(this);
    main->setContentsMargins(0,0,0,0);
    main->setSpacing(0);

    anomalyList = new QTableWidget(this);
    main->addWidget(anomalyList);
    anomalyList->setColumnCount(2);
    anomalyList->setColumnHidden(0, true);
    anomalyList->setSortingEnabled(false);
    QStringList header;
    header << "Id" << "Anomalies";
    anomalyList->setHorizontalHeaderLabels(header);
    anomalyList->horizontalHeader()->setStretchLastSection(true);
    anomalyList->verticalHeader()->hide();
    anomalyList->setShowGrid(false);
    anomalyList->setSelectionMode(QAbstractItemView::SingleSelection);
    anomalyList->setSelectionBehavior(QAbstractItemView::SelectRows);
}

AnomalyDialog::~AnomalyDialog()
{
}

// close doesnt really close, just hides
void
AnomalyDialog::closeEvent(QCloseEvent* event)
{
    event->ignore();
    hide();
}

void
AnomalyDialog::reject()
{
    hide();
}

QSize EditorTabBar::tabSizeHint(int index) const
{
    QSize def = QTabBar::tabSizeHint(index);
    def.setWidth(20 *dpiXFactor); // totally ignored, I hate QT sometimes
    return def;
}

///
/// XDataEditor
///
XDataEditor::XDataEditor(QWidget *parent, QString xdata) : QTableView(parent), xdata(xdata)
{

    _model = new XDataTableModel(NULL, xdata);

#ifdef Q_OS_WIN
    QStyle *cde = QStyleFactory::create(OS_STYLE);
    verticalScrollBar()->setStyle(cde);
    horizontalScrollBar()->setStyle(cde);
#endif
    verticalHeader()->setDefaultSectionSize(20 *dpiYFactor);
    setModel(_model);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionMode(QAbstractItemView::ContiguousSelection);
    setGridStyle(Qt::NoPen);
    setItemDelegate(new XDataCellDelegate(this));
    setContextMenuPolicy(Qt::CustomContextMenu);
    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    setContentsMargins(0,0,0,0);
    installEventFilter(this);

    configChanged();
}

void XDataEditor::configChanged()
{

    QPalette palette;
    palette.setColor(QPalette::Active, QPalette::Background, GColor(CPLOTBACKGROUND));
    palette.setColor(QPalette::Active, QPalette::Base, GColor(CPLOTBACKGROUND));
    palette.setColor(QPalette::Active, QPalette::WindowText, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::Active, QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::Active, QPalette::Window, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::Inactive, QPalette::Background, GColor(CPLOTBACKGROUND));
    palette.setColor(QPalette::Inactive, QPalette::Base, GColor(CPLOTBACKGROUND));
    palette.setColor(QPalette::Inactive, QPalette::WindowText, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::Inactive, QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::Inactive, QPalette::Window, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    setPalette(palette);
    setFrameStyle(QFrame::NoFrame);
    setStyleSheet(QString("QTableView QTableCornerButton::section { background-color: %1; color: %2; border: %1 }"
                                  "QHeaderView { background-color: %1; color: %2; border: %1 }")
                    .arg(GColor(CPLOTBACKGROUND).name())
                    .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name()));
    horizontalHeader()->setStyleSheet(QString("QHeaderView::section { background-color: %1; color: %2; border: 0px; border-bottom: %3px solid %2; }")
                    .arg(GColor(CPLOTBACKGROUND).name())
                    .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name())
                    .arg(2 * dpiYFactor));
    verticalHeader()->setStyleSheet(QString("QHeaderView::section { background-color: %1; color: %2; border: 0px }")
                    .arg(GColor(CPLOTBACKGROUND).name())
                    .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name()));
#ifndef Q_OS_MAC
    verticalScrollBar()->setStyleSheet(TabView::ourStyleSheet());
    horizontalScrollBar()->setStyleSheet(TabView::ourStyleSheet());
#endif
}

void XDataEditor::setRideItem(RideItem *item)
{
    _model->setRide(item->ride());

    // resize appropriately
    resizeColumnsToContents();

    // but time is xx:xx:xx:xxx
    QFontMetrics fm(font());
    int cwidth=fm.charWidth("X",0);
    setColumnWidth(0, 15 * cwidth * dpiXFactor);
}

void
XDataEditor::selectIntervals(QList<IntervalItem*> intervals)
{
    // no model or no series, nothing to select
    if (_model == NULL || _model->series == NULL) return;

    // highlight selection and jump to last
    foreach(IntervalItem *interval, intervals) {

        // what is the first dataPoint index for this interval?
        int start = _model->series->timeIndex(interval->start);
        int end = _model->series->timeIndex(interval->stop);
        if (end < _model->series->datapoints.count()-1) end--;

        // select all the rows
        selectionModel()->clearSelection();
        selectionModel()->setCurrentIndex(_model->index(start,0), QItemSelectionModel::Select);
        selectionModel()->select(QItemSelection(_model->index(start,0),
                                 _model->index(end,_model->columnCount()-1)),
                                 QItemSelectionModel::Select);

    }
}

void
XDataEditor::setModelValue(int row, int col, double value)
{
    _model->setValue(row, col, value);
}

bool
XDataEditor::eventFilter(QObject *object, QEvent *e)
{
    // not for me?
    if (object != (QObject *)this) return false;

    // what happened?
    switch(e->type())
    {
        case QEvent::ContextMenu:
            borderMenu(mapFromGlobal(QCursor::pos()));
            return true;
            break;


        case QEvent::KeyPress:
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
            if (keyEvent->modifiers() & Qt::ControlModifier) {
                switch (keyEvent->key()) {

                    case Qt::Key_C: // defacto standard for copy
                        copy();
                        return true;

                    case Qt::Key_V: // defacto standard for paste
                        paste();
                        return true;

                    case Qt::Key_X: // defacto standard for cut
                        cut();
                        return true;

                    /*case Qt::Key_Y: // emerging standard for redo
                         redo();
                         return true;

                    case Qt::Key_Z: // common standard for undo
                         undo();
                         return true;

                    case Qt::Key_0:
                        clear();
                        return true;*/

                    default:
                        return false;
                }
            }
            break;
        }

        default:
            break;
    }
    return false;
}

void
XDataEditor::borderMenu(const QPoint &pos)
{

    int column=0, row=0;
    currentRow=currentColumn=-1;

    // but we need to set the row or column to zero since
    // we are in the border, this seems an easy and quick way
    // to do this (the indexAt function assumes pos starts from
    // 0 for row/col 0 and does not include the vertical
    // or horizontal header width (which is what we go passed)
    if (pos.y() <= horizontalHeader()->height()) {
        row = 0;
        column = horizontalHeader()->logicalIndexAt(pos - QPoint(verticalHeader()->width(), 0));
    }
    if (pos.x() <= verticalHeader()->width()) {
        column = 0;
        row = verticalHeader()->logicalIndexAt(pos - QPoint(0, horizontalHeader()->height()));
    }

    // set to negative if in the border
    if (pos.x() < verticalHeader()->width()) column = -1;
    if (pos.y() < horizontalHeader()->height()) row = -1;

    QMenu menu(this);

    QIcon cutIcon(":images/toolbar/cut.png");
    QIcon pasteIcon(":images/toolbar/paste.png");
    QIcon copyIcon(":images/toolbar/copy.png");

    bool pastable = QApplication::clipboard()->text() == "" ? false : true;

    QAction *cutAct = new QAction(cutIcon, tr("Cut"), this);
    cutAct->setShortcut(QKeySequence("Ctrl+X"));
    cutAct->setEnabled(isRowSelected() || isColumnSelected());
    menu.addAction(cutAct);
    connect(cutAct, SIGNAL(triggered()), this, SLOT(cut()));

    QAction *copyAct = new QAction(copyIcon, tr("Copy"), this);
    copyAct->setShortcut(QKeySequence("Ctrl+C"));
    copyAct->setEnabled(true);
    menu.addAction(copyAct);
    connect(copyAct, SIGNAL(triggered()), this, SLOT(copy()));

    QAction *pasteAct = new QAction(pasteIcon, tr("Paste"), this);
    pasteAct->setShortcut(QKeySequence("Ctrl+V"));
    pasteAct->setEnabled(pastable);
    menu.addAction(pasteAct);
    connect(pasteAct, SIGNAL(triggered()), this, SLOT(paste()));

    menu.addSeparator();

    if (row >= 0 && column < 0) {

        QAction *delAct = new QAction(tr("Delete Row"), this);
        delAct->setEnabled(isRowSelected());
        menu.addAction(delAct);
        connect(delAct, SIGNAL(triggered()), this, SLOT(delRow()));

    }

    if (column <= 0) {

        if ((row+1) == _model->rowCount()) {
            QAction *appAct = new QAction(tr("Append Row"), this);
            appAct->setEnabled(true);
            menu.addAction(appAct);
            connect(appAct, SIGNAL(triggered()), this, SLOT(appRow()));
        }
        if (row >= 0) {
            QAction *insAct = new QAction(tr("Insert Row"), this);
            insAct->setEnabled(true);
            menu.addAction(insAct);
            connect(insAct, SIGNAL(triggered()), this, SLOT(insRow()));
        }
    }

    if (column >= 2){

        QAction *delAct = new QAction(tr("Remove Column"), this);
        delAct->setEnabled(isColumnSelected());
        menu.addAction(delAct);
        connect(delAct, SIGNAL(triggered()), this, SLOT(delCol()));
    }

    if (row < 0) {
        QAction *insAct = new QAction(tr("Add Column"), this);
        insAct->setEnabled(true);
        menu.addAction(insAct);
        connect(insAct, SIGNAL(triggered()), this, SLOT(insCol()));
    }

    currentRow=row;
    currentColumn=column;

    //currentCell.row = row < 0 ? 0 : row;
    //currentCell.column = column < 0 ? 0 : column;
    menu.exec(mapToGlobal(QPoint(pos.x()+5, pos.y()+5)));
}


bool
XDataEditor::isRowSelected()
{
    QList<QModelIndex> selection = selectionModel()->selection().indexes();

    if (selection.count() > 0 &&
        selection[0].column() == 0 &&
        selection[selection.count()-1].column() == (_model->columnCount()-1))
        return true;

    return false;
}

bool
XDataEditor::isColumnSelected()
{
    QList<QModelIndex> selection = selectionModel()->selection().indexes();

    // if no rows then always true
    if (_model->rowCount()==0) return true;

    // must select every row for this column to be selected
    if (selection.count() > 0 &&
        selection[0].row() == 0 &&
        selection[selection.count()-1].row() == (_model->rowCount()-1))
        return true;

    return false;
}

void
XDataEditor::insCol()
{
    QString name, unit;
    XDataSeriesSettingsDialog *dialog = new  XDataSeriesSettingsDialog(this, name, unit);
    int ret = dialog->exec();

    if (ret == QDialog::Accepted && name != "") {
        _model->insertColumn(name);
    }
}
void
XDataEditor::delCol()
{
    _model->removeColumn(currentColumn);
}
void
XDataEditor::insRow()
{
    _model->insertRow(currentRow, QModelIndex());
}
void
XDataEditor::appRow()
{
    QVector<XDataPoint*> rows;
    rows << new XDataPoint;
    _model->appendRows(rows);  
}
void
XDataEditor::appRows(int count)
{
    QVector<XDataPoint*> rows;
    for (int i=0;i<count;i++)
    rows << new XDataPoint;
    _model->appendRows(rows);
}
void
XDataEditor::delRow()
{
 // run through the selected rows and zap them
    QList<QModelIndex> selection = selectionModel()->selection().indexes();

    if (selection.count() > 0) {

        // delete from table - we do in one hit since row-by-row is VERY slow
        _model->ride->command->startLUW("Delete XData Rows");
        _model->removeRows(selection[0].row(),
                          selection[selection.count()-1].row() - selection[0].row() + 1, QModelIndex());
        _model->ride->command->endLUW();

    }
}

void
XDataEditor::copy()
{
    QList<QModelIndex> selection = selectionModel()->selection().indexes();

    if (selection.count() > 0) {
        QString text;
        for (int row = selection[0].row(); row <= selection[selection.count()-1].row(); row++) {

            for (int column = selection[0].column();  column <= selection[selection.count()-1].column(); column++) {
                if (column == selection[selection.count()-1].column())
                    text += QString("%1").arg(_model->getValue(row,column-2), 0, 'g', 11);
                else
                    text += QString("%1\t").arg(_model->getValue(row,column-2), 0, 'g', 11);
            }
            text += "\n";
        }
        QApplication::clipboard()->setText(text);
    }
}

void
XDataEditor::cut()
{
    copy();
    if (isRowSelected()) delRow();
    else if (isColumnSelected()) delCol();
}

void
XDataEditor::paste()
{
    QVector<QVector<double> > cells;
    QStringList seps, head;
    seps << "\t";

    getPaste(cells, seps, head, false);

    // empty paste buffer
    if (cells.count() == 0 || cells[0].count() == 0) return;

    // if selected range is not the same
    // size as the copy buffer then barf
    // unless just a single cell selected
    QList<QModelIndex> selection = selectionModel()->selection().indexes();

    // is anything selected?
    if (selection.count() == 0) {
        // wrong size
        QMessageBox oops(QMessageBox::Critical, tr("Paste error"),
                         tr("Please select target cell or cells to paste values into."));
        oops.exec();
        return;
    }

    int selectedrow = selection[0].row();
    int selectedcol = selection[0].column();
    int selectedrows = selection[selection.count()-1].row() - selectedrow + 1;
    int selectedcols = selection[selection.count()-1].column() - selectedcol + 1;

    if (selection.count() > 1 &&
        (selectedrows != cells.count() || selectedcols != cells[0].count())) {

        // wrong size
        QMessageBox oops(QMessageBox::Critical, tr("Paste error"),
                         tr("Copy buffer and selected area are diffferent sizes."));
        oops.exec();
        return;
    }

    // overrun cols?
    if (selection.count() == 1 &&
        (selectedcol + cells[0].count()) > _model->columnCount(QModelIndex())) {
        QMessageBox oops(QMessageBox::Critical, tr("Paste error"),
                         tr("Copy buffer has more columns than available."));
        oops.exec();
        return;
    }

    // overrun rows?
    if (selection.count() == 1 &&
        (selectedrow + cells.count()) > _model->rowCount(QModelIndex()) ) {
        QMessageBox oops(QMessageBox::Critical, tr("Paste error"),
                         tr("Copy buffer has more rows than available."));
        oops.exec();
        return;
    }

    // go paste!
    _model->ride->command->startLUW("Paste XData Cells");

    for (int i=0; i<cells.count(); i++) {

        // just in case check boundary (i.e. truncate)
        if (selectedrow+i > _model->rowCount() ) break;
        for(int j=0; j<cells[i].count(); j++) {

            // just in case check boundary (i.e. truncate)
            if ((selectedcol+j > _model->columnCount()-1)) break;

            // set table
            setModelValue(selectedrow+i, selectedcol+j, cells[i][j]);
        }
    }
    _model->ride->command->endLUW();
}



