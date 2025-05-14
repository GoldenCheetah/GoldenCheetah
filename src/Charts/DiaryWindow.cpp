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


#include "DiaryWindow.h"
#include "RideMetadata.h"
#include "RideCache.h"
#include "RideCacheModel.h"
#include "Athlete.h"
#include "Context.h"
#include "AbstractView.h"
#include "HelpWhatsThis.h"

DiaryWindow::DiaryWindow(Context *context) :
    GcChartWindow(context), ride(nullptr), context(context), active(false)
{
    setControls(NULL);

    // get config
    fieldDefinitions = GlobalContext::context()->rideMetadata->getFields();

    QVBoxLayout *vlayout = new QVBoxLayout;
    setChartLayout(vlayout);

    // controls
    QHBoxLayout *controls = new QHBoxLayout;
    QFont bold;
    bold.setPointSize(14);
    bold.setWeight(QFont::Bold);
    title = new QDateEdit(this);
    title->setDisplayFormat("MMMM yyyy");
    title->setCurrentSection(QDateTimeEdit::YearSection);
    title->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    title->setFont(bold);

    QIcon prevIcon(":images/toolbar/back_alt.png");
    QIcon nextIcon(":images/toolbar/forward_alt.png");
    next = new QPushButton(nextIcon, "", this);
    prev = new QPushButton(prevIcon, "", this);
#ifdef Q_OS_MAC
    next->setFlat(true);
    prev->setFlat(true);
#endif

    controls->addWidget(prev);
    controls->addWidget(next);
    controls->addStretch();
    controls->addWidget(title, Qt::AlignCenter | Qt::AlignVCenter);
    controls->addStretch();

    vlayout->addLayout(controls);

    // monthly view
    calendarModel = new GcCalendarModel(this, &fieldDefinitions, context);
    calendarModel->setSourceModel(context->athlete->rideCache->model());

    monthlyView = new QTableView(this);
    monthlyView->setItemDelegate(new GcCalendarDelegate);
    monthlyView->setModel(calendarModel);
    monthlyView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    monthlyView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    monthlyView->verticalHeader()->hide();
    monthlyView->viewport()->installEventFilter(this);
    monthlyView->setGridStyle(Qt::DotLine);
    monthlyView->setFrameStyle(QFrame::NoFrame);

    HelpWhatsThis *helpView = new HelpWhatsThis(monthlyView);
    monthlyView->setWhatsThis(helpView->getWhatsThisText(HelpWhatsThis::ChartDiary_Calendar));

    allViews = new QStackedWidget(this);
    allViews->addWidget(monthlyView);
    allViews->setCurrentIndex(0);

    vlayout->addWidget(allViews);

    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(context, SIGNAL(filterChanged()), this, SLOT(rideSelected()));
    connect(context, SIGNAL(homeFilterChanged()), this, SLOT(rideSelected()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(title, SIGNAL(dateChanged(const QDate)), this, SLOT(dateChanged(const QDate)));
    connect(next, SIGNAL(clicked()), this, SLOT(nextClicked()));
    connect(prev, SIGNAL(clicked()), this, SLOT(prevClicked()));

    // get colors sorted
    configChanged(CONFIG_FIELDS | CONFIG_APPEARANCE);
}

void
DiaryWindow::configChanged(qint32)
{
    // get config
    fieldDefinitions = GlobalContext::context()->rideMetadata->getFields();

    // change colors to reflect preferences
    setProperty("color", GColor(CPLOTBACKGROUND));

    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setBrush(QPalette::Base, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::Normal, QPalette::Window, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    setPalette(palette);
    monthlyView->setPalette(palette);
    monthlyView->setStyleSheet(QString("QTableView QTableCornerButton::section { background-color: %1; color: %2; border: %1 }")
                    .arg(GColor(CPLOTBACKGROUND).name())
                    .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name()));
    monthlyView->horizontalHeader()->setStyleSheet(QString("QHeaderView::section { background-color: %1; color: %2; border: 0px }")
                    .arg(GColor(CPLOTBACKGROUND).name())
                    .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name()));
    monthlyView->verticalHeader()->setStyleSheet(QString("QHeaderView::section { background-color: %1; color: %2; border: 0px }")
                    .arg(GColor(CPLOTBACKGROUND).name())
                    .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name()));
#ifndef Q_OS_MAC
    monthlyView->verticalScrollBar()->setStyleSheet(AbstractView::ourStyleSheet());
    monthlyView->horizontalScrollBar()->setStyleSheet(AbstractView::ourStyleSheet());
#endif
    title->setStyleSheet(QString("background: %1; color: %2;").arg(GColor(CPLOTBACKGROUND).name())
                                                              .arg(GColor(CPLOTMARKER).name()));
}

void
DiaryWindow::setDefaultView(int view)
{
    appsettings->setCValue(context->athlete->cyclist, GC_DIARY_VIEW, view);
}
void
DiaryWindow::rideSelected()
{
    // ignore if already active or the selected activity has not changed
    if (active || ride == myRideItem) {
        return;
    }

    ride = myRideItem;

    // ignore if not active or null ride
    if (!ride) {
        return;
    }

    // set the date range to put the current ride in view...
    QDate when = ride->dateTime.date();
    title->setDate(when);

    // monthly view updates
    calendarModel->setStale();
    calendarModel->setMonth(when.month(), when.year());

    when = when.addDays(Qt::Monday - when.dayOfWeek());

    repaint();
    next->show();
    prev->show();
}

void
DiaryWindow::dateChanged(const QDate &date)
{
    calendarModel->setMonth(date.month(), date.year());
}

void
DiaryWindow::prevClicked()
{
    int month = calendarModel->getMonth();
    int year = calendarModel->getYear();
    QDate when = QDate(year, month, 1).addDays(-1);
    calendarModel->setMonth(when.month(), when.year());
    title->setDate(when);
}

void
DiaryWindow::nextClicked()
{
    int month = calendarModel->getMonth();
    int year = calendarModel->getYear();
    QDate when = QDate(year, month, 1).addMonths(1);
    calendarModel->setMonth(when.month(), when.year());
    title->setDate(when);
}

bool
DiaryWindow::eventFilter(QObject *object, QEvent *e)
{

    switch (e->type()) {
    case QEvent::MouseButtonPress:
        {
        // Get a list of rides for the point clicked
        QModelIndex index = monthlyView->indexAt(static_cast<QMouseEvent*>(e)->pos());
        QStringList files = calendarModel->data(index, GcCalendarModel::FilenamesRole).toStringList();

        // worry about where we clicked in the cell
        int y = static_cast<QMouseEvent*>(e)->pos().y();
        QRect c = monthlyView->visualRect(index);

        // clicked on heading
        if (y <= (c.y()+15)) return true; // clicked on heading 

        // clicked on cell contents
        if (files.count() == 1) {
            if (files[0] == "calendar") ; // handle planned rides
            else context->athlete->selectRideFile(QFileInfo(files[0]).fileName());

        } else if (files.count()) {

            // which ride?
            int h = (c.height()-15) / files.count();
            int i;
            for(i=files.count()-1; i>=0; i--) if (y > (c.y()+15+(h*i))) break;

            if (files[i] == "calendar") ; // handle planned rides
            else context->athlete->selectRideFile(QFileInfo(files[i]).fileName());
        }

        // force a repaint 
        calendarModel->setStale();
        calendarModel->setMonth(calendarModel->getMonth(), calendarModel->getYear());
        return true;
        }
        break;
    // ignore click, doubleclick
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    case QEvent::MouseButtonDblClick:
        return true;
    default:
        return QObject::eventFilter(object, e);
    }
    return true;
}
