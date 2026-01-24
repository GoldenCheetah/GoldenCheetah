/*
 * Copyright (c) 2020 Mark Liversedge <liversedge@gmail.com>
 * Additional functionality Copyright (c) 2026 Paul Johnson (paulj49457@gmail.com)
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

#include "AthleteView.h"
#include "AthleteConfigDialog.h"
#include "AbstractView.h"
#include "JsonRideFile.h" // for DATETIME_FORMAT
#include "HelpWhatsThis.h"

// athlete card
static bool _registerItems()
{
    // get the factory
    ChartSpaceItemRegistry &registry = ChartSpaceItemRegistry::instance();
    registry.addItem(OverviewItemType::ATHLETE,    QObject::tr("Athlete"),    QObject::tr("Athlete Card"),                       OverviewScope::ATHLETES,                       AthleteCard::create);

    return true;
}
static bool registered = _registerItems();

static const int gl_athletes_per_row = 5;
static const int gl_athletes_deep = 15;
static const float gl_locked_width = 1.5;
static const int gl_avatar_width = 5;
static const int gl_progress_width = ROWHEIGHT/2;
static const int gl_button_height = ROWHEIGHT*1.5;
static const int gl_button_width = ROWHEIGHT*5;

AthleteView::AthleteView(Context *context) : ChartSpace(context, OverviewScope::ATHLETES, NULL)
{
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ScopeBar_Athletes));

    // fixed width - 1200 col width, 70 margins + scrollbar
    setFixedZoom(50 + (gl_athletes_per_row*1200) + ((gl_athletes_per_row+1) * 70));

    connect(GlobalContext::context(), &GlobalContext::configChanged, this, &AthleteView::configChanged);

    // lets look for athletes
    QStringListIterator i(QDir(gcroot).entryList(QDir::Dirs | QDir::NoDotAndDotDot));
    while (i.hasNext()) {

        QString name = i.next();

        newAthlete(name);
    }

    // set colors
    configChanged(0);

    // athlete config dialog...
    connect(this, &ChartSpace::itemConfigRequested, this, &AthleteView::configItem);
    // new athlete
    connect(context->mainWindow, &MainWindow::newAthlete, this, &AthleteView::newAthlete);
    // delete athlete
    connect(context->mainWindow, &MainWindow::deletedAthlete, this, &AthleteView::deleteAthlete);
}

void
AthleteView::newAthlete(QString name)
{
    // ignore non-athlete folders created by Qt or users
    AthleteDirectoryStructure athleteHome(QDir(gcroot + "/" + name));
    if (!athleteHome.upgradedDirectoriesHaveData()) return;

    // determine tile position, good for initial load,
    // best approx once tiles have been dragged.
    int row = allItems().size() / gl_athletes_per_row;
    int col = allItems().size() % gl_athletes_per_row;

    // add a card for each athlete
    AthleteCard *ath = new AthleteCard(this, name);
    addItem(row,col,1,gl_athletes_deep,ath);

    // setup geometry
    updateGeometry();
    // sometimes needs another go to prevent overlapping tiles
    updateGeometry();
}

void
AthleteView::deleteAthlete(QString name)
{
    foreach(ChartSpaceItem* item, allItems()) {
        if (item->name == name) {
            removeItem(item);
            break;
        }
    }
}

void
AthleteView::currentAthlete(const QString& name)
{
    foreach(ChartSpaceItem* item, allItems()) {
        static_cast<AthleteCard*>(item)->currentAthlete(item->name == name);
    }
}

void
AthleteView::configChanged(qint32)
{
    // appearances
    setStyleSheet(AbstractView::ourStyleSheet());
}

void
AthleteView::configItem(ChartSpaceItem *item, QPoint)
{
    AthleteCard *card = static_cast<AthleteCard*>(item);
    card->configAthlete();
}


AthleteCard::AthleteCard(ChartSpace *parent, QString name) :
    ChartSpaceItem(parent, name), refresh(false), actualActivities(0),
    plannedActivities(0), unsavedActivities(0), lastActivity(QDateTime()),
    anchor(parent->context->athlete->cyclist == name) 
{
    // no edit icon thanks
    setShowEdit(false);

    // anchor's locked image
    locked = QImage(":/images/toolbar/passwords.png");

    // avatar
    QRectF img(ROWHEIGHT,ROWHEIGHT*2,ROWHEIGHT* gl_avatar_width, ROWHEIGHT* gl_avatar_width);

    QString filename = gcroot + "/" + name + "/config/avatar.png";
    // athletes created using old versions may not have avatar
    QImage original = QImage(QFile(filename).exists() ?  filename : ":/images/noavatar.png");

    QPixmap canvas(img.width(), img.height());
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    painter.setBrush(original.scaled(img.width(), img.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    painter.setPen(Qt::NoPen);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawEllipse(0, 0, img.width(), img.height());
    avatar = canvas.toImage();

    // backup button
    backupButton = new Button(this, tr("Backup..."));
    backupButton->setFont(parent->midfont);
    backupButton->setGeometry(geometry().width()-(gl_button_width+(2*ROWHEIGHT)), 2*ROWHEIGHT,
                        gl_button_width, gl_button_height);
    connect(backupButton, &Button::clicked, this, [parent, name] () {
        parent->context->mainWindow->backupAthlete(name);
    });

    // save all unsaved rides button
    saveAllUnsavedRidesButton = new Button(this, tr("Saved"));
    saveAllUnsavedRidesButton->setFont(parent->midfont);
    saveAllUnsavedRidesButton->setGeometry(geometry().width()-(gl_button_width+(2*ROWHEIGHT)), 4*ROWHEIGHT,
                        gl_button_width, gl_button_height);
    connect(saveAllUnsavedRidesButton, &Button::clicked, this, [parent, this] () {
        parent->context->mainWindow->saveAllUnsavedRides(this->context_);
    });

    // delete button
    deleteButton = new Button(this, tr("Delete..."));
    deleteButton->setFont(parent->midfont);
    deleteButton->setGeometry(ROWHEIGHT, geometry().height()-(gl_button_height+ROWHEIGHT),
                        gl_button_width, gl_button_height);
    connect(deleteButton, &Button::clicked, this, [parent, name] () { 
        parent->context->mainWindow->deleteAthlete(name);
    });

    // open close button
    openCloseButton = new Button(this, tr("Open"));
    openCloseButton->setFont(parent->midfont);
    openCloseButton->setGeometry(geometry().width()-(gl_button_width+ROWHEIGHT), geometry().height()-(gl_button_height+ROWHEIGHT),
                        gl_button_width, gl_button_height);
    connect(openCloseButton, &Button::clicked, this, &AthleteCard::openCloseAthlete);

    // context and signals for the anchor athlete
    // note: opening() and loadDone() are never called for the anchor athlete
    if (anchor) {
        context_ = parent->context;
        loadprogress = 100;
        openCloseButton->setText(tr("Close"));
        openCloseButton->hide();
        deleteButton->hide();

        // watch metric updates
        connect(context_, &Context::refreshStart, this, &AthleteCard::refreshStart);
        connect(context_, &Context::refreshEnd, this, &AthleteCard::refreshEnd);
        connect(context_, &Context::refreshUpdate, this, &AthleteCard::refreshUpdate); // we might miss 1st one

        // watch activity changes
        connect(context_, &Context::rideAdded, this, &AthleteCard::refreshStats);
        connect(context_, &Context::rideDeleted, this, &AthleteCard::refreshStats);
        connect(context_, &Context::rideChanged, this, &AthleteCard::refreshStats);
        connect(context_, &Context::rideSaved, this, &AthleteCard::refreshStats);

    } else {
        context_ = NULL;
        loadprogress = 0;
        saveAllUnsavedRidesButton->hide();
    }

    setShowConfig(anchor);
    currentAthlete(anchor);

    connect(parent->context->mainWindow, &MainWindow::openingAthlete, this, &AthleteCard::opening);
    connect(parent->context, &Context::athleteClose, this, &AthleteCard::closing);

    // need to fetch by looking at file names, rideDB parse is too expensive here
    if (loadprogress == 0) {

        // unloaded athletes we just say when last activity was
        QDir dir(gcroot + "/" + name + "/activities");
        QStringList activityList = RideFileFactory::instance().listRideFiles(dir);
        actualActivities = activityList.size();
        for (const QString& filename : activityList) {

            static QDateTime date;
            if (RideFile::parseRideFileName(filename, &date)) {

                // find last actual activity date
                if (lastActivity==QDateTime() || date > lastActivity) lastActivity = date;
            }
        }

        dir = gcroot + "/" + name + "/planned";
        plannedActivities = RideFileFactory::instance().listRideFiles(dir).size();

    } else {

        // use ridecache
        refreshStats();
    }
    update();
}

void
AthleteCard::refreshStats(RideItem * /* item = nullptr */ )
{
    if (context_ == NULL) return;

    unsavedActivities=0;
    actualActivities=0;
    plannedActivities=0;
    lastActivity=QDateTime();

    foreach(RideItem *item, context_->athlete->rideCache->rides()) {

        if (item->planned) {
            plannedActivities++;
        } else {
            actualActivities++;
            // last actual activity date?
            if (lastActivity==QDateTime() || item->dateTime > lastActivity) lastActivity = item->dateTime;
        }
        if (item->isDirty()) unsavedActivities++;
    }
}

