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
#include "Measures.h"

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
        QCheckBox *metricRunPace, *metricSwimPace;

    public slots:
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
        QCheckBox *openLastAthlete;
#ifdef GC_WANT_HTTP
        QCheckBox *startHttp;
#endif
#ifdef GC_WANT_R
        QCheckBox *embedR;
#endif
#ifdef GC_WANT_PYTHON
        QCheckBox *embedPython;
#endif
        QCheckBox *opendata;
        QLineEdit *garminHWMarkedit;
        QLineEdit *hystedit;
        QLineEdit *athleteDirectory;
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
        QLabel *athleteLabel;

        struct {
            int unit;
            bool metricRunPace, metricSwimPace;
            float hyst;
            int wbal;
            bool warn;
#ifdef GC_WANT_HTTP
            bool starthttp;
#endif
        } b4;


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

    public slots:
        void browseWorkoutDir();

    private:
        Context     *context;
        QLabel      *workoutLabel;
        QLineEdit   *workoutDirectory;
        QPushButton *workoutBrowseButton;
        QCheckBox   *useSimulatedSpeed;
        QCheckBox   *useSimulatedHypoxia;
        QCheckBox   *multiCheck;
        QCheckBox   *autoConnect;
        QLabel      *delayLabel;
        QSpinBox    *startDelay;
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
    const QString m_label;
    const char*   m_path;
    double        m_defaultValue;
    double        m_decimalPlaces;
    const QString m_tooltip;
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
        ActualTrainerAltitudeM,
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
        void searchFilter(QString);

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
        QLineEdit *searchEdit;
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

class MeasuresConfigPage : public QWidget
{
    Q_OBJECT
    G_OBJECT

    public:
        MeasuresConfigPage(QWidget *parent, Context *context);
        ~MeasuresConfigPage();
        qint32 saveClicked();

    public slots:

    private:
        Context *context;
        Measures *measures;

        QTreeWidget *measuresTable;
        QTreeWidget *measuresFieldsTable;

        QPushButton *resetMeasures, *editMeasures, *addMeasures, *removeMeasures;
        QPushButton *editMeasuresField, *addMeasuresField, *removeMeasuresField;

        void refreshMeasuresTable();
        void refreshMeasuresFieldsTable();

    private slots:
        void measuresSelected();
        void measuresDoubleClicked(QTreeWidgetItem *item, int column);
        void measuresFieldDoubleClicked(QTreeWidgetItem *item, int column);

        void resetMeasuresClicked();
        void editMeasuresClicked();
        void addMeasuresClicked();
        void removeMeasuresClicked();

        void editMeasuresFieldClicked();
        void addMeasuresFieldClicked();
        void removeMeasuresFieldClicked();
};

class MeasuresSettingsDialog : public QDialog
{
    Q_OBJECT

    public:
        MeasuresSettingsDialog(QWidget *parent, QString &symbol, QString &name);

    private slots:
        void okClicked();

    private:
        QString &symbol, &name;

        QLineEdit *symbolEdit;
        QLineEdit *nameEdit;

        QPushButton *cancelButton, *okButton;

};

class MeasuresFieldSettingsDialog : public QDialog
{
    Q_OBJECT

    public:
        MeasuresFieldSettingsDialog(QWidget *parent, MeasuresField &field);

    private slots:
        void okClicked();

    private:
        MeasuresField &field;
        QLineEdit *symbolEdit;
        QLineEdit *nameEdit;
        QLineEdit *metricUnitsEdit;
        QLineEdit *imperialUnitsEdit;
        QDoubleSpinBox *unitsFactorEdit;
        QLineEdit *headersEdit;

        QPushButton *cancelButton, *okButton;
};

#endif
