/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef PAGES_H
#define PAGES_H
#include "GoldenCheetah.h"

#include <QtGui>
#include <QLineEdit>
#include <QComboBox>
#include <QGroupBox>
#include <QCalendarWidget>
#include <QPushButton>
#include <QTreeWidget>
#include <QTableView>
#include <QModelIndex>
#include <QCheckBox>
#include <QListWidget>
#include <QList>
#include <QFileDialog>
#include "Zones.h"
#include "HrZones.h"
#include "PaceZones.h"
#include <QLabel>
#include <QDateEdit>
#include <QCheckBox>
#include <QValidator>
#include <QGridLayout>
#include <QProgressDialog>
#include <QFontComboBox>
#include "DeviceTypes.h"
#include "DeviceConfiguration.h"
#include "RideMetadata.h"
#include "DataProcessor.h"
#include "Season.h"
#include "SeasonParser.h"
#include "RideAutoImportConfig.h"
#include "RemoteControl.h"

class QGroupBox;
class QHBoxLayout;
class QVBoxLayout;
class ColorsPage;
class IntervalMetricsPage;
class ZonePage;
class HrZonePage;
class PaceZonePage;
class SummaryMetricsPage;
class MetadataPage;
class KeywordsPage;
class FieldsPage;
class Colors;
class AboutRiderPage;
class SeasonsPage;
class DevicePage;
class RemotePage;

class GeneralPage : public QWidget
{
    Q_OBJECT
    G_OBJECT

    public:
        GeneralPage(Context *context);
        qint32 saveClicked();

        QString athleteWAS; // remember what we started with !
        QComboBox *unitCombo;

    public slots:
        void browseWorkoutDir();
        void browseAthleteDir();
#ifdef GC_WANT_R
        void browseRDir();
        void embedRchanged(int);
#endif

    private:
        Context *context;

        QComboBox *langCombo;
        QComboBox *wbalForm;
        QCheckBox *garminSmartRecord;
        QCheckBox *warnOnExit;
#ifdef GC_WANT_HTTP
        QCheckBox *startHttp;
#endif
#ifdef GC_WANT_R
        QCheckBox *embedR;
#endif
        QLineEdit *garminHWMarkedit;
        QLineEdit *hystedit;
        QLineEdit *athleteDirectory;
        QLineEdit *workoutDirectory;
        QPushButton *workoutBrowseButton;
        QPushButton *athleteBrowseButton;

#ifdef GC_WANT_R
        QPushButton *rBrowseButton;
        QLineEdit *rDirectory;
        QLabel *rLabel;
#endif

        QLabel *langLabel;
        QLabel *warningLabel;
        QLabel *workoutLabel;
        QLabel *athleteLabel;

        struct {
            int unit;
            float hyst;
            int wbal;
            bool warn;
#ifdef GC_WANT_HTTP
            bool starthttp;
#endif
        } b4;


};

class RiderPhysPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        RiderPhysPage(QWidget *parent, Context *context);
        qint32 saveClicked();

    public slots:
        void unitChanged(int currentIndex);

    private:
        Context *context;

        QLabel *wbaltaulabel;
        QLabel *weightlabel;
        QLabel *heightlabel;
        QDoubleSpinBox *weight;
        QDoubleSpinBox *height;
        QSpinBox *wbaltau;
        QLabel *perfManSTSLabel;
        QLabel *perfManLTSLabel;
        QLineEdit *perfManSTSavg;
        QLineEdit *perfManLTSavg;
        QIntValidator *perfManSTSavgValidator;
        QIntValidator *perfManLTSavgValidator;
        QCheckBox *showSBToday;

    struct {
        double weight;
        double height;
        int lts,sts;
    } b4;

    private slots:
};

class AboutRiderPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        AboutRiderPage(QWidget *parent, Context *context);
        qint32 saveClicked();

    public slots:
        void chooseAvatar();

    private:
        Context *context;

        QLineEdit *nickname;
        QDateEdit *dob;
        QComboBox *sex;
        QPushButton *avatarButton;
        QPixmap     avatar;
        QComboBox *crankLengthCombo;
        QComboBox *rimSizeCombo;
        QComboBox *tireSizeCombo;
        QLineEdit *wheelSizeEdit;
        QSpinBox *autoBackupPeriod;
        QLineEdit *autoBackupFolder;
        QPushButton *autoBackupFolderBrowse;


    struct {
        int wheel;
        int crank;
    } b4;

    private slots:
        void calcWheelSize();
        void resetWheelSize();
        void chooseAutoBackupFolder();
};

