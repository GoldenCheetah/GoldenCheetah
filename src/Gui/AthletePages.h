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
        QLabel *weightlabel;
        QDoubleSpinBox *weight;
        QLabel *heightlabel;
        QDoubleSpinBox *height;
        QPushButton *avatarButton;
        QPixmap     avatar;
        QComboBox *crankLengthCombo;
        QComboBox *rimSizeCombo;
        QComboBox *tireSizeCombo;
        QLineEdit *wheelSizeEdit;


    struct {
        double weight;
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

//
// Power Zones
//
class SchemePage : public QWidget
{
    Q_OBJECT

    public:
        SchemePage(Zones *zones);
        ~SchemePage();

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
        SpinBoxEditDelegate *zoneFromDelegate;
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
        ~CPPage();

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
        void initializeRanges();

    private slots:
        void rangeEdited(const QModelIndex &modelIndex);
        void estimate();

    private:
        bool active;

        DateEditDelegate *dateDelegate;
        SpinBoxEditDelegate *cpDelegate;
        SpinBoxEditDelegate *aetDelegate;
        SpinBoxEditDelegate *ftpDelegate;
        SpinBoxEditDelegate *wDelegate;
        SpinBoxEditDelegate *pmaxDelegate;
        NoEditDelegate *statusDelegate;
        NoEditDelegate *closestDelegate;

        SpinBoxEditDelegate *zoneFromDelegate;

        Context *context;
        Zones *zones_;
        SchemePage *schemePage;
        TreeWidget6 *ranges;
        QTreeWidget *zones;
        QPushButton *addButton, *deleteButton;
        QPushButton *adoptButton;
        QPushButton *addZoneButton, *deleteZoneButton, *defaultButton;
        QPushButton *newZoneRequired;

        bool getValuesFor(const QDate &date, bool allowDefaults, int &cp, int &aetp, int &ftp, int &wprime, int &pmax, int &estOffset, bool &defaults, QDate *startDate = nullptr) const;
        void setEstimateStatus(QTreeWidgetItem *item);
        void setRangeData(QModelIndex modelIndex, int column, QVariant data);
        bool needsNewRange() const;
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
        QDoubleSpinBox *aetEdit;
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
        bool metricPace;

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
        QTimeEdit *cvEdit;
        QTimeEdit *aetEdit;

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

#endif
