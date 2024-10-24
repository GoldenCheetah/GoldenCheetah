/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
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

#ifndef ATHLETEPAGES_H
#define ATHLETEPAGES_H
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
#include <QDialogButtonBox>
#include "DeviceTypes.h"
#include "DeviceConfiguration.h"
#include "RideMetadata.h"
#include "DataProcessor.h"
#include "Season.h"
#include "RideAutoImportConfig.h"
#include "RemoteControl.h"
#include "Measures.h"
#include "StyledItemDelegates.h"

class MeasuresPage : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        MeasuresPage(QWidget *parent, Context *context, MeasuresGroup *measuresGroup);
        qint32 saveClicked();

    public slots:
        void unitChanged(int currentIndex);
        void addOReditClicked();
        void deleteClicked();
        void rangeEdited();
        void rangeSelectionChanged();

    private:
        Context *context;
        MeasuresGroup *measuresGroup;
        bool metricUnits;
        QList<Measure> measures;

        QLabel *dateLabel;
        QDateTimeEdit *dateTimeEdit;

        QVector<QLabel*> valuesLabel;
        QVector<QDoubleSpinBox*> valuesEdit;

        QLabel *commentlabel;
        QLineEdit *comment;

        QTreeWidget *measuresTree;
        QPushButton *addButton, *updateButton, *deleteButton;

    struct {
        unsigned long fingerprint;
    } b4;

    private slots:
};

class WheelSizeCalculator : public QDialog
{
    Q_OBJECT

    public:
        WheelSizeCalculator(QWidget *parent = nullptr);

        int getWheelSize() const;

    private:
        QComboBox *rimSizeCombo;
        QComboBox *tireSizeCombo;
        QLabel *wheelSizeLabel;
        QDialogButtonBox *buttonBox;

        int wheelSize = -1;

    private slots:
        void calc();
};

class AboutRiderPage : public QWidget
{
    Q_OBJECT

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
        QDoubleSpinBox *weight;
        QDoubleSpinBox *height;
        QPushButton *avatarButton;
        QPixmap avatar;
        QComboBox *crankLengthCombo;
        QSpinBox *wheelSizeEdit;
        QPushButton *wheelSizeCalculatorButton;

    struct {
        double weight;
        double height;
        int wheel;
        int crank;
    } b4;

    private slots:
        void openWheelSizeCalculator();
};

class AboutModelPage : public QWidget
{
    Q_OBJECT

    public:
        AboutModelPage(Context *context);
        qint32 saveClicked();

    private:
        Context *context;

        QSpinBox *wbaltau;
        QSpinBox *perfManSTSavg;
        QSpinBox *perfManLTSavg;
        QCheckBox *showSBToday;
        QPushButton *resetButton;

        static const int defaultWbaltau = 300;
        static const int defaultSTSavg = 7;
        static const int defaultLTSavg = 42;
        static const int defaultSBToday = 0;

    struct {
        int lts,sts;
    } b4;

    private slots:
        void restoreDefaults();
        void anyChanged();
};

class BackupPage : public QWidget
{
    Q_OBJECT

    public:
        BackupPage(Context *context);
        qint32 saveClicked();

    private:
        Context *context;

        QSpinBox *autoBackupPeriod;
        DirectoryPathWidget *autoBackupFolder;

    private slots:
        void backupNow();
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

//
// Power Zones
//
class SchemePage : public QWidget
{
    Q_OBJECT

    public:
        SchemePage(Zones *zones);

        ZoneScheme getScheme();
        qint32 saveClicked();

    private slots:
        void addClicked();
        void deleteClicked();
        void updateButtons();

    private:
        Zones *zones;
        QTreeWidget *scheme;
        QPushButton *addButton, *deleteButton;
        SpinBoxEditDelegate zoneFromDelegate;
};


// Compatibility helper for Qt5
// exposes methods that turned public in Qt6 from protected in Qt5
#if QT_VERSION < 0x060000
class TreeWidget6 : public QTreeWidget
{
    Q_OBJECT