void
AthleteCard::currentAthlete(bool status)
{
    // update card & button background colours
    selectedAthlete = status;
    QColor cardBkgdColor = selectedAthlete ? GCColor::selectedColor(GColor(CTOOLBAR)) : GColor(CCARDBACKGROUND);
    bgcolor = cardBkgdColor.name();

    if (loadprogress != 0) {
        openCloseButton->setBkgdColor(cardBkgdColor);
        backupButton->setBkgdColor(cardBkgdColor);
    }
    update();
}

void
AthleteCard::configChanged(qint32)
{
    // refresh card & button background colours
    QColor cardBkgdColor = selectedAthlete ? GCColor::selectedColor(GColor(CTOOLBAR)) : GColor(CCARDBACKGROUND);
    bgcolor = cardBkgdColor.name();
    openCloseButton->setBkgdColor(cardBkgdColor);
    deleteButton->setBkgdColor(cardBkgdColor);
    backupButton->setBkgdColor(cardBkgdColor);
}

void
AthleteCard::configAthlete()
{
    AthleteConfigDialog *dialog = new AthleteConfigDialog(context_->athlete->home->root(), context_);
    dialog->exec();
}

void
AthleteCard::openCloseAthlete()
{
    if (loadprogress==100) {
        // closing athlete
        deleteButton->show();
        saveAllUnsavedRidesButton->hide();
        parent->context->mainWindow->closeAthleteTab(name);
    } else {
        // opening athlete
        deleteButton->hide();
        saveAllUnsavedRidesButton->show();
        parent->context->mainWindow->openAthleteTab(name);
    }
}

