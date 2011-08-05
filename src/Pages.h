#ifndef PAGES_H
#define PAGES_H

#include <QtGui>
#include <QLineEdit>
#include <QComboBox>
#include <QCalendarWidget>
#include <QPushButton>
#include <QTreeWidget>
#include <QTableView>
#include <QModelIndex>
#include <QCheckBox>
#include <QList>
#include "Zones.h"
#include "HrZones.h"
#include <QLabel>
#include <QDateEdit>
#include <QCheckBox>
#include <QValidator>
#include <QGridLayout>
#include <QProgressDialog>
#include "DeviceTypes.h"
#include "DeviceConfiguration.h"
#include "RideMetadata.h"
#include "DataProcessor.h"

#ifdef GC_HAVE_LIBOAUTH
extern "C" {
#include <oauth.h>
}
#endif

class QGroupBox;
class QHBoxLayout;
class QVBoxLayout;
class ColorsPage;
class IntervalMetricsPage;
class SummaryMetricsPage;
class MetadataPage;
class KeywordsPage;
class FieldsPage;
class Colors;
class ZonePage;
class HrZonePage;
class RiderPage;

class ConfigurationPage : public QWidget
{
    Q_OBJECT
    public:
        ConfigurationPage(MainWindow *main);
        void saveClicked();
        QComboBox *langCombo;
        QComboBox *unitCombo;
        QComboBox *crankLengthCombo;
        QCheckBox *allRidesAscending;
        QCheckBox *garminSmartRecord;
        QLineEdit *garminHWMarkedit;
        QLineEdit *mapIntervaledit;
        QLineEdit *BSdaysEdit;
        QComboBox *bsModeCombo;
        QLineEdit *workoutDirectory;
        QPushButton *workoutBrowseButton;

    public slots:
        void browseWorkoutDir();

    private:
        MainWindow *main;
        ColorsPage *colorsPage;
        SummaryMetricsPage *summaryMetrics;
        IntervalMetricsPage *intervalMetrics;
        MetadataPage *metadataPage;

        QGroupBox *configGroup;
        QLabel *langLabel;
        QLabel *unitLabel;
        QLabel *warningLabel;
        QLabel *workoutLabel;
        QHBoxLayout *langLayout;
        QHBoxLayout *unitLayout;
        QHBoxLayout *warningLayout;
        QHBoxLayout *workoutLayout;
        QVBoxLayout *configLayout;
        QVBoxLayout *mainLayout;
        QGridLayout *bsDaysLayout;
        QHBoxLayout *bsModeLayout;
        QGridLayout *garminLayout;
        QGridLayout *mapIntervalLayout;
};

class CyclistPage : public QWidget
{
    Q_OBJECT
    public:
        CyclistPage(MainWindow *mainWindow);
        void saveClicked();

        QLabel *perfManLabel;
        QLabel *perfManStartLabel;
        QLabel *perfManSTSLabel;
        QLabel *perfManLTSLabel;
        QLabel *perfManDaysLabel;
        QLineEdit *perfManStart;
        QLineEdit *perfManSTSavg;
        QLineEdit *perfManLTSavg;
        QLineEdit *perfManDays;
        QCheckBox *showSBToday;

    private:
        ZonePage *zonePage;
        HrZonePage *hrZonePage;
        RiderPage *riderPage;
        MainWindow *main;

        QGroupBox *cyclistGroup;
        QVBoxLayout *perfManLayout;
        QHBoxLayout *perfManStartValLayout;
        QHBoxLayout *perfManSTSavgLayout;
        QHBoxLayout *perfManLTSavgLayout;
        QHBoxLayout *perfManDaysLayout;
        QVBoxLayout *cyclistLayout;
        QVBoxLayout *mainLayout;
        QIntValidator *perfManStartValidator;
        QIntValidator *perfManSTSavgValidator;
        QIntValidator *perfManLTSavgValidator;
        QIntValidator *perfManDaysValidator;
};

class deviceModel : public QAbstractTableModel
{
    Q_OBJECT

     public:
        deviceModel(QObject *parent=0);
        QObject *parent;

        // sets up the headers
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;

        // how much data do we have?
        int rowCount(const QModelIndex &parent) const;
        int columnCount(const QModelIndex &parent) const;

        // manipulate the data - data() gets and setData() sets (set/get might be better?)
        QVariant data(const QModelIndex &index, int role) const;
        bool setData(const QModelIndex &index, const QVariant &value, int role);

