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
static const int gl_avatar_width = 5;
static const int gl_progress_width = ROWHEIGHT/2;
static const int gl_button_height = ROWHEIGHT*1.5;
static const int gl_button_width = ROWHEIGHT*5;

AthleteView::AthleteView(Context *mainWindowContext)
    : ChartSpace(mainWindowContext, OverviewScope::ATHLETES, NULL), mainWindow_(mainWindowContext->mainWindow)
{
    // AthleteView will have a lifetime greater than the mainWindowContext which is only required to
    // construct chartspace, so to ensure we do not register signals, events or store this context its
    // set to null to ensure misuse is detected, and prevent unintended behaviour
    this->context = mainWindowContext = nullptr;

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
    connect(mainWindow_, &MainWindow::newAthlete, this, &AthleteView::newAthlete);
    // delete athlete
    connect(mainWindow_, &MainWindow::deletedAthlete, this, &AthleteView::deleteAthlete);
    // opening athlete
    connect(mainWindow_, &MainWindow::openingAthlete, this, &AthleteView::openingAthlete);
    // closing athlete
    connect(mainWindow_, &MainWindow::closingAthlete, this, &AthleteView::closingAthlete);
    // current athlete
    connect(mainWindow_, &MainWindow::currentAthlete, this, &AthleteView::currentAthlete);
}

