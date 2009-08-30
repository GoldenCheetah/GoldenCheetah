#include <QCalendarWidget>
#include <QMultiMap>
#include <QPainter>
#include <QObject>
#include <QDate>
#include <QAbstractItemView>
#include <QSize>
#include <QTextCharFormat>
#include <QPen>

#include "RideCalendar.h"

RideCalendar::RideCalendar(QWidget *parent)
                   : QCalendarWidget(parent)
{
};

void RideCalendar::addEvent(QDate date, QString string, QColor color)
{
    _text[date] = string;
    _color[date] = color;
    update();
}

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