        // insert/remove and update
        void add(DeviceConfiguration &);   // add a new DeviceConfiguration
        void del();                        // add a new DeviceConfiguration
        bool insertRows(int position, int rows, const QModelIndex &index=QModelIndex());
        bool removeRows(int position, int rows, const QModelIndex &index=QModelIndex());


        QList<DeviceConfiguration> Configuration;  // the actual data
 };

class DevicePage : public QWidget
{
    Q_OBJECT
    public:
        DevicePage(QWidget *parent = 0);
        void setConfigPane();
        void pairClicked(DeviceConfiguration *, QProgressDialog *);

        QList<DeviceType> devices;

        // GUI Elements
        QGroupBox *deviceGroup;
        QLabel *nameLabel;
        QLineEdit *deviceName;

        QLabel *typeLabel;
        QComboBox *typeSelector;

        QLabel *specLabel;
        QLabel *specHint;   // hints at the format for a port spec
        QLabel *profHint;   // hints at the format for profile info
        QLineEdit *deviceSpecifier;

        QLabel *profLabel;
        QLineEdit *deviceProfile;

        QCheckBox *isDefaultDownload;
        QCheckBox *isDefaultRealtime;

        QLabel *virtualPowerLabel; // do we compute power using an algorithm?
        QComboBox *virtualPower;

        QTableView *deviceList;

        QPushButton *addButton;
        QPushButton *delButton;
        QPushButton *pairButton;

        QGridLayout *leftLayout;
        QVBoxLayout *rightLayout;

        QGridLayout *inLayout;
        QVBoxLayout *mainLayout;

        deviceModel *deviceListModel;
};

class IntervalMetricsPage : public QWidget
{
    Q_OBJECT

    public:

        IntervalMetricsPage(QWidget *parent = NULL);

    public slots:

        void upClicked();
        void downClicked();
        void leftClicked();
        void rightClicked();
        void availChanged();
        void selectedChanged();
        void saveClicked();

    protected:

        bool changed;
        QListWidget *availList;
        QListWidget *selectedList;
        QPushButton *upButton;
        QPushButton *downButton;
        QPushButton *leftButton;
        QPushButton *rightButton;
};

class SummaryMetricsPage : public QWidget
{
    Q_OBJECT

    public:

        SummaryMetricsPage(QWidget *parent = NULL);

    public slots:

        void upClicked();
        void downClicked();
        void leftClicked();
        void rightClicked();
        void availChanged();
        void selectedChanged();
        void saveClicked();

    protected:

        bool changed;
        QListWidget *availList;
        QListWidget *selectedList;
        QPushButton *upButton;
        QPushButton *downButton;
        QPushButton *leftButton;
        QPushButton *rightButton;
};


class KeywordsPage : public QWidget
{
    Q_OBJECT

    public:
        KeywordsPage(QWidget *parent, QList<KeywordDefinition>);
        void getDefinitions(QList<KeywordDefinition>&);

    public slots:
        void addClicked();
        void upClicked();
        void downClicked();
        void renameClicked();
        void deleteClicked();

    private:

        QTreeWidget *keywords;

        QPushButton *upButton, *downButton, *addButton, *renameButton, *deleteButton;
};

class ColorsPage : public QWidget
{
    Q_OBJECT

    public:
        ColorsPage(QWidget *parent);
        void saveClicked();

    public slots:

    private:
        QTreeWidget *colors;
        const Colors *colorSet;
};

class FieldsPage : public QWidget
{
    Q_OBJECT

    public:
        FieldsPage(QWidget *parent, QList<FieldDefinition>);
        void getDefinitions(QList<FieldDefinition>&);

    public slots:
        void addClicked();
        void upClicked();
        void downClicked();
        void renameClicked();
        void deleteClicked();

    private:
        void addFieldTypes(QComboBox *p);

        QTreeWidget *fields;

        QPushButton *upButton, *downButton, *addButton, *renameButton, *deleteButton;
};

class ProcessorPage : public QWidget
{
    Q_OBJECT

    public:

        ProcessorPage(MainWindow *main);
        void saveClicked();

    public slots:

        //void upClicked();
        //void downClicked();

    protected:

        MainWindow *main;
        QMap<QString, DataProcessor*> processors;

        QTreeWidget *processorTree;
        //QPushButton *upButton, *downButton;

};

class MetadataPage : public QWidget
{
    Q_OBJECT

    public:

        MetadataPage(MainWindow *);
        void saveClicked();

    public slots:


    protected:

        MainWindow *main;
        bool changed;

        QTabWidget *tabs;
        KeywordsPage *keywordsPage;
        FieldsPage *fieldsPage;
        ProcessorPage *processorPage;

