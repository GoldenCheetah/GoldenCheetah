#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QSettings>
#include "Pages.h"
#include "MainWindow.h"

class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class MainWindow;
class Zones;

class ConfigDialog : public QDialog
{
    Q_OBJECT
    public:
        ConfigDialog(QDir home, Zones *zones, MainWindow *mainWindow);

    public slots:
        void changePage(QListWidgetItem *current, QListWidgetItem *previous);
        void save_Clicked();

        // device config slots
        void changedType(int);
        void devaddClicked();
        void devpairClicked();
        void devdelClicked();

    private:
        void createIcons();
        void calculateZones();
	void createNewRange();
	void moveCalendarToCurrentRange();

        MainWindow *mainWindow;
        ConfigurationPage *configPage;
        CyclistPage *cyclistPage;
        DevicePage *devicePage;
        QPushButton *saveButton;
        QStackedWidget *pagesWidget;
	QPushButton *closeButton;
	QHBoxLayout *horizontalLayout;
	QHBoxLayout *buttonsLayout;
	QVBoxLayout *mainLayout;
        QListWidget *contentsWidget;

        QSettings *settings;
        QDir home;
        Zones *zones;

        // used by device config
    QList<QTreeWidgetItem> twiNames, twiSpecs, twiTypes, twiDefaults;
};

#endif
