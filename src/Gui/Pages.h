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
#include "RideAutoImportConfig.h"
#include "RemoteControl.h"
#include "Measures.h"
#include "MetricSelect.h"
#include "TagStore.h"
#include "ActionButtonBox.h"
#include "StyledItemDelegates.h"
#ifdef GC_WANT_PYTHON
#include "FixPyScriptsDialog.h"
#endif

class QGroupBox;
class QHBoxLayout;
class QVBoxLayout;
class ColorsPage;
class ZonePage;
class HrZonePage;
class PaceZonePage;
class MetadataPage;
class KeywordsPage;
class FieldsPage;
class Colors;
class AboutRiderPage;
class BackupPage;
class DevicePage;
class RemotePage;
class SimBicyclePage;

class GeneralPage : public QWidget
{
    Q_OBJECT

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
        void addPythonPathClicked();
        void removePythonPathClicked();
#endif
#ifdef GC_WANT_R
        void browseRDir();
#endif

    private:
        Context *context;

        QComboBox *langCombo;
        QComboBox* startupView;
        QComboBox *wbalForm;
        QCheckBox *garminSmartRecord;
        QCheckBox *warnOnExit;
        QCheckBox *openLastAthlete;
#ifdef GC_WANT_HTTP
        QCheckBox *startHttp;
#endif
#ifdef GC_WANT_R
        QCheckBox *embedR;
        QWidget *rDirectorySel;
#endif
#ifdef GC_WANT_PYTHON
        QCheckBox *embedPython;
        QWidget *pythonDirectorySel;
        QListWidget *pythonPathList;
        ActionButtonBox *pythonActions;
#endif
        QCheckBox *opendata;
        QSpinBox *garminHWMarkedit;
        QDoubleSpinBox *hystedit;
        QLineEdit *athleteDirectory;

#ifdef GC_WANT_PYTHON
        QLineEdit *pythonDirectory;
#endif
#ifdef GC_WANT_R
        QLineEdit *rDirectory;
#endif

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
        deviceModel *deviceListModel;
};

class TrainOptionsPage : public QWidget
{
    Q_OBJECT

    public:
        TrainOptionsPage(QWidget *parent, Context *context);
        qint32 saveClicked();

    private:
        Context     *context;
        DirectoryPathWidget *workoutDirectory;
        QCheckBox   *useSimulatedSpeed;
        QCheckBox   *useSimulatedHypoxia;
        QCheckBox   *multiCheck;
        QCheckBox   *autoConnect;
        QSpinBox    *startDelay;
        QCheckBox   *autoHide;
        QCheckBox   *lapAlert;
        QCheckBox   *coalesce;
        QCheckBox   *tooltips;
        QComboBox   *telemetryScaling;
};

class RemotePage : public QWidget
{
    Q_OBJECT

    public:
        RemotePage(QWidget *parent, Context *context);
        qint32 saveClicked();

    private:
        RemoteControl *remote;
        Context *context;
        QTreeWidget *fields;
        NoEditDelegate nativeCmdDelegate;
        ComboBoxDelegate cmdDelegate;
};

struct SimBicyclePartEntry
{
    const QString m_label;
    const char*   m_path;
    double        m_defaultValue;
    double        m_decimalPlaces;
    const QString m_unit;
    const QString m_tooltip;
};

class SimBicyclePage : public QWidget
{
    Q_OBJECT

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
        StatsTotalKEMass = 0,
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

    QString         m_LabelTextArr[LastPart];
    QDoubleSpinBox  *m_SpinBoxArr[LastPart];
    QString         m_StatsTextArr[StatsLastPart];
    QLabel          *m_StatsLabelArr[StatsLastPart];
};



class WorkoutTagManagerPage: public QWidget
{
    Q_OBJECT

public:
    WorkoutTagManagerPage(TagStore *tagStore, QWidget *parent = nullptr);
    ~WorkoutTagManagerPage();

    qint32 saveClicked();

public slots:
    void currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
    void tagStoreChanged(int idAdded, int idRemoved, int idUpdated);
    void deleteTag();
    void addTag();

    void editorClosed(QWidget *editor, QAbstractItemDelegate::EndEditHint hint);
    void modelCleaner();

private:
    TagStore *tagStore;
    QTreeWidget *tw;
    QList<int> deleted;

    UniqueLabelEditDelegate labelEditDelegate;
    NoEditDelegate numDelegate;
};



class CustomMetricsPage : public QWidget
{
    Q_OBJECT

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

#ifdef GC_HAS_CLOUD_DB
        QPushButton *uploadButton;
        QPushButton *downloadButton;
#endif
        QPushButton *exportButton;
        QPushButton *importButton;
        QTreeWidget *table;
        QList<UserMetricSettings> metrics;

        int skipcompat;

        struct {
            quint16 crc;
        } b4;
};

class FavouriteMetricsPage : public QWidget
{
    Q_OBJECT

    public:
        FavouriteMetricsPage(QWidget *parent = NULL);

    public slots:
        qint32 saveClicked();

    protected:
        bool changed = false;
        MultiMetricSelector *multiMetricSelector;
};

class KeywordsPage : public QWidget
{
    Q_OBJECT

    public:
        KeywordsPage(MetadataPage *parent, QList<KeywordDefinition>);
        void getDefinitions(QList<KeywordDefinition>&);
        QCheckBox *rideBG;

    public slots:
        void addClicked();
        void upClicked();
        void downClicked();
        void deleteClicked();

