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
        AboutDialog(Context *context);

    private:
        Context *context;

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
        AboutPage(Context *context);

    private:
        Context *context;
};

class VersionPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        VersionPage(Context *context);

    private:
        Context *context;
};

class ConfigPage : public QWidget
{
    Q_OBJECT

    public:
        ConfigPage(Context *context);

    private:
        Context *context;
};

class ContributorsPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        ContributorsPage(Context *context);

    private:
        Context *context;

};


#endif // ABOUTDIALOG_H