void
AthleteCard::opening(QString name, Context *context)
{
    // are we being opened?
    if (name == this->name) {
        context_ = context;
        loadprogress = 100;
        openCloseButton->setText(tr("Close"));
        openCloseButton->hide();
        deleteButton->hide();
        saveAllUnsavedRidesButton->hide();

        connect(context_, &Context::loadProgress, this, &AthleteCard::loadProgress);
        connect(context_, &Context::loadDone, this, &AthleteCard::loadDone);
        connect(context_, &Context::athleteClose, this, &AthleteCard::closing);

        // refresh updates
        connect(context_, &Context::refreshStart, this, &AthleteCard::refreshStart);
        connect(context_, &Context::refreshEnd, this, &AthleteCard::refreshEnd);
        connect(context_, &Context::refreshUpdate, this, &AthleteCard::refreshUpdate); // we might miss 1st one
    }
}

// track refreshes
void AthleteCard::refreshStart() { refresh = true; update(); }
void AthleteCard::refreshEnd() { refresh = false; update(); }
void AthleteCard::refreshUpdate(QDate) { refresh = true; update();  }

void AthleteCard::itemGeometryChanged()
{
    openCloseButton->setGeometry(geometry().width()-(gl_button_width+ROWHEIGHT), geometry().height()-(gl_button_height+ROWHEIGHT),
                        gl_button_width, gl_button_height);

    deleteButton->setGeometry(ROWHEIGHT, geometry().height()-(gl_button_height+ROWHEIGHT),
                        gl_button_width, gl_button_height);

    backupButton->setGeometry(geometry().width()-(gl_button_width+(2*ROWHEIGHT)), 2*ROWHEIGHT,
                        gl_button_width, gl_button_height);

    saveAllUnsavedRidesButton->setGeometry(geometry().width()-(gl_button_width+(2*ROWHEIGHT)), 4*ROWHEIGHT,
                        gl_button_width, gl_button_height);
}

