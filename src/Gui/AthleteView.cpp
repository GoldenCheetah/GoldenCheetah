#include "AthleteView.h"
#include "TabView.h"

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
static const int gl_progress_width = ROWHEIGHT / 2;

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
    // context and signals
    if (parent->context->athlete->cyclist == path) {
        context = parent->context;
        loadprogress = 100;
    } else {
        context = NULL;
        loadprogress = 0;
    }

    connect(parent->context->mainWindow, SIGNAL(openingAthlete(QString,Context*)), this, SLOT(opening(QString,Context*)));
    connect(parent->context, SIGNAL(athleteClose(QString,Context*)), this, SLOT(closing(QString,Context*)));

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

#if 0
    // ridecache raw
    if (loadprogress == 0) {
        QTime timer;
        timer.start();
        QFile rideDB(gcroot + "/" + path + "/cache/rideDB.json");
        if (rideDB.exists() && rideDB.open(QFile::ReadOnly)) {

            QByteArray contents = rideDB.readAll();
            rideDB.close();
            QJsonDocument json = QJsonDocument::fromJson(contents);
        }
        fprintf(stderr, "'%s' read rideDB took %d usecs\n", path.toStdString().c_str(), timer.elapsed()); fflush(stderr);
    }
#endif

}

void
AthleteCard::opening(QString name, Context*context)
{
    // are we being opened?
    if (name == path) {
        this->context = context;
        loadprogress = 100;
        connect(context,SIGNAL(loadProgress(QString,double)), this, SLOT(loadProgress(QString,double)));
        connect(context,SIGNAL(athleteClose(QString,Context*)), this, SLOT(closing(QString,Context*)));
    }

}

void
AthleteCard::closing(QString name, Context *context)
{
    if (name == path) {
        this->context = NULL;
        loadprogress = 0;
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

    // load status
    QRectF progressbar(0, geometry().height()-gl_progress_width, geometry().width() * (loadprogress/100), gl_progress_width);
    painter->fillRect(progressbar, QBrush(GColor(CPLOTMARKER)));
}
