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
class ZonePage;
class HrZonePage;
class PaceZonePage;
class SummaryMetricsPage;
class MetadataPage;
class KeywordsPage;
class FieldsPage;
class MeasuresPage;
class Colors;
class RiderPage;
class SeasonsPage;

class GeneralPage : public QWidget
{
    Q_OBJECT
    G_OBJECT

    public:
        GeneralPage(Context *context);
        void saveClicked();

        QString athleteWAS; // remember what we started with !

    public slots:
        void browseWorkoutDir();
        void browseAthleteDir();

    private:
        Context *context;

        QComboBox *langCombo;
        QComboBox *unitCombo;
        QComboBox *crankLengthCombo;
        QComboBox *wheelSizeCombo;
        QComboBox *wbalForm;
        QCheckBox *garminSmartRecord;
        QLineEdit *garminHWMarkedit;
        QLineEdit *hystedit;
        QLineEdit *athleteDirectory;
        QLineEdit *workoutDirectory;
        QPushButton *workoutBrowseButton;
        QPushButton *athleteBrowseButton;

        QLabel *langLabel;
        QLabel *unitLabel;
        QLabel *warningLabel;
        QLabel *workoutLabel;
        QLabel *athleteLabel;

        QLabel *perfManSTSLabel;
        QLabel *perfManLTSLabel;
        QLineEdit *perfManSTSavg;
        QLineEdit *perfManLTSavg;
        QCheckBox *showSBToday;
        QIntValidator *perfManSTSavgValidator;
        QIntValidator *perfManLTSavgValidator;
};

class RiderPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        RiderPage(QWidget *parent, Context *context);
        void saveClicked();

    public slots:
        void chooseAvatar();
        void unitChanged(int currentIndex);

    private:
        Context *context;

        QLineEdit *nickname;
        QDateEdit *dob;
        QComboBox *sex;
        QLabel *weightlabel;
        QLabel *heightlabel;
        QLabel *wbaltaulabel;
        QComboBox *unitCombo;
        QDoubleSpinBox *weight;
        QDoubleSpinBox *height;
        QSpinBox *wbaltau;
        QTextEdit  *bio;
        QPushButton *avatarButton;
        QPixmap     avatar;

};

class CredentialsPage : public QScrollArea
{
    Q_OBJECT
    G_OBJECT


    public:
        CredentialsPage(QWidget *parent, Context *context);
        void saveClicked();

    public slots:
#ifdef GC_HAVE_LIBOAUTH
        void authoriseTwitter();
        void authoriseStrava();
        void authoriseCyclingAnalytics();
#endif

    private:
        Context *context;

        QLineEdit *tpURL; // url for training peaks.com ... http://www.trainingpeaks.com
        QLineEdit *tpUser;
        QLineEdit *tpPass;
        QComboBox *tpType;
        QPushButton *tpTest;

        QLineEdit *gcURL; // url for gc racing (not available yet)
        QLineEdit *gcUser;
        QLineEdit *gcPass;

#ifdef GC_HAVE_LIBOAUTH
        QLineEdit *twitterURL; // url for twitter.com
        QPushButton *twitterAuthorise;
        QLineEdit *twitterPIN;
        char *t_key, *t_secret;

        QPushButton *stravaAuthorise, *stravaAuthorised, *twitterAuthorised;
        char *s_id, *s_secret;

        QPushButton *cyclingAnalyticsAuthorise, *cyclingAnalyticsAuthorised;
#endif

        QLineEdit *rideWithGPSUser;
        QLineEdit *rideWithGPSPass;

        QLineEdit *ttbUser;
        QLineEdit *ttbPass;

        QLineEdit *veloHeroUser;
        QLineEdit *veloHeroPass;

        QLineEdit *selUser;
        QLineEdit *selPass;

        QLineEdit *wiURL; // url for withings
        QLineEdit *wiUser;
        QLineEdit *wiPass;

        QLineEdit *zeoURL; // url for myzeo
        QLineEdit *zeoUser;
        QLineEdit *zeoPass;

        QLineEdit *webcalURL; // url for webcal calendar (read only, TP.com, Google Calendar)

        QLineEdit *dvURL; // url for calDAV calendar (read/write, e.g. Google, Hotmail)
        QLineEdit *dvUser;
        QLineEdit *dvPass;
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
 };

class DevicePage : public QWidget
{
    Q_OBJECT
    G_OBJECT

