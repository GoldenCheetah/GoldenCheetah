#ifndef PAGES_H
#define PAGES_H

#include <QWidget>
#include <QComboBox>
#include <QCalendarWidget>
#include <QPushButton>
#include <QList>
#include "Zones.h"
#include <QLabel>
#include <QDateEdit>
#include <QCheckBox>

class ConfigurationPage : public QWidget
{
    public:
        ConfigurationPage(QWidget *parent = 0);
        QComboBox *unitCombo;
        QCheckBox *allRidesAscending;
};

class CyclistPage : public QWidget
{
    public:
        CyclistPage(QWidget *parent = 0, Zones *_zones = 0, bool emptyZone = true);
        int thresholdPower;
        QLineEdit *txtThreshold;
        QString getText();
        QCalendarWidget *calendar;
        void setCurrentRange(int range);
        QPushButton *btnBack;
        QPushButton *btnForward;
        QPushButton *btnNew;
        QLabel *lblCurRange;
        QDateEdit *txtStartDate;
        QDateEdit *txtEndDate;
        QLabel *lblStartDate;
        QLabel *lblEndDate;

        int getCurrentRange();
        void setChoseNewZone(bool _newZone);
        bool isNewZone();

    private:
        Zones *zones;
        int currentRange;
        bool newZone;

};

/*
class PowerPage : QWidget
{
    public:
        PowerPage(QWidget *parent = 0);
        
};

*/
#endif