void
AthleteView::newAthlete(QString name)
{
    // ignore non-athlete folders created by Qt or users
    AthleteDirectoryStructure athleteHome(QDir(gcroot + "/" + name));
    if (!athleteHome.upgradedDirectoriesHaveData()) return;

    // position the new athlete tile in the first available vacant position
    int col, row;
    bool posOccupied = true;

    for ( row = -1 ; posOccupied && (row < (gl_athletes_deep-1)); ) {
        row++;

        for ( col = -1 ; posOccupied && (col < (gl_athletes_per_row-1)); ) {
            col++;

            posOccupied = false;
            for (const ChartSpaceItem* item : allItems()) {

                if ((item->column == col) && (item->order == row)) {
                    posOccupied = true;
                    break;
                }
            }
        }
    }

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
AthleteView::openingAthlete(QString name, Context *context)
{
    foreach(ChartSpaceItem* item, allItems()) {
        if (item->name == name) {
            static_cast<AthleteCard*>(item)->openingAthlete(name, context);
        }
    }
}

void
AthleteView::closingAthlete(QString name, Context *context)
{
    foreach(ChartSpaceItem* item, allItems()) {
        if (item->name == name) {
            static_cast<AthleteCard*>(item)->closingAthlete(name, context);
        }    
    }

    if (openAthletes() == 1) {
        // find the selected one, and update it
        foreach(ChartSpaceItem* item, allItems()) {
            if (!static_cast<AthleteCard*>(item)->isAthleteClosed()) {

                // must be the selected item, so update it
                static_cast<AthleteCard*>(item)->setCurrentAthlete(true);
                break;
            }
        }
    }
}

uint32_t
AthleteView::openAthletes()
{
    uint32_t openAthletes=0;
    foreach(ChartSpaceItem* item, allItems()) {
        if (static_cast<AthleteCard*>(item)->isAthleteClosed() == false) openAthletes++;
    }
    return openAthletes;
}

void
AthleteView::currentAthlete(QString name)
{
    foreach(ChartSpaceItem* item, allItems()) {
        static_cast<AthleteCard*>(item)->setCurrentAthlete(item->name == name);
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
    ChartSpaceItem(parent, name),loadProgress_(0), context_(NULL), refresh_(false), actualActivities(0),
    plannedActivities(0), unsavedActivities(0), lastActivity(QDateTime()), currentAthlete_(false) 
{
    // no edit & config icon thanks
    setShowEdit(false);
    setShowConfig(false);

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
        static_cast<AthleteView*>(parent)->mainWindow_->backupAthlete(name);
    });

    // save all unsaved rides button
    saveAllUnsavedRidesButton = new Button(this, tr("Saved"));
    saveAllUnsavedRidesButton->setFont(parent->midfont);
    saveAllUnsavedRidesButton->setBkgdColor(QColor(Qt::darkRed));
    saveAllUnsavedRidesButton->setGeometry(geometry().width()-(gl_button_width+(2*ROWHEIGHT)), 4*ROWHEIGHT,
                        gl_button_width, gl_button_height);
    saveAllUnsavedRidesButton->hide();
    connect(saveAllUnsavedRidesButton, &Button::clicked, this, [this, parent] () {
        if (this->context_) static_cast<AthleteView*>(parent)->mainWindow_->saveAllUnsavedRides(this->context_);
    });

    // delete button
    deleteButton = new Button(this, tr("Delete..."));
    deleteButton->setFont(parent->midfont);
    deleteButton->setGeometry(ROWHEIGHT, geometry().height()-(gl_button_height+ROWHEIGHT),
                        gl_button_width, gl_button_height);
    connect(deleteButton, &Button::clicked, this, [parent, name] () { 
        static_cast<AthleteView*>(parent)->mainWindow_->deleteAthlete(name);
    });

    // open close button
    openCloseButton = new Button(this, tr("Open"));
    openCloseButton->setFont(parent->midfont);
    openCloseButton->setGeometry(geometry().width()-(gl_button_width+ROWHEIGHT), geometry().height()-(gl_button_height+ROWHEIGHT),
                        gl_button_width, gl_button_height);
    connect(openCloseButton, &Button::clicked, this, &AthleteCard::openCloseAthlete);

    refreshStats();
    update();
}

void
AthleteCard::refreshStats(RideItem * /* item = nullptr */ )
{
    actualActivities=0;
    plannedActivities=0;
    lastActivity=QDateTime();

    // need to fetch by looking at file names if no context_
    if (context_ == NULL) {

        unsavedActivities=0;

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

        int preUnsavedActivities = unsavedActivities;
        unsavedActivities=0;

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

        // update buttons related to unsave activities (if required)
        if (unsavedActivities != preUnsavedActivities) setCurrentAthlete(currentAthlete_);
    }
}

void
AthleteCard::setCurrentAthlete(bool status)
{
    currentAthlete_ = status;

    // refresh card & button background colours
    QColor cardBkgdColor = currentAthlete_ ? GCColor::selectedColor(GColor(CTOOLBAR)) : GColor(CCARDBACKGROUND);
    bgcolor = cardBkgdColor.name(); // set chartspace item background

    openCloseButton->setBkgdColor(cardBkgdColor);
    backupButton->setBkgdColor(cardBkgdColor);
    deleteButton->setBkgdColor(cardBkgdColor);

    if ((currentAthlete_) && (static_cast<AthleteView*>(parent)->openAthletes() == 1)) {  // current athlete must be loaded
        openCloseButton->setText((unsavedActivities != 0) ? tr("Shutdown...") : tr("Shutdown"));
    } else if (loadProgress_ == 0) {
        openCloseButton->setText(tr("Open"));
    } else if (loadProgress_ == 100) {
        openCloseButton->setText((unsavedActivities != 0) ? tr("Close...") : tr("Close"));
    }
}

void
AthleteCard::configChanged(qint32)
{
    // refresh card & button background colours
    QColor cardBkgdColor = currentAthlete_ ? GCColor::selectedColor(GColor(CTOOLBAR)) : GColor(CCARDBACKGROUND);
    bgcolor = cardBkgdColor.name(); // set chartspace item background

    openCloseButton->setBkgdColor(cardBkgdColor);
    deleteButton->setBkgdColor(cardBkgdColor);
    backupButton->setBkgdColor(cardBkgdColor);
}

void
AthleteCard::configAthlete()
{
    if (context_ == NULL) return;

    AthleteConfigDialog *dialog = new AthleteConfigDialog(context_->athlete->home->root(), context_);
    dialog->exec();
}

void
AthleteCard::openCloseAthlete()
{
    if (loadProgress_ == 100) {
        // athlete is open, so close the athlete
        if (static_cast<AthleteView*>(parent)->mainWindow_->closeAthleteTab(name)) {
            loadProgress_=0;
            update();
        }
        return;
    }

    if (loadProgress_ == 0) {
        // athlete is closed, so open the athlete
        if (static_cast<AthleteView*>(parent)->mainWindow_->openAthleteTab(name)) {
            loadProgress_=1;
            update();
        }
        return;
    }
}

void
AthleteCard::openingAthlete(QString, Context *context)
{
    // store new athlete context
    context_ = context;

    openCloseButton->setText(tr("Close"));
    openCloseButton->hide();
    deleteButton->hide();

    // register for athlete change events
    loadProgress_=99;
    connect(context_, &Context::loadProgress, this, &AthleteCard::loadProgress);
    connect(context_, &Context::loadDone, this, &AthleteCard::loadDone);

    // refresh updates
    connect(context_, &Context::refreshStart, this, &AthleteCard::refreshStart);
    connect(context_, &Context::refreshEnd, this, &AthleteCard::refreshEnd);
    connect(context_, &Context::refreshUpdate, this, &AthleteCard::refreshUpdate); // we might miss 1st one
}

// track refreshes
void AthleteCard::refreshStart() { refresh_ = true; update(); }
void AthleteCard::refreshEnd() { refresh_ = false; update(); }
void AthleteCard::refreshUpdate(QDate) { refresh_ = true; update();  }

void
AthleteCard::itemGeometryChanged()
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

void
AthleteCard::loadDone(QString name, Context *)
{
    // are we loaded?
    if (name == this->name) {

        setShowConfig(true);
        loadProgress_=100;

        if (!drag) openCloseButton->show();

        refreshStats();
        update();

        disconnect(context_, &Context::loadProgress, this, &AthleteCard::loadProgress);
        disconnect(context_, &Context::loadDone, this, &AthleteCard::loadDone);

        // watch activity changes
        registerRideEvents(true);
    }
}

void
AthleteCard::registerRideEvents(bool enable)
{
    if (enable) {
        connect(context_, &Context::rideAdded, this, &AthleteCard::refreshStats);
        connect(context_, &Context::rideDeleted, this, &AthleteCard::refreshStats);
        connect(context_, &Context::rideChanged, this, &AthleteCard::refreshStats);
        connect(context_, &Context::rideSaved, this, &AthleteCard::refreshStats);
    } else {
        disconnect(context_, &Context::rideAdded, this, &AthleteCard::refreshStats);
        disconnect(context_, &Context::rideDeleted, this, &AthleteCard::refreshStats);
        disconnect(context_, &Context::rideChanged, this, &AthleteCard::refreshStats);
        disconnect(context_, &Context::rideSaved, this, &AthleteCard::refreshStats);
    }
}

void
AthleteCard::dragChanged(bool drag)
{
    if (drag) {
        // drag started, hide all the buttons for the duration of the drag
        openCloseButton->hide();
        backupButton->hide();
        deleteButton->hide();
        saveAllUnsavedRidesButton->hide();
    } else {
        // drag finished, restore buttons depending on the card state.
        backupButton->show();
        openCloseButton->show();
        if ((loadProgress_ == 100) && (unsavedActivities != 0)) saveAllUnsavedRidesButton->show();
        if (loadProgress_ == 0) deleteButton->show();
    }
}

void
AthleteCard::closingAthlete(QString, Context *)
{
    registerRideEvents(false); // stop watching activity changes
    context_ = NULL; // MainWindow deletes the context when the athlete is closed
    setShowConfig(false);
    openCloseButton->setText(tr("Open"));
    deleteButton->show();
    saveAllUnsavedRidesButton->hide();
    unsavedActivities=0;
    loadProgress_=0;
    update();
}

void
AthleteCard::loadProgress(QString, double prog)
{
    loadProgress_ = prog > 100 ? 100 : prog;
    update();
}

void
AthleteCard::itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    // lets paint the athlete's avatar
    QRectF img(ROWHEIGHT,ROWHEIGHT*2,ROWHEIGHT* gl_avatar_width, ROWHEIGHT* gl_avatar_width);
    painter->drawImage(img, avatar);

    // the current athlete's background will be different, so need to invert
    // each color to get the correct foreground colour to maintain contrast.
    QColor tc = GCColor::invertColor(bgcolor);
    tc.setAlpha(200);
    painter->setPen(tc);
    painter->setFont(parent->midfont);

    QString message = (actualActivities != 0 || plannedActivities != 0) ? 
            tr("%1 activities, %2 planned").arg(actualActivities).arg(plannedActivities) : tr("No activities");
    QRectF rectf = QRectF(ROWHEIGHT,geometry().height()-(ROWHEIGHT*6), geometry().width()-(ROWHEIGHT*2), ROWHEIGHT*1.5);
    painter->drawText(rectf, message, Qt::AlignHCenter | Qt::AlignVCenter);

    if (lastActivity != QDateTime()) {
        QRectF rectf = QRectF(ROWHEIGHT,geometry().height()-(ROWHEIGHT*4.5), geometry().width()-(ROWHEIGHT*2), ROWHEIGHT*1.5);
        message = QString(tr("Last activity %1 days ago")).arg(lastActivity.daysTo(QDateTime::currentDateTime()));
        painter->drawText(rectf, message, Qt::AlignHCenter | Qt::AlignVCenter);
    }

    if ((context_) && (unsavedActivities != 0)) {
        saveAllUnsavedRidesButton->show();
        saveAllUnsavedRidesButton->setText(tr("Unsaved:")+QString::number(unsavedActivities));
    } else {
        saveAllUnsavedRidesButton->hide();
    }

    // load status
    QRectF progressbar(0, geometry().height()-gl_progress_width, geometry().width() * (loadProgress_/100), gl_progress_width);
    painter->fillRect(progressbar, QBrush(GColor(CPLOTMARKER)));

    // refresh status
    if (refresh_ && Context::isValid(context_)) {
        QRectF progressbar(0, geometry().height()-gl_progress_width, geometry().width() * (double(context_->athlete->rideCache->progress())/100), gl_progress_width);
        QColor over(Qt::white);
        over.setAlpha(128);
        painter->fillRect(progressbar, QBrush(over));
    }
}
