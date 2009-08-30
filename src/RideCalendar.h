#ifndef EVENT_CALENDAR_WIDGET_H
#define EVENT_CALENDAR_WIDGET_H

#include <QCalendarWidget>
#include <QMultiMap>

class RideCalendar : public QCalendarWidget
{
    Q_OBJECT

public:
    RideCalendar(QWidget *parent = 0);

    void addEvent(QDate, QString, QColor);
    QSize sizeHint() const;

protected:
    void paintCell(QPainter *, const QRect &, const QDate &) const;

private:
    QMap<QDate, QString> _text;
    QMap<QDate, QColor> _color;
};

#endif
