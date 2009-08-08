#ifndef PAGES_H
#define PAGES_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCalendarWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QList>
#include "Zones.h"
#include <QLabel>
#include <QDateEdit>
#include <QCheckBox>
#include <QValidator>
#include <QGridLayout>

class QGroupBox;
class QHBoxLayout;
class QVBoxLayout;

class ConfigurationPage : public QWidget
{
    public:
        ~ConfigurationPage();
        ConfigurationPage();
        QComboBox *unitCombo;
        QComboBox *crankLengthCombo;
        QCheckBox *allRidesAscending;
	QLineEdit *BSdaysEdit;
	QComboBox *bsModeCombo;
        

    private:
	QGroupBox *configGroup;
	QLabel *unitLabel;
	QLabel *warningLabel;
	QHBoxLayout *unitLayout;
	QHBoxLayout *warningLayout;
	QVBoxLayout *configLayout;
	QVBoxLayout *mainLayout;
	QGridLayout *bsDaysLayout;
	QHBoxLayout *bsModeLayout;
};

class CyclistPage : public QWidget
{
    public:
        ~CyclistPage();
        CyclistPage(Zones **_zones);
        int thresholdPower;
        QString getText();
        int getCP();
	void setCP(int cp);
	void setSelectedDate(QDate date);
        void setCurrentRange(int range = -1);
        QPushButton *btnBack;
        QPushButton *btnForward;
        QPushButton *btnDelete;
        QCheckBox *checkboxNew;
        QCalendarWidget *calendar;
        QLabel *lblCurRange;
        QLabel *txtStartDate;
        QLabel *txtEndDate;
        QLabel *lblStartDate;
        QLabel *lblEndDate;

        int getCurrentRange();
	bool isNewMode();

	inline void setCPFocus() {
	    txtThreshold->setFocus();
	}

	inline QDate selectedDate() {
	    return calendar->selectedDate();
	}

    private:
	QGroupBox *cyclistGroup;
        Zones **zones;
        int currentRange;
	QLabel *lblThreshold;
        QLineEdit *txtThreshold;
	QIntValidator *txtThresholdValidator;
	QHBoxLayout *powerLayout;
	QHBoxLayout *rangeLayout;
	QHBoxLayout *dateRangeLayout;
	QHBoxLayout *zoneLayout;
	QHBoxLayout *calendarLayout;
	QVBoxLayout *cyclistLayout;
	QVBoxLayout *mainLayout;
};
#endif
