#ifndef EVENT_CALENDAR_WIDGET_H
#define EVENT_CALENDAR_WIDGET_H

#include <QCalendarWidget>
#include <QMultiMap>
#include "RideItem.h"

class RideCalendar : public QCalendarWidget
{
    Q_OBJECT

public:
    RideCalendar(QWidget *parent = 0);
    void removeRide(RideItem*);
    void addRide(RideItem*);
    QSize sizeHint() const;
    void setHome(const QDir&);
    void addWorkoutCode(QString, QColor);

protected:
    void paintCell(QPainter *, const QRect &, const QDate &) const;

private:
    void addEvent(QDate, QString, QColor);
    void removeEvent(QDate);
    QMap<QDate, QString> _text;
    QMap<QDate, QColor> _color;
    QMap<QString, QColor> workoutCodes;
    QDir home;
};

#endif
