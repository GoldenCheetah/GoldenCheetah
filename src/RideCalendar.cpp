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
#include "RideCalendar.h"

RideCalendar::RideCalendar(QWidget *parent)
                   : QCalendarWidget(parent)
{
    this->setFirstDayOfWeek(Qt::Monday);
    this->addWorkoutCode(QString("race"), QColor(255,128,128));
    this->addWorkoutCode(QString("sick"), QColor(255,255,128));
    this->addWorkoutCode(QString("swim"), QColor(128,128,255));
    this->addWorkoutCode(QString("gym"), QColor(Qt::lightGray));
};

void RideCalendar::paintCell(QPainter *painter, const QRect &rect, const QDate &date) const
{
    if (!_rides.contains(date)) {
        QCalendarWidget::paintCell(painter, rect, date);
        return;
    }
    painter->save();

    /*
     * Fill in the text and color to some default
     */
    QRect textRect;
    int number = _rides.count(date);
    float count = 1;
    int startY, stopY;
    QPen pen(Qt::SolidLine);
    painter->setPen(pen);
    pen.setCapStyle(Qt::SquareCap);
    QMap<QDateTime, RideItem*> ridesToday;
    RideItem * ride;
    QFont font = painter->font();
    font.setPointSize(font.pointSize() - 2);
    painter->setFont(font);

    /*
     *  Loop over all the matching rides, and record the time.
     *  That way, the entries are sorted earliest -> latest.
     */
    QMultiMap<QDate, RideItem *>::const_iterator j = _rides.find(date);
    while (j != _rides.end() && j.key() == date) {
        ride = j.value();
        ridesToday.insert(ride->dateTime, ride);
        ++j;
    }

    /*
     *  Loop over all the rides for today, and record colour and text.
     */
    QMap<QDateTime, RideItem *>::const_iterator i = ridesToday.begin();
    while (i != ridesToday.end()) {
        RideItem *ride = i.value();

        QString notesPath = home.absolutePath() + "/" + ride->notesFileName;
        QFile notesFile(notesPath);
        QColor color(128, 255, 128);
        QString line("Ride");
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
        ++i;
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
 * Add a ride, to a specific date.
 */
void RideCalendar::addRide(RideItem* ride)
{
    _rides.insert(ride->dateTime.date(), ride);
    update();
}

/*
 * Remove the info for a current ride.
 */
void RideCalendar::removeRide(RideItem* ride)
{
    QDate date = ride->dateTime.date();
    _rides.erase(_rides.find(date, ride));
    update();
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

