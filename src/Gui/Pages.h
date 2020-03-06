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
#include "BodyMeasures.h"
#include "HrvMeasures.h"

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
class BackupPage;
class SeasonsPage;
class DevicePage;
class RemotePage;
class SimBicyclePage;

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
#ifdef GC_WANT_PYTHON
        void browsePythonDir();
        void embedPythonchanged(int);
#endif
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
#ifdef GC_WANT_PYTHON
        QCheckBox *embedPython;
#endif
#if QT_VERSION >= 0x050000
        QCheckBox *opendata;
#endif
        QLineEdit *garminHWMarkedit;
        QLineEdit *hystedit;
        QLineEdit *athleteDirectory;
        QLineEdit *workoutDirectory;
        QPushButton *workoutBrowseButton;
        QPushButton *athleteBrowseButton;

#ifdef GC_WANT_PYTHON
        QPushButton *pythonBrowseButton;
        QLineEdit *pythonDirectory;
        QLabel *pythonLabel;
#endif
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
        void addOReditClicked();
        void deleteClicked();
        void rangeEdited();
        void rangeSelectionChanged();

    private:
        Context *context;
        bool metricUnits;
        QList<BodyMeasure> bodyMeasures;

        QLabel *defaultWeightlabel;
        QDoubleSpinBox *defaultWeight;

        QLabel *dateLabel;
        QDateTimeEdit *dateTimeEdit;
        QLabel *weightlabel;
        QDoubleSpinBox *weight;
        QLabel *fatkglabel;
        QDoubleSpinBox *fatkg;
        QLabel *musclekglabel;
        QDoubleSpinBox *musclekg;
        QLabel *boneskglabel;
        QDoubleSpinBox *boneskg;
        QLabel *leankglabel;
        QDoubleSpinBox *leankg;
        QLabel *fatpercentlabel;
        QDoubleSpinBox *fatpercent;
        QLabel *commentlabel;
        QLineEdit *comment;

        QTreeWidget *bmTree;
        QPushButton *addButton, *updateButton, *deleteButton;

    struct {
        double defaultWeight;
        unsigned long fingerprint;
    } b4;

    private slots:
};

class HrvPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        HrvPage(QWidget *parent, Context *context);
        qint32 saveClicked();

    public slots:
        void addOReditClicked();
        void deleteClicked();
        void rangeEdited();
        void rangeSelectionChanged();

    private:
        Context *context;
        QList<HrvMeasure> hrvMeasures;

        QLabel *dateLabel;
        QDateTimeEdit *dateTimeEdit;
        QLabel *rmssdlabel;
        QDoubleSpinBox *rmssd;
        QLabel *hrlabel;
        QDoubleSpinBox *hr;
        QLabel *avnnlabel;
        QDoubleSpinBox *avnn;
        QLabel *sdnnlabel;
        QDoubleSpinBox *sdnn;
        QLabel *pnn50label;
        QDoubleSpinBox *pnn50;
        QLabel *lflabel;
        QDoubleSpinBox *lf;
        QLabel *hflabel;
        QDoubleSpinBox *hf;
        QLabel *recovery_pointslabel;
        QDoubleSpinBox *recovery_points;
        QLabel *commentlabel;
        QLineEdit *comment;

        QTreeWidget *hrvTree;
        QPushButton *addButton, *updateButton, *deleteButton;

    struct {
        unsigned long fingerprint;
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
        void unitChanged(int currentIndex);

    private:
        Context *context;
        bool metricUnits;

        QLineEdit *nickname;
        QDateEdit *dob;
        QComboBox *sex;
        QLabel *heightlabel;
        QDoubleSpinBox *height;
        QPushButton *avatarButton;
        QPixmap     avatar;
        QComboBox *crankLengthCombo;
        QComboBox *rimSizeCombo;
        QComboBox *tireSizeCombo;
        QLineEdit *wheelSizeEdit;


    struct {
        double height;
        int wheel;
        int crank;
    } b4;

    private slots:
        void calcWheelSize();
        void resetWheelSize();
};

class AboutModelPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        AboutModelPage(Context *context);
        qint32 saveClicked();

    private:
        Context *context;

        QLabel *wbaltaulabel;
        QSpinBox *wbaltau;
        QLabel *perfManSTSLabel;
        QLabel *perfManLTSLabel;
        QLineEdit *perfManSTSavg;
        QLineEdit *perfManLTSavg;
        QIntValidator *perfManSTSavgValidator;
        QIntValidator *perfManLTSavgValidator;
        QCheckBox *showSBToday;

    struct {
        int lts,sts;
    } b4;

};

class BackupPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        BackupPage(Context *context);
        qint32 saveClicked();

    private:
        Context *context;

        QSpinBox *autoBackupPeriod;
        QLineEdit *autoBackupFolder;
        QPushButton *autoBackupFolderBrowse;

    private slots:

        void chooseAutoBackupFolder();
};

class CredentialsPage : public QScrollArea
{
    Q_OBJECT
    G_OBJECT


    public:
        CredentialsPage(Context *context);

        qint32 saveClicked();
        void resetList();

    public slots:
        void addClicked();
        void deleteClicked();
        void editClicked();


    private:

        Context *context;
        QTreeWidget *accounts;
        QPushButton *addButton, *editButton, *deleteButton;

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
};

class TrainOptionsPage : public QWidget
{
    Q_OBJECT
    G_OBJECT

    public:
        TrainOptionsPage(QWidget *parent, Context *context);
        qint32 saveClicked();

    private:
        Context     *context;
        QCheckBox   *useSimulatedSpeed;
        QCheckBox   *multiCheck;
        QCheckBox   *autoConnect;
        QCheckBox   *autoHide;
        QCheckBox   *lapAlert;
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

struct SimBicyclePartEntry
{
    const char* m_label;
    const char* m_path;
    double      m_defaultValue;
    double      m_decimalPlaces;
    const char* m_tooltip;
};

class SimBicyclePage : public QWidget
{
    Q_OBJECT
    G_OBJECT

public:
    SimBicyclePage(QWidget *parent, Context *context);
    qint32 saveClicked();

    // Order of this enum must be synced with static array in GetSimBicyclePartEntry.
    enum BicycleParts {
        BicycleWithoutWheelsG = 0,
        FrontWheelG, FrontSpokeCount, FrontSpokeNippleG, FrontRimG, FrontRotorG, FrontSkewerG, FrontTireG, FrontTubeSealantG, FrontOuterRadiusM, FrontRimInnerRadiusM,
        RearWheelG, RearSpokeCount, RearSpokeNippleG, RearRimG, RearRotorG, RearSkewerG, RearTireG, RearTubeSealantG, RearOuterRadiusM, RearRimInnerRadiusM,
        CassetteG, CRR, Cm, Cd, Am2, Tk,
        LastPart
    };

    enum BicycleStats {
        StatsLabel = 0,
        StatsTotalKEMass,
        StatsFrontWheelKEMass,
        StatsFrontWheelMass,
        StatsFrontWheelEquivMass,
        StatsFrontWheelI,
        StatsRearWheelKEMass,
        StatsRearWheelMass,
        StatsRearWheelEquivMass,
        StatsRearWheelI,
        StatsLastPart
    };

    static const SimBicyclePartEntry& GetSimBicyclePartEntry(int e);

    static double                     GetBicyclePartValue(Context *context, int e);

public slots:
    void SetStatsLabelArray(double d = 0.);

private:
    void AddSpecBox(int ePart);


    Context         *context;

    QLabel          *m_LabelArr  [LastPart];
    QDoubleSpinBox  *m_SpinBoxArr[LastPart];
    QLabel          *m_StatsLabelArr[StatsLastPart];
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
#ifdef GC_HAS_CLOUD_DB
        void uploadClicked();
        void downloadClicked();
#endif
        void exportClicked();
        void importClicked();
        void doubleClicked(QTreeWidgetItem *item, int column);

    protected:
        Context *context;

        QPushButton *addButton,
                    *deleteButton,
                    *editButton,
#ifdef GC_HAS_CLOUD_DB
                    *uploadButton,
                    *downloadButton,
#endif
                    *exportButton,
                    *importButton;
        QTreeWidget *table;
        QList<UserMetricSettings> metrics;

        int skipcompat;

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

        void scaleFont();

    private:

        // General stuff
        QCheckBox *antiAliased;
#ifndef Q_OS_MAC // they do scrollbars nicely
        QCheckBox *rideScroll;
        QCheckBox *rideHead;
#endif
        QDoubleSpinBox *lineWidth;

        // Fonts
        QFontComboBox *def;
        QSlider *fontscale;
        QLabel *fonttext;

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
            double fontscale;
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
