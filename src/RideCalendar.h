#ifndef EVENT_CALENDAR_WIDGET_H
#define EVENT_CALENDAR_WIDGET_H

#include <QCalendarWidget>
#include <QMultiMap>
#include "RideItem.h"
class MainWindow;

class RideCalendar : public QCalendarWidget
{
    Q_OBJECT

public:
    RideCalendar(MainWindow *parent);
    QSize sizeHint() const;
    void setHome(const QDir&);
    void addWorkoutCode(QString, QColor);

protected:
    void paintCell(QPainter *, const QRect &, const QDate &) const;

private:
    QMap<QString, QColor> workoutCodes;
    QDir home;
    MainWindow *mainWindow;
};

#endif
