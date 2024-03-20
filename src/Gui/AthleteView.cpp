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

AthleteView::AthleteView(Context *context) : ChartSpace(context, OverviewScope::ATHLETES, NULL)
{
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ScopeBar_Athletes));

    // fixed width - 1200 col width, 70 margins + scrollbar
    setFixedZoom(50 + (gl_athletes_per_row*1200) + ((gl_athletes_per_row+1) * 70));

    connect(GlobalContext::context(), SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // lets look for athletes
    row=0; col=0;
    QStringListIterator i(QDir(gcroot).entryList(QDir::Dirs | QDir::NoDotAndDotDot));
    while (i.hasNext()) {

        QString name = i.next();

        newAthlete(name);
    }

    // set colors
    configChanged(0);

    // athlete config dialog...
    connect(this, SIGNAL(itemConfigRequested(ChartSpaceItem*)), this, SLOT(configItem(ChartSpaceItem*)));
    // new athlete
    connect(context->mainWindow, SIGNAL(newAthlete(QString)), this, SLOT(newAthlete(QString)));
    // delete athlete
    connect(context->mainWindow, SIGNAL(deletedAthlete(QString)), this, SLOT(deleteAthlete(QString)));
}

void
AthleteView::newAthlete(QString name)
{
    // ignore non-athlete folders created by Qt or users
    AthleteDirectoryStructure athleteHome(QDir(gcroot + "/" + name));
    if (!athleteHome.upgradedDirectoriesHaveData()) return;

    // add a card for each athlete
    AthleteCard *ath = new AthleteCard(this, name);
    addItem(row,col,1,gl_athletes_deep,ath);

    // we have 5 athletes per row
    if (++col >= gl_athletes_per_row) {
        col=0;
        row++;
    }
    // setup geometry
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
AthleteView::configChanged(qint32)
{
    // appearances
    setStyleSheet(AbstractView::ourStyleSheet());

}

void
AthleteView::configItem(ChartSpaceItem*item)
{
    AthleteCard *card = static_cast<AthleteCard*>(item);
    card->configAthlete();
}

AthleteCard::AthleteCard(ChartSpace *parent, QString path) : ChartSpaceItem(parent, path), path(path), refresh(false)
{
    // no config icon thanks
    setShowConfig(false);

    // avatar
    QRectF img(ROWHEIGHT,ROWHEIGHT*2,ROWHEIGHT* gl_avatar_width, ROWHEIGHT* gl_avatar_width);

    QString filename = gcroot + "/" + path + "/config/avatar.png";
    // athletes created using old versions may not have avatar
    QImage original = QImage(QFile(filename).exists() ?  filename : ":/images/noavatar.png");

    QPixmap canvas(img.width(), img.height());
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    painter.setBrush(original.scaled(img.width(), img.height()));
    painter.setPen(Qt::NoPen);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawEllipse(0, 0, img.width(), img.height());
    avatar = canvas.toImage();

    // open close button
    button = new Button(this, tr("Open"));
    button->setFont(parent->midfont);
    button->setGeometry(geometry().width()-(gl_button_width+ROWHEIGHT), geometry().height()-(gl_button_height+ROWHEIGHT),
                        gl_button_width, gl_button_height);
    connect(button, SIGNAL(clicked()), this, SLOT(clicked()));

    // context and signals
    if (parent->context->athlete->cyclist == path) {
        context = parent->context;
        loadprogress = 100;
        anchor=true;
        setShowConfig(true);
        button->setText(tr("Close"));
        button->hide();

        // watch metric updates
        connect(context, SIGNAL(refreshStart()), this, SLOT(refreshStart()));
        connect(context, SIGNAL(refreshEnd()), this, SLOT(refreshEnd()));
        connect(context, SIGNAL(refreshUpdate(QDate)), this, SLOT(refreshUpdate(QDate))); // we might miss 1st one

    } else {
        context = NULL;
        loadprogress = 0;
        anchor=false;
    }

    connect(parent->context->mainWindow, SIGNAL(openingAthlete(QString,Context*)), this, SLOT(opening(QString,Context*)));
    connect(parent->context, SIGNAL(athleteClose(QString,Context*)), this, SLOT(closing(QString,Context*)));

    // set stats to none
    count=0;
    last=QDateTime();

    // need to fetch by looking at file names, rideDB parse is too expensive here
    if (loadprogress == 0) {

        // unloaded athletes we just say when last activity was
        //QDate today = QDateTime::currentDateTime().date();
        QDir dir(gcroot + "/" + path + "/activities");
        QStringListIterator i(RideFileFactory::instance().listRideFiles(dir));
        while (i.hasNext()) {

            QString name = i.next();
            QDateTime date;

            if (RideFile::parseRideFileName(name, &date)) {

                //int ago= date.date().daysTo(today);
                if (last==QDateTime() || date > last) last = date;
                count++;
            }
        }

        // when was rideDB.json last updated- thats when the athlete was last viewed
        //QFileInfo fi(gcroot + "/" + path + "/cache/rideDB.json");
        //fprintf(stderr, "%s: last accessed %s\n", path.toStdString().c_str(), fi.lastModified().date().toString().toStdString().c_str());

    } else {

        // use ridecache
        refreshStats();
    }
}

void
AthleteCard::refreshStats()
{
    if (context == NULL) return;

    // last 90 days
    count=context->athlete->rideCache->rides().count();
    foreach(RideItem *item, context->athlete->rideCache->rides()) {

        // last activity date?
        if (last==QDateTime() || item->dateTime > last) last = item->dateTime;
    }
}

void
AthleteCard::configAthlete()
{
    AthleteConfigDialog *dialog = new AthleteConfigDialog(context->athlete->home->root(), context);
    dialog->exec();
}

void
AthleteCard::clicked()
{
    if (loadprogress==100) parent->context->mainWindow->closeAthleteTab(path);
    else parent->context->mainWindow->openAthleteTab(path);
}

void
AthleteCard::opening(QString name, Context*context)
{
    // are we being opened?
    if (name == path) {
        this->context = context;
        loadprogress = 100;
        button->setText(tr("Close"));
        button->hide();
        connect(context,SIGNAL(loadProgress(QString,double)), this, SLOT(loadProgress(QString,double)));
        connect(context,SIGNAL(loadDone(QString,Context*)), this, SLOT(loadDone(QString,Context*)));
        connect(context,SIGNAL(athleteClose(QString,Context*)), this, SLOT(closing(QString,Context*)));

        // refresh updates
        connect(context, SIGNAL(refreshStart()), this, SLOT(refreshStart()));
        connect(context, SIGNAL(refreshEnd()), this, SLOT(refreshEnd()));
        connect(context, SIGNAL(refreshUpdate(QDate)), this, SLOT(refreshUpdate(QDate))); // we might miss 1st one
    }
}

// track refreshes
void AthleteCard::refreshStart() { refresh = true; update(); }
void AthleteCard::refreshEnd() { refresh = false; update(); }
void AthleteCard::refreshUpdate(QDate) { refresh = true; update();  }

void AthleteCard::itemGeometryChanged()
{
    button->setGeometry(geometry().width()-(gl_button_width+ROWHEIGHT), geometry().height()-(gl_button_height+ROWHEIGHT),
                        gl_button_width, gl_button_height);
}

void AthleteCard::loadDone(QString name, Context *)
{
    if (this->name == name) {
        setShowConfig(true);
        loadprogress = 100;
        refreshStats();
        if (!anchor) button->show();
        update();
    }
}

void AthleteCard::dragChanged(bool drag)
{
    if (drag || (loadprogress != 100 && loadprogress != 0)) button->hide();
    else if (!anchor) button->show();
}

void
AthleteCard::closing(QString name, Context *)
{
    if (name == path) {
        setShowConfig(false);
        this->context = NULL;
        loadprogress = 0;
        button->setText(tr("Open"));
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
    // lets paint the image
    QRectF img(ROWHEIGHT,ROWHEIGHT*2,ROWHEIGHT* gl_avatar_width, ROWHEIGHT* gl_avatar_width);
    painter->drawImage(img, avatar);

    // last workout if nothing recent
    if (/*maxy == 0 && */last != QDateTime() || count == 0) {
        QRectF rectf = QRectF(ROWHEIGHT,geometry().height()-(ROWHEIGHT*5), geometry().width()-(ROWHEIGHT*2), ROWHEIGHT*1.5);
        QString message;
        if (count == 0) message = tr("No activities.");
        else message = QString(tr("Last workout %1 days ago")).arg(last.daysTo(QDateTime::currentDateTime()));
        painter->setFont(parent->midfont);
        painter->setPen(QColor(150,150,150));
        painter->drawText(rectf, message, Qt::AlignHCenter | Qt::AlignVCenter);
    }

    // load status
    QRectF progressbar(0, geometry().height()-gl_progress_width, geometry().width() * (loadprogress/100), gl_progress_width);
    painter->fillRect(progressbar, QBrush(GColor(GCol::PLOTMARKER)));

    // refresh status
    if (refresh && Context::isValid(context)) {
        QRectF progressbar(0, geometry().height()-gl_progress_width, geometry().width() * (double(context->athlete->rideCache->progress())/100), gl_progress_width);
        QColor over(Qt::white);
        over.setAlpha(128);
        painter->fillRect(progressbar, QBrush(over));
    }
}
