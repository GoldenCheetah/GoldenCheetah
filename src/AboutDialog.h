#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H
#include "GoldenCheetah.h"

#include "Pages.h"
#include <QDialog>

class Context;
class AboutPage;
class VersionPage;
class ConfigPage;
class ContributorsPage;

class AboutDialog: public QDialog
{
    Q_OBJECT
    G_OBJECT

    public:
        AboutDialog(Context *context, QDir home);

    private:
        Context *context;
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
        AboutPage(Context *context, QDir home);

    private:
        Context *context;
        QDir home;
};

class VersionPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        VersionPage(Context *context, QDir home);

    private:
        Context *context;
        QDir home;
};

class ConfigPage : public QWidget
{
    Q_OBJECT

    public:
        ConfigPage(Context *context, QDir home);

    private:
        Context *context;
        QDir home;
};

class ContributorsPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        ContributorsPage(Context *context, QDir home);

    private:
        Context *context;
        QDir home;

};


#endif // ABOUTDIALOG_H
