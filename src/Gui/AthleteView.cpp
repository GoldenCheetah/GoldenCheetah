#include "AthleteView.h"
#include "TabView.h"
#include "JsonRideFile.h" // for DATETIME_FORMAT

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

AthleteView::AthleteView(Context *context) : ChartSpace(context, OverviewScope::ATHLETES)
{
    // fixed width - 1200 col width, 70 margins + scrollbar
    setFixedZoom(50 + (gl_athletes_per_row*1200) + ((gl_athletes_per_row+1) * 70));

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // lets look for athletes
    int row=0, col=0;
    QStringListIterator i(QDir(gcroot).entryList(QDir::Dirs | QDir::NoDotAndDotDot));
    while (i.hasNext()) {

        QString name = i.next();

        // if there is no avatar its not a real folder, even a blank athlete
        // will get the default avatar. This is to avoid having to work out
        // what all the random folders are that Qt creates on Windows when
        // using QtWebEngine, or Python or R or created by user scripts
        if (!QFile(gcroot + "/" + name + "/config/avatar.png").exists()) continue;

        // add a card for each athlete
        AthleteCard *ath = new AthleteCard(this, name);
        addItem(row,col,gl_athletes_deep,ath);

        // we have 3 athletes per row
        if (++col >= gl_athletes_per_row) {
            col=0;
            row++;
        }
    }

    // set colors
    configChanged(0);

    // setup geometry
    updateGeometry();
}

void
AthleteView::configChanged(qint32)
{
    // appearances
    setStyleSheet(TabView::ourStyleSheet());

}

AthleteCard::AthleteCard(ChartSpace *parent, QString path) : ChartSpaceItem(parent, path), path(path)
{
    // no config icon thanks
    setShowConfig(false);

    // avatar
    QRectF img(ROWHEIGHT,ROWHEIGHT*2,ROWHEIGHT* gl_avatar_width, ROWHEIGHT* gl_avatar_width);

    QImage original = QImage(gcroot + "/" + path + "/config/avatar.png").scaled(img.width(), img.height());
    QPixmap canvas(img.width(), img.height());
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    painter.setBrush(original);
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
        button->setText("Close");
        button->hide();
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
        QDate today = QDateTime::currentDateTime().date();
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
AthleteCard::clicked()
{
    if (loadprogress==100) parent->context->mainWindow->closeTab(path);
    else parent->context->mainWindow->openTab(path);
}

void
AthleteCard::opening(QString name, Context*context)
{
    // are we being opened?
    if (name == path) {
        this->context = context;
        loadprogress = 100;
        button->setText("Close");
        button->hide();
        connect(context,SIGNAL(loadProgress(QString,double)), this, SLOT(loadProgress(QString,double)));
        connect(context,SIGNAL(loadDone(QString,Context*)), this, SLOT(loadDone(QString,Context*)));
        connect(context,SIGNAL(athleteClose(QString,Context*)), this, SLOT(closing(QString,Context*)));
    }

}

void AthleteCard::itemGeometryChanged()
{
    button->setGeometry(geometry().width()-(gl_button_width+ROWHEIGHT), geometry().height()-(gl_button_height+ROWHEIGHT),
                        gl_button_width, gl_button_height);
}

void AthleteCard::loadDone(QString name, Context *)
{
    if (this->name == name) {
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
        this->context = NULL;
        loadprogress = 0;
        button->setText("Open");
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
        if (count == 0) message = "No activities.";
        else message = QString("Last workout %1 days ago").arg(last.daysTo(QDateTime::currentDateTime()));
        painter->setFont(parent->midfont);
        painter->setPen(QColor(150,150,150));
        painter->drawText(rectf, message, Qt::AlignHCenter | Qt::AlignVCenter);
    }


    // load status
    QRectF progressbar(0, geometry().height()-gl_progress_width, geometry().width() * (loadprogress/100), gl_progress_width);
    painter->fillRect(progressbar, QBrush(GColor(CPLOTMARKER)));
}