class CredentialsPage : public QScrollArea
{
    Q_OBJECT
    G_OBJECT


    public:
        CredentialsPage(QWidget *parent, Context *context);
        qint32 saveClicked();

    public slots:
#ifdef GC_HAVE_KQOAUTH
        void authoriseTwitter();
#endif
#if QT_VERSION >= 0x050000
        void authoriseDropbox();
        void chooseDropboxFolder();
        void authoriseGoogleDrive();
        void chooseGoogleDriveFolder();
        void chooseGoogleDriveAuthScope(const QString& scope);
#endif
        void authoriseStrava();
        void authoriseCyclingAnalytics();
        void authoriseGoogleCalendar();
        void dvCALDAVTypeChanged(int);
        void chooseLocalFileStoreFolder();

    private:
        enum GoogleType {
            CALENDAR = 1,
            DRIVE,
        };
        void authoriseGoogle(GoogleType type);

        Context *context;

        QLineEdit *tpUser;
        QLineEdit *tpPass;
        QComboBox *tpType;
        QPushButton *tpTest;

#ifdef GC_HAVE_KQOAUTH
        QPushButton *twitterAuthorise;
#endif
#if QT_VERSION >= 0x050000
        QPushButton *dropboxAuthorise;
        QPushButton *dropboxAuthorised, *dropboxBrowse;
        QLineEdit *dropboxFolder;
        QPushButton *googleDriveAuthorise;
        QPushButton *googleDriveAuthorised;
        QPushButton *googleDriveBrowse;
        QLineEdit *googleDriveFolder;
#endif

        QComboBox *dvCALDAVType;
        QPushButton *stravaAuthorise, *stravaAuthorised, *twitterAuthorised;
        QPushButton *networkFileStoreFolderBrowse;
        QLineEdit *networkFileStoreFolder;
        QPushButton *cyclingAnalyticsAuthorise, *cyclingAnalyticsAuthorised;
        QPushButton *googleCalendarAuthorise, *googleCalendarAuthorised;

        QLineEdit *rideWithGPSUser;
        QLineEdit *rideWithGPSPass;

        QLineEdit *ttbUser;
        QLineEdit *ttbPass;

        QLineEdit *veloHeroUser;
        QLineEdit *veloHeroPass;

        QLineEdit *sphUser;
        QLineEdit *sphPass;

        QLineEdit *selUser;
        QLineEdit *selPass;

        QLineEdit *wiURL; // url for withings
        QLineEdit *wiUser;
        QLineEdit *wiPass;

        QLineEdit *webcalURL; // url for webcal calendar (read only, TP.com, Google Calendar)

        QLineEdit *dvURL; // url for calDAV calendar (read/write, e.g. Hotmail)
        QLineEdit *dvUser;
        QLineEdit *dvPass;
        QLineEdit *dvGoogleCalid;
};

class deviceModel : public QAbstractTableModel
{
    Q_OBJECT
    G_OBJECT


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

    public slots:
        // config changed by wizard so reset
        void doReset();

 };

class DevicePage : public QWidget
{
    Q_OBJECT
    G_OBJECT

    public:
        DevicePage(QWidget *parent, Context *context);
        qint32 saveClicked();

        QTableView *deviceList;

    public slots:
        void devaddClicked();
        void devdelClicked();

    private:
        Context *context;

        QList<DeviceType> devices;

        QPushButton *addButton;
        QPushButton *delButton;

        QGridLayout *leftLayout;
        QVBoxLayout *rightLayout;

        QGridLayout *inLayout;
        QVBoxLayout *mainLayout;

        deviceModel *deviceListModel;

        QCheckBox   *multiCheck;
        QCheckBox   *autoConnect;
};

class RemotePage : public QWidget
{
    Q_OBJECT
    G_OBJECT

    public:
        RemotePage(QWidget *parent, Context *context);
        qint32 saveClicked();

    private:
        RemoteControl *remote;
        Context       *context;
        QTreeWidget   *fields;
};

class BestsMetricsPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        BestsMetricsPage(QWidget *parent = NULL);

    public slots:

        void upClicked();
        void downClicked();
        void leftClicked();
        void rightClicked();
        void availChanged();
        void selectedChanged();
        qint32 saveClicked();

    protected:

        bool changed;
        QListWidget *availList;
        QListWidget *selectedList;
