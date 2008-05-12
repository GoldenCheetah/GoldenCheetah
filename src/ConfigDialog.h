#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QSettings>
#include <Pages.h>
#include "MainWindow.h"

class QListWidget;
class QListWidgetItem;
class QStackedWidget;

class ConfigDialog : public QDialog
{
    Q_OBJECT
    public:
        ConfigDialog(QDir home);
    public slots:
        void changePage(QListWidgetItem *current, QListWidgetItem *previous);
        void save_Clicked();
      
    private:
        void createIcons();
        void calculateZones();

        QListWidget *contentsWidget;
        QStackedWidget *pagesWidget;

        QSettings settings;
        ConfigurationPage *configPage;
        CyclistPage *cyclistPage;
        QPushButton *saveButton;
        QDir home;
};

#endif