void AthleteCard::loadDone(QString name, Context *)
{
    // are we loaded?
    if (name == this->name) {

        // watch activity changes
        connect(context_, &Context::rideAdded, this, &AthleteCard::refreshStats);
        connect(context_, &Context::rideDeleted, this, &AthleteCard::refreshStats);
        connect(context_, &Context::rideChanged, this, &AthleteCard::refreshStats);
        connect(context_, &Context::rideSaved, this, &AthleteCard::refreshStats);

        setShowConfig(true);
        loadprogress = 100;
        if (!anchor) {
            openCloseButton->show();
            saveAllUnsavedRidesButton->show();
        }
        refreshStats();
        update();
    }
}

void AthleteCard::dragChanged(bool drag)
{
    if (drag || (loadprogress != 100 && loadprogress != 0)) {
        // drag started
        openCloseButton->hide();
        backupButton->hide();
        deleteButton->hide();
        saveAllUnsavedRidesButton->hide();
    } else {
        // drag finished
        backupButton->show();
        if (loadprogress != 0) saveAllUnsavedRidesButton->show();
        if (!anchor) {
            openCloseButton->show();
            if (loadprogress == 0) deleteButton->show();
        }
    }
}

void
AthleteCard::closing(QString name, Context *)
{
    // are we being closed?
    if (name == this->name) {
        setShowConfig(false);
        context_ = NULL;
        loadprogress = 0;
        openCloseButton->setText(tr("Open"));
        deleteButton->show();
        saveAllUnsavedRidesButton->hide();
        update();
    }
}

void
AthleteCard::loadProgress(QString, double prog)
{
    loadprogress = prog > 100 ? 100 : prog;
    update();
}

void
AthleteCard::itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    // lets paint the athlete's avatar
    QRectF img(ROWHEIGHT,ROWHEIGHT*2,ROWHEIGHT* gl_avatar_width, ROWHEIGHT* gl_avatar_width);
    painter->drawImage(img, avatar);

    // paint the anchor athlete's lock symbol
    if (anchor) {
        QRectF lockedImg(((geometry().width()/2)-(ROWHEIGHT* gl_locked_width/2)),
                        geometry().height()-(ROWHEIGHT*2.5), ROWHEIGHT* gl_locked_width, ROWHEIGHT* gl_locked_width);
        painter->drawImage(lockedImg, locked);
    }

    painter->setFont(parent->midfont);
    painter->setPen(QColor(150,150,150));

    QString message = (actualActivities != 0 || plannedActivities != 0) ? 
            tr("%1 workouts, %2 planned").arg(actualActivities).arg(plannedActivities) : tr("No Workouts");
    QRectF rectf = QRectF(ROWHEIGHT,geometry().height()-(ROWHEIGHT*6), geometry().width()-(ROWHEIGHT*2), ROWHEIGHT*1.5);
    painter->drawText(rectf, message, Qt::AlignHCenter | Qt::AlignVCenter);

    if (lastActivity != QDateTime()) {
        QRectF rectf = QRectF(ROWHEIGHT,geometry().height()-(ROWHEIGHT*4.5), geometry().width()-(ROWHEIGHT*2), ROWHEIGHT*1.5);
        message = QString(tr("Last workout %1 days ago")).arg(lastActivity.daysTo(QDateTime::currentDateTime()));
        painter->drawText(rectf, message, Qt::AlignHCenter | Qt::AlignVCenter);
    }

    if (unsavedActivities) {
        saveAllUnsavedRidesButton->setText(tr("Unsaved:")+QString::number(unsavedActivities));
        saveAllUnsavedRidesButton->setBkgdColor(QColor(Qt::darkRed));
    } else {
        saveAllUnsavedRidesButton->setText(tr("Saved"));
        saveAllUnsavedRidesButton->setBkgdColor(QColor(Qt::darkGreen));
    }

    // load status
    QRectF progressbar(0, geometry().height()-gl_progress_width, geometry().width() * (loadprogress/100), gl_progress_width);
    painter->fillRect(progressbar, QBrush(GColor(CPLOTMARKER)));

    // refresh status
    if (refresh && Context::isValid(context_)) {
        QRectF progressbar(0, geometry().height()-gl_progress_width, geometry().width() * (double(context_->athlete->rideCache->progress())/100), gl_progress_width);
        QColor over(Qt::white);
        over.setAlpha(128);
        painter->fillRect(progressbar, QBrush(over));
    }
}