    public:
        DevicePage(QWidget *, Context *);
        void saveClicked();

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
        void saveClicked();

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
        void saveClicked();

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
        void saveClicked();

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
        void saveClicked();

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
        void saveClicked();

    public slots:

        //void upClicked();
        //void downClicked();

    protected:

        Context *context;
        QMap<QString, DataProcessor*> processors;

        QTreeWidget *processorTree;
        //QPushButton *upButton, *downButton;

};

class MetadataPage : public QWidget
{
    Q_OBJECT
    G_OBJECT

    friend class ::KeywordsPage;

    public:

        MetadataPage(Context *);
        void saveClicked();

    public slots:


    protected:

        Context *context;
        bool changed;

        QTabWidget *tabs;
        KeywordsPage *keywordsPage;
        FieldsPage *fieldsPage;
        ProcessorPage *processorPage;

        // local versions for modification
        QList<KeywordDefinition> keywordDefinitions;
        QList<FieldDefinition>   fieldDefinitions;
        QString colorfield;
};

//
// Power Zones
//
class SchemePage : public QWidget
{
    Q_OBJECT
    G_OBJECT


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
    G_OBJECT


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
        QDoubleSpinBox *wEdit;

        ZonePage *zonePage;
        QTreeWidget *ranges;
        QTreeWidget *zones;
        QPushButton *addButton, *deleteButton, *defaultButton;
        QPushButton *addZoneButton, *deleteZoneButton;
};

class ZonePage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        ZonePage(Context *);
        void saveClicked();

        //ZoneScheme scheme;
        Zones zones;

        // Children talk to each other
        SchemePage *schemePage;
        CPPage *cpPage;

    public slots:


    protected:

        Context *context;
        bool changed;

        QTabWidget *tabs;

        // local versions for modification
};

//
// Heartrate Zones
//
class HrSchemePage : public QWidget
{
    Q_OBJECT
    G_OBJECT


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
    G_OBJECT


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
    G_OBJECT


public:

    HrZonePage(Context *);
    void saveClicked();

    //ZoneScheme scheme;
    HrZones zones;

    // Children talk to each other
    HrSchemePage *schemePage;
    LTPage *ltPage;

    public slots:


protected:

    Context *context;
    bool changed;

    QTabWidget *tabs;

    // local versions for modification
};

//
// Pace Zones
//
class PaceSchemePage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        PaceSchemePage(PaceZonePage *parent);
        PaceZoneScheme getScheme();
        void saveClicked();

    public slots:
        void addClicked();
        void deleteClicked();
        void renameClicked();

    private:
        PaceZonePage *zonePage;
        QTreeWidget *scheme;
        QPushButton *addButton, *renameButton, *deleteButton;
};

class CVPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        CVPage(PaceZonePage *parent);
        QCheckBox *metric;

    public slots:
        void addClicked();
        void deleteClicked();
        void defaultClicked();
        void rangeSelectionChanged();
        void addZoneClicked();
        void deleteZoneClicked();
        void zonesChanged();

        void metricChanged();

    private:
        bool active;

        QDateEdit *dateEdit;
        QTimeEdit *cvEdit;

        PaceZonePage *zonePage;
        QTreeWidget *ranges;
        QTreeWidget *zones;
        QPushButton *addButton, *deleteButton, *defaultButton;
        QPushButton *addZoneButton, *deleteZoneButton;
        QLabel *per;
};

class PaceZonePage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        PaceZonePage(Context *);
        void saveClicked();

        PaceZones zones;

        // Children talk to each other
        PaceSchemePage *schemePage;
        CVPage *cvPage;

    public slots:


    protected:

        Context *context;
        bool changed;

        QTabWidget *tabs;

        // local versions for modification
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
        void saveClicked();

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

class MeasuresPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        MeasuresPage(Context *context);
        void getDefinitions(QList<FieldDefinition>&);

    public slots:
        void addClicked();
        void upClicked();
        void downClicked();
        void renameClicked();
        void deleteClicked();
        void saveClicked();

    private:

        Context *context;
        QTreeWidget *fields;
#ifndef Q_OS_MAC
        QToolButton *upButton, *downButton;
#else
        QPushButton *upButton, *downButton;
#endif
        QPushButton *addButton, *renameButton, *deleteButton;
};

class AutoImportPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        AutoImportPage(Context *);
        void saveClicked();
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


#endif
