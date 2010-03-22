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
#include <QLabel>
#include <QDateEdit>
#include <QCheckBox>
#include <QValidator>
#include <QGridLayout>
#include <QProgressDialog>
#include "DeviceTypes.h"
#include "DeviceConfiguration.h"
#include "RideMetadata.h"

class QGroupBox;
class QHBoxLayout;
class QVBoxLayout;
class ColorsPage;
class IntervalMetricsPage;
class MetadataPage;
class KeywordsPage;
class FieldsPage;
class Colors;

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
        QLineEdit *BSdaysEdit;
        QComboBox *bsModeCombo;
        QLineEdit *workoutDirectory;
        QPushButton *workoutBrowseButton;

    public slots:
        void browseWorkoutDir();

    private:
        MainWindow *main;
        ColorsPage *colorsPage;
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
};

class CyclistPage : public QWidget
{
    Q_OBJECT
    public:
        CyclistPage(const Zones *_zones);
        int thresholdPower;
        QString getText();
        int getCP();
	void setCP(int cp);
	void setSelectedDate(QDate date);
        void setCurrentRange(int range = -1);
        QPushButton *btnBack;
        QPushButton *btnForward;
        QPushButton *btnDelete;
        QCheckBox *checkboxNew;
        QCalendarWidget *calendar;
        QLabel *lblCurRange;
        QLabel *txtStartDate;
        QLabel *txtEndDate;
        QLabel *lblStartDate;
        QLabel *lblEndDate;
        QLabel *perfManLabel;
        QLabel *perfManStartLabel;
        QLabel *perfManSTSLabel;
        QLabel *perfManLTSLabel;
        QLineEdit *perfManStart;
        QLineEdit *perfManSTSavg;
        QLineEdit *perfManLTSavg;
        QCheckBox *showSBToday;

        int getCurrentRange();
        bool isNewMode();

        inline void setCPFocus() {
            txtThreshold->setFocus();
        }

        inline QDate selectedDate() {
            return calendar->selectedDate();
        }

    private:
        QGroupBox *cyclistGroup;
        const Zones *zones;
        int currentRange;
        QLabel *lblThreshold;
        QLineEdit *txtThreshold;
        QIntValidator *txtThresholdValidator;
        QVBoxLayout *perfManLayout;
        QHBoxLayout *perfManStartValLayout;
        QHBoxLayout *perfManSTSavgLayout;
        QHBoxLayout *perfManLTSavgLayout;
        QHBoxLayout *powerLayout;
        QHBoxLayout *rangeLayout;
        QHBoxLayout *dateRangeLayout;
        QHBoxLayout *zoneLayout;
        QHBoxLayout *calendarLayout;
        QVBoxLayout *cyclistLayout;
        QVBoxLayout *mainLayout;
        QIntValidator *perfManStartValidator;
        QIntValidator *perfManSTSavgValidator;
        QIntValidator *perfManLTSavgValidator;
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

        QTreeWidget *fields;

        QPushButton *upButton, *downButton, *addButton, *renameButton, *deleteButton;
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

        // local versions for modification
        QList<KeywordDefinition> keywordDefinitions;
        QList<FieldDefinition>   fieldDefinitions;
};

#endif