#ifndef Q_OS_MAC
        QToolButton *upButton;
        QToolButton *downButton;
        QToolButton *leftButton;
        QToolButton *rightButton;
#else
        QPushButton *upButton;
        QPushButton *downButton;
        QPushButton *leftButton;
        QPushButton *rightButton;
#endif
};

class CustomMetricsPage : public QWidget
{
    Q_OBJECT
    G_OBJECT

    public:

    CustomMetricsPage(QWidget *parent, Context *context);

    public slots:

        void refreshTable();
        qint32 saveClicked();

        void deleteClicked();
        void addClicked();
        void editClicked();
        void doubleClicked(QTreeWidgetItem *item, int column);

    protected:
        Context *context;

        QPushButton *addButton,
                    *deleteButton,
                    *editButton;
        QTreeWidget *table;
        QList<UserMetricSettings> metrics;

        struct {
            quint16 crc;
        } b4;
};

class IntervalMetricsPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        IntervalMetricsPage(QWidget *parent = NULL);

    public slots:

        void upClicked();
        void downClicked();
        void leftClicked();
        void rightClicked();
        void availChanged();
        void selectedChanged();
        qint32 saveClicked();

    protected:

        bool changed;
        QListWidget *availList;
        QListWidget *selectedList;
#ifndef Q_OS_MAC
        QToolButton *upButton;
        QToolButton *downButton;
        QToolButton *leftButton;
        QToolButton *rightButton;
#else
        QPushButton *upButton;
        QPushButton *downButton;
        QPushButton *leftButton;
        QPushButton *rightButton;
#endif
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
        qint32 saveClicked();

    protected:

        bool changed;
        QListWidget *availList;
        QListWidget *selectedList;
#ifndef Q_OS_MAC
        QToolButton *upButton;
        QToolButton *downButton;
        QToolButton *leftButton;
        QToolButton *rightButton;
#else
        QPushButton *upButton;
        QPushButton *downButton;
        QPushButton *leftButton;
        QPushButton *rightButton;
#endif
};


class KeywordsPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        KeywordsPage(MetadataPage *parent, QList<KeywordDefinition>);
        void getDefinitions(QList<KeywordDefinition>&);
        QCheckBox *rideBG;

    public slots:
        void addClicked();
        void upClicked();
        void downClicked();
        void renameClicked();
        void deleteClicked();

        void pageSelected(); // reset the list of fields when we are selected...
        void colorfieldChanged();

    private:

        QTreeWidget *keywords;

#ifndef Q_OS_MAC
        QToolButton *upButton, *downButton;
#else
        QPushButton *upButton, *downButton;
#endif
        QPushButton *addButton, *renameButton, *deleteButton;
        QLabel *fieldLabel;
        QComboBox *fieldChooser;

        MetadataPage *parent;
};

class ColorsPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        ColorsPage(QWidget *parent);
        qint32 saveClicked();

    public slots:
        void applyThemeClicked();
        void tabChanged();

    private:

        // General stuff
        QCheckBox *antiAliased;
#ifndef Q_OS_MAC // they do scrollbars nicely
        QCheckBox *rideScroll;
        QCheckBox *rideHead;
#endif
        QDoubleSpinBox *lineWidth;

        // theme
        QComboBox *chromeCombo;

        // Fonts
        QFontComboBox *def,
                      *titles,
                      *chartmarkers,
                      *chartlabels,
                      *calendar;

        QComboBox *defaultSize,
                  *titlesSize,
                  *chartmarkersSize,
                  *chartlabelsSize,
                  *calendarSize;

        // tabbed view between colors and themes
        QTabWidget *colorTab;

        // Colors
        QTreeWidget *colors;
        QTreeWidget *themes;
        const Colors *colorSet;
        QPushButton *applyTheme;

        struct {
            bool alias, scroll, head;
            double line;
            int chrome;
            unsigned long fingerprint;
        } b4;
};

class FieldsPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        FieldsPage(QWidget *parent, QList<FieldDefinition>);
        void getDefinitions(QList<FieldDefinition>&);
        static void addFieldTypes(QComboBox *p);

    public slots:
        void addClicked();
        void upClicked();
        void downClicked();
        void renameClicked();
        void deleteClicked();

    private:

        QTreeWidget *fields;

#ifndef Q_OS_MAC
        QToolButton *upButton, *downButton;
