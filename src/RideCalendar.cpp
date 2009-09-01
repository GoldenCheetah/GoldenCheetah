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
};

void RideCalendar::paintCell(QPainter *painter, const QRect &rect, const QDate &date) const
{
    if (_text.contains(date)) {
        painter->save();

        /*
         *  Draw a rectangle in the color specified.  If this is the
         *  currently selected date, draw a black outline.
         */
        QPen pen(Qt::SolidLine);
        pen.setCapStyle(Qt::SquareCap);
        painter->setBrush(_color[date]);
        if (date == selectedDate()) {
            pen.setColor(Qt::black);
            pen.setWidth(1);
        } else {
            pen.setColor(_color[date]);
        }
        painter->setPen(pen);
        /*
         * We have to draw to height-1 and width-1 because Qt draws outlines
         * outside the box by default.
         */
        painter->drawRect(rect.x(), rect.y(), rect.width() - 1, rect.height() - 1);

        /*
         * Display the text.
         */
        pen.setColor(Qt::black);
        painter->setPen(pen);
        QString text = QString::number(date.day());
        text = text + "\n" + _text[date];
        QFont font = painter->font();
        font.setPointSize(font.pointSize() - 2);
        painter->setFont(font);
        painter->drawText(rect, Qt::AlignHCenter | Qt::TextWordWrap, text);

        painter->restore();
    } else {
        QCalendarWidget::paintCell(painter, rect, date);
    }
}

void RideCalendar::setHome(const QDir &homeDir)
{
    home = homeDir;
}

void RideCalendar::addRide(RideItem* ride)
{
    /*
     *  We want to display these things inside the Calendar.
     *  Pick a colour (this should really be configurable)
     *    - red for races
     *    - yellow for sick days
     *    - green for rides
     */
    QDateTime dt = ride->dateTime;
    QString notesPath = home.absolutePath() + "/" + ride->notesFileName;
    QFile notesFile(notesPath);
    QColor color(Qt::green);
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
    addEvent(dt.date(), line, color);
}

void RideCalendar::removeRide(RideItem* ride)
{
    removeEvent(ride->dateTime.date());
}

void RideCalendar::addWorkoutCode(QString string, QColor color)
{
    workoutCodes[string] = color;
}

/*
 * Private:
 * Add a string, and a color, to a specific date.
 */
void RideCalendar::addEvent(QDate date, QString string, QColor color)
{
    _text[date] = string;
    _color[date] = color;
    update();
}

/*
 * Private:
 * Remove the info for a current date.
 */
void RideCalendar::removeEvent(QDate date)
{
    if (_text.contains(date)) {
	_text.remove(date);
	_color.remove(date);
    }
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