    public:
        TreeWidget6(QWidget *parent = nullptr): QTreeWidget(parent) {
        }

        QModelIndex indexFromItem(const QTreeWidgetItem *item, int column = 0) const {
            return QTreeWidget::indexFromItem(item, column);
        }

        QTreeWidgetItem* itemFromIndex(const QModelIndex &index) const {
            return QTreeWidget::itemFromIndex(index);
        }

};
#else
typedef QTreeWidget TreeWidget6;
#endif


class CPPage : public QWidget
{
    Q_OBJECT

    public:
        CPPage(Context *context, Zones *zones, SchemePage *schemePage);

        qint32 saveClicked();

        QComboBox *useModel;
        QComboBox *useCPForFTPCombo;

        struct {
            int modelIdx;
            int cpforftp;
        } b4;

    public slots:
        void addClicked();
        void deleteClicked();
        void defaultClicked();
        void rangeSelectionChanged();
        void addZoneClicked();
        void deleteZoneClicked();
        void zonesChanged();
        void initializeRanges(int selectIndex = -1);
        void reInitializeRanges();

    private slots:
#if QT_VERSION < 0x060000
        void rangeChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
#else
        void rangeChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());
#endif

        void review();
        void updateButtons();

    private:
        bool active;

        DateEditDelegate dateDelegate;
        SpinBoxEditDelegate cpDelegate;
        SpinBoxEditDelegate aetDelegate;
        SpinBoxEditDelegate ftpDelegate;
        SpinBoxEditDelegate wDelegate;
        SpinBoxEditDelegate pmaxDelegate;
        NoEditDelegate statusDelegate;

        SpinBoxEditDelegate zoneFromDelegate;

        Context *context;
        Zones *zones_;
        SchemePage *schemePage;
        TreeWidget6 *ranges;
        QTreeWidget *zones;
        QPushButton *addButton, *deleteButton;
        QPushButton *reviewButton;
        QPushButton *addZoneButton, *deleteZoneButton, *defaultButton;
        QPushButton *newZoneRequired;

        bool getValuesFor(const QDate &date, bool allowDefaults, int &cp, int &aetp, int &ftp, int &wprime, int &pmax, int &estOffset, bool &defaults, QDate *startDate = nullptr) const;
        void setEstimateStatus(QTreeWidgetItem *item);
        void setRangeData(QModelIndex modelIndex, int column, QVariant data);
        bool needsNewRange() const;
        void mkReviewRow(QGridLayout *grid, int row, int labelId, const QString &unit, int cur, int est, QCheckBox *&accept, const QString &infoText = QString()) const;
        void connectReviewDialogApplyButton(QVector<QCheckBox*> checkboxes, QPushButton *applyButton) const;
        bool modelHasFtp() const;
        bool modelHasPmax() const;
        QWidget *mkInfoLabel(int labelId, const QString &infoText = QString()) const;
        QString getText(int id, int value = 0) const;
        bool addDialogManual(QDate &date, int &cp, int &aetp, int &wprime, int &ftp, int &pmax) const;
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

        QLabel *sportLabel;
        QComboBox *sportCombo;

        //ZoneScheme scheme;
        QHash<QString, Zones*> zones;
        QHash<QString, quint16> b4Fingerprint; // how did it start ?
        QHash<QString, SchemePage*> schemePage;
        QHash<QString, CPPage*> cpPage;

    private slots:
        void changeSport();

};


//
// Heartrate Zones
//
class HrSchemePage : public QWidget
{
    Q_OBJECT

    public:
        HrSchemePage(HrZones *hrZones);

        HrZoneScheme getScheme();
        qint32 saveClicked();

    private slots:
        void addClicked();
        void deleteClicked();
        void updateButtons();

    private:
        HrZones *hrZones;
        QTreeWidget *scheme;
        QPushButton *addButton, *deleteButton;