        void pageSelected(); // reset the list of fields when we are selected...
        void colorfieldChanged();

    private:
        QTreeWidget *keywords;
        ActionButtonBox *actionButtons;

        QLabel *fieldLabel;
        QComboBox *fieldChooser;
        ListEditDelegate relatedDelegate;

        MetadataPage *parent;
};

class IconsPage : public QWidget
{
    Q_OBJECT

    public:
        IconsPage(const QList<FieldDefinition> &fieldDefinitions, QWidget *parent = nullptr);
        qint32 saveClicked();

    public slots:

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;

    private:
        QList<FieldDefinition> fieldDefinitions;
        QTreeWidget *sportTree;
        QListWidget *iconList;
        QLabel *trash;
        QIcon trashIcon;
        QPoint sportTreeDragStartPos;
        bool sportTreeDragWatch = false;
        QPoint iconListDragStartPos;
        bool iconListDragWatch = false;

        bool eventFilterTrash(QEvent *event);
        bool eventFilterSportTree(QEvent *event);
        bool eventFilterSportTreeViewport(QEvent *event);
        bool eventFilterIconList(QEvent *event);
        bool eventFilterIconListViewport(QEvent *event);

        void initSportTree();
        void updateIconList();
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
        void applyThemeIndex(int index);
        void tabChanged();

        void scaleFont();
        void searchFilter(QString);

        void resetClicked();

    private:

        // General stuff
        QCheckBox *antiAliased;
#ifndef Q_OS_MAC // they do scrollbars nicely
        QCheckBox *macForms;
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
            bool alias;
#ifndef Q_OS_MAC
            bool macForms, scroll, head;
#endif
            double line;
            double fontscale;
            unsigned long fingerprint;
        } b4;
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
        void deleteClicked();

    private:
        QTreeWidget *fields;
        ActionButtonBox *actionButtons;
        CompleterEditDelegate tabDelegate;
        UniqueLabelEditDelegate fieldDelegate;
        ComboBoxDelegate fieldTypeDelegate;
        ListEditDelegate valueDelegate;
};

class ProcessorPage : public QWidget
{
    Q_OBJECT

    public:
        ProcessorPage(Context *context);
        qint32 saveClicked();

    private slots:
        void processorSelected(QTreeWidgetItem *selectedItem);
        void reload();
        void reload(const QString &selectName);
        void reload(int selectRow);
#ifdef GC_WANT_PYTHON
        void addProcessor();
        void delProcessor();
        void editProcessor();
        void dblClickProcessor(QTreeWidgetItem *item, int col);
        void toggleCoreProcessors(bool checked);
#endif
        void automationChanged(int index);
        void toggleAutomatedOnly(bool checked);
        void dataChanged(const QModelIndex &topLeft);

    protected:
        Context *context;

        QTreeWidget *processorTree;

    private:
        NoEditDelegate processorDelegate;
        ComboBoxDelegate automationDelegate;
        QList<QComboBox*> automationCombos;
        QList<QCheckBox*> automatedCheckBoxes;
        QList<DataProcessorConfig*> configs;

#ifdef GC_WANT_PYTHON
        ActionButtonBox *actionButtons = nullptr;
        QCheckBox *hideButton = nullptr;
#endif

        QStackedWidget *settingsStack;
};


class DefaultsPage : public QWidget
{
    Q_OBJECT

    public:
        DefaultsPage(MetadataPage *parent, QList<DefaultDefinition>);
        void getDefinitions(QList<DefaultDefinition>&);

    public slots:
        void addClicked();
        void upClicked();
        void downClicked();
        void deleteClicked();

    protected:
        QTreeWidget *defaults;
        MetadataPage *parent;
        CompleterEditDelegate fieldDelegate;
        CompleterEditDelegate linkedDelegate;

        bool eventFilter(QObject *obj, QEvent *event) override;
};

class MetadataPage : public QWidget
{
    Q_OBJECT
    G_OBJECT

    friend class ::KeywordsPage;
    friend class ::DefaultsPage;

    public:

        MetadataPage(Context *);
        qint32 saveClicked();

    public slots:


    protected:

        Context *context;
        bool changed;

        QTabWidget *tabs;
        KeywordsPage *keywordsPage;
        IconsPage *iconsPage;
        FieldsPage *fieldsPage;
        DefaultsPage *defaultsPage;
        ProcessorPage *processorPage;

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

    public:
        MeasuresConfigPage(QWidget *parent, Context *context);
        ~MeasuresConfigPage();
        qint32 saveClicked();

    public slots:
        void resetMeasuresClicked();

    private:
        Context *context;
        Measures *measures;

        QTreeWidget *measuresTable;
        UniqueLabelEditDelegate meNameDelegate;
        UniqueLabelEditDelegate meSymbolDelegate;

        QLabel *mflabel;
        QTreeWidget *measuresFieldsTable;
        UniqueLabelEditDelegate meFiNameDelegate;
        UniqueLabelEditDelegate meFiSymbolDelegate;
        DoubleSpinBoxEditDelegate meFiFactorDelegate;
        ListEditDelegate meFiHeaderDelegate;

        void refreshMeasuresTable();
        void refreshMeasuresFieldsTable();

    private slots:
        void measuresSelected();
        void measureChanged(const QModelIndex &topLeft);

        void addMeasuresClicked();
        void removeMeasuresClicked();

        void measureFieldChanged(const QModelIndex &topLeft);
        void addMeasuresFieldClicked();
        void removeMeasuresFieldClicked();
};

#endif