        // local versions for modification
        QList<KeywordDefinition> keywordDefinitions;
        QList<FieldDefinition>   fieldDefinitions;
};

class SchemePage : public QWidget
{
    Q_OBJECT

    public:
        SchemePage(ZonePage *parent);
        ZoneScheme getScheme();
        void saveClicked();

    public slots:
        void addClicked();
        void deleteClicked();
        void renameClicked();

    private:
        ZonePage *zonePage;
        QTreeWidget *scheme;
        QPushButton *addButton, *renameButton, *deleteButton;
};

class CPPage : public QWidget
{
    Q_OBJECT

    public:
        CPPage(ZonePage *parent);

    public slots:
        void addClicked();
        void deleteClicked();
        void defaultClicked();
        void rangeSelectionChanged();
        void addZoneClicked();
        void deleteZoneClicked();
        void zonesChanged();

    private:
        bool active;

        QDateEdit *dateEdit;
        QDoubleSpinBox *cpEdit;

        ZonePage *zonePage;
        QTreeWidget *ranges;
        QTreeWidget *zones;
        QPushButton *addButton, *deleteButton, *defaultButton;
        QPushButton *addZoneButton, *deleteZoneButton;
};

class ZonePage : public QWidget
{
    Q_OBJECT

    public:

        ZonePage(MainWindow *);
        void saveClicked();

        //ZoneScheme scheme;
        Zones zones;

        // Children talk to each other
        SchemePage *schemePage;
        CPPage *cpPage;

    public slots:


    protected:

        MainWindow *main;
        bool changed;

        QTabWidget *tabs;

        // local versions for modification
};

class HrSchemePage : public QWidget
{
    Q_OBJECT

public:
    HrSchemePage(HrZonePage *parent);
    HrZoneScheme getScheme();
    void saveClicked();

    public slots:
    void addClicked();
    void deleteClicked();
    void renameClicked();

private:
    HrZonePage *zonePage;
    QTreeWidget *scheme;
    QPushButton *addButton, *renameButton, *deleteButton;
};


class LTPage : public QWidget
{
    Q_OBJECT

public:
    LTPage(HrZonePage *parent);

    public slots:
    void addClicked();
    void deleteClicked();
    void defaultClicked();
    void rangeSelectionChanged();
    void addZoneClicked();
    void deleteZoneClicked();
    void zonesChanged();

private:
    bool active;

    QDateEdit *dateEdit;
    QDoubleSpinBox *ltEdit;
    QDoubleSpinBox *restHrEdit;
    QDoubleSpinBox *maxHrEdit;

    HrZonePage  *zonePage;
    QTreeWidget *ranges;
    QTreeWidget *zones;
    QPushButton *addButton, *deleteButton, *defaultButton;
    QPushButton *addZoneButton, *deleteZoneButton;
};

class HrZonePage : public QWidget
{
    Q_OBJECT

public:

    HrZonePage(MainWindow *);
    void saveClicked();

    //ZoneScheme scheme;
    HrZones zones;

    // Children talk to each other
    HrSchemePage *schemePage;
    LTPage *ltPage;

    public slots:


protected:

    MainWindow *main;
    bool changed;

    QTabWidget *tabs;

    // local versions for modification
};

class RiderPage : public QWidget
{
    Q_OBJECT

    public:
        RiderPage(QWidget *parent, MainWindow *mainWindow);
        void saveClicked();

    public slots:
        void chooseAvatar();

    private:
        MainWindow *mainWindow;
        bool useMetricUnits;
        QLineEdit *nickname;
        QDateEdit *dob;
        QComboBox *sex;
        QDoubleSpinBox *weight;
        QTextEdit  *bio;
        QPushButton *avatarButton;
        QPixmap     avatar;
        QString cyclist;
};

class TwitterPage : public QWidget
{
    Q_OBJECT

    public:

        TwitterPage(QWidget *parent = 0);

        // Children talk to each other
        SchemePage *schemePage;
        CPPage *cpPage;
        QLineEdit *twitterPIN;
        void saveClicked();
    public slots:

#ifdef GC_HAVE_LIBOAUTH
        void authorizeClicked();
#endif

    protected:

        MainWindow *main;
        bool changed;

        QTabWidget *tabs;

        // local versions for modification
    private:
        QLabel *twitterPinLabel;
        QPushButton *authorizeButton;
        char *t_key;
        char *t_secret;
        boost::shared_ptr<QSettings> settings;

};

#endif