#else
        QPushButton *upButton, *downButton;
#endif
        QPushButton *addButton, *renameButton, *deleteButton;
};

class ProcessorPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        ProcessorPage(Context *context);
        qint32 saveClicked();

    public slots:

        //void upClicked();
        //void downClicked();

    protected:

        Context *context;
        QMap<QString, DataProcessor*> processors;

        QTreeWidget *processorTree;
        //QPushButton *upButton, *downButton;

};

class DefaultsPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        DefaultsPage(QWidget *parent, QList<DefaultDefinition>);
        void getDefinitions(QList<DefaultDefinition>&);

    public slots:
        void addClicked();
        void upClicked();
        void downClicked();
        void deleteClicked();

    protected:

        QTreeWidget *defaults;

#ifndef Q_OS_MAC
        QToolButton *upButton, *downButton;
#else
        QPushButton *upButton, *downButton;
#endif
        QPushButton *addButton, *deleteButton;

};

class MetadataPage : public QWidget
{
    Q_OBJECT
    G_OBJECT

    friend class ::KeywordsPage;

    public:

        MetadataPage(Context *);
        qint32 saveClicked();

    public slots:


    protected:

        Context *context;
        bool changed;

        QTabWidget *tabs;
        KeywordsPage *keywordsPage;
        FieldsPage *fieldsPage;
        ProcessorPage *processorPage;
        DefaultsPage *defaultsPage;

        // local versions for modification
        QList<KeywordDefinition> keywordDefinitions;
        QList<FieldDefinition>   fieldDefinitions;
        QList<DefaultDefinition>  defaultDefinitions;
        QString colorfield;

        // initial values
        struct {
            unsigned long fieldFingerprint;
            unsigned long keywordFingerprint;
            QString colorfield;
        } b4;
};

//
// Power Zones
//
class SchemePage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        SchemePage(Zones *zones);
        ZoneScheme getScheme();
        qint32 saveClicked();

    public slots:
        void addClicked();
        void deleteClicked();
        void renameClicked();

    private:
        Zones *zones;
        QTreeWidget *scheme;
        QPushButton *addButton, *renameButton, *deleteButton;
};

class CPPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        CPPage(Context *context, Zones *zones, SchemePage *schemePage);
        QComboBox *useCPForFTPCombo;
        qint32 saveClicked();

    struct {
        int cpforftp;
    } b4;

    public slots:
        void addClicked();
        void editClicked();
        void deleteClicked();
        void defaultClicked();
        void rangeEdited();
        void rangeSelectionChanged();
        void addZoneClicked();
        void deleteZoneClicked();
        void zonesChanged();
        void initializeRanges();

    private:
        bool active;

        QDateEdit *dateEdit;
        QDoubleSpinBox *cpEdit;
        QDoubleSpinBox *ftpEdit;
        QDoubleSpinBox *wEdit;
        QDoubleSpinBox *pmaxEdit;

        Context *context;
        Zones *zones_;
        SchemePage *schemePage;
        QTreeWidget *ranges;
        QTreeWidget *zones;
        QPushButton *addButton, *updateButton, *deleteButton;
        QPushButton *addZoneButton, *deleteZoneButton, *defaultButton;
};

class ZonePage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        ZonePage(Context *);
        ~ZonePage();
        qint32 saveClicked();

    public slots:


    protected:

        Context *context;

        QTabWidget *tabs;

    private:

        static const int nSports = 2;

        QLabel *sportLabel;
        QComboBox *sportCombo;

        //ZoneScheme scheme;
        Zones *zones[nSports];
        quint16 b4Fingerprint[nSports]; // how did it start ?
        SchemePage *schemePage[nSports];
        CPPage *cpPage[nSports];

    private slots:
        void changeSport(int i);

};

//
// Heartrate Zones
//
class HrSchemePage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        HrSchemePage(HrZones *hrZones);
        HrZoneScheme getScheme();
        qint32 saveClicked();

    public slots:
        void addClicked();
        void deleteClicked();
        void renameClicked();

    private:
        HrZones *hrZones;
        QTreeWidget *scheme;
        QPushButton *addButton, *renameButton, *deleteButton;
};


class LTPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        LTPage(Context *context, HrZones *hrZones, HrSchemePage *schemePage);

    public slots:
        void addClicked();
        void editClicked();
        void deleteClicked();
        void defaultClicked();
        void rangeEdited();
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

        Context *context;
        HrZones *hrZones;
        HrSchemePage *schemePage;
        QTreeWidget *ranges;
        QTreeWidget *zones;
        QPushButton *addButton, *updateButton, *deleteButton;
        QPushButton *addZoneButton, *deleteZoneButton, *defaultButton;
};

class HrZonePage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        HrZonePage(Context *);
        ~HrZonePage();
        qint32 saveClicked();

    public slots:


    protected:

        Context *context;

        QTabWidget *tabs;

    private:

        static const int nSports = 2;

        QLabel *sportLabel;
        QComboBox *sportCombo;

        HrZones *hrZones[nSports];
        quint16 b4Fingerprint[nSports]; // how did it start ?
        HrSchemePage *schemePage[nSports];
        LTPage *ltPage[nSports];

    private slots:
        
        void changeSport(int i);

};

//
// Pace Zones
//
class PaceSchemePage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        PaceSchemePage(PaceZones* paceZones);
        PaceZoneScheme getScheme();
        qint32 saveClicked();

    public slots:
        void addClicked();
        void deleteClicked();
        void renameClicked();

    private:
        PaceZones* paceZones;
        QTreeWidget *scheme;
        QPushButton *addButton, *renameButton, *deleteButton;
};

class CVPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        CVPage(PaceZones* paceZones, PaceSchemePage *schemePage);
        QCheckBox *metric;

    public slots:
        void addClicked();
        void editClicked();
        void deleteClicked();
        void defaultClicked();
        void rangeEdited();
        void rangeSelectionChanged();
        void addZoneClicked();
        void deleteZoneClicked();
        void zonesChanged();

        void metricChanged();

    private:
        bool active;

        QDateEdit *dateEdit;
        QTimeEdit *cvEdit;

        PaceZones* paceZones;
        PaceSchemePage *schemePage;
        QTreeWidget *ranges;
        QTreeWidget *zones;
        QPushButton *addButton, *updateButton, *deleteButton;
        QPushButton *addZoneButton, *deleteZoneButton, *defaultButton;
        QLabel *per;
};

class PaceZonePage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        PaceZonePage(Context *);
        ~PaceZonePage();
        qint32 saveClicked();

    public slots:


    protected:

        Context *context;

        QTabWidget *tabs;

    private:

        static const int nSports = 2;

        QLabel *sportLabel;
        QComboBox *sportCombo;

        PaceZones* paceZones[nSports];
        quint16 b4Fingerprint[nSports]; // how did it start ?
        PaceSchemePage* schemePages[nSports];
        CVPage* cvPages[nSports];

    private slots:
        void changeSport(int i);

};

// Seasons

class SeasonsPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        SeasonsPage(QWidget *parent, Context *context);
        void getDefinitions(QList<Season>&);

    public slots:
        void addClicked();
        void upClicked();
        void downClicked();
        void renameClicked();
        void deleteClicked();
        void clearEdit();
        qint32 saveClicked();

    private:

        QTreeWidget *seasons;
        Context *context;

        QLineEdit *nameEdit;
        QComboBox *typeEdit;
        QDateEdit *fromEdit, *toEdit;
#ifndef Q_OS_MAC
        QToolButton *upButton, *downButton;
#else
        QPushButton *upButton, *downButton;
#endif
        QPushButton *addButton, *renameButton, *deleteButton;

        QList<Season> array;
};

class AutoImportPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        AutoImportPage(Context *);
        qint32 saveClicked();
        void addRuleTypes(QComboBox *p);

    public slots:

        void addClicked();
        void upClicked();
        void downClicked();
        void deleteClicked();
        void browseImportDir();


    private:

        Context *context;
        QList<RideAutoImportRule> rules;

        QTreeWidget *fields;

#ifndef Q_OS_MAC
        QToolButton *upButton, *downButton;
#else
        QPushButton *upButton, *downButton;
#endif
        QPushButton *addButton, *renameButton, *deleteButton, *browseButton;

};

class IntervalsPage : public QWidget
{
    Q_OBJECT
    G_OBJECT

    public:
        IntervalsPage(Context *context);
        qint32 saveClicked();

        unsigned int discoveryWAS;

    public slots:

    private:
        Context *context;
        int user; //index of user interval type

        QList <QCheckBox*> checkBoxes;

        struct {
            int discovery;
        } b4;

    private slots:
};

#endif
