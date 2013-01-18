#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H
#include "GoldenCheetah.h"

#include "Pages.h"
#include <QDialog>

class MainWindow;
class AboutPage;
class VersionPage;
class ConfigPage;
class ContributorsPage;

class AboutDialog: public QDialog
{
    Q_OBJECT
    G_OBJECT

    public:
        AboutDialog(MainWindow *mainWindow, QDir home);

    private:
        MainWindow *mainWindow;
        QDir home;

        AboutPage *aboutPage;
        VersionPage *versionPage;
        ConfigPage *configPage;
        ContributorsPage *contributorsPage;

        QTabWidget *tabWidget;
        QVBoxLayout *mainLayout;
};


class AboutPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        AboutPage(MainWindow *main, QDir home);

    private:
        MainWindow *main;
        QDir home;
};

class VersionPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        VersionPage(MainWindow *main, QDir home);

    private:
        MainWindow *main;
        QDir home;
};

class ConfigPage : public QWidget
{
    Q_OBJECT

    public:
        ConfigPage(MainWindow*main, QDir home);

    private:
        MainWindow *main;
        QDir home;
};

class ContributorsPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        ContributorsPage(MainWindow *main, QDir home);

    private:
        MainWindow *main;
        QDir home;

};


#endif // ABOUTDIALOG_H