        SpinBoxEditDelegate ltDelegate;
        DoubleSpinBoxEditDelegate trimpkDelegate;
};


class LTPage : public QWidget
{
    Q_OBJECT

    public:
        LTPage(Context *context, HrZones *hrZones, HrSchemePage *schemePage);

    private slots:
        void addClicked();
        void deleteClicked();
        void defaultClicked();
        void rangeChanged(const QModelIndex &modelIndex);
        void rangeSelectionChanged();
        void addZoneClicked();
        void deleteZoneClicked();
        void zonesChanged();
        void updateButtons();

    private:
        bool active;

        DateEditDelegate dateDelegate;
        SpinBoxEditDelegate ltDelegate;
        SpinBoxEditDelegate aetDelegate;
        SpinBoxEditDelegate restHrDelegate;
        SpinBoxEditDelegate maxHrDelegate;

        SpinBoxEditDelegate zoneLoDelegate;
        DoubleSpinBoxEditDelegate zoneTrimpDelegate;

        Context *context;
        HrZones *hrZones;
        HrSchemePage *schemePage;
        QTreeWidget *ranges;
        QTreeWidget *zones;
        QPushButton *addButton, *deleteButton;
        QPushButton *addZoneButton, *deleteZoneButton, *defaultButton;
};

class HrZonePage : public QWidget
{
    Q_OBJECT

    public:
        HrZonePage(Context *);
        ~HrZonePage();
        qint32 saveClicked();

    protected:
        Context *context;

        QTabWidget *tabs;

    private:
        QLabel *sportLabel;
        QComboBox *sportCombo;

        QHash<QString, HrZones*> hrZones;
        QHash<QString, quint16> b4Fingerprint; // how did it start ?
        QHash<QString, HrSchemePage*> schemePage;
        QHash<QString, LTPage*> ltPage;

    private slots:
        void changeSport();

};

//
// Pace Zones
//
class PaceSchemePage : public QWidget
{
    Q_OBJECT

    public:
        PaceSchemePage(PaceZones* paceZones);

        PaceZoneScheme getScheme();
        qint32 saveClicked();

    private slots:
        void addClicked();
        void deleteClicked();
        void updateButtons();

    private:
        PaceZones* paceZones;
        QTreeWidget *scheme;
        QPushButton *addButton, *deleteButton;
        SpinBoxEditDelegate fromDelegate;
};

class CVPage : public QWidget
{
    Q_OBJECT

    public:
        CVPage(PaceZones* paceZones, PaceSchemePage *schemePage);

        bool metricPace;

    private slots:
        void addClicked();
        void deleteClicked();
        void defaultClicked();
#if QT_VERSION < 0x060000
        void rangeChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
#else
        void rangeChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());
#endif
        void rangeSelectionChanged();
        void addZoneClicked();
        void deleteZoneClicked();
        void zonesChanged();
        void updateButtons();

    private:
        bool active;

        DateEditDelegate dateDelegate;
        TimeEditDelegate cvDelegate;
        TimeEditDelegate aetDelegate;

        TimeEditDelegate zoneFromDelegate;

        PaceZones* paceZones;
        PaceSchemePage *schemePage;
        QTreeWidget *ranges;
        QTreeWidget *zones;
        QPushButton *addButton, *deleteButton;
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

    public:
        AutoImportPage(Context *context);

        qint32 saveClicked();
        void addRuleTypes(QComboBox *p);

    public slots:
        void addClicked();
        void upClicked();
        void downClicked();
        void deleteClicked();

    private:
        Context *context;
        QList<RideAutoImportRule> rules;

        QTreeWidget *fields;
        ComboBoxDelegate ruleDelegate;
        DirectoryPathDelegate dirDelegate;

#ifndef Q_OS_MAC
        QToolButton *upButton, *downButton;
#else
        QPushButton *upButton, *downButton;
#endif
        QPushButton *addButton, *renameButton, *deleteButton, *browseButton;

};

extern void basicTreeWidgetStyle(QTreeWidget *tree);

#endif
