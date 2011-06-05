#include <QCalendarWidget>
#include <QMultiMap>
#include <QPainter>
#include <QObject>
#include <QDate>
#include <QAbstractItemView>
#include <QSize>
#include <QTextCharFormat>
#include <QPen>

#include "RideItem.h"
#include "RideMetadata.h"
#include "RideCalendar.h"
#include "MainWindow.h"
#include "Settings.h"

#include <algorithm>

RideCalendar::RideCalendar(MainWindow *parent) :
    QCalendarWidget(parent), mainWindow(parent)
{
    defaultColor = Qt::white;
    // make font 2 point sizes smaller that the default
    QFont font;
    font.setPointSize(font.pointSize()-2);
    setFont(font);
    this->setFirstDayOfWeek(Qt::Monday);

    // setup workout codes
    configUpdate();

    connect(mainWindow, SIGNAL(configChanged()), this, SLOT(configUpdate()));
};

void
RideCalendar::configUpdate()
{
    // clear existing
    workoutCodes.clear();

    // setup the keyword/color combinations from config settings
    foreach (KeywordDefinition keyword, mainWindow->rideMetadata()->getKeywords()) {
        if (keyword.name == "Default")
            defaultColor = keyword.color;
        else {
            addWorkoutCode(keyword.name, keyword.color);

            // alternative texts in notes
            foreach (QString token, keyword.tokens) {
                addWorkoutCode(token, keyword.color);
            }
        }
    }

    // force a repaint
    setSelectedDate(selectedDate());
}

struct RideIter
{
    typedef std::random_access_iterator_tag iterator_category;
    typedef const RideItem *value_type;
    typedef int difference_type;
    typedef value_type& reference;
    typedef value_type* pointer;

    MainWindow *mainWindow;
    int i;

    RideIter() : mainWindow(NULL), i(0) {}
    RideIter(MainWindow *mainWindow, int i = 0) : mainWindow(mainWindow), i(i) {}
    const RideItem *operator*() {
        assert(mainWindow);
        assert(i < mainWindow->allRideItems()->childCount());
        return dynamic_cast<const RideItem*>(mainWindow->allRideItems()->child(i));
    }
    RideIter operator++() { /* preincrement */ ++i; return *this; }
    RideIter operator++(int) { RideIter result(mainWindow, i); ++i; return result; }
    const RideIter &operator+=(int j) { i += j; return *this; }
};

bool operator==(const RideIter &a, const RideIter &b) { return a.i == b.i; }
bool operator!=(const RideIter &a, const RideIter &b) { return a.i != b.i; }
int operator-(const RideIter &a, const RideIter &b) { return a.i - b.i; }

struct RideItemDateLessThan
{
    bool operator()(const RideItem *a, const RideItem *b) { return a->dateTime < b->dateTime; }
};

struct RideItemDateGreaterThan
{
    bool operator()(const RideItem *a, const RideItem *b) { return a->dateTime > b->dateTime; }
};

void RideCalendar::paintCell(QPainter *painter, const QRect &rect, const QDate &date) const
{
    QMultiMap<QDateTime, const RideItem*> ridesToday;
    RideIter begin(mainWindow, 0);
    RideIter end(mainWindow, mainWindow->allRideItems()->childCount());

    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    bool ascending = settings->value(GC_ALLRIDES_ASCENDING, Qt::Checked).toInt() > 0;
    RideIter i;
    if (ascending) {
        RideItemDateLessThan comp;
        RideItem search(0, "", "", QDateTime(date), NULL, NULL, "", NULL);
        i = std::lower_bound(begin, end, &search, comp);
    }
    else {
        RideItemDateGreaterThan comp;
        RideItem search(0, "", "", QDateTime(date.addDays(1)), NULL, NULL, "", NULL);
        i = std::upper_bound(begin, end, &search, comp);
    }

    while (i != end) {
        const RideItem *ride = *i;
        if (ride->dateTime.date() != date)
            break;
        ridesToday.insert(ride->dateTime, ride);
        ++i;
    }

    if (ridesToday.isEmpty()) {
        QCalendarWidget::paintCell(painter, rect, date);
        return;
    }
    painter->save();

    /*
     * Fill in the text and color to some default
     */
    QRect textRect;
    int number = ridesToday.size();
    float count = 1;
    int startY, stopY;
    QPen pen(Qt::SolidLine);
    painter->setPen(pen);
    pen.setCapStyle(Qt::SquareCap);
    QFont font = painter->font();
    font.setPointSize(font.pointSize() - 2);
    painter->setFont(font);

    /*
     *  Loop over all the rides for today, and record colour and text.
     */
    foreach (const RideItem *ride, ridesToday.values()) {
        QString notesPath = home.absolutePath() + "/" + ride->notesFileName;
        QFile notesFile(notesPath);
        QColor color = defaultColor;
        QString line(tr("Ride"));
        QString code;
        if (notesFile.exists()) {
            if (notesFile.open(QFile::ReadOnly | QFile::Text)) {
                QTextStream in(&notesFile);
                line = in.readLine();
                notesFile.close();
                foreach(code, workoutCodes.keys()) {
                    if (line.contains(code, Qt::CaseInsensitive)) {
                       color = workoutCodes[code];
                    }
                }
            }
        }
        startY = rect.y() + ((count - 1) * rect.height()) / number;
        stopY = rect.height() / number;
        /* fillRect() doesn't need width-1, height-1.  drawRect() does */
        painter->fillRect(rect.x(), startY,
                          rect.width(), stopY,
                          color);
        textRect = QRect(rect.x(), startY, rect.width(), stopY);
        pen.setColor(Qt::black);
        pen.setStyle(Qt::SolidLine);
        painter->setPen(pen);
        painter->drawText(textRect, Qt::AlignHCenter | Qt::TextWordWrap, line);
        pen.setColor(QColor(0, 0, 0, 63));
        pen.setStyle(Qt::SolidLine);
        painter->setPen(pen);
        painter->drawRoundRect(textRect, 10, 10);
        ++count;
    }

    /*
     *  Draw a rectangle in the color specified.  If this is the
     *  currently selected date, draw a black outline.
     */
    if (date == selectedDate()) {
        pen.setColor(Qt::black);
        pen.setStyle(Qt::SolidLine);
        pen.setWidth(2);
        painter->setPen(pen);
        /* drawRect() draws an outline, so needs width-1, height-1 */
        /* We set +1, +1, -2, -2 because the width is 2. */
        painter->drawRect(rect.x() + 1, rect.y() + 1,
                          rect.width() - 2, rect.height() - 2);
    }

    /*
     *  Display the date.
     */
    pen.setColor(QColor(0, 0, 0, 63));
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(1);
    painter->setPen(pen);
    QString textDate = QString::number(date.day());
    painter->drawText(rect,
                      Qt::AlignHCenter | Qt::TextWordWrap | Qt::AlignVCenter,
                      textDate);

    /* We're done, restore */
    painter->restore();
}

void RideCalendar::setHome(const QDir &homeDir)
{
    home = homeDir;
}

void RideCalendar::addWorkoutCode(QString string, QColor color)
{
    workoutCodes[string] = color;
}

/*
 * We extend QT's QCalendarWidget's sizeHint() so we claim a little bit of
 * extra space.
 */
QSize RideCalendar::sizeHint() const
{
    QSize hint = QCalendarWidget::sizeHint();
    hint.setHeight(hint.height() * 2);
    hint.setWidth(hint.width() * 2);
    return hint;
}

