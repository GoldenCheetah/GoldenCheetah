/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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

#include "Athlete.h"
#include <QtGui>
#include <QIntValidator>

#include <assert.h>

#include "Pages.h"
#include "Units.h"
#include "Settings.h"
#include "UserMetricParser.h"
#include "Colors.h"
#include "AddDeviceWizard.h"
#include "AddCloudWizard.h"
#include "DeviceTypes.h"
#include "DeviceConfiguration.h"
#include "ColorButton.h"
#include "SpecialFields.h"
#include "DataProcessor.h"
#include "OAuthDialog.h"
#include "RideAutoImportConfig.h"
#include "HelpWhatsThis.h"
#include "GcUpgrade.h"
#include "Dropbox.h"
#include "LocalFileStore.h"
#include "Secrets.h"
#include "Utils.h"
#include "IconManager.h"
#ifdef GC_WANT_PYTHON
#include "PythonEmbed.h"
#include "FixPySettings.h"
#endif
#ifdef GC_HAS_CLOUD_DB
#include "CloudDBUserMetric.h"
#endif
#include "MainWindow.h"
extern ConfigDialog *configdialog_ptr;

#define HLO "<h4>"
#define HLC "</h4>"


//
// Main Config Page - tabs for each sub-page
//
GeneralPage::GeneralPage(Context *context) : context(context)
{
    //
    // Language Selection
    //
    langCombo = new QComboBox();
    langCombo->addItem(tr("English"));
    langCombo->addItem(tr("French"));
    langCombo->addItem(tr("Japanese"));
    langCombo->addItem(tr("Portugese (Brazil)"));
    langCombo->addItem(tr("Italian"));
    langCombo->addItem(tr("German"));
    langCombo->addItem(tr("Russian"));
    langCombo->addItem(tr("Czech"));
    langCombo->addItem(tr("Spanish"));
    langCombo->addItem(tr("Portugese"));
    langCombo->addItem(tr("Chinese (Simplified)"));
    langCombo->addItem(tr("Chinese (Traditional)"));
    langCombo->addItem(tr("Dutch"));
    langCombo->addItem(tr("Swedish"));

    // Default to system locale
    QVariant lang = appsettings->value(this, GC_LANG, QLocale::system().name());

    if(lang.toString().startsWith("en")) langCombo->setCurrentIndex(0);
    else if(lang.toString().startsWith("fr")) langCombo->setCurrentIndex(1);
    else if(lang.toString().startsWith("ja")) langCombo->setCurrentIndex(2);
    else if(lang.toString().startsWith("pt-br")) langCombo->setCurrentIndex(3);
    else if(lang.toString().startsWith("it")) langCombo->setCurrentIndex(4);
    else if(lang.toString().startsWith("de")) langCombo->setCurrentIndex(5);
    else if(lang.toString().startsWith("ru")) langCombo->setCurrentIndex(6);
    else if(lang.toString().startsWith("cs")) langCombo->setCurrentIndex(7);
    else if(lang.toString().startsWith("es")) langCombo->setCurrentIndex(8);
    else if(lang.toString().startsWith("pt")) langCombo->setCurrentIndex(9);
    else if(lang.toString().startsWith("zh-cn")) langCombo->setCurrentIndex(10);
    else if (lang.toString().startsWith("zh-tw")) langCombo->setCurrentIndex(11);
    else if (lang.toString().startsWith("nl")) langCombo->setCurrentIndex(12);
    else if (lang.toString().startsWith("sv")) langCombo->setCurrentIndex(13);
    else langCombo->setCurrentIndex(0);

    //
    // Units
    //
    unitCombo = new QComboBox();
    unitCombo->addItem(tr("Metric"));
    unitCombo->addItem(tr("Imperial"));

    if (GlobalContext::context()->useMetricUnits)
        unitCombo->setCurrentIndex(0);
    else
        unitCombo->setCurrentIndex(1);

    metricRunPace = new QCheckBox(tr("Metric Run Pace"), this);
    metricRunPace->setCheckState(appsettings->value(this, GC_PACE, GlobalContext::context()->useMetricUnits).toBool() ? Qt::Checked : Qt::Unchecked);

    metricSwimPace = new QCheckBox(tr("Metric Swim Pace"), this);
    metricSwimPace->setCheckState(appsettings->value(this, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool() ? Qt::Checked : Qt::Unchecked);

    //
    // Garmin crap
    //
    // garmin Smart Recording options
    QVariant garminHWMark = appsettings->value(this, GC_GARMIN_HWMARK);
    garminSmartRecord = new QCheckBox(tr("Use Garmin Smart Recording"), this);
    QVariant isGarminSmartRecording = appsettings->value(this, GC_GARMIN_SMARTRECORD, Qt::Checked);

    // by default, set the threshold to 25 seconds
    if (garminHWMark.isNull() || garminHWMark.toInt() == 0) garminHWMark.setValue(25);
    garminHWMarkedit = new QSpinBox();
    garminHWMarkedit->setSingleStep(1);
    garminHWMarkedit->setRange(1, 300);
    garminHWMarkedit->setSuffix(" " + tr("s"));
    garminHWMarkedit->setValue(garminHWMark.toInt());

    connect(garminSmartRecord, &QCheckBox::stateChanged, [=](int state) { garminHWMarkedit->setEnabled(state); });
    garminSmartRecord->setCheckState(! (isGarminSmartRecording.toInt() > 0) ? Qt::Checked : Qt::Unchecked);
    garminSmartRecord->setCheckState(isGarminSmartRecording.toInt() > 0 ? Qt::Checked : Qt::Unchecked);

    // Elevation hysterisis  GC_ELEVATION_HYSTERISIS
    QVariant elevationHysteresis = appsettings->value(this, GC_ELEVATION_HYSTERESIS);
    if (elevationHysteresis.isNull() || elevationHysteresis.toFloat() == 0.0)
       elevationHysteresis.setValue(3.0);  // default is 3 meters

    hystedit = new QDoubleSpinBox();
    hystedit->setDecimals(1);
    hystedit->setSingleStep(0.1);
    hystedit->setRange(0.1, 10); // minimum value is enforced in metric code
    hystedit->setSuffix(" " + tr("m"));
    hystedit->setValue(elevationHysteresis.toFloat());

    // wbal formula preference
    wbalForm = new QComboBox(this);
    wbalForm->addItem(tr("Differential"));
    wbalForm->addItem(tr("Integral"));
    if (appsettings->value(this, GC_WBALFORM, "diff").toString() == "diff") wbalForm->setCurrentIndex(0);
    else wbalForm->setCurrentIndex(1);

    //
    // Warn to save on exit
    warnOnExit = new QCheckBox(tr("Warn for unsaved activities on exit"), this);
    warnOnExit->setChecked(appsettings->value(NULL, GC_WARNEXIT, true).toBool());

    //
    // Open last Athlete
    openLastAthlete = new QCheckBox(tr("Start with last opened Athlete"), this);
    openLastAthlete->setChecked(appsettings->value(NULL, GC_OPENLASTATHLETE, true).toBool());

    //
    // Run API web services when running
    //
#ifdef GC_WANT_HTTP
    startHttp = new QCheckBox(tr("Enable API Web Services"));
    startHttp->setChecked(appsettings->value(NULL, GC_START_HTTP, false).toBool());
#endif
#ifdef GC_WANT_R
    embedR = new QCheckBox(tr("Enable R"));
    //
    // R Home directory
    //
    QVariant rDir = appsettings->value(this, GC_R_HOME, "");
    // fix old bug..
    if (rDir == "0") rDir = "";
    rDirectory = new QLineEdit;
    rDirectory->setText(rDir.toString());
    QPushButton *rBrowseButton = new QPushButton(tr("Browse"));
    rDirectorySel = new QWidget();
    QHBoxLayout *rDirectoryLayout = new QHBoxLayout(rDirectorySel);
    rDirectoryLayout->setContentsMargins(0, 0, 0, 0);
    rDirectoryLayout->addWidget(rDirectory);
    rDirectoryLayout->addWidget(rBrowseButton);
    //XXrBrowseButton->setFixedWidth(120);

    connect(rBrowseButton, SIGNAL(clicked()), this, SLOT(browseRDir()));
    connect(embedR, &QCheckBox::stateChanged, [=](int state) { rDirectorySel->setEnabled(state); });

    embedR->setChecked(! appsettings->value(NULL, GC_EMBED_R, true).toBool());
    embedR->setChecked(appsettings->value(NULL, GC_EMBED_R, true).toBool());
#endif

#ifdef GC_WANT_PYTHON
    embedPython = new QCheckBox(tr("Enable Python"));
    //
    // Python Home directory
    //
    QVariant pythonDir = appsettings->value(this, GC_PYTHON_HOME, "");
    // fix old bug..
    pythonDirectory = new QLineEdit;
    pythonDirectory->setText(pythonDir.toString());
    QPushButton *pythonBrowseButton = new QPushButton(tr("Browse"));
    pythonDirectorySel = new QWidget();
    QHBoxLayout *pythonDirectoryLayout = new QHBoxLayout(pythonDirectorySel);
    pythonDirectoryLayout->setContentsMargins(0, 0, 0, 0);
    pythonDirectoryLayout->addWidget(pythonDirectory);
    pythonDirectoryLayout->addWidget(pythonBrowseButton);

    connect(pythonBrowseButton, SIGNAL(clicked()), this, SLOT(browsePythonDir()));
    connect(embedPython, &QCheckBox::stateChanged, [=](int state) { pythonDirectorySel->setEnabled(state); });

    embedPython->setChecked(! appsettings->value(NULL, GC_EMBED_PYTHON, true).toBool());
    embedPython->setChecked(appsettings->value(NULL, GC_EMBED_PYTHON, true).toBool());
#endif

    opendata = new QCheckBox(tr("Share to the OpenData project"), this);
    QString grant = appsettings->cvalue(context->athlete->cyclist, GC_OPENDATA_GRANTED, "X").toString();
    opendata->setChecked(grant == "Y");
    if (grant == "X") opendata->hide();

    //
    // Athlete directory (home of athletes)
    //
    QVariant athleteDir = appsettings->value(this, GC_HOMEDIR);
    athleteDirectory = new QLineEdit;
    athleteDirectory->setPlaceholderText(tr("Deviate from default location"));
    athleteDirectory->setText(athleteDir.toString() == "0" ? "" : athleteDir.toString());
    athleteWAS = athleteDirectory->text(); // remember what we started with ...
    QPushButton *athleteBrowseButton = new QPushButton(tr("Browse"));
    QHBoxLayout *athleteDirectoryLayout = new QHBoxLayout();
    athleteDirectoryLayout->addWidget(athleteDirectory);
    athleteDirectoryLayout->addWidget(athleteBrowseButton);
    //XXathleteBrowseButton->setFixedWidth(120);

    connect(athleteBrowseButton, SIGNAL(clicked()), this, SLOT(browseAthleteDir()));

    QFormLayout *form = newQFormLayout();
    form->addRow(new QLabel(HLO + tr("Localization") + HLC));
    form->addRow(tr("Language"), langCombo);
    form->addRow(tr("Unit"), unitCombo);
    form->addRow("", metricRunPace);
    form->addRow("", metricSwimPace);
    form->addItem(new QSpacerItem(0, 15 * dpiYFactor));
    form->addRow(new QLabel(HLO + tr("Application Behaviour") + HLC));
    form->addRow(tr("Athlete Library"), athleteDirectoryLayout);
    form->addRow("", warnOnExit);
    form->addRow("", openLastAthlete);
    form->addRow("", opendata);

    form->addItem(new QSpacerItem(0, 15 * dpiYFactor));
    form->addRow(new QLabel(HLO + tr("Recording and Calculation") + HLC));
    form->addRow("", garminSmartRecord);
    form->addRow(tr("Smart Recording Threshold"), garminHWMarkedit);
    form->addRow(tr("Elevation hysteresis"), hystedit);
    form->addRow(tr("W' bal formula"), wbalForm);
#if defined(GC_WANT_HTTP) || defined(GC_WANT_PYTHON) || defined(GC_WANT_R)
    form->addItem(new QSpacerItem(0, 15 * dpiYFactor));
    form->addRow(new QLabel(HLO + tr("Integration") + HLC));
#endif
#ifdef GC_WANT_HTTP
    form->addRow("", startHttp);
#endif
#ifdef GC_WANT_PYTHON
    form->addRow("", embedPython);
    form->addRow(tr("Python Home"), pythonDirectorySel);
#endif
#ifdef GC_WANT_R
    form->addRow("", embedR);
    form->addRow(tr("R Installation Directory"), rDirectorySel);
#endif

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidget(centerLayoutInWidget(form, false));
    scrollArea->setWidgetResizable(true);

    QVBoxLayout *all = new QVBoxLayout(this);
    all->addWidget(scrollArea);

    // save away initial values
    b4.unit = unitCombo->currentIndex();
    b4.metricRunPace = metricRunPace->isChecked();
    b4.metricSwimPace = metricSwimPace->isChecked();
    b4.hyst = elevationHysteresis.toFloat();
    b4.wbal = wbalForm->currentIndex();
#ifdef GC_WANT_HTTP
    b4.starthttp = startHttp->isChecked();
#endif
}

qint32
GeneralPage::saveClicked()
{
    // Language
    static const QString langs[] = {
        "en", "fr", "ja", "pt-br", "it", "de", "ru", "cs", "es", "pt", "zh-cn", "zh-tw", "nl", "sv"
    };
    appsettings->setValue(GC_LANG, langs[langCombo->currentIndex()]);

    // Garmin and cranks
    appsettings->setValue(GC_GARMIN_HWMARK, garminHWMarkedit->value());
    appsettings->setValue(GC_GARMIN_SMARTRECORD, garminSmartRecord->checkState());

    // save on exit
    appsettings->setValue(GC_WARNEXIT, warnOnExit->isChecked());

    // open last athlete on start
    appsettings->setValue(GC_OPENLASTATHLETE, openLastAthlete->isChecked());

    // Directories
    appsettings->setValue(GC_HOMEDIR, athleteDirectory->text());
#ifdef GC_WANT_R
    appsettings->setValue(GC_R_HOME, rDirectory->text());
#endif
#ifdef GC_WANT_PYTHON
    appsettings->setValue(GC_PYTHON_HOME, pythonDirectory->text());
#endif

    // update to reflect the state - if hidden user hasn't been asked yet to
    // its neither set or unset !
    if (!opendata->isHidden()) appsettings->setCValue(context->athlete->cyclist, GC_OPENDATA_GRANTED, opendata->isChecked() ? "Y" : "N");

    // Elevation
    appsettings->setValue(GC_ELEVATION_HYSTERESIS, hystedit->value());

    // wbal formula
    appsettings->setValue(GC_WBALFORM, wbalForm->currentIndex() ? "int" : "diff");

    // Units
    if (unitCombo->currentIndex()==0)
        appsettings->setValue(GC_UNIT, GC_UNIT_METRIC);
    else if (unitCombo->currentIndex()==1)
        appsettings->setValue(GC_UNIT, GC_UNIT_IMPERIAL);

    appsettings->setValue(GC_PACE, metricRunPace->isChecked());
    appsettings->setValue(GC_SWIMPACE, metricSwimPace->isChecked());

#ifdef GC_WANT_HTTP
    // start http
    appsettings->setValue(GC_START_HTTP, startHttp->isChecked());
#endif
#ifdef GC_WANT_R
    appsettings->setValue(GC_EMBED_R, embedR->isChecked());
#endif
#ifdef GC_WANT_PYTHON
    appsettings->setValue(GC_EMBED_PYTHON, embedPython->isChecked());
    if (!embedPython->isChecked()) fixPySettings->disableFixPy();
#endif


    qint32 state=0;

    // general stuff changed ?
#ifdef GC_WANT_HTTP
    if (b4.hyst != hystedit->text().toFloat() ||
        b4.starthttp != startHttp->isChecked())
#else
    if (b4.hyst != hystedit->text().toFloat())
#endif
        state += CONFIG_GENERAL;

    if (b4.wbal != wbalForm->currentIndex())
        state += CONFIG_WBAL;

    // units prefs changed?
    if (b4.unit != unitCombo->currentIndex() ||
        b4.metricRunPace != metricRunPace->isChecked() ||
        b4.metricSwimPace != metricSwimPace->isChecked())
        state += CONFIG_UNITS;

    return state;
}

#ifdef GC_WANT_R
void
GeneralPage::browseRDir()
{
    QString currentDir = rDirectory->text();
    if (!QDir(currentDir).exists()) currentDir = "";
    QString dir = QFileDialog::getExistingDirectory(this, tr("R Installation (R_HOME)"),
                            currentDir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir != "") rDirectory->setText(dir);  //only overwrite current dir, if a new was selected
}
#endif
#ifdef GC_WANT_PYTHON
void
GeneralPage::browsePythonDir()
{
    QString currentDir = pythonDirectory->text();
    if (!QDir(currentDir).exists()) currentDir = "";
    QString dir = QFileDialog::getExistingDirectory(this, tr("Python Installation (PYTHONHOME)"),
                            currentDir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir != "") {
        QString pybin, pypath;

        // is it installed there?
        bool isgood = PythonEmbed::pythonInstalled(pybin, pypath, dir);

        if (!isgood) {
            QMessageBox nope(QMessageBox::Warning, tr("Invalid Folder"), tr("Python does not appear to be installed in that location.\n"));
            nope.exec();
        } else {
            pythonDirectory->setText(dir);  //only overwrite current dir, if a new was selected
        }
     }
}
#endif

void
GeneralPage::browseAthleteDir()
{
    QString currentDir = athleteDirectory->text();
    if (!QDir(currentDir).exists()) currentDir = "";
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Athlete Library"),
                            currentDir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir != "") athleteDirectory->setText(dir);  //only overwrite current dir, if a new was selected
}

//
// Realtime devices page
//
DevicePage::DevicePage(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_Training_TrainDevices));

    DeviceTypes all;
    devices = all.getList();

    deviceList = new QTableView(this);
    deviceListModel = new deviceModel(this);
    // replace standard model with ours
    QItemSelectionModel *stdmodel = deviceList->selectionModel();
    deviceList->setModel(deviceListModel);
    delete stdmodel;
    deviceList->setSortingEnabled(false);
    deviceList->setSelectionBehavior(QAbstractItemView::SelectRows);
    deviceList->horizontalHeader()->setStretchLastSection(true);
    deviceList->verticalHeader()->hide();
    deviceList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    deviceList->setSelectionMode(QAbstractItemView::SingleSelection);

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(deviceList);
    mainLayout->addWidget(actionButtons);

    connect(actionButtons, &ActionButtonBox::addRequested, this, &DevicePage::devaddClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &DevicePage::devdelClicked);
    connect(context, SIGNAL(configChanged(qint32)), deviceListModel, SLOT(doReset()));
}

qint32
DevicePage::saveClicked()
{
    // Save the device configuration...
    DeviceConfigurations all;
    all.writeConfig(deviceListModel->Configuration);

    return 0;
}

void
DevicePage::devaddClicked()
{
    DeviceConfiguration add;
    AddDeviceWizard *p = new AddDeviceWizard(context);
    p->show();
}

void
DevicePage::devdelClicked()
{
    deviceListModel->del();
}

// add a new configuration
void
deviceModel::add(DeviceConfiguration &newone)
{
    insertRows(0,1, QModelIndex());

    // insert name
    QModelIndex index = deviceModel::index(0,0, QModelIndex());
    setData(index, newone.name, Qt::EditRole);

    // insert type
    index = deviceModel::index(0,1, QModelIndex());
    setData(index, newone.type, Qt::EditRole);

    // insert portSpec
    index = deviceModel::index(0,2, QModelIndex());
    setData(index, newone.portSpec, Qt::EditRole);

    // insert Profile
    index = deviceModel::index(0,3, QModelIndex());
    setData(index, newone.deviceProfile, Qt::EditRole);

    // insert virtualPowerIndex
    index = deviceModel::index(0,4, QModelIndex());
    setData(index, newone.postProcess, Qt::EditRole);
}

// delete an existing configuration
void
deviceModel::del()
{
    // which row is selected in the table?
    DevicePage *temp = static_cast<DevicePage*>(parent);
    QItemSelectionModel *selectionModel = temp->deviceList->selectionModel();

    QModelIndexList indexes = selectionModel->selectedRows();
    QModelIndex index;

    foreach (index, indexes) {
        //int row = this->mapToSource(index).row();
        removeRows(index.row(), 1, QModelIndex());
    }
}

deviceModel::deviceModel(QObject *parent) : QAbstractTableModel(parent)
{
    this->parent = parent;

    // get current configuration
    DeviceConfigurations all;
    Configuration = all.getList();
}

void
deviceModel::doReset()
{
    beginResetModel();
    DeviceConfigurations all;
    Configuration = all.getList();
    endResetModel();
}

int
deviceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return Configuration.size();
}

int
deviceModel::columnCount(const QModelIndex &) const
{
    return 5;
}


// setup the headings!
QVariant deviceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
     if (role != Qt::DisplayRole) return QVariant(); // no display, no game!

     if (orientation == Qt::Horizontal) {
         switch (section) {
             case 0:
                 return tr("Device Name");
             case 1:
                 return tr("Device Type");
             case 2:
                 return tr("Port Spec");
             case 3:
                 return tr("Profile");
             case 4:
                 return tr("Virtual");
             default:
                 return QVariant();
         }
     }
     return QVariant();
 }

// return data item for row/col specified in index
QVariant deviceModel::data(const QModelIndex &index, int role) const
{
     if (!index.isValid()) return QVariant();
     if (index.row() >= Configuration.size() || index.row() < 0) return QVariant();

     if (role == Qt::DisplayRole) {
         DeviceConfiguration Entry = Configuration.at(index.row());

         switch(index.column()) {
            case 0 : return Entry.name;
                break;
            case 1 :
                {
                DeviceTypes all;
                DeviceType lookupType = all.getType (Entry.type);
                return (const char*)(lookupType.name);
                }
                break;
            case 2 :
                return Entry.portSpec;
                break;
            case 3 :
                return Entry.deviceProfile;
            case 4 :
                return Entry.postProcess;
         }
     }

     // how did we get here!?
     return QVariant();
}

// update the model with new data
bool deviceModel::insertRows(int position, int rows, const QModelIndex &index)
{
     Q_UNUSED(index);
     beginInsertRows(QModelIndex(), position, position+rows-1);

     for (int row=0; row < rows; row++) {
         DeviceConfiguration emptyEntry;
         Configuration.insert(position, emptyEntry);
     }
     endInsertRows();
     return true;
}

// delete a row!
bool deviceModel::removeRows(int position, int rows, const QModelIndex &index)
{
     Q_UNUSED(index);
     beginRemoveRows(QModelIndex(), position, position+rows-1);

     for (int row=0; row < rows; ++row) {
         Configuration.removeAt(position);
     }

     endRemoveRows();
     return true;
}

bool deviceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
        if (index.isValid() && role == Qt::EditRole) {
            int row = index.row();
            DeviceConfiguration p = Configuration.value(row);
            switch (index.column()) {
                case 0 : //name
                    p.name = value.toString();
                    break;
                case 1 : //type
                    p.type = value.toInt();
                    break;
                case 2 : // spec
                    p.portSpec = value.toString();
                    break;
                case 3 : // Profile
                    p.deviceProfile = value.toString();
                    break;
                case 4 : // Virtual
                    p.postProcess = value.toInt();
                    break;
            }
            Configuration.replace(row,p);
            emit(dataChanged(index, index));

            return true;
        }

        return false;
}

//
// Train view options page
//
TrainOptionsPage::TrainOptionsPage(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_Training_Preferences));

    //
    // Workout directory (train view)
    //
    QVariant workoutDir = appsettings->value(this, GC_WORKOUTDIR, "");
    // fix old bug..
    if (workoutDir == "0") workoutDir = "";
    workoutDirectory = new DirectoryPathWidget();
    workoutDirectory->setPath(workoutDir.toString());

    useSimulatedSpeed = new QCheckBox(tr("Simulate Speed From Power"), this);
    useSimulatedSpeed->setChecked(appsettings->value(this, TRAIN_USESIMULATEDSPEED, true).toBool());
    useSimulatedSpeed->setToolTip(tr("Simulation physics uses current athlete parameters and settings\n"
                                     "from the virtual bicycle specifications tab. For Erg Mode workouts\n"
                                     "the slope is assumed to be zero."));

    useSimulatedHypoxia = new QCheckBox(tr("Simulate Relative Hypoxia"), this);
    useSimulatedHypoxia->setChecked(appsettings->value(this, TRAIN_USESIMULATEDHYPOXIA, false).toBool());
    useSimulatedHypoxia->setToolTip(tr("Power used by simulation is adjusted for hypoxia relative to\n"
                                       "ActualTrainingAltitude value in virtual bicycle specifications\n"
                                       "tab."));

    autoConnect = new QCheckBox(tr("Auto-connect devices in Train View"), this);
    autoConnect->setChecked(appsettings->value(this, TRAIN_AUTOCONNECT, false).toBool());

    multiCheck = new QCheckBox(tr("Allow multiple devices in Train View"), this);
    multiCheck->setChecked(appsettings->value(this, TRAIN_MULTI, false).toBool());

    autoHide = new QCheckBox(tr("Auto-hide bottom bar in Train View"), this);
    autoHide->setChecked(appsettings->value(this, TRAIN_AUTOHIDE, false).toBool());

    lapAlert = new QCheckBox(tr("Play sound before new lap"), this);
    lapAlert->setChecked(appsettings->value(this, TRAIN_LAPALERT, false).toBool());

    coalesce = new QCheckBox(tr("Coalesce contiguous sections of same wattage"), this);
    coalesce->setChecked(appsettings->value(this, TRAIN_COALESCE_SECTIONS, false).toBool());

    tooltips = new QCheckBox(tr("Enable Tooltips"), this);
    tooltips->setChecked(appsettings->value(this, TRAIN_TOOLTIPS, true).toBool());

    telemetryScaling = new QComboBox();
    telemetryScaling->addItem(tr("Fit to height only"), 0);
    telemetryScaling->addItem(tr("Fit to height and width"), 1);
    telemetryScaling->setCurrentIndex(appsettings->value(this, TRAIN_TELEMETRY_FONT_SCALING, 0).toInt());

    startDelay = new QSpinBox(this);
    startDelay->setMaximum(600);
    startDelay->setMinimum(0);
    startDelay->setSuffix(tr(" secs"));
    startDelay->setValue(appsettings->value(this, TRAIN_STARTDELAY, 0).toUInt());
    startDelay->setToolTip(tr("Countdown for workout start"));

    QFormLayout *form = newQFormLayout();
    form->addRow(tr("Workout and VideoSync Library"), workoutDirectory);
    form->addRow("", useSimulatedSpeed);
    form->addRow("", useSimulatedHypoxia);
    form->addRow("", multiCheck);
    form->addRow("", autoConnect);
    form->addRow("", autoHide);
    form->addRow("", lapAlert);
    form->addRow("", coalesce);
    form->addRow("", tooltips);
    form->addRow(tr("Start Countdown"), startDelay);
    form->addRow(tr("Telemetry font scaling"), telemetryScaling);

    setLayout(centerLayout(form));
}


qint32
TrainOptionsPage::saveClicked()
{
    // Save the train view settings...
    appsettings->setValue(GC_WORKOUTDIR, workoutDirectory->getPath());
    appsettings->setValue(TRAIN_USESIMULATEDSPEED, useSimulatedSpeed->isChecked());
    appsettings->setValue(TRAIN_USESIMULATEDHYPOXIA, useSimulatedHypoxia->isChecked());
    appsettings->setValue(TRAIN_MULTI, multiCheck->isChecked());
    appsettings->setValue(TRAIN_AUTOCONNECT, autoConnect->isChecked());
    appsettings->setValue(TRAIN_STARTDELAY, startDelay->value());
    appsettings->setValue(TRAIN_AUTOHIDE, autoHide->isChecked());
    appsettings->setValue(TRAIN_LAPALERT, lapAlert->isChecked());
    appsettings->setValue(TRAIN_COALESCE_SECTIONS, coalesce->isChecked());
    appsettings->setValue(TRAIN_TOOLTIPS, tooltips->isChecked());
    appsettings->setValue(TRAIN_TELEMETRY_FONT_SCALING, telemetryScaling->currentIndex());

    return 0;
}


//
// Remote control page
//
RemotePage::RemotePage(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_Training_RemoteControls));

    remote = new RemoteControl;
    QList <RemoteCmd> nativeCmds = remote->getNativeCmds(); // Load the native command list
    QList<RemoteCmd> antCmds = remote->getAntCmds(); // Load the ant command list
    QList<CmdMap> cmdMaps = remote->getMappings(); // Load the remote control mappings

    fields = new QTreeWidget;
    fields->headerItem()->setText(0, tr("Action"));
    fields->headerItem()->setText(1, tr("ANT+ Command"));
    fields->setColumnCount(2);
    basicTreeWidgetStyle(fields);
    fields->setItemDelegateForColumn(0, &nativeCmdDelegate);
    fields->setItemDelegateForColumn(1, &cmdDelegate);

    QStringList cmds;
    cmds << tr("<unset>");
    for (const RemoteCmd &antCmd : antCmds) {
        cmds << antCmd.getCmdStr();
    }
    cmdDelegate.addItems(cmds);

    // create a row for each native command
    int selected = 0;
    for (const RemoteCmd &nativeCmd : nativeCmds) {
        selected = 0;
        // is this native command mapped to an ANT command?
        for (const CmdMap &cmdMapping : cmdMaps) {
            if (   cmdMapping.getNativeCmdId() == nativeCmd.getCmdId()
                && cmdMapping.getAntCmdId() != 0xFFFF) {
                int i = 0;
                for (const RemoteCmd &antCmd : antCmds) {
                    i++; // increment first to skip <unset>
                    if (cmdMapping.getAntCmdId() == antCmd.getCmdId()) {
                        selected = i;
                    }
                }
            }
        }

        QTreeWidgetItem *add = new QTreeWidgetItem(fields);
        add->setFlags(add->flags() | Qt::ItemIsEditable);
        add->setData(0, Qt::DisplayRole, nativeCmd.getDisplayStr());
        add->setData(1, Qt::DisplayRole, selected);
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(fields, 0, Qt::Alignment());

    fields->setCurrentItem(fields->invisibleRootItem()->child(0));
}

qint32
RemotePage::saveClicked()
{
    QList<RemoteCmd> antCmds = remote->getAntCmds(); // Load the ant command list
    QList<CmdMap> cmdMaps = remote->getMappings(); // Load the remote control mappings

    for (int i = 0; i < cmdMaps.size(); i++) {
        int cmdIndex = fields->invisibleRootItem()->child(i)->data(1, Qt::DisplayRole).toInt();
        if (cmdIndex) {
            cmdMaps[i].setAntCmdId(antCmds[cmdIndex - 1].getCmdId());
        } else {
            cmdMaps[i].setAntCmdId(0xFFFF); // no command
        }
    }

    remote->writeConfig(cmdMaps);
    return 0;
}

const SimBicyclePartEntry& SimBicyclePage::GetSimBicyclePartEntry(int e)
{
    // Bike mass values approximate a good current bike. Wheels are shimano c40 with conti tubeless tires.

    static const SimBicyclePartEntry arr[] = {
          // SpinBox Title                              Path to athlete value                Default Value      Decimal Unit     Tooltip                                                                              enum
        { tr("Bicycle Mass Without Wheels"      )  , GC_SIM_BICYCLE_MASSWITHOUTWHEELSG,   4000,              0,      "g",     tr("Mass of everything that isn't wheels, tires, skewers...")},                       // BicycleWithoutWheelsG
        { tr("Front Wheel Mass"                 )  , GC_SIM_BICYCLE_FRONTWHEELG,          739,               0,      "g",     tr("Mass of front wheel excluding tires and skewers...")},                            // FrontWheelG
        { tr("Front Spoke Count"                )  , GC_SIM_BICYCLE_FRONTSPOKECOUNT,      24,                0,      "",      tr("")},                                                                              // FrontSpokeCount
        { tr("Front Spoke & Nipple Mass - Each" )  , GC_SIM_BICYCLE_FRONTSPOKENIPPLEG,    5.6,               1,      "g",     tr("Mass of a single spoke and nipple, washers, etc.")},                              // FrontSpokeNippleG
        { tr("Front Rim Mass"                   )  , GC_SIM_BICYCLE_FRONTRIMG,            330,               0,      "g",     tr("")},                                                                              // FrontRimG
        { tr("Front Rotor Mass"                 )  , GC_SIM_BICYCLE_FRONTROTORG,          120,               0,      "g",     tr("Mass of rotor including bolts")},                                                 // FrontRotorG
        { tr("Front Skewer Mass"                )  , GC_SIM_BICYCLE_FRONTSKEWERG,         40,                0,      "g",     tr("")},                                                                              // FrontSkewerG
        { tr("Front Tire Mass"                  )  , GC_SIM_BICYCLE_FRONTTIREG,           220,               0,      "g",     tr("")},                                                                              // FrontTireG
        { tr("Front Tube or Sealant Mass"       )  , GC_SIM_BICYCLE_FRONTTUBESEALANTG,    26,                0,      "g",     tr("Mass of anything inside the tire: sealant, tube...")},                            // FrontTubeSealantG
        { tr("Front Rim Outer Radius"           )  , GC_SIM_BICYCLE_FRONTOUTERRADIUSM,    .35,               3,      "m",     tr("Functional outer radius of wheel, used for computing wheel circumference")},      // FrontOuterRadiusM
        { tr("Front Rim Inner Radius"           )  , GC_SIM_BICYCLE_FRONTRIMINNERRADIUSM, .3,                3,      "m",     tr("Inner radius of rim, for computing wheel inertia")},                              // FrontRimInnerRadiusM
        { tr("Rear Wheel Mass"                  )  , GC_SIM_BICYCLE_REARWHEELG,           739,               0,      "g",     tr("Mass of rear wheel excluding tires and skewers...")},                             // RearWheelG
        { tr("Rear Spoke Count"                 )  , GC_SIM_BICYCLE_REARSPOKECOUNT,       24,                0,      "",      tr("")},                                                                              // RearSpokeCount
        { tr("Rear Spoke & Nipple Mass - Each"  )  , GC_SIM_BICYCLE_REARSPOKENIPPLEG,     5.6,               1,      "g",     tr("Mass of a single spoke and nipple, washers, etc.")},                              // RearSpokeNippleG
        { tr("Rear Rim Mass"                    )  , GC_SIM_BICYCLE_REARRIMG,             330,               0,      "g",     tr("")},                                                                              // RearRimG
        { tr("Rear Rotor Mass"                  )  , GC_SIM_BICYCLE_REARROTORG,           120,               0,      "g",     tr("Mass of rotor including bolts")},                                                 // RearRotorG
        { tr("Rear Skewer Mass"                 )  , GC_SIM_BICYCLE_REARSKEWERG,           40,               0,      "g",     tr("Mass of skewer/axle/funbolts, etc...")},                                          // RearSkewerG
        { tr("Rear Tire Mass"                   )  , GC_SIM_BICYCLE_REARTIREG,            220,               0,      "g",     tr("Mass of tire not including tube or sealant")},                                    // RearTireG
        { tr("Rear Tube or Sealant Mass"        )  , GC_SIM_BICYCLE_REARTUBESEALANTG,      26,               0,      "g",     tr("Mass of anything inside the tire: sealant, tube...")},                            // RearTubeSealantG
        { tr("Rear Rim Outer Radius"            )  , GC_SIM_BICYCLE_REAROUTERRADIUSM,     .35,               3,      "m",     tr("Functional outer radius of wheel, used for computing wheel circumference")},      // RearOuterRadiusM
        { tr("Rear Rim Inner Radius"            )  , GC_SIM_BICYCLE_REARRIMINNERRADIUSM,  .3,                3,      "m",     tr("Inner radius of rim, for computing wheel inertia")},                              // RearRimInnerRadiusM
        { tr("Rear Cassette Mass"               )  , GC_SIM_BICYCLE_CASSETTEG,            190,               0,      "g",     tr("Mass of rear cassette, including lockring")},                                     // CassetteG
        { tr("Coefficient of rolling resistance")  , GC_SIM_BICYCLE_CRR,                  0.004,             4,      "",      tr("Total coefficient of rolling resistance for bicycle")},                           // CRR
        { tr("Coefficient of power train loss"  )  , GC_SIM_BICYCLE_Cm,                   1.0,               3,      "",      tr("Power train loss between reported watts and wheel. For direct drive trainer like kickr there is no relevant loss and value shold be 1.0.")},      // Cm
        { tr("Coefficient of drag"              )  , GC_SIM_BICYCLE_Cd,        (1.0 - 0.0045),               5,      "",      tr("Coefficient of drag of rider and bicycle")},                                      // Cd
        { tr("Frontal Area"                     )  , GC_SIM_BICYCLE_Am2,                  0.5,               2,      "m^2",   tr("Effective frontal area of rider and bicycle")},                                   // Am2
        { tr("Temperature"                      )  , GC_SIM_BICYCLE_Tk,                 293.15,              2,      "K",     tr("Temperature in kelvin, used with altitude to compute air density")},              // Tk
        { tr("ActualTrainerAltitude"            )  , GC_SIM_BICYCLE_ACTUALTRAINERALTITUDEM, 0.,              0,      "m",     tr("Actual altitude of indoor trainer, in meters")}                                   // ActualTrainerAltitudeM
    };

    if (e < 0 || e >= LastPart) e = 0;

    return arr[e];
}

double
SimBicyclePage::GetBicyclePartValue(Context* context, int e)
{
    const SimBicyclePartEntry &r = GetSimBicyclePartEntry(e);

    if (!context) return r.m_defaultValue;

    return appsettings->cvalue(
        context->athlete->cyclist,
        r.m_path,
        r.m_defaultValue).toDouble();
}

void
SimBicyclePage::AddSpecBox(int ePart)
{
    const SimBicyclePartEntry & entry = GetSimBicyclePartEntry(ePart);

    m_LabelTextArr[ePart] = entry.m_label;

    QDoubleSpinBox * pSpinBox = new QDoubleSpinBox(this);

    pSpinBox->setMaximum(99999);
    pSpinBox->setMinimum(0.0);
    pSpinBox->setDecimals(entry.m_decimalPlaces);
    pSpinBox->setValue(GetBicyclePartValue(context, ePart));
    pSpinBox->setToolTip(entry.m_tooltip);
    if (! entry.m_unit.isEmpty()) {
        pSpinBox->setSuffix(" " + entry.m_unit);
    }
    double singlestep = 1.;
    for (int i = 0; i < entry.m_decimalPlaces; i++)
        singlestep /= 10.;

    pSpinBox->setSingleStep(singlestep);

    m_SpinBoxArr[ePart] = pSpinBox;
}

void
SimBicyclePage::SetStatsLabelArray(double )
{
    double riderMassKG = 0;

    const double bicycleMassWithoutWheelsG = m_SpinBoxArr[SimBicyclePage::BicycleWithoutWheelsG]->value();
    const double bareFrontWheelG           = m_SpinBoxArr[SimBicyclePage::FrontWheelG          ]->value();
    const double frontSpokeCount           = m_SpinBoxArr[SimBicyclePage::FrontSpokeCount      ]->value();
    const double frontSpokeNippleG         = m_SpinBoxArr[SimBicyclePage::FrontSpokeNippleG    ]->value();
    const double frontWheelOuterRadiusM    = m_SpinBoxArr[SimBicyclePage::FrontOuterRadiusM    ]->value();
    const double frontRimInnerRadiusM      = m_SpinBoxArr[SimBicyclePage::FrontRimInnerRadiusM ]->value();
    const double frontRimG                 = m_SpinBoxArr[SimBicyclePage::FrontRimG            ]->value();
    const double frontRotorG               = m_SpinBoxArr[SimBicyclePage::FrontRotorG          ]->value();
    const double frontSkewerG              = m_SpinBoxArr[SimBicyclePage::FrontSkewerG         ]->value();
    const double frontTireG                = m_SpinBoxArr[SimBicyclePage::FrontTireG           ]->value();
    const double frontTubeOrSealantG       = m_SpinBoxArr[SimBicyclePage::FrontTubeSealantG    ]->value();
    const double bareRearWheelG            = m_SpinBoxArr[SimBicyclePage::RearWheelG           ]->value();
    const double rearSpokeCount            = m_SpinBoxArr[SimBicyclePage::RearSpokeCount       ]->value();
    const double rearSpokeNippleG          = m_SpinBoxArr[SimBicyclePage::RearSpokeNippleG     ]->value();
    const double rearWheelOuterRadiusM     = m_SpinBoxArr[SimBicyclePage::RearOuterRadiusM     ]->value();
    const double rearRimInnerRadiusM       = m_SpinBoxArr[SimBicyclePage::RearRimInnerRadiusM  ]->value();
    const double rearRimG                  = m_SpinBoxArr[SimBicyclePage::RearRimG             ]->value();
    const double rearRotorG                = m_SpinBoxArr[SimBicyclePage::RearRotorG           ]->value();
    const double rearSkewerG               = m_SpinBoxArr[SimBicyclePage::RearSkewerG          ]->value();
    const double rearTireG                 = m_SpinBoxArr[SimBicyclePage::RearTireG            ]->value();
    const double rearTubeOrSealantG        = m_SpinBoxArr[SimBicyclePage::RearTubeSealantG     ]->value();
    const double cassetteG                 = m_SpinBoxArr[SimBicyclePage::CassetteG            ]->value();

    const double frontWheelG = bareFrontWheelG + frontRotorG + frontSkewerG + frontTireG + frontTubeOrSealantG;
    const double frontWheelRotatingG = frontRimG + frontTireG + frontTubeOrSealantG + (frontSpokeCount * frontSpokeNippleG);
    const double frontWheelCenterG = frontWheelG - frontWheelRotatingG;

    BicycleWheel frontWheel(frontWheelOuterRadiusM, frontRimInnerRadiusM, frontWheelG / 1000, frontWheelCenterG /1000, frontSpokeCount, frontSpokeNippleG/1000);

    const double rearWheelG = bareRearWheelG + cassetteG + rearRotorG + rearSkewerG + rearTireG + rearTubeOrSealantG;
    const double rearWheelRotatingG = rearRimG + rearTireG + rearTubeOrSealantG + (rearSpokeCount * rearSpokeNippleG);
    const double rearWheelCenterG = rearWheelG - rearWheelRotatingG;

    BicycleWheel rearWheel (rearWheelOuterRadiusM,  rearRimInnerRadiusM,  rearWheelG / 1000,  rearWheelCenterG / 1000,  rearSpokeCount,  rearSpokeNippleG/1000);

    BicycleConstants constants(
        m_SpinBoxArr[SimBicyclePage::CRR]->value(),
        m_SpinBoxArr[SimBicyclePage::Cm] ->value(),
        m_SpinBoxArr[SimBicyclePage::Cd] ->value(),
        m_SpinBoxArr[SimBicyclePage::Am2]->value(),
        m_SpinBoxArr[SimBicyclePage::Tk] ->value(),
        1.);

    Bicycle bicycle(NULL, constants, riderMassKG, bicycleMassWithoutWheelsG / 1000., frontWheel, rearWheel);

    m_StatsTextArr[StatsTotalKEMass]         = tr("Total KEMass");
    m_StatsLabelArr[StatsTotalKEMass]        ->setText(tr("%1 g").arg(bicycle.KEMass()));
    m_StatsTextArr[StatsFrontWheelKEMass]    = tr("FrontWheel KEMass");
    m_StatsLabelArr[StatsFrontWheelKEMass]   ->setText(tr("%1 g").arg(bicycle.FrontWheel().KEMass() * 1000));
    m_StatsTextArr[StatsFrontWheelMass]      = tr("FrontWheel Mass");
    m_StatsLabelArr[StatsFrontWheelMass]     ->setText(tr("%1 g").arg(bicycle.FrontWheel().MassKG() * 1000));
    m_StatsTextArr[StatsFrontWheelEquivMass] = tr("FrontWheel EquivMass");
    m_StatsLabelArr[StatsFrontWheelEquivMass]->setText(tr("%1 g").arg(bicycle.FrontWheel().EquivalentMassKG() * 1000));
    m_StatsTextArr[StatsFrontWheelI]         = tr("FrontWheel I");
    m_StatsLabelArr[StatsFrontWheelI]        ->setText(tr("%1").arg(bicycle.FrontWheel().I()));
    m_StatsTextArr[StatsRearWheelKEMass]     = tr("Rear Wheel KEMass");
    m_StatsLabelArr[StatsRearWheelKEMass]    ->setText(tr("%1 g").arg(bicycle.RearWheel().KEMass() * 1000));
    m_StatsTextArr[StatsRearWheelMass]       = tr("Rear Wheel Mass");
    m_StatsLabelArr[StatsRearWheelMass]      ->setText(tr("%1 g").arg(bicycle.RearWheel().MassKG() * 1000));
    m_StatsTextArr[StatsRearWheelEquivMass]  = tr("Rear Wheel EquivMass");
    m_StatsLabelArr[StatsRearWheelEquivMass] ->setText(tr("%1 g").arg(bicycle.RearWheel().EquivalentMassKG() * 1000));
    m_StatsTextArr[StatsRearWheelI]          = tr("Rear Wheel I");
    m_StatsLabelArr[StatsRearWheelI]         ->setText(tr("%1").arg(bicycle.RearWheel().I()));
}

SimBicyclePage::SimBicyclePage(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_Training_VirtualBicycleSpecifications));

    // Populate m_LabelTextArr and m_SpinBoxArr
    for (int e = 0; e < LastPart; e++) {
        AddSpecBox(e);
    }

    // Two sections. Bike mass properties are in two rows to the left.
    // Other properties like cd, ca and temp go in section to the right.
    int Section1Start = 0;
    int Section1End = BicycleParts::CRR;
    int Section2Start = Section1End;
    int Section2End = BicycleParts::LastPart;

    QLabel *descLabel = new QLabel(tr("The values on this page inform the bicycle physics "
                                      "models for simulating speed in trainer mode. These "
                                      "values are used by smart trainers and also by the "
                                      "speed simulation enabled by the <i>Simulate Speed From "
                                      "Power</i> option in the <i>Training</i> &gt; "
                                      "<i>Preferences</i> tab."));
    descLabel->setWordWrap(true);

    QFormLayout *lForm = newQFormLayout();
    lForm->addRow(new QLabel(HLO + tr("Resistance and Drag") + HLC));
    for (int i = Section2Start; i < Section2End; i++) {
        lForm->addRow(m_LabelTextArr[i], m_SpinBoxArr[i]);
    }

    // Create and Populate Stats Labels
    for (int i = StatsTotalKEMass; i < StatsLastPart; i++) {
        m_StatsLabelArr[i] = new QLabel();
    }
    SetStatsLabelArray();
    QFormLayout *rForm = newQFormLayout();
    rForm->addRow(new QLabel(HLO + tr("Derived Statistics") + HLC));
    for (int i = StatsTotalKEMass; i < StatsLastPart; i++) {
        rForm->addRow(m_StatsTextArr[i], m_StatsLabelArr[i]);
    }

    lForm->addItem(new QSpacerItem(1, 15 * dpiYFactor));
    lForm->addRow(new QLabel(HLO + tr("Bike & Wheels") + HLC));
    for (int i = Section1Start; i < Section1End; i++) {
        lForm->addRow(m_LabelTextArr[i], m_SpinBoxArr[i]);
    }

    QScrollArea *scroller = new QScrollArea();
    scroller->setWidget(centerLayoutInWidget(lForm));
    scroller->setWidgetResizable(true);

    QHBoxLayout *dataLayout = new QHBoxLayout();
    dataLayout->addWidget(scroller, 2);
    dataLayout->addSpacing(10 * dpiXFactor);
    dataLayout->addLayout(centerLayout(rForm), 1);

    QVBoxLayout *all = new QVBoxLayout(this);
    all->addWidget(descLabel);
    all->addSpacing(10 * dpiYFactor);
    all->addLayout(dataLayout);

    for (int i = 0; i < LastPart; i++) {
        connect(m_SpinBoxArr[i], SIGNAL(valueChanged(double)), this, SLOT(SetStatsLabelArray(double)));
    }
}

qint32
SimBicyclePage::saveClicked()
{
    for (int e = 0; e < BicycleParts::LastPart; e++) {
        const SimBicyclePartEntry& entry = GetSimBicyclePartEntry(e);
        appsettings->setCValue(context->athlete->cyclist, entry.m_path, m_SpinBoxArr[e]->value());
    }

    qint32 state = CONFIG_ATHLETE;

    return state;
}


////////////////////////////////////////////////////
// Workout Tag Manager page
//

WorkoutTagManagerPage::WorkoutTagManagerPage
(TagStore *tagStore, QWidget *parent)
: QWidget(parent), tagStore(tagStore)
{
    tw = new QTreeWidget();
    tw->setColumnCount(4);
    tw->hideColumn(2);
    tw->hideColumn(3);
    tw->setSortingEnabled(true);
    tw->sortByColumn(0, Qt::AscendingOrder);
    basicTreeWidgetStyle(tw);
    tw->setItemDelegateForColumn(0, &labelEditDelegate);
    tw->setItemDelegateForColumn(1, &numDelegate);

    QStringList headers;
    headers << tr("Tag")
            << tr("Assigned to # workouts");
    tw->setHeaderLabels(headers);

    QList<QTreeWidgetItem *> items;
    for (const auto &tag: tagStore->getTags()) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setData(0, Qt::DisplayRole, tag.label);
        item->setData(1, Qt::DisplayRole, tagStore->countTagUsage(tag.id));
        item->setData(2, Qt::DisplayRole, tag.id);
        item->setData(3, Qt::DisplayRole, false);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        items.append(item);
    }
    tw->insertTopLevelItems(0, items);

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);
    actionButtons->defaultConnect(ActionButtonBox::AddDeleteGroup, tw);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(tw);
    layout->addWidget(actionButtons);

    connect(actionButtons, &ActionButtonBox::addRequested, this, &WorkoutTagManagerPage::addTag);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &WorkoutTagManagerPage::deleteTag);
    connect(&labelEditDelegate, SIGNAL(closeEditor(QWidget*, QAbstractItemDelegate::EndEditHint)), this, SLOT(editorClosed(QWidget*, QAbstractItemDelegate::EndEditHint)));
    connect(tw, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(tw->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)), this, SLOT(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)));
    connect(dynamic_cast<QObject*>(tagStore), SIGNAL(tagsChanged(int, int, int)), this, SLOT(tagStoreChanged(int, int, int)));

    tw->setCurrentItem(tw->invisibleRootItem()->child(0));
}


WorkoutTagManagerPage::~WorkoutTagManagerPage
()
{
}


qint32
WorkoutTagManagerPage::saveClicked
()
{
    tagStore->deferTagSignals(true);
    QList<QTreeWidgetItem*> foundItems = tw->findItems(QString::number(TAGSTORE_UNDEFINED_ID), Qt::MatchExactly, 2);
    for (auto &item : foundItems) {
        tagStore->addTag(item->data(0, Qt::DisplayRole).toString());
        delete item;
    }
    for (auto &id : deleted) {
        tagStore->deleteTag(id);
    }
    deleted.clear();
    tagStore->deferTagSignals(false);
    return foundItems.size() > 0 ? CONFIG_WORKOUTTAGMANAGER : 0;
}


void
WorkoutTagManagerPage::currentItemChanged
(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous);
    tw->setCurrentItem(current, 0);
}


void
WorkoutTagManagerPage::dataChanged
(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_UNUSED(bottomRight);
    Q_UNUSED(roles);
    if (topLeft.column() == 3 && topLeft.data().toBool()) {
        tagStore->updateTag(topLeft.siblingAtColumn(2).data().toInt(), topLeft.siblingAtColumn(0).data().toString());
        tw->model()->setData(topLeft, false);
    }
}


void
WorkoutTagManagerPage::tagStoreChanged
(int idAdded, int idRemoved, int idUpdated)
{
    QList<QTreeWidgetItem*> foundItems;

    if (   idAdded != TAGSTORE_UNDEFINED_ID
        && tw->findItems(QString::number(idAdded), Qt::MatchExactly, 2).size() == 0) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setData(0, Qt::DisplayRole, tagStore->getTagLabel(idAdded));
        item->setData(1, Qt::DisplayRole, tagStore->countTagUsage(idAdded));
        item->setData(2, Qt::DisplayRole, idAdded);
        item->setData(3, Qt::DisplayRole, false);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        tw->addTopLevelItem(item);
    }
    if (   idRemoved != TAGSTORE_UNDEFINED_ID
        && (foundItems = tw->findItems(QString::number(idRemoved), Qt::MatchExactly, 2)).size() == 1) {
        delete foundItems.takeFirst();
    }
    if (   idUpdated != TAGSTORE_UNDEFINED_ID
        && (foundItems = tw->findItems(QString::number(idUpdated), Qt::MatchExactly, 2)).size() == 1) {
        foundItems[0]->setData(0, Qt::DisplayRole, tagStore->getTagLabel(idUpdated));
        foundItems[0]->setData(1, Qt::DisplayRole, tagStore->countTagUsage(idUpdated));
    }
}


void
WorkoutTagManagerPage::deleteTag
()
{
    if (tw->currentItem() != nullptr) {
        int id = tw->currentItem()->data(2, Qt::DisplayRole).toInt();
        if (id != TAGSTORE_UNDEFINED_ID) {
            deleted << id;
        }
        delete tw->currentItem();
    }
}


void
WorkoutTagManagerPage::addTag
()
{
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setData(0, Qt::DisplayRole, "");
    item->setData(1, Qt::DisplayRole, 0);
    item->setData(2, Qt::DisplayRole, TAGSTORE_UNDEFINED_ID);
    item->setData(3, Qt::DisplayRole, true);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    tw->addTopLevelItem(item);
    tw->setCurrentItem(item);
    tw->editItem(item, 0);
}


void
WorkoutTagManagerPage::editorClosed
(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    Q_UNUSED(editor);
    Q_UNUSED(hint);
    QTimer::singleShot(0, this, SLOT(modelCleaner()));
}


void
WorkoutTagManagerPage::modelCleaner
()
{
    QList<QTreeWidgetItem*> foundItems = tw->findItems(QString(), Qt::MatchExactly, 0);
    for (auto &item : foundItems) {
        delete item;
    }
}



//
// Appearances page
//
ColorsPage::ColorsPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    themes = new QTreeWidget;
    themes->headerItem()->setText(0, tr("Swatch"));
    themes->headerItem()->setText(1, tr("Name"));
    themes->setColumnCount(2);
    themes->setColumnWidth(0,240 *dpiXFactor);
    themes->setSelectionMode(QAbstractItemView::SingleSelection);
    //colors->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    themes->setUniformRowHeights(true); // causes height problems when adding - in case of non-text fields
    themes->setIndentation(0);
    //colors->header()->resizeSection(0,300);
    themes->setAlternatingRowColors(true);

    QLabel *searchLabel = new QLabel(tr("Search"));
    searchEdit = new QLineEdit(this);
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(searchEdit);

    colors = new QTreeWidget;
    colors->headerItem()->setText(0, tr("Group"));
    colors->headerItem()->setText(1, tr("Color"));
    colors->headerItem()->setText(2, tr("Select"));
    colors->setColumnCount(3);
    basicTreeWidgetStyle(colors, false);
    colors->setSelectionMode(QAbstractItemView::NoSelection);

    antiAliased = new QCheckBox(tr("Antialias"));
    antiAliased->setChecked(appsettings->value(this, GC_ANTIALIAS, true).toBool());
#ifndef Q_OS_MAC
    macForms = new QCheckBox(tr("Mac styled Forms"));
    macForms->setChecked(appsettings->value(this, GC_MAC_FORMS, true).toBool());
    rideScroll = new QCheckBox(tr("Activity Scrollbar"));
    rideScroll->setChecked(appsettings->value(this, GC_RIDESCROLL, true).toBool());
    rideHead = new QCheckBox(tr("Activity Headings"));
    rideHead->setChecked(appsettings->value(this, GC_RIDEHEAD, true).toBool());
#endif
    lineWidth = new QDoubleSpinBox;
    lineWidth->setMaximum(5);
    lineWidth->setMinimum(0.5);
    lineWidth->setSingleStep(0.5);
    applyTheme = new QPushButton(tr("Apply Theme"));
    lineWidth->setValue(appsettings->value(this, GC_LINEWIDTH, 0.5*dpiXFactor).toDouble());

    def = new QFontComboBox(this);

    // get round QTBUG
    def->setCurrentIndex(0);
    def->setCurrentIndex(1);
    def->setCurrentFont(baseFont);
    def->setCurrentFont(baseFont);

    // font scaling
    double scale = appsettings->value(this, GC_FONT_SCALE, 1.0).toDouble();
    fontscale = new QSlider(this);
    fontscale->setMinimum(0);
    fontscale->setMaximum(11);
    fontscale->setTickInterval(1);
    fontscale->setValue(3);
    fontscale->setOrientation(Qt::Horizontal);
    for(int i=0; i<12; i++) {
        if (scalefactors[i] == scale) {
            fontscale->setValue(i);
            break;
        }
    }

    QFont font=baseFont;
    font.setPointSizeF(baseFont.pointSizeF() * scale);
    fonttext = new QLabel(this);
    fonttext->setText(tr("The quick brown fox jumps over the lazy dog"));
    fonttext->setFont(font);
    fonttext->setFixedHeight(90 * dpiYFactor);

    QFormLayout *lForm = newQFormLayout();
    lForm->addRow(tr("Font"), def);
    lForm->addRow(tr("Font Scaling"), fontscale);
    lForm->addRow(fonttext);

    QFormLayout *rForm = newQFormLayout();
    rForm->addRow(tr("Line Width"), lineWidth);
    rForm->addRow("", antiAliased);
#ifndef Q_OS_MAC
    rForm->addRow("", macForms);
    rForm->addRow("", rideScroll);
    rForm->addRow("", rideHead);    // add to prevent memory leak...
    rideHead->setVisible(false);    // ...but hide
#endif

    QHBoxLayout *formsContainer = new QHBoxLayout();
    formsContainer->addLayout(lForm, 3);
    formsContainer->addSpacerItem(new QSpacerItem(15 * dpiXFactor, 0));
    formsContainer->addLayout(rForm, 1);
    mainLayout->addLayout(formsContainer);

    colorTab = new QTabWidget(this);
    colorTab->addTab(themes, tr("Theme"));

    QWidget *colortab= new QWidget(this);
    QVBoxLayout *colorLayout = new QVBoxLayout(colortab);
    colorLayout->addLayout(searchLayout);
    colorLayout->addWidget(colors);
    colorTab->addTab(colortab, tr("Colors"));
    colorTab->setCornerWidget(applyTheme);

    mainLayout->addWidget(colorTab);

    colorSet = GCColor::colorSet();
    for (int i=0; colorSet[i].name != ""; i++) {

        QTreeWidgetItem *add;
        ColorButton *colorButton = new ColorButton(this, colorSet[i].name, colorSet[i].color);
        add = new QTreeWidgetItem(colors->invisibleRootItem());
        add->setData(0, Qt::UserRole, i); // remember which index it is for since gets sorted
        add->setText(0, colorSet[i].group);
        add->setText(1, colorSet[i].name);
        colors->setItemWidget(add, 2, colorButton);

    }
    colors->setSortingEnabled(true);
    colors->sortByColumn(1, Qt::AscendingOrder); // first sort by name
    colors->sortByColumn(0, Qt::AscendingOrder); // now by group

    connect(applyTheme, SIGNAL(clicked()), this, SLOT(applyThemeClicked()));

    foreach(ColorTheme theme, GCColor::themes().themes) {

        QTreeWidgetItem *add;
        ColorLabel *swatch = new ColorLabel(theme);
        swatch->setFixedHeight(30*dpiYFactor);
        add = new QTreeWidgetItem(themes->invisibleRootItem());
        themes->setItemWidget(add, 0, swatch);
        add->setText(1, theme.name);

    }
    connect(colorTab, SIGNAL(currentChanged(int)), this, SLOT(tabChanged()));
    connect(def, SIGNAL(currentFontChanged(QFont)), this, SLOT(scaleFont()));
    connect(fontscale, SIGNAL(valueChanged(int)), this, SLOT(scaleFont()));
    connect(searchEdit, SIGNAL(textChanged(QString)), this, SLOT(searchFilter(QString)));

    // save initial values
    b4.alias = antiAliased->isChecked();
#ifndef Q_OS_MAC
    b4.macForms = macForms->isChecked();
    b4.scroll = rideScroll->isChecked();
    b4.head = rideHead->isChecked();
#endif
    b4.line = lineWidth->value();
    b4.fingerprint = Colors::fingerprint(colorSet);
}

void
ColorsPage::searchFilter(QString text)
{
    QStringList toks = text.split(" ", Qt::SkipEmptyParts);
    bool empty;
    if (toks.count() == 0 || text == "") empty=true;
    else empty=false;

    for(int i=0; i<colors->invisibleRootItem()->childCount(); i++) {
        if (empty) colors->setRowHidden(i, colors->rootIndex(), false);
        else {
            QString text = colors->invisibleRootItem()->child(i)->text(1);
            bool found=false;
            foreach(QString tok, toks) {
                if (text.contains(tok, Qt::CaseInsensitive)) {
                    found = true;
                    break;
                }
            }
            colors->setRowHidden(i, colors->rootIndex(), !found);
        }
    }
}

void
ColorsPage::scaleFont()
{
    QFont font=baseFont;
    font.setFamily(def->currentFont().family());
    font.setPointSizeF(baseFont.pointSizeF()*scalefactors[fontscale->value()]);
    fonttext->setFont(font);
}

void
ColorsPage::tabChanged()
{
    // are we on the them page
    if (colorTab->currentIndex() == 0) applyTheme->show();
    else applyTheme->hide();
}

void
ColorsPage::applyThemeClicked()
{
    int index=0;

    // first check we have a selection!
    if (themes->currentItem() && (index=themes->invisibleRootItem()->indexOfChild(themes->currentItem())) >= 0) {

        applyThemeIndex(index);
    }
}

void
ColorsPage::applyThemeIndex(int index)
{
        // now get the theme selected
        ColorTheme theme = GCColor::themes().themes[index];

        // reset to base
        colorSet = GCColor::defaultColorSet(theme.dark);

        // reset the color selection tools
        colors->clear();
        colors->setSortingEnabled(false);

        for (int i=0; colorSet[i].name != ""; i++) {

            // apply theme to color
            QColor color = GCColor::getThemeColor(theme, i);

            QTreeWidgetItem *add;
            ColorButton *colorButton = new ColorButton(this, colorSet[i].name, color);
            add = new QTreeWidgetItem(colors->invisibleRootItem());
            add->setData(0, Qt::UserRole, i); // remember which index it is for since gets sorted
            add->setText(0, colorSet[i].group);
            add->setText(1, colorSet[i].name);
            colors->setItemWidget(add, 2, colorButton);

        }
        colors->setSortingEnabled(true);
        colors->sortByColumn(1, Qt::AscendingOrder); // first sort by name
        colors->sortByColumn(0, Qt::AscendingOrder);
}

void
ColorsPage::resetClicked()
{
    AppearanceSettings defaults = GSettings::defaultAppearanceSettings();

    def->setCurrentFont(QFont(defaults.fontfamily));
    fontscale->setValue(defaults.fontscaleindex);
    lineWidth->setValue(defaults.linewidth);
    antiAliased->setChecked(defaults.antialias);
#ifndef Q_OS_MAC // they do scrollbars nicely
    macForms->setChecked(defaults.macForms);
    rideHead->setChecked(defaults.head);
    rideScroll->setChecked(defaults.scrollbar);
#endif

    applyThemeIndex(defaults.theme);
}

qint32
ColorsPage::saveClicked()
{
    appsettings->setValue(GC_LINEWIDTH, lineWidth->value());
    appsettings->setValue(GC_ANTIALIAS, antiAliased->isChecked());
    appsettings->setValue(GC_FONT_SCALE, scalefactors[fontscale->value()]);
    appsettings->setValue(GC_FONT_DEFAULT, def->font().family());
#ifndef Q_OS_MAC
    appsettings->setValue(GC_MAC_FORMS, macForms->isChecked());
    appsettings->setValue(GC_RIDESCROLL, rideScroll->isChecked());
    appsettings->setValue(GC_RIDEHEAD, rideHead->isChecked());
#endif

    // run down and get the current colors and save
    for (int i=0; colorSet[i].name != ""; i++) {
        QTreeWidgetItem *current = colors->invisibleRootItem()->child(i);
        QColor newColor = ((ColorButton*)colors->itemWidget(current, 2))->getColor();
        QString colorstring = QString("%1:%2:%3").arg(newColor.red())
                                                 .arg(newColor.green())
                                                 .arg(newColor.blue());
        int colornum = current->data(0, Qt::UserRole).toInt();
        appsettings->setValue(colorSet[colornum].setting, colorstring);
    }

    // update basefont family
    baseFont.setFamily(def->currentFont().family());

    // application font needs to adapt to scale
    QFont font = baseFont;
    font.setPointSizeF(baseFont.pointSizeF() * scalefactors[fontscale->value()]);
    QApplication::setFont(font);

    // set the "old" user settings to ensure charts adopt etc
    appsettings->setValue(GC_FONT_DEFAULT, def->currentFont().toString());
    appsettings->setValue(GC_FONT_CHARTLABELS, def->currentFont().toString());
    appsettings->setValue(GC_FONT_DEFAULT_SIZE, font.pointSizeF());
    appsettings->setValue(GC_FONT_CHARTLABELS_SIZE, font.pointSizeF() * 0.8);

    // reread into colorset so we can check for changes
    GCColor::readConfig();

    // did we change anything ?
    if(b4.alias != antiAliased->isChecked() ||
#ifndef Q_OS_MAC // not needed on mac
       b4.scroll != rideScroll->isChecked() ||
       b4.head != rideHead->isChecked() ||
#endif
       b4.line != lineWidth->value() ||
       b4.fontscale != scalefactors[fontscale->value()] ||
       b4.fingerprint != Colors::fingerprint(colorSet))
        return CONFIG_APPEARANCE;
    else
        return 0;
}

FavouriteMetricsPage::FavouriteMetricsPage(QWidget *parent) :
    QWidget(parent), changed(false)
{
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_Metrics_Favourites));

    availList = new QListWidget;
    availList->setSortingEnabled(true);
    availList->setAlternatingRowColors(true);
    availList->setSelectionMode(QAbstractItemView::SingleSelection);

    QVBoxLayout *availLayout = new QVBoxLayout;
    availLayout->addWidget(new QLabel(HLO + tr("Available Metrics") + HLC));
    availLayout->addWidget(availList);

    selectedList = new QListWidget;
    selectedList->setAlternatingRowColors(true);
    selectedList->setSelectionMode(QAbstractItemView::SingleSelection);

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::UpDownGroup);
    actionButtons->defaultConnect(ActionButtonBox::UpDownGroup, selectedList);

    QVBoxLayout *selectedLayout = new QVBoxLayout;
    selectedLayout->addWidget(new QLabel(HLO + tr("Favourites") + HLC));
    selectedLayout->addWidget(selectedList);
    selectedLayout->addWidget(actionButtons);

#ifndef Q_OS_MAC
    leftButton = new QToolButton(this);
    leftButton->setArrowType(Qt::LeftArrow);
    leftButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    rightButton = new QToolButton(this);
    rightButton->setArrowType(Qt::RightArrow);
    rightButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    leftButton = new QPushButton("<");
    rightButton = new QPushButton(">");
#endif
    leftButton->setEnabled(false);
    rightButton->setEnabled(false);

    QHBoxLayout *inexcLayout = new QHBoxLayout;
    inexcLayout->addStretch();
    inexcLayout->addWidget(leftButton);
    inexcLayout->addWidget(rightButton);
    inexcLayout->addStretch();

    QVBoxLayout *buttonGrid = new QVBoxLayout;
    buttonGrid->addStretch();
    buttonGrid->addLayout(inexcLayout);
    buttonGrid->addStretch();

    QHBoxLayout *hlayout = new QHBoxLayout(this);;
    hlayout->addLayout(availLayout, 2);
    hlayout->addLayout(buttonGrid, 1);
    hlayout->addLayout(selectedLayout, 2);

    QString s;
    if (appsettings->contains(GC_SETTINGS_FAVOURITE_METRICS))
        s = appsettings->value(this, GC_SETTINGS_FAVOURITE_METRICS).toString();
    else
        s = GC_SETTINGS_FAVOURITE_METRICS_DEFAULT;
    QStringList selectedMetrics = s.split(",");

    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i = 0; i < factory.metricCount(); ++i) {
        QString symbol = factory.metricName(i);
        if (selectedMetrics.contains(symbol) || symbol.startsWith("compatibility_"))
            continue;
        QSharedPointer<RideMetric> m(factory.newMetric(symbol));
        QListWidgetItem *item = new QListWidgetItem(Utils::unprotect(m->name()));
        item->setData(Qt::UserRole, symbol);
        item->setToolTip(m->description());
        availList->addItem(item);
    }
    foreach (QString symbol, selectedMetrics) {
        if (!factory.haveMetric(symbol))
            continue;
        QSharedPointer<RideMetric> m(factory.newMetric(symbol));
        QListWidgetItem *item = new QListWidgetItem(Utils::unprotect(m->name()));
        item->setData(Qt::UserRole, symbol);
        item->setToolTip(m->description());
        selectedList->addItem(item);
    }

    connect(actionButtons, &ActionButtonBox::upRequested, this, &FavouriteMetricsPage::upClicked);
    connect(actionButtons, &ActionButtonBox::downRequested, this, &FavouriteMetricsPage::downClicked);
    connect(leftButton, SIGNAL(clicked()), this, SLOT(leftClicked()));
    connect(rightButton, SIGNAL(clicked()), this, SLOT(rightClicked()));
    connect(availList, SIGNAL(itemSelectionChanged()), this, SLOT(availChanged()));
    connect(selectedList, SIGNAL(itemSelectionChanged()), this, SLOT(selectedChanged()));
}

void
FavouriteMetricsPage::upClicked()
{
    assert(!selectedList->selectedItems().isEmpty());
    QListWidgetItem *item = selectedList->selectedItems().first();
    int row = selectedList->row(item);
    assert(row > 0);
    selectedList->takeItem(row);
    selectedList->insertItem(row - 1, item);
    selectedList->setCurrentItem(item);
    changed = true;
}

void
FavouriteMetricsPage::downClicked()
{
    assert(!selectedList->selectedItems().isEmpty());
    QListWidgetItem *item = selectedList->selectedItems().first();
    int row = selectedList->row(item);
    assert(row < selectedList->count() - 1);
    selectedList->takeItem(row);
    selectedList->insertItem(row + 1, item);
    selectedList->setCurrentItem(item);
    changed = true;
}

void
FavouriteMetricsPage::leftClicked()
{
    assert(!selectedList->selectedItems().isEmpty());
    QListWidgetItem *item = selectedList->selectedItems().first();
    selectedList->takeItem(selectedList->row(item));
    availList->addItem(item);
    changed = true;
    selectedChanged();
}

void
FavouriteMetricsPage::rightClicked()
{
    assert(!availList->selectedItems().isEmpty());
    QListWidgetItem *item = availList->selectedItems().first();
    availList->takeItem(availList->row(item));
    selectedList->addItem(item);
    changed = true;
}

void
FavouriteMetricsPage::availChanged()
{
    rightButton->setEnabled(!availList->selectedItems().isEmpty());
}

void
FavouriteMetricsPage::selectedChanged()
{
    if (selectedList->selectedItems().isEmpty()) {
        leftButton->setEnabled(false);
        return;
    }
    leftButton->setEnabled(true);
}

qint32
FavouriteMetricsPage::saveClicked()
{
    if (!changed) return 0;

    QStringList metrics;
    for (int i = 0; i < selectedList->count(); ++i)
        metrics << selectedList->item(i)->data(Qt::UserRole).toString();
    appsettings->setValue(GC_SETTINGS_FAVOURITE_METRICS, metrics.join(","));

    return 0;
}

static quint16 userMetricsCRC(QList<UserMetricSettings> userMetrics)
{
    // run through metrics and compute a CRC to detect changes
    quint16 crc = 0;
    foreach(UserMetricSettings userMetric, userMetrics)
        crc += userMetric.getCRC();

    return crc;
}

CustomMetricsPage::CustomMetricsPage(QWidget *parent, Context *context) :
    QWidget(parent), context(context)
{
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_Metrics_Custom));

    // copy as current, so we can edit...
    metrics = _userMetrics;
    b4.crc = userMetricsCRC(metrics);

    table = new QTreeWidget;
    table->headerItem()->setText(0, tr("Symbol"));
    table->headerItem()->setText(1, tr("Name"));
    table->setColumnCount(2);
    basicTreeWidgetStyle(table);

    exportButton = new QPushButton(tr("Export"));
    exportButton->setEnabled(false);
    importButton = new QPushButton(tr("Import"));
    importButton->setEnabled(false);
#ifdef GC_HAS_CLOUD_DB
    uploadButton = new QPushButton(tr("Upload"));
    uploadButton->setEnabled(false);
    downloadButton = new QPushButton(tr("Download"));
    downloadButton->setEnabled(false);
#endif

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup | ActionButtonBox::EditGroup);
    actionButtons->setButtonEnabled(ActionButtonBox::Edit, false);
    actionButtons->defaultConnect(ActionButtonBox::AddDeleteGroup, table);

    actionButtons->addWidget(exportButton);
    actionButtons->addWidget(importButton);
#ifdef GC_HAS_CLOUD_DB
    actionButtons->addStretch();
    actionButtons->addWidget(uploadButton);
    actionButtons->addWidget(downloadButton);
#endif

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(table);
    layout->addWidget(actionButtons);

    connect(table, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(doubleClicked(QTreeWidgetItem*, int)));
    connect(table, &QTreeWidget::currentItemChanged, [=] (QTreeWidgetItem*) {
            bool selected = table->currentItem() != nullptr;
            actionButtons->setButtonEnabled(ActionButtonBox::Edit, selected);
            exportButton->setEnabled(selected);
            importButton->setEnabled(selected);
#ifdef GC_HAS_CLOUD_DB
            uploadButton->setEnabled(selected);
            downloadButton->setEnabled(selected);
#endif
        });
    connect(actionButtons, &ActionButtonBox::addRequested, this, &CustomMetricsPage::addClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &CustomMetricsPage::deleteClicked);
    connect(actionButtons, &ActionButtonBox::editRequested, this, &CustomMetricsPage::editClicked);
    connect(exportButton, SIGNAL(clicked()), this, SLOT(exportClicked()));
    connect(importButton, SIGNAL(clicked()), this, SLOT(importClicked()));
#ifdef GC_HAS_CLOUD_DB
    connect(uploadButton, SIGNAL(clicked()), this, SLOT(uploadClicked()));
    connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadClicked()));
#endif

    refreshTable();
}

void
CustomMetricsPage::refreshTable()
{
    table->clear();
    skipcompat=0;
    bool first = true;
    foreach(UserMetricSettings m, metrics) {
        if (m.symbol.startsWith("compatibility_")) {
            skipcompat++;
            continue;
        }

        // user metrics are silently discarded if the symbol is already in use
        if (!table->findItems(m.symbol, Qt::MatchExactly, 0).isEmpty())
            QMessageBox::warning(this, tr("User Metrics"), tr("Duplicate Symbol: %1, one metric will be discarded").arg(m.symbol));

        // duplicate names are allowed, but not recommended
        if (!table->findItems(m.name, Qt::MatchExactly, 1).isEmpty())
            QMessageBox::warning(this, tr("User Metrics"), tr("Duplicate Name: %1, one metric will not be acessible in formulas").arg(m.name));

        QTreeWidgetItem *add = new QTreeWidgetItem(table->invisibleRootItem());
        add->setText(0, m.symbol);
        add->setToolTip(0, m.description);
        add->setText(1, m.name);
        add->setToolTip(1, m.description);
        if (first) {
            table->setCurrentItem(add);
            first = false;
        }
    }
}

void
CustomMetricsPage::deleteClicked()
{
    // nothing selected
    if (table->selectedItems().count() <= 0) return;

    // which one?
    QTreeWidgetItem *item = table->selectedItems().first();
    int row = table->invisibleRootItem()->indexOfChild(item);

    // Are you sure ?
    QMessageBox msgBox;
    msgBox.setText(tr("Are you sure you want to delete this metric?"));
    msgBox.setInformativeText(metrics[row+skipcompat].name);
    QPushButton *deleteButton = msgBox.addButton(tr("Remove"),QMessageBox::YesRole);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();

    // nope, don't want to
    if(msgBox.clickedButton() != deleteButton) return;

    // wipe it away
    metrics.removeAt(row+skipcompat);

    // refresh
    refreshTable();
}

void
CustomMetricsPage::addClicked()
{
    UserMetricSettings here;

    // provide an example program to edit....
    here.symbol = "My_Average_Power";
    here.name = "My Average Power";
    here.type = 1;
    here.precision = 0;
    here.istime = false;
    here.description = "Average Power";
    here.unitsMetric = "watts";
    here.unitsImperial = "watts";
    here.conversion = 1.00;
    here.conversionSum = 0.00;
    here.program = R"({
    # only calculate for rides containing power
    relevant { Data contains "P"; }

    # initialise aggregating variables
    # does nothing, update as needed
    init { 0; }

    # calculate metric value at end
    value { mean(samples(POWER)); }
    count { Duration; }
})";

    EditUserMetricDialog editor(this, context, here);
    if (editor.exec() == QDialog::Accepted) {
        // add to the list
        metrics.append(here);
        refreshTable();
    }
}

void
CustomMetricsPage::editClicked()
{
    // nothing selected
    if (table->selectedItems().count() <= 0) return;

    // which one?
    QTreeWidgetItem *item = table->selectedItems().first();

    doubleClicked(item, 0);
}

void
CustomMetricsPage::doubleClicked(QTreeWidgetItem *item, int)
{
    // nothing selected
    if (item == NULL) return;

    // find row
    int row = table->invisibleRootItem()->indexOfChild(item);

    // edit it
    UserMetricSettings here = metrics[row+skipcompat];

    EditUserMetricDialog editor(this, context, here);
    if (editor.exec() == QDialog::Accepted) {
        // add to the list
        metrics[row+skipcompat] = here;
        refreshTable();
    }
}

void
CustomMetricsPage::exportClicked()
{
    // nothing selected
    if (table->selectedItems().count() <= 0) return;

    // which one?
    QTreeWidgetItem *item = table->selectedItems().first();

    // nothing selected
    if (item == NULL) return;

    // find row
    int row = table->invisibleRootItem()->indexOfChild(item);

    // metric to export
    UserMetricSettings here = metrics[row+skipcompat];

    // get a filename to export to...
    QString filename = QFileDialog::getSaveFileName(this, tr("Export Metric"), QDir::homePath() + "/" + here.symbol + ".gmetric", tr("GoldenCheetah Metric File (*.gmetric)"));

    // nothing given
    if (filename.isEmpty()) return;

    UserMetricParser::serialize(filename, QList<UserMetricSettings>() << here);
}

void
CustomMetricsPage::importClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Metric file to import"), "", tr("GoldenCheetah Metric Files (*.gmetric)"));

    if (fileName.isEmpty()) {
        QMessageBox::critical(this, tr("Import Metric"), tr("No Metric file selected!"));
        return;
    }

    QList<UserMetricSettings> imported;
    QFile metricFile(fileName);

    // setup XML processor
    QXmlInputSource source( &metricFile );
    QXmlSimpleReader xmlReader;
    UserMetricParser handler;
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);

    // parse and get return values
    xmlReader.parse(source);
    imported = handler.getSettings();
    if (imported.isEmpty()) {
        QMessageBox::critical(this, tr("Import Metric"), tr("No Metric found in the selected file!"));
        return;
    }

    UserMetricSettings here = imported.first();

    EditUserMetricDialog editor(this, context, here);
    if (editor.exec() == QDialog::Accepted) {
        // add to the list
        metrics.append(here);
        refreshTable();
    }
}

#ifdef GC_HAS_CLOUD_DB
void
CustomMetricsPage::uploadClicked()
{
    // nothing selected
    if (table->selectedItems().count() <= 0) return;

    // which one?
    QTreeWidgetItem *item = table->selectedItems().first();

    // nothing selected
    if (item == NULL) return;

    // find row
    int row = table->invisibleRootItem()->indexOfChild(item);

    // metric to export
    UserMetricSettings here = metrics[row+skipcompat];

    // check for CloudDB T&C acceptance
    if (!(appsettings->cvalue(context->athlete->cyclist, GC_CLOUDDB_TC_ACCEPTANCE, false).toBool())) {
        CloudDBAcceptConditionsDialog acceptDialog(context->athlete->cyclist);
        acceptDialog.setModal(true);
        if (acceptDialog.exec() == QDialog::Rejected) {
            return;
        }
    }

    UserMetricAPIv1 usermetric;
    usermetric.Header.Key = here.symbol;
    usermetric.Header.Name = here.name;
    usermetric.Header.Description = here.description;
    int version = VERSION_LATEST;
    usermetric.Header.GcVersion =  QString::number(version);
    // get the usermetric - definition xml
    QTextStream out(&usermetric.UserMetricXML);
    UserMetricParser::serializeToQTextStream(out, QList<UserMetricSettings>() << here);

    usermetric.Header.CreatorId = appsettings->cvalue(context->athlete->cyclist, GC_ATHLETE_ID, "").toString();
    usermetric.Header.Curated = false;
    usermetric.Header.Deleted = false;

    // now complete the usermetric with for the user manually added fields
    CloudDBUserMetricObjectDialog dialog(usermetric, context->athlete->cyclist);
    if (dialog.exec() == QDialog::Accepted) {
        CloudDBUserMetricClient c;
        if (c.postUserMetric(dialog.getUserMetric())) {
            CloudDBHeader::setUserMetricHeaderStale(true);
        }
    }

}

void
CustomMetricsPage::downloadClicked()
{
    if (!(appsettings->cvalue(context->athlete->cyclist, GC_CLOUDDB_TC_ACCEPTANCE, false).toBool())) {
       CloudDBAcceptConditionsDialog acceptDialog(context->athlete->cyclist);
       acceptDialog.setModal(true);
       if (acceptDialog.exec() == QDialog::Rejected) {
          return;
       }
    }

    if (context->cdbUserMetricListDialog == NULL) {
        context->cdbUserMetricListDialog = new CloudDBUserMetricListDialog();
    }

    if (context->cdbUserMetricListDialog->prepareData(context->athlete->cyclist, CloudDBCommon::UserImport)) {
        if (context->cdbUserMetricListDialog->exec() == QDialog::Accepted) {

            QList<QString> usermetricDefs = context->cdbUserMetricListDialog->getSelectedSettings();

            foreach (QString usermetricDef, usermetricDefs) {
                QList<UserMetricSettings> imported;

                // setup XML processor
                QXmlInputSource source;
                source.setData(usermetricDef);
                QXmlSimpleReader xmlReader;
                UserMetricParser handler;
                xmlReader.setContentHandler(&handler);
                xmlReader.setErrorHandler(&handler);

                // parse and get return values
                xmlReader.parse(source);
                imported = handler.getSettings();
                if (imported.isEmpty()) {
                    QMessageBox::critical(this, tr("Download Metric"), tr("No valid Metric found!"));
                    continue;
                }

                UserMetricSettings here = imported.first();

                EditUserMetricDialog editor(this, context, here);
                if (editor.exec() == QDialog::Accepted) {
                    // add to the list
                    metrics.append(here);
                    refreshTable();
                }
            }
        }
    }
}
#endif

qint32
CustomMetricsPage::saveClicked()
{
    qint32 returning=0;

    // did we actually change them ?
    if (b4.crc != userMetricsCRC(metrics))
        returning |= CONFIG_USERMETRICS;

    // save away OUR version to top-level NOT athlete dir
    // and don't overwrite in memory version the signal handles that
    QString filename = QString("%1/../usermetrics.xml").arg(context->athlete->home->root().absolutePath());
    UserMetricParser::serialize(filename, metrics);

    // the change will need to be signalled by context, not us
    return returning;
}

MetadataPage::MetadataPage(Context *context) : context(context)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // get current config using default file
    keywordDefinitions = GlobalContext::context()->rideMetadata->getKeywords();
    fieldDefinitions = GlobalContext::context()->rideMetadata->getFields();
    colorfield = GlobalContext::context()->rideMetadata->getColorField();
    defaultDefinitions = GlobalContext::context()->rideMetadata->getDefaults();

    // setup maintenance pages using current config
    fieldsPage = new FieldsPage(this, fieldDefinitions);
    keywordsPage = new KeywordsPage(this, keywordDefinitions);
    iconsPage = new IconsPage(fieldDefinitions, this);
    defaultsPage = new DefaultsPage(this, defaultDefinitions);
    processorPage = new ProcessorPage(context);

    tabs = new QTabWidget(this);
    tabs->addTab(fieldsPage, tr("Fields"));
    tabs->addTab(keywordsPage, tr("Colour Keywords"));
    tabs->addTab(iconsPage, tr("Icons"));
    tabs->addTab(defaultsPage, tr("Defaults"));
    tabs->addTab(processorPage, tr("Processors && Automation"));

    // refresh the keywords combo when change tabs .. will do more often than
    // needed but better that than less often than needed
    connect (tabs, SIGNAL(currentChanged(int)), keywordsPage, SLOT(pageSelected()));

    layout->addWidget(tabs);

    // save initial values
    b4.keywordFingerprint = KeywordDefinition::fingerprint(keywordDefinitions);
    b4.colorfield = colorfield;
    b4.fieldFingerprint = FieldDefinition::fingerprint(fieldDefinitions);
}

qint32
MetadataPage::saveClicked()
{
    // get current state
    fieldsPage->getDefinitions(fieldDefinitions);
    keywordsPage->getDefinitions(keywordDefinitions);
    defaultsPage->getDefinitions(defaultDefinitions);

    // save settings
    appsettings->setValue(GC_RIDEBG, keywordsPage->rideBG->isChecked());

    // write to metadata.xml
    RideMetadata::serialize(QDir(gcroot).canonicalPath() + "/metadata.xml", keywordDefinitions, fieldDefinitions, colorfield, defaultDefinitions);

    // save processors config
    processorPage->saveClicked();

    qint32 state = 0;

    if (b4.keywordFingerprint != KeywordDefinition::fingerprint(keywordDefinitions) || b4.colorfield != colorfield)
        state += CONFIG_NOTECOLOR;

    if (b4.fieldFingerprint != FieldDefinition::fingerprint(fieldDefinitions))
        state += CONFIG_FIELDS;

    state |= iconsPage->saveClicked();

    return state;
}


//
// Calendar coloring page
//
KeywordsPage::KeywordsPage(MetadataPage *parent, QList<KeywordDefinition>keywordDefinitions) : QWidget(parent), parent(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_DataFields_Notes_Keywords));

    relatedDelegate.setTitle(tr("<h3>Alternative Keywords</h3>Add additional keyword to have the same color"));

    QHBoxLayout *field = new QHBoxLayout();
    fieldLabel = new QLabel(tr("Field"),this);
    fieldChooser = new QComboBox(this);
    field->addWidget(fieldLabel);
    field->addWidget(fieldChooser);
    field->addStretch();
    mainLayout->addLayout(field);

    rideBG = new QCheckBox(tr("Use for Background"));
    rideBG->setChecked(appsettings->value(this, GC_RIDEBG, false).toBool());
    field->addWidget(rideBG);

    keywords = new QTreeWidget;
    keywords->headerItem()->setText(0, tr("Keyword"));
    keywords->headerItem()->setText(1, tr("Color"));
    keywords->headerItem()->setText(2, tr("Related Notes Words"));
    keywords->setColumnCount(3);
    keywords->setItemDelegateForColumn(2, &relatedDelegate);
    basicTreeWidgetStyle(keywords);

    actionButtons = new ActionButtonBox(ActionButtonBox::UpDownGroup | ActionButtonBox::AddDeleteGroup);
    actionButtons->defaultConnect(ActionButtonBox::UpDownGroup, keywords);
    actionButtons->defaultConnect(ActionButtonBox::AddDeleteGroup, keywords);

    foreach(KeywordDefinition keyword, keywordDefinitions) {
        QTreeWidgetItem *add;
        ColorButton *colorButton = new ColorButton(this, keyword.name, keyword.color);
        add = new QTreeWidgetItem(keywords->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // keyword
        add->setText(0, keyword.name);

        // color button
        add->setTextAlignment(1, Qt::AlignHCenter);
        keywords->setItemWidget(add, 1, colorButton);

        QString text;
        for (int i=0; i< keyword.tokens.count(); i++) {
            if (i != keyword.tokens.count()-1)
                text += keyword.tokens[i] + ",";
            else
                text += keyword.tokens[i];
        }

        // notes texts
        add->setText(2, text);
    }

    mainLayout->addWidget(keywords);
    mainLayout->addWidget(actionButtons);

    // connect up slots
    connect(actionButtons, &ActionButtonBox::upRequested, this, &KeywordsPage::upClicked);
    connect(actionButtons, &ActionButtonBox::downRequested, this, &KeywordsPage::downClicked);
    connect(actionButtons, &ActionButtonBox::addRequested, this, &KeywordsPage::addClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &KeywordsPage::deleteClicked);
    connect(fieldChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(colorfieldChanged()));

    keywords->setCurrentItem(keywords->invisibleRootItem()->child(0));
}

void
KeywordsPage::pageSelected()
{
    SpecialFields& sp = SpecialFields::getInstance();
    QString prev = "";

    // remember what was selected, if anything?
    if (fieldChooser->count()) {
        prev = sp.internalName(fieldChooser->itemText(fieldChooser->currentIndex()));
        parent->colorfield = prev;
    } else prev = parent->colorfield;
    // load in texts from metadata
    fieldChooser->clear();

    // get the current fields definitions
    QList<FieldDefinition> fromFieldsPage;
    parent->fieldsPage->getDefinitions(fromFieldsPage);
    foreach(FieldDefinition x, fromFieldsPage) {
        if (x.isTextField()) {
            fieldChooser->addItem(sp.displayName(x.name));
        }
    }
    fieldChooser->setCurrentIndex(fieldChooser->findText(sp.displayName(prev)));
}

void
KeywordsPage::colorfieldChanged()
{
    int index = fieldChooser->currentIndex();
    if (index >=0) parent->colorfield = SpecialFields::getInstance().internalName(fieldChooser->itemText(fieldChooser->currentIndex()));
}


void
KeywordsPage::upClicked()
{
    if (keywords->currentItem()) {
        int index = keywords->invisibleRootItem()->indexOfChild(keywords->currentItem());
        if (index == 0) return; // its at the top already

        // movin on up!
        QWidget *button = keywords->itemWidget(keywords->currentItem(),1);
        ColorButton *colorButton = new ColorButton(this, ((ColorButton*)button)->getName(), ((ColorButton*)button)->getColor());
        QTreeWidgetItem* moved = keywords->invisibleRootItem()->takeChild(index);
        keywords->invisibleRootItem()->insertChild(index-1, moved);
        keywords->setItemWidget(moved, 1, colorButton);
        keywords->setCurrentItem(moved);
        //LTMSettings save = (*presets)[index];
        //presets->removeAt(index);
        //presets->insert(index-1, save);
    }
}

void
KeywordsPage::downClicked()
{
    if (keywords->currentItem()) {
        int index = keywords->invisibleRootItem()->indexOfChild(keywords->currentItem());
        if (index == (keywords->invisibleRootItem()->childCount()-1)) return; // its at the bottom already

        // movin on up!
        QWidget *button = keywords->itemWidget(keywords->currentItem(),1);
        ColorButton *colorButton = new ColorButton(this, ((ColorButton*)button)->getName(), ((ColorButton*)button)->getColor());
        QTreeWidgetItem* moved = keywords->invisibleRootItem()->takeChild(index);
        keywords->invisibleRootItem()->insertChild(index+1, moved);
        keywords->setItemWidget(moved, 1, colorButton);
        keywords->setCurrentItem(moved);
    }
}

void
KeywordsPage::addClicked()
{
    int index = keywords->invisibleRootItem()->indexOfChild(keywords->currentItem());
    if (index < 0) index = 0;
    QTreeWidgetItem *add;
    ColorButton *colorButton = new ColorButton(this, tr("New"), QColor(Qt::blue));
    add = new QTreeWidgetItem;
    keywords->invisibleRootItem()->insertChild(index, add);
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    // keyword
    QString text = tr("New");
    for (int i=0; keywords->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);

    // color button
    add->setTextAlignment(1, Qt::AlignHCenter);
    keywords->setItemWidget(add, 1, colorButton);

    // notes texts
    add->setText(2, "");

    keywords->setCurrentItem(add);
}

void
KeywordsPage::deleteClicked()
{
    if (keywords->currentItem()) {
        int index = keywords->invisibleRootItem()->indexOfChild(keywords->currentItem());

        // zap!
        delete keywords->invisibleRootItem()->takeChild(index);
    }
}

void
KeywordsPage::getDefinitions(QList<KeywordDefinition> &keywordList)
{
    // clear current just in case
    keywordList.clear();

    for (int idx =0; idx < keywords->invisibleRootItem()->childCount(); idx++) {
        KeywordDefinition add;
        QTreeWidgetItem *item = keywords->invisibleRootItem()->child(idx);

        add.name = item->text(0);
        add.color = ((ColorButton*)keywords->itemWidget(item, 1))->getColor();
        add.tokens = item->text(2).split(",", Qt::SkipEmptyParts);

        keywordList.append(add);
    }
}


//
// Icons page
//

#define ICONSPAGE_L_W 64 * dpiXFactor
#define ICONSPAGE_L_H 64 * dpiXFactor
#define ICONSPAGE_L QSize(ICONSPAGE_L_W, ICONSPAGE_L_H)
#define ICONSPAGE_L_SPACE QSize(80 * dpiXFactor, 80 * dpiYFactor)
#define ICONSPAGE_S_W 48 * dpiXFactor
#define ICONSPAGE_S_H 48 * dpiXFactor
#define ICONSPAGE_S QSize(ICONSPAGE_S_W, ICONSPAGE_S_H)
#define ICONSPAGE_MARGIN 2 * dpiXFactor

IconsPage::IconsPage
(const QList<FieldDefinition> &fieldDefinitions, QWidget *parent)
: QWidget(parent), fieldDefinitions(fieldDefinitions)
{
    QPalette palette;

    sportTree = new QTreeWidget();
    sportTree->setColumnCount(3);
    basicTreeWidgetStyle(sportTree);
    sportTree->setHeaderLabels({ tr("Field"), tr("Value"), tr("Icon") });
    sportTree->setIconSize(ICONSPAGE_S);
    sportTree->setAcceptDrops(true);
    sportTree->installEventFilter(this);
    sportTree->viewport()->installEventFilter(this);
    initSportTree();

    iconList = new QListWidget();
    iconList->setViewMode(QListView::IconMode);
    iconList->setIconSize(ICONSPAGE_L);
    iconList->setGridSize(ICONSPAGE_L_SPACE);
    iconList->setResizeMode(QListView::Adjust);
    iconList->setWrapping(true);
    iconList->setFlow(QListView::LeftToRight);
    iconList->setSpacing(10 * dpiXFactor);
    iconList->setMovement(QListView::Static);
    iconList->setUniformItemSizes(true);
    iconList->setSelectionMode(QAbstractItemView::SingleSelection);
    iconList->setDragEnabled(false);
    iconList->setAcceptDrops(true);
    iconList->installEventFilter(this);
    iconList->viewport()->installEventFilter(this);
    updateIconList();

    QPixmap trashPixmap = svgAsColoredPixmap(":images/breeze/trash-empty.svg", ICONSPAGE_L, ICONSPAGE_MARGIN, palette.color(QPalette::WindowText));
    QPixmap trashPixmapActive = svgAsColoredPixmap(":images/breeze/trash-empty.svg", ICONSPAGE_L, ICONSPAGE_MARGIN, QColor("#F79130"));
    trashIcon.addPixmap(trashPixmap, QIcon::Normal);
    trashIcon.addPixmap(trashPixmapActive, QIcon::Active);

    trash = new QLabel();
    trash->setAcceptDrops(true);
    trash->setPixmap(trashIcon.pixmap(ICONSPAGE_L, QIcon::Normal));
    trash->installEventFilter(this);
    QPushButton *downloadButton = new QPushButton(tr("Download Default"));
    QPushButton *importButton = new QPushButton(tr("Import"));
    QPushButton *exportButton = new QPushButton(tr("Export"));

    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->addWidget(sportTree);
    contentLayout->addWidget(iconList);

    QHBoxLayout *actionLayout = new QHBoxLayout();
    actionLayout->addWidget(trash);
    actionLayout->addStretch();
    actionLayout->addWidget(downloadButton);
    actionLayout->addWidget(importButton);
    actionLayout->addWidget(exportButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(contentLayout);
    mainLayout->addLayout(actionLayout);

    connect(downloadButton, &QPushButton::clicked, [=]() {
        QUrl url(QString("%1/icons.zip").arg(VERSION_CONFIG_PREFIX));
        if (IconManager::instance().importBundle(url)) {
            initSportTree();
            updateIconList();
        } else {
            QMessageBox::warning(nullptr, tr("Icon Bundle"), tr("Bundle file %1 cannot be imported.").arg(url.toString()));
        }
    });
    connect(importButton, &QPushButton::clicked, [=]() {
        QString zipFile = QFileDialog::getOpenFileName(this, tr("Import Icons"), "", tr("Zip Files (*.zip)"));
        if (! zipFile.isEmpty() && IconManager::instance().importBundle(zipFile)) {
            initSportTree();
            updateIconList();
        } else {
            QMessageBox::warning(nullptr, tr("Icon Bundle"), tr("Bundle file %1 cannot be imported.").arg(zipFile));
        }
    });
    connect(exportButton, &QPushButton::clicked, [=]() {
        QString zipFile = QFileDialog::getSaveFileName(this, tr("Export Icons"), "", tr("Zip Files (*.zip)"));
        if (zipFile.isEmpty() || ! IconManager::instance().exportBundle(zipFile)) {
            QMessageBox::warning(nullptr, tr("Icon Bundle"), tr("Bundle file %1 cannot be created.").arg(zipFile));
        }
    });
}


qint32
IconsPage::saveClicked
()
{
    bool changed = false;

    int rowCount = sportTree->topLevelItemCount();
    for (int i = 0; i < rowCount; ++i) {
        QTreeWidgetItem *item = sportTree->topLevelItem(i);
        if (! item) {
            continue;
        }
        QString originalIcon = item->data(0, Qt::UserRole + 1).toString();
        QString newIcon = item->data(0, Qt::UserRole + 2).toString();
        if (originalIcon != newIcon) {
            QString type = item->data(0, Qt::UserRole).toString();
            QString key = item->data(1, Qt::DisplayRole).toString();
            IconManager::instance().assignIcon(type, key, newIcon);
        }
    }

    if (changed) {
        return CONFIG_APPEARANCE;
    } else {
        return 0;
    }
}


bool
IconsPage::eventFilter
(QObject *watched, QEvent *event)
{
    bool handled = false;
    if (watched == trash) {
        handled = eventFilterTrash(event);
    } else if (watched == sportTree) {
        handled = eventFilterSportTree(event);
    } else if (watched == sportTree->viewport()) {
        handled = eventFilterSportTreeViewport(event);
    } else if (watched == iconList) {
        handled = eventFilterIconList(event);
    } else if (watched == iconList->viewport()) {
        handled = eventFilterIconListViewport(event);
    }
    return handled ? true : QObject::eventFilter(watched, event);
}


bool
IconsPage::eventFilterTrash
(QEvent *event)
{
    if (   event->type() == QEvent::DragEnter
        || event->type() == QEvent::Drop) {
        QDropEvent *dropEvent = static_cast<QDropEvent*>(event);
        if (   dropEvent->mimeData()->hasFormat("application/x-gc-icon")
            && (dropEvent->possibleActions() & Qt::MoveAction)
            && (   dropEvent->source() == sportTree
                || dropEvent->source() == iconList)) {
            dropEvent->setDropAction(Qt::MoveAction);
            dropEvent->accept();
            trash->setPixmap(trashIcon.pixmap(ICONSPAGE_L, event->type() == QEvent::Drop ? QIcon::Normal : QIcon::Active));
            return true;
        }
    } else if (event->type() == QEvent::DragLeave) {
        trash->setPixmap(trashIcon.pixmap(ICONSPAGE_L, QIcon::Normal));
        return true;
    }
    return false;
}


bool
IconsPage::eventFilterSportTree
(QEvent *event)
{
    if (event->type() == QEvent::DragEnter) {
        QDragEnterEvent *dragEnterEvent = static_cast<QDragEnterEvent*>(event);
        if (   dragEnterEvent->mimeData()->hasFormat("application/x-gc-icon")
            && (dragEnterEvent->possibleActions() & Qt::LinkAction)
            && dragEnterEvent->source() == iconList) {
            sportTree->setStyleSheet("QTreeWidget { border: 2px solid #F79130; border-radius: 8px; }");
            dragEnterEvent->setDropAction(Qt::LinkAction);
            dragEnterEvent->accept();
            return true;
        }
    } else if (event->type() == QEvent::DragMove) {
        QDragMoveEvent *dragMoveEvent = static_cast<QDragMoveEvent*>(event);
        if (   dragMoveEvent->mimeData()->hasFormat("application/x-gc-icon")
            && (dragMoveEvent->possibleActions() & Qt::LinkAction)
            && dragMoveEvent->source() == iconList) {
            QPoint globalCursor = QCursor::pos();
            QPoint pos = sportTree->viewport()->mapFromGlobal(globalCursor);
            QTreeWidgetItem* targetItem = sportTree->itemAt(pos);
            if (targetItem) {
                sportTree->setCurrentItem(targetItem);
                dragMoveEvent->setDropAction(Qt::LinkAction);
                dragMoveEvent->accept();
            } else {
                dragMoveEvent->ignore();
            }
            return true;
        }
    } else if (event->type() == QEvent::DragLeave) {
        sportTree->setStyleSheet("");
        return true;
    } else if (event->type() == QEvent::Drop) {
        QDropEvent *dropEvent = static_cast<QDropEvent*>(event);
        if (   dropEvent->mimeData()->hasFormat("application/x-gc-icon")
            && (dropEvent->possibleActions() & Qt::LinkAction)
            && dropEvent->source() == iconList) {
            QByteArray iconBytes = dropEvent->mimeData()->data("application/x-gc-icon");
            QString iconFile = QString::fromUtf8(iconBytes);
            QPoint globalCursor = QCursor::pos();
            QPoint pos = sportTree->viewport()->mapFromGlobal(globalCursor);
            QTreeWidgetItem* targetItem = sportTree->itemAt(pos);
            if (targetItem) {
                QPalette palette;
                QElapsedTimer timer;
                timer.start();
                QPixmap pixmap = svgAsColoredPixmap(IconManager::instance().toFilepath(iconFile), QSize(1000, 1000), 0, palette.color(QPalette::Text));
                qint64 elapsed = timer.elapsed();
                if (   elapsed < 50
                    || QMessageBox::question(this, tr("Complex Icon"), tr("The selected icon %1 appears to be complex and could impact performance. Are you sure you want to use this icon?").arg(iconFile)) == QMessageBox::Yes) {
                    QPixmap pixmap = svgAsColoredPixmap(IconManager::instance().toFilepath(iconFile), ICONSPAGE_S, ICONSPAGE_MARGIN, palette.color(QPalette::Text));
                    targetItem->setData(0, Qt::UserRole + 2, iconFile);
                    targetItem->setIcon(2, QIcon(pixmap));
                    dropEvent->setDropAction(Qt::LinkAction);
                    dropEvent->accept();
                } else {
                    dropEvent->ignore();
                }
            }
        }
        sportTree->setStyleSheet("");
        return true;
    }
    return false;
}


bool
IconsPage::eventFilterSportTreeViewport
(QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
#if QT_VERSION >= 0x060000
            QTreeWidgetItem *item = sportTree->itemAt(mouseEvent->position().toPoint());
#else
            QTreeWidgetItem *item = sportTree->itemAt(mouseEvent->pos());
#endif
            if (item && ! item->data(0, Qt::UserRole + 2).toString().isEmpty()) {
#if QT_VERSION >= 0x060000
                sportTreeDragStartPos = mouseEvent->position().toPoint();
#else
                sportTreeDragStartPos = mouseEvent->pos();
#endif
                sportTreeDragWatch = true;
            } else {
                sportTreeDragWatch = false;
            }
        }
        return true;
    } else if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (   ! (mouseEvent->buttons() & Qt::LeftButton)
            || ! sportTreeDragWatch
#if QT_VERSION >= 0x060000
            || (mouseEvent->position().toPoint() - sportTreeDragStartPos).manhattanLength() < QApplication::startDragDistance()) {
#else
            || (mouseEvent->pos() - sportTreeDragStartPos).manhattanLength() < QApplication::startDragDistance()) {
#endif
            return true;
        }
        sportTreeDragWatch = false;
#if QT_VERSION >= 0x060000
        QTreeWidgetItem *item = sportTree->itemAt(mouseEvent->position().toPoint());
#else
        QTreeWidgetItem *item = sportTree->itemAt(mouseEvent->pos());
#endif
        if (! item) {
            return true;
        }

        QByteArray entryBytes = item->data(0, Qt::UserRole).toString().toUtf8();
        QMimeData *mimeData = new QMimeData();
        mimeData->setData("application/x-gc-icon", entryBytes);

        QDrag *drag = new QDrag(sportTree);
        drag->setMimeData(mimeData);
        drag->setPixmap(item->icon(2).pixmap(ICONSPAGE_S));
        drag->setHotSpot(QPoint(ICONSPAGE_S_W / 2, ICONSPAGE_S_H / 2));
        Qt::DropAction dropAction = drag->exec(Qt::MoveAction);
        if (dropAction == Qt::MoveAction) {
            QPixmap pixmap(ICONSPAGE_S);
            pixmap.fill(Qt::transparent);
            QIcon icon(pixmap);
            item->setData(0, Qt::UserRole + 2, "");
            item->setIcon(2, icon);
        }
        return true;
    }
    return false;
}


bool
IconsPage::eventFilterIconList
(QEvent *event)
{
    if (event->type() == QEvent::DragEnter) {
        QDragEnterEvent *dragEnterEvent = static_cast<QDragEnterEvent*>(event);
        if (   dragEnterEvent->mimeData()->hasFormat("application/x-gc-icon")
            && (dragEnterEvent->possibleActions() & Qt::MoveAction)
            && dragEnterEvent->source() == sportTree) {
            dragEnterEvent->setDropAction(Qt::MoveAction);
            dragEnterEvent->accept();
        } else if (   dragEnterEvent->mimeData()->hasUrls()
                   && (dragEnterEvent->possibleActions() & Qt::CopyAction)) {
            const QList<QUrl> urls = dragEnterEvent->mimeData()->urls();
            int svgs = 0;
            for (const QUrl &url : urls) {
                QString path = url.toLocalFile();
                if (path.endsWith(".svg")) {
                    ++svgs;
                }
            }
            if (svgs > 0) {
                dragEnterEvent->setDropAction(Qt::CopyAction);
                dragEnterEvent->accept();
            }
        }
        if (dragEnterEvent->isAccepted()) {
            iconList->setStyleSheet("QListWidget { border: 2px solid #F79130; border-radius: 8px; }");
        }
        return true;
    } else if (event->type() == QEvent::DragLeave) {
        iconList->setStyleSheet("");
        return true;
    } else if (event->type() == QEvent::Drop) {
        QDropEvent *dropEvent = static_cast<QDropEvent*>(event);
        if (   dropEvent->mimeData()->hasFormat("application/x-gc-icon")
            && (dropEvent->possibleActions() & Qt::MoveAction)
            && dropEvent->source() == sportTree) {
            dropEvent->setDropAction(Qt::MoveAction);
            dropEvent->accept();
        } else if (   dropEvent->mimeData()->hasUrls()
                   && (dropEvent->possibleActions() & Qt::CopyAction)) {
            const QList<QUrl> urls = dropEvent->mimeData()->urls();
            int added = 0;
            for (const QUrl &url : urls) {
                if (IconManager::instance().addIconFile(QFile(url.toLocalFile()))) {
                    ++added;
                }
            }
            if (added > 0) {
                dropEvent->setDropAction(Qt::CopyAction);
                dropEvent->accept();
                updateIconList();
            }
        }
        iconList->setStyleSheet("");
        return true;
    }
    return false;
}


bool
IconsPage::eventFilterIconListViewport
(QEvent *event)
{
    if (event->type() == QEvent::Paint) {
        if (iconList->count() == 0) {
            QPaintEvent *paintEvent = static_cast<QPaintEvent*>(event);
            QPainter painter(iconList->viewport());
            QPalette palette;
            QColor pixmapColor = palette.color(QPalette::Disabled, QPalette::Text);
            if (palette.color(QPalette::Base).lightness() < 127) {
                pixmapColor = pixmapColor.darker(135);
            } else {
                pixmapColor = pixmapColor.lighter(135);
            }
            QPixmap pixmap = svgAsColoredPixmap(":/images/breeze/edit-image-face-add.svg", QSize(256 * dpiXFactor, 256 * dpiYFactor), 0, pixmapColor);
            painter.drawPixmap((paintEvent->rect().width() - pixmap.width()) / 2, (paintEvent->rect().height() - pixmap.height()) / 2, pixmap);
            painter.drawText(paintEvent->rect(), Qt::AlignCenter | Qt::TextWordWrap, tr("No icons available.\nDrag and drop .svg files here to add icons."));
            return true;
        }
        return false;
    } else if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
#if QT_VERSION >= 0x060000
            QListWidgetItem *item = iconList->itemAt(mouseEvent->position().toPoint());
#else
            QListWidgetItem *item = iconList->itemAt(mouseEvent->pos());
#endif
            if (item) {
#if QT_VERSION >= 0x060000
                iconListDragStartPos = mouseEvent->position().toPoint();
#else
                iconListDragStartPos = mouseEvent->pos();
#endif
                iconListDragWatch = true;
            } else {
                iconListDragWatch = false;
            }
        }
        return true;
    } else if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (   ! (mouseEvent->buttons() & Qt::LeftButton)
            || ! iconListDragWatch
#if QT_VERSION >= 0x060000
            || (mouseEvent->position().toPoint() - iconListDragStartPos).manhattanLength() < QApplication::startDragDistance()) {
#else
            || (mouseEvent->pos() - iconListDragStartPos).manhattanLength() < QApplication::startDragDistance()) {
#endif
            return true;
        }
        iconListDragWatch = false;
#if QT_VERSION >= 0x060000
        QListWidgetItem *item = iconList->itemAt(mouseEvent->position().toPoint());
#else
        QListWidgetItem *item = iconList->itemAt(mouseEvent->pos());
#endif
        if (! item) {
            return true;
        }

        QString iconFile = item->data(Qt::UserRole).toString();
        QMimeData *mimeData = new QMimeData();
        mimeData->setData("application/x-gc-icon", iconFile.toUtf8());

        QDrag *drag = new QDrag(iconList);
        drag->setMimeData(mimeData);
        drag->setPixmap(item->icon().pixmap(ICONSPAGE_L));
        drag->setHotSpot(QPoint(ICONSPAGE_L_W / 2, ICONSPAGE_L_H / 2));
        Qt::DropAction dropAction = drag->exec(Qt::MoveAction | Qt::LinkAction);
        if (dropAction == Qt::MoveAction) {
            if (IconManager::instance().deleteIconFile(iconFile)) {
                updateIconList();
                int rowCount = sportTree->topLevelItemCount();
                for (int i = 0; i < rowCount; ++i) {
                    QTreeWidgetItem *item = sportTree->topLevelItem(i);
                    if (item && item->data(0, Qt::UserRole + 2).toString() == iconFile) {
                        QPixmap pixmap(ICONSPAGE_S);
                        pixmap.fill(Qt::transparent);
                        item->setData(0, Qt::UserRole + 2, "");
                        item->setIcon(2, QIcon(pixmap));
                    }
                }
            }
        }
        return true;
    }
    return false;
}


void
IconsPage::initSportTree
()
{
    sportTree->clear();
    QPalette palette;
    SpecialFields &specials = SpecialFields::getInstance();
    for (QString field : { "Sport", "SubSport" }) {
        for (const FieldDefinition &fieldDefinition : fieldDefinitions) {
            if (fieldDefinition.name == field) {
                for (const QString &fieldValue : fieldDefinition.values) {
                    QIcon icon;
                    QString assignedFile = IconManager::instance().assignedIcon(fieldDefinition.name, fieldValue);
                    if (! assignedFile.isEmpty()) {
                        QPixmap pixmap = svgAsColoredPixmap(IconManager::instance().toFilepath(assignedFile), ICONSPAGE_S, ICONSPAGE_MARGIN, palette.color(QPalette::Text));
                        icon.addPixmap(pixmap);
                    } else {
                        QPixmap pixmap(ICONSPAGE_S);
                        pixmap.fill(Qt::transparent);
                        icon.addPixmap(pixmap);
                    }
                    QTreeWidgetItem *item = new QTreeWidgetItem();
                    item->setData(0, Qt::DisplayRole, specials.displayName(fieldDefinition.name));
                    item->setData(0, Qt::UserRole, fieldDefinition.name);
                    item->setData(0, Qt::UserRole + 1, assignedFile);
                    item->setData(0, Qt::UserRole + 2, assignedFile);
                    item->setData(1, Qt::DisplayRole, fieldValue);
                    item->setIcon(2, icon);
                    sportTree->addTopLevelItem(item);
                }
            }
        }
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    sportTree->viewport()->update();
#endif
}


void
IconsPage::updateIconList
()
{
    iconList->clear();
    QPalette palette;
    QStringList icons = IconManager::instance().listIconFiles();
    for (QString icon : icons) {
        QPixmap pixmap = svgAsColoredPixmap(IconManager::instance().toFilepath(icon), ICONSPAGE_L, ICONSPAGE_MARGIN, palette.color(QPalette::Text));
        QListWidgetItem *item = new QListWidgetItem(QIcon(pixmap), "");
        item->setData(Qt::UserRole, icon);
        iconList->addItem(item);
    }
}


//
// Ride metadata page
//
FieldsPage::FieldsPage(QWidget *parent, QList<FieldDefinition>fieldDefinitions) : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_DataFields_Fields));

    valueDelegate.setTitle(tr("<h3>Manage allowed values</h3>"
                              "If the list is empty, any value is accepted. A list containing "
                              "<tt>*</tt> as its only entry indicates previous values for the "
                              "same field will be used to autocomplete input."));
    valueDelegate.setDisplayLength(15, 2);

    fields = new QTreeWidget;
    fields->headerItem()->setText(0, tr("Screen Tab"));
    fields->headerItem()->setText(1, tr("Field"));
    fields->headerItem()->setText(2, tr("Type"));
    fields->headerItem()->setText(3, tr("Values"));
    fields->headerItem()->setText(4, tr("Summary"));
    fields->headerItem()->setText(5, tr("Interval"));
    fields->headerItem()->setText(6, tr("Expression"));
    fields->setColumnCount(7);
    fieldTypeDelegate.addItems( {
        tr("Text"),
        tr("Textbox"),
        tr("ShortText"),
        tr("Integer"),
        tr("Double"),
        tr("Date"),
        tr("Time"),
        tr("Checkbox")
    } );
    fields->setItemDelegateForColumn(0, &tabDelegate);
    fields->setItemDelegateForColumn(1, &fieldDelegate);
    fields->setItemDelegateForColumn(2, &fieldTypeDelegate);
    fields->setItemDelegateForColumn(3, &valueDelegate);
    basicTreeWidgetStyle(fields);

    actionButtons = new ActionButtonBox(ActionButtonBox::UpDownGroup | ActionButtonBox::AddDeleteGroup);
    actionButtons->defaultConnect(ActionButtonBox::UpDownGroup, fields);
    actionButtons->defaultConnect(ActionButtonBox::AddDeleteGroup, fields);

    SpecialFields& specials = SpecialFields::getInstance();
    SpecialTabs& specialTabs = SpecialTabs::getInstance();
    foreach(FieldDefinition field, fieldDefinitions) {
        QCheckBox *checkBox = new QCheckBox("", this);
        checkBox->setChecked(field.diary);

        QCheckBox *checkBoxInt = new QCheckBox("", this);
        checkBoxInt->setChecked(field.interval);

        QTreeWidgetItem *add = new QTreeWidgetItem(fields->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);
        add->setText(0, specialTabs.displayName(field.tab)); // tab name
        add->setText(1, specials.displayName(field.name)); // field name
        add->setData(2, Qt::DisplayRole, static_cast<int>(field.type));
        add->setText(3, field.values.join(",")); // values
        fields->setItemWidget(add, 4, checkBox);
        fields->setItemWidget(add, 5, checkBoxInt);
        add->setText(6, field.expression); // expression
    }

    mainLayout->addWidget(fields);
    mainLayout->addWidget(actionButtons);

    // connect up slots
    connect(actionButtons, &ActionButtonBox::upRequested, this, &FieldsPage::upClicked);
    connect(actionButtons, &ActionButtonBox::downRequested, this, &FieldsPage::downClicked);
    connect(actionButtons, &ActionButtonBox::addRequested, this, &FieldsPage::addClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &FieldsPage::deleteClicked);

    fields->setCurrentItem(fields->invisibleRootItem()->child(0));
}

void
FieldsPage::upClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
        if (index == 0) return; // its at the top already

        // movin on up!
        QWidget *check = fields->itemWidget(fields->currentItem(),4);
        QWidget *checkInt = fields->itemWidget(fields->currentItem(),5);
        QCheckBox *checkBox = new QCheckBox("", this);
        checkBox->setChecked(((QCheckBox*)check)->isChecked());
        QCheckBox *checkBoxInt = new QCheckBox("", this);
        checkBoxInt->setChecked(((QCheckBox*)checkInt)->isChecked());
        QTreeWidgetItem* moved = fields->invisibleRootItem()->takeChild(index);
        fields->invisibleRootItem()->insertChild(index-1, moved);
        fields->setItemWidget(moved, 4, checkBox);
        fields->setItemWidget(moved, 5, checkBoxInt);
        fields->setCurrentItem(moved);
    }
}

void
FieldsPage::downClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
        if (index == (fields->invisibleRootItem()->childCount()-1)) return; // its at the bottom already

        QWidget *check = fields->itemWidget(fields->currentItem(),4);
        QWidget *checkInt = fields->itemWidget(fields->currentItem(),5);
        QCheckBox *checkBox = new QCheckBox("", this);
        checkBox->setChecked(((QCheckBox*)check)->isChecked());
        QCheckBox *checkBoxInt = new QCheckBox("", this);
        checkBoxInt->setChecked(((QCheckBox*)checkInt)->isChecked());
        QTreeWidgetItem* moved = fields->invisibleRootItem()->takeChild(index);
        fields->invisibleRootItem()->insertChild(index+1, moved);
        fields->setItemWidget(moved, 4, checkBox);
        fields->setItemWidget(moved, 5, checkBoxInt);
        fields->setCurrentItem(moved);
    }
}

void
FieldsPage::addClicked()
{
    int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
    if (index < 0) index = 0;
    QTreeWidgetItem *add;
    QCheckBox *checkBox = new QCheckBox("", this);
    QCheckBox *checkBoxInt = new QCheckBox("", this);

    add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);
    fields->invisibleRootItem()->insertChild(index, add);

    // field
    QString text = tr("New");
    for (int i=0; fields->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(1, text);

    // type button
    fields->setItemWidget(add, 4, checkBox);
    fields->setItemWidget(add, 5, checkBoxInt);

    fields->setCurrentItem(add);
}

void
FieldsPage::deleteClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());

        // zap!
        delete fields->invisibleRootItem()->takeChild(index);
    }
}


void
FieldsPage::getDefinitions(QList<FieldDefinition> &fieldList)
{
    SpecialFields& sp = SpecialFields::getInstance();
    SpecialTabs& st = SpecialTabs::getInstance();
    QStringList checkdups;

    // clear current just in case
    fieldList.clear();

    for (int idx =0; idx < fields->invisibleRootItem()->childCount(); idx++) {

        FieldDefinition add;
        QTreeWidgetItem *item = fields->invisibleRootItem()->child(idx);

        // silently ignore duplicates
        if (checkdups.contains(item->text(1))) continue;
        else checkdups << item->text(1);

        add.tab = st.internalName(item->text(0));
        add.name = sp.internalName(item->text(1));
        add.values = item->text(3).split(QRegularExpression("(, *|,)"), Qt::KeepEmptyParts);
        add.diary = ((QCheckBox*)fields->itemWidget(item, 4))->isChecked();
        add.interval = ((QCheckBox*)fields->itemWidget(item, 5))->isChecked();
        add.expression = item->text(6);

        if (sp.isMetric(add.name))
            add.type = GcFieldType::FIELD_DOUBLE;
        else
            add.type = static_cast<GcFieldType>(item->data(2, Qt::DisplayRole).toInt());

        fieldList.append(add);
    }
}

//
// Data processors config page
//

#define PROCESSORTREE_COL_PROCESSOR 0
#define PROCESSORTREE_COL_AUTOMATION 1
#define PROCESSORTREE_COL_ROWNUM 2
#define PROCESSORTREE_COL_CORE 3
#define PROCESSORTREE_COL_AUTOMATEDONLY 4
#define PROCESSORTREE_COL_ID 5
#define PROCESSORTREE_NUM_COLS 6

ProcessorPage::ProcessorPage(Context *context) : context(context)
{
    automationDelegate.addItems({ tr("None"), tr("On Import"), tr("On Save") });

    processorTree = new QTreeWidget();
    processorTree->headerItem()->setText(PROCESSORTREE_COL_PROCESSOR, tr("Processor"));
    processorTree->headerItem()->setText(PROCESSORTREE_COL_AUTOMATION, tr("Automation"));
    processorTree->headerItem()->setText(PROCESSORTREE_COL_ROWNUM, "_rownum");
    processorTree->headerItem()->setText(PROCESSORTREE_COL_CORE, "_core");
    processorTree->headerItem()->setText(PROCESSORTREE_COL_AUTOMATEDONLY, "_automatedonly");
    processorTree->headerItem()->setText(PROCESSORTREE_COL_ID, "_id");
    processorTree->setColumnCount(PROCESSORTREE_NUM_COLS);
    processorTree->setItemDelegateForColumn(PROCESSORTREE_COL_PROCESSOR, &processorDelegate);
    processorTree->setItemDelegateForColumn(PROCESSORTREE_COL_AUTOMATION, &automationDelegate);
    processorTree->setColumnHidden(PROCESSORTREE_COL_ROWNUM, true);
    processorTree->setColumnHidden(PROCESSORTREE_COL_CORE, true);
    processorTree->setColumnHidden(PROCESSORTREE_COL_AUTOMATEDONLY, true);
    processorTree->setColumnHidden(PROCESSORTREE_COL_ID, true);
    HelpWhatsThis *help = new HelpWhatsThis(processorTree);
    processorTree->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_DataFields_Processing));
    basicTreeWidgetStyle(processorTree);

    QWidget *leftWidget = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 5 * dpiXFactor, 0);
    leftLayout->addWidget(processorTree);
#ifdef GC_WANT_PYTHON
    if (appsettings->value(nullptr, GC_EMBED_PYTHON, true).toBool()) {
        hideButton = new QCheckBox(tr("Hide Core Processors"));
        actionButtons = new ActionButtonBox(ActionButtonBox::EditGroup | ActionButtonBox::AddDeleteGroup);
        actionButtons->addWidget(hideButton);

        leftLayout->addWidget(actionButtons);

        connect(actionButtons, &ActionButtonBox::addRequested, this, &ProcessorPage::addProcessor);
        connect(actionButtons, &ActionButtonBox::deleteRequested, this, &ProcessorPage::delProcessor);
        connect(actionButtons, &ActionButtonBox::editRequested, this, &ProcessorPage::editProcessor);
        connect(hideButton, SIGNAL(toggled(bool)), this, SLOT(toggleCoreProcessors(bool)));
    }
#endif

    settingsStack = new QStackedWidget();
    settingsStack->setContentsMargins(5 * dpiXFactor, 0, 0, 0);
    settingsStack->addWidget(new QLabel(tr("<center><h1>No Processor selected</h1></center>")));

    QSplitter *splitter = new QSplitter();
    splitter->setHandleWidth(0);
    splitter->addWidget(leftWidget);
    splitter->addWidget(settingsStack);
    splitter->setSizes(QList<int>({INT_MAX / 3, INT_MAX / 2}));

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(splitter);

    connect(processorTree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(processorSelected(QTreeWidgetItem*)));
    connect(processorTree->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(dataChanged(const QModelIndex&)));
#ifdef GC_WANT_PYTHON
    connect(processorTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(dblClickProcessor(QTreeWidgetItem*, int)));
#endif

    reload(0);
}

qint32
ProcessorPage::saveClicked()
{
    QMap<QString, DataProcessor*> dps = DataProcessorFactory::instance().getProcessors(false);

    // call each processor config widget's saveConfig() to
    // write away separately
    for (int i = 0; i < processorTree->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *item = processorTree->invisibleRootItem()->child(i);
        QString id = item->data(PROCESSORTREE_COL_ID, Qt::DisplayRole).toString();

        if (dps.contains(id)) {
            dps[id]->setAutomation(static_cast<DataProcessor::Automation>(item->data(PROCESSORTREE_COL_AUTOMATION, Qt::DisplayRole).toInt()));
            dps[id]->setAutomatedOnly(item->data(PROCESSORTREE_COL_AUTOMATEDONLY, Qt::DisplayRole).toBool());
        }
        if (configs[i] != nullptr) {
            configs[i]->saveConfig();
        }
    }

    return 0;
}


void
ProcessorPage::processorSelected
(QTreeWidgetItem *selectedItem)
{
    if (selectedItem == nullptr) {
        settingsStack->setCurrentIndex(0);
    } else {
        int rownum = selectedItem->data(PROCESSORTREE_COL_ROWNUM, Qt::DisplayRole).toInt();
        if (rownum >= 0 && rownum < settingsStack->count() - 1) {
            settingsStack->setCurrentIndex(rownum + 1);
#ifdef GC_WANT_PYTHON
            if (actionButtons != nullptr) {
                bool isCoreProcessor = selectedItem->data(PROCESSORTREE_COL_CORE, Qt::DisplayRole).toBool();
                actionButtons->setButtonEnabled(ActionButtonBox::Delete, ! isCoreProcessor);
                actionButtons->setButtonEnabled(ActionButtonBox::Edit, ! isCoreProcessor);
            }
#endif
        }
    }
}


void
ProcessorPage::reload
()
{
    processorTree->model()->blockSignals(true);
    configs.clear();
    automationCombos.clear();
    automatedCheckBoxes.clear();
    processorTree->blockSignals(true);
    processorTree->clear();
    processorTree->blockSignals(false);
    while (settingsStack->count() > 1) {
        QWidget *widget = settingsStack->widget(settingsStack->count() - 1);
        delete widget;
    }

    // get the available processors
    const DataProcessorFactory &factory = DataProcessorFactory::instance();
    QList<DataProcessor*> processors = factory.getProcessorsSorted();

    // iterate over all the processors and add an entry to the
    int rownum = 0;
    for (QList<DataProcessor*>::iterator iter = processors.begin(); iter != processors.end(); ++iter) {
        QTreeWidgetItem *add = new QTreeWidgetItem(processorTree->invisibleRootItem());
        QFont font = add->font(0);
        font.setWeight((*iter)->isAutomatedOnly() ? QFont::ExtraLight : QFont::Normal);
        add->setFlags(add->flags() | Qt::ItemIsEditable);
        add->setData(PROCESSORTREE_COL_PROCESSOR, Qt::DisplayRole, (*iter)->name()); // Processor Name: it shows the localized name
        add->setData(PROCESSORTREE_COL_PROCESSOR, Qt::FontRole, font);
        add->setData(PROCESSORTREE_COL_AUTOMATION, Qt::DisplayRole, (*iter)->getAutomation());
        add->setData(PROCESSORTREE_COL_ROWNUM, Qt::DisplayRole, rownum);
        add->setData(PROCESSORTREE_COL_CORE, Qt::DisplayRole, (*iter)->isCoreProcessor());
        add->setData(PROCESSORTREE_COL_AUTOMATEDONLY, Qt::DisplayRole, (*iter)->isAutomatedOnly());
        add->setData(PROCESSORTREE_COL_ID, Qt::DisplayRole, (*iter)->id());

        // Get and Set the Config Widget
        DataProcessorConfig *config = (*iter)->processorConfig(this);
        config->readConfig();
        configs << config;

        QLabel *detailsHL = new QLabel();
        if (appsettings->value(NULL, GC_EMBED_PYTHON, true).toBool()) {
            detailsHL->setText(QString("<center><h2>%1<br><small>%2</small></h2></center>")
                                     .arg((*iter)->name())
                                     .arg((*iter)->isCoreProcessor() ? tr("Core Processor") : tr("Custom Python Processor")));
        } else {
            detailsHL->setText(QString("<center><h2>%1</h2></center>").arg((*iter)->name()));
        }
        detailsHL->setWordWrap(true);
        QLabel *automationHL = new QLabel(HLO + tr("Automation") + HLC);

        QFormLayout *setupLayout = newQFormLayout();
        QCheckBox *automatedOnly = new QCheckBox(tr("Automated execution only"));
        automatedOnly->setChecked((*iter)->isAutomatedOnly());
        automatedCheckBoxes << automatedOnly;

        QComboBox *automationCombo = new QComboBox();
        automationCombo->addItems({ tr("None"), tr("On Import"), tr("On Save") });
        automationCombo->setCurrentIndex((*iter)->getAutomation());
        automationCombos << automationCombo;

        QLabel *postprocess = new QLabel(QString("<tt>postprocess(\"%1\", <i>&lt;YOUR FILTER&gt;</i>)</tt>").arg((*iter)->id()));
        postprocess->setTextInteractionFlags(Qt::TextSelectableByMouse);

        setupLayout->addRow(tr("Automation"), automationCombo);
        setupLayout->addRow("", automatedOnly);
        setupLayout->addRow(tr("Use as Filter"), postprocess);

        connect(automationCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(automationChanged(int)));
        connect(automatedOnly, SIGNAL(toggled(bool)), this, SLOT(toggleAutomatedOnly(bool)));

        QLabel *configHL = new QLabel(HLO + tr("Default Settings") + HLC);
        QLabel *descriptionHL = new QLabel(HLO + tr("Description") + HLC);
        QLabel *explainLabel = new QLabel((*iter)->explain());
        explainLabel->setWordWrap(true);

        QWidget *detailsContent = new QWidget();
        QVBoxLayout *detailsScrollLayout = new QVBoxLayout(detailsContent);
        detailsScrollLayout->addWidget(automationHL);
        detailsScrollLayout->addLayout(setupLayout);
        detailsScrollLayout->addSpacing(10 * dpiYFactor);
        detailsScrollLayout->addWidget(configHL);
        detailsScrollLayout->addWidget(config);
        if (! config->sizeHint().isValid()) {
            configHL->setHidden(true);
            config->setHidden(true);
        } else {
            detailsScrollLayout->addSpacing(10 * dpiYFactor);
        }
        detailsScrollLayout->addWidget(descriptionHL);
        detailsScrollLayout->addWidget(explainLabel);
        detailsScrollLayout->addStretch();

        QScrollArea *detailsScroller = new QScrollArea();
        detailsScroller->setWidgetResizable(true);
        detailsScroller->setWidget(detailsContent);

        QWidget *details = new QWidget();
        QVBoxLayout *detailsLayout = new QVBoxLayout(details);
        detailsLayout->setContentsMargins(0, 0, 0, 0);
        detailsLayout->addWidget(detailsHL);
        detailsLayout->addWidget(detailsScroller);

        settingsStack->addWidget(details);

        ++rownum;
    }

#ifdef GC_WANT_PYTHON
    if (hideButton != nullptr) {
        toggleCoreProcessors(hideButton->isChecked());
    }
#endif
    processorTree->model()->blockSignals(false);
}


void
ProcessorPage::reload
(const QString &selectName)
{
    reload();
    QTreeWidgetItem *selItem = nullptr;
    for (int i = 0; i < processorTree->invisibleRootItem()->childCount(); ++i) {
        QTreeWidgetItem *nextItem = processorTree->invisibleRootItem()->child(i);
        if (! nextItem->isHidden()) {
            selItem = nextItem;
        }
        if (nextItem->data(PROCESSORTREE_COL_ID, Qt::DisplayRole).toString() == selectName) {
            break;
        }
    }
    processorTree->setCurrentItem(selItem);
}


void
ProcessorPage::reload
(int selectRow)
{
    reload();
    QTreeWidgetItem *selItem = nullptr;
    for (int i = 0; i <= selectRow && i < processorTree->invisibleRootItem()->childCount(); ++i) {
        QTreeWidgetItem *nextItem = processorTree->invisibleRootItem()->child(i);
        if (! nextItem->isHidden()) {
            selItem = nextItem;
        }
    }
    processorTree->setCurrentItem(selItem);
}


#ifdef GC_WANT_PYTHON
void
ProcessorPage::addProcessor
()
{
    EditFixPyScriptDialog editDlg(context, nullptr, this);
    if (editDlg.exec() == QDialog::Accepted) {
        reload(editDlg.getName());
    }
}


void
ProcessorPage::delProcessor
()
{
    QTreeWidgetItem *item = processorTree->currentItem();
    if (item == nullptr || item->data(PROCESSORTREE_COL_CORE, Qt::DisplayRole).toBool()) {
        return;
    }
    QString name = item->data(PROCESSORTREE_COL_ID, Qt::DisplayRole).toString();
    QString msg = tr("Are you sure you want to delete %1?").arg(name);
    if (QMessageBox::question(this, "GoldenCheetah", msg) == QMessageBox::No) {
        return;
    }
    fixPySettings->deleteScript(name);
    reload(item->data(PROCESSORTREE_COL_ROWNUM, Qt::DisplayRole).toInt());
}


void
ProcessorPage::editProcessor
()
{
    QTreeWidgetItem *item = processorTree->currentItem();
    if (item == nullptr || item->data(PROCESSORTREE_COL_CORE, Qt::DisplayRole).toBool()) {
        return;
    }
    QVariant automation = item->data(PROCESSORTREE_COL_AUTOMATION, Qt::DisplayRole);
    QVariant automatedOnly = item->data(PROCESSORTREE_COL_AUTOMATEDONLY, Qt::DisplayRole);
    QString name = item->data(PROCESSORTREE_COL_PROCESSOR, Qt::DisplayRole).toString();
    FixPyScript *script = fixPySettings->getScript(name);
    EditFixPyScriptDialog editDlg(context, script, this);
    if (editDlg.exec() == QDialog::Accepted) {
        reload(script->name);
        item = processorTree->currentItem();
        if (item != nullptr && item->data(PROCESSORTREE_COL_PROCESSOR, Qt::DisplayRole).toString() == script->name) {
            item->setData(PROCESSORTREE_COL_AUTOMATION, Qt::DisplayRole, automation);
            item->setData(PROCESSORTREE_COL_AUTOMATEDONLY, Qt::DisplayRole, automatedOnly);
        }
    }
}


void
ProcessorPage::dblClickProcessor
(QTreeWidgetItem *item, int col)
{
    Q_UNUSED(item)

    if (col != PROCESSORTREE_COL_AUTOMATION) {
        editProcessor();
    }
}


void
ProcessorPage::toggleCoreProcessors
(bool checked)
{
    QTreeWidgetItem *current = processorTree->currentItem();
    QTreeWidgetItem *firstVisible = nullptr;
    for (int i = 0; i < processorTree->invisibleRootItem()->childCount(); ++i) {
        QTreeWidgetItem *item = processorTree->invisibleRootItem()->child(i);
        bool isCore = item->data(PROCESSORTREE_COL_CORE, Qt::DisplayRole).toBool();
        item->setHidden(checked && (! checked || isCore));
        if (firstVisible == nullptr && ! item->isHidden()) {
            firstVisible = item;
        }
    }
    if (current == nullptr || current->isHidden()) {
        processorTree->setCurrentItem(firstVisible);
    }
    if (processorTree->currentItem() != nullptr) {
        processorTree->scrollToItem(processorTree->currentItem(), QAbstractItemView::PositionAtCenter);
    }
}
#endif


void
ProcessorPage::automationChanged
(int index)
{
    QTreeWidgetItem *current = processorTree->currentItem();
    if (current == nullptr) {
        return;
    }
    current->setData(PROCESSORTREE_COL_AUTOMATION, Qt::DisplayRole, index);
}


void
ProcessorPage::toggleAutomatedOnly
(bool checked)
{
    QTreeWidgetItem *current = processorTree->currentItem();
    if (current == nullptr) {
        return;
    }
    QFont font = current->font(0);
    font.setWeight(checked ? QFont::ExtraLight : QFont::Normal);
    current->setData(PROCESSORTREE_COL_PROCESSOR, Qt::FontRole, font);
    current->setData(PROCESSORTREE_COL_AUTOMATEDONLY, Qt::DisplayRole, checked);
}


void
ProcessorPage::dataChanged
(const QModelIndex &topLeft)
{
    int rowNum = topLeft.siblingAtColumn(PROCESSORTREE_COL_ROWNUM).data(Qt::DisplayRole).toInt();
    if (topLeft.column() == PROCESSORTREE_COL_AUTOMATION) {
        if (rowNum < automationCombos.count()) {
            automationCombos[rowNum]->blockSignals(true);
            automationCombos[rowNum]->setCurrentIndex(topLeft.data(Qt::DisplayRole).toInt());
            automationCombos[rowNum]->blockSignals(false);
        }
    } else if (topLeft.column() == PROCESSORTREE_COL_AUTOMATEDONLY) {
        if (rowNum < automatedCheckBoxes.count()) {
            bool checked = topLeft.data(Qt::DisplayRole).toBool();
            automatedCheckBoxes[rowNum]->blockSignals(true);
            automatedCheckBoxes[rowNum]->setChecked(checked);
            automatedCheckBoxes[rowNum]->blockSignals(false);
            processorTree->model()->blockSignals(true);
            toggleAutomatedOnly(checked);
            processorTree->model()->blockSignals(false);
        }
    }
}


//
// Default values page
//
DefaultsPage::DefaultsPage
(MetadataPage *parent, QList<DefaultDefinition>defaultDefinitions)
: QWidget(parent), parent(parent)
{
    installEventFilter(this);
    fieldDelegate.setDataSource(CompleterEditDelegate::External);
    linkedDelegate.setDataSource(CompleterEditDelegate::External);

    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_DataFields_Defaults));

    defaults = new QTreeWidget;
    defaults->headerItem()->setText(0, tr("Field").leftJustified(30, ' '));
    defaults->headerItem()->setText(1, tr("Value"));
    defaults->headerItem()->setText(2, tr("Linked field").leftJustified(30, ' '));
    defaults->headerItem()->setText(3, tr("Default Value"));
    defaults->setColumnCount(4);
    defaults->setItemDelegateForColumn(0, &fieldDelegate);
    defaults->setItemDelegateForColumn(2, &linkedDelegate);
    basicTreeWidgetStyle(defaults);

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::UpDownGroup | ActionButtonBox::AddDeleteGroup);
    actionButtons->defaultConnect(ActionButtonBox::UpDownGroup, defaults);
    actionButtons->defaultConnect(ActionButtonBox::AddDeleteGroup, defaults);

    SpecialFields& specials = SpecialFields::getInstance();
    foreach(DefaultDefinition adefault, defaultDefinitions) {
        QTreeWidgetItem *add;

        add = new QTreeWidgetItem(defaults->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // field name
        add->setText(0, specials.displayName(adefault.field));
        // value
        add->setText(1, adefault.value);
        // Linked field
        add->setText(2, adefault.linkedField);
        // Default Value
        add->setText(3, adefault.linkedValue);
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(defaults);
    mainLayout->addWidget(actionButtons);

    // connect up slots
    connect(actionButtons, &ActionButtonBox::upRequested, this, &DefaultsPage::upClicked);
    connect(actionButtons, &ActionButtonBox::downRequested, this, &DefaultsPage::downClicked);
    connect(actionButtons, &ActionButtonBox::addRequested, this, &DefaultsPage::addClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &DefaultsPage::deleteClicked);

    defaults->setCurrentItem(defaults->invisibleRootItem()->child(0));
}

void
DefaultsPage::upClicked()
{
    if (defaults->currentItem()) {
        int index = defaults->invisibleRootItem()->indexOfChild(defaults->currentItem());
        if (index == 0) return; // its at the top already

        QTreeWidgetItem* moved = defaults->invisibleRootItem()->takeChild(index);
        defaults->invisibleRootItem()->insertChild(index-1, moved);
        defaults->setCurrentItem(moved);
    }
}

void
DefaultsPage::downClicked()
{
    if (defaults->currentItem()) {
        int index = defaults->invisibleRootItem()->indexOfChild(defaults->currentItem());
        if (index == (defaults->invisibleRootItem()->childCount()-1)) return; // its at the bottom already

        QTreeWidgetItem* moved = defaults->invisibleRootItem()->takeChild(index);
        defaults->invisibleRootItem()->insertChild(index+1, moved);
        defaults->setCurrentItem(moved);
    }
}

void
DefaultsPage::addClicked()
{
    int index = defaults->invisibleRootItem()->indexOfChild(defaults->currentItem());
    if (index < 0) index = 0;
    QTreeWidgetItem *add;

    add = new QTreeWidgetItem;
    defaults->invisibleRootItem()->insertChild(index, add);
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    // field
    QString text = tr("New");
    for (int i=0; defaults->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);
}

void
DefaultsPage::deleteClicked()
{
    if (defaults->currentItem()) {
        int index = defaults->invisibleRootItem()->indexOfChild(defaults->currentItem());

        // zap!
        delete defaults->invisibleRootItem()->takeChild(index);
    }
}

void
DefaultsPage::getDefinitions(QList<DefaultDefinition> &defaultList)
{
    SpecialFields& sp = SpecialFields::getInstance();

    // clear current just in case
    defaultList.clear();

    for (int idx =0; idx < defaults->invisibleRootItem()->childCount(); idx++) {

        DefaultDefinition add;
        QTreeWidgetItem *item = defaults->invisibleRootItem()->child(idx);

        add.field = sp.internalName(item->text(0));
        add.value = item->text(1);
        add.linkedField = sp.internalName(item->text(2));
        add.linkedValue = item->text(3);

        defaultList.append(add);
    }
}


bool
DefaultsPage::eventFilter
(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Show && ! event->spontaneous()) {
        SpecialFields& sp = SpecialFields::getInstance();
        QList<FieldDefinition> fromFieldsPage;
        parent->fieldsPage->getDefinitions(fromFieldsPage);
        QStringList fieldNames;
        foreach(FieldDefinition x, fromFieldsPage) {
            fieldNames << sp.displayName(x.name);
        }
        fieldDelegate.setCompletionList(fieldNames);
        linkedDelegate.setCompletionList(fieldNames);
        return false;
    } else {
        return QObject::eventFilter(obj, event);
    }
}


IntervalsPage::IntervalsPage(Context *context) : context(context)
{
    // get config
    b4.discovery = appsettings->cvalue(context->athlete->cyclist, GC_DISCOVERY, 57).toInt(); // 57 does not include search for PEAKs

    QFormLayout *form = newQFormLayout(this);
    form->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    form->addRow("", new QLabel(HLO + tr("Enable interval auto-discovery") + HLC));
    user = 99;
    for(int i=0; i<= static_cast<int>(RideFileInterval::last()); i++) {
        // ignore until we get past user interval type
        if (i == static_cast<int>(RideFileInterval::USER)) user=i;

        // if past user then add
        if (i>user) {
            QCheckBox *add = new QCheckBox(RideFileInterval::typeDescriptionLong(static_cast<RideFileInterval::IntervalType>(i)));
            checkBoxes << add;
            add->setChecked(b4.discovery & RideFileInterval::intervalTypeBits(static_cast<RideFileInterval::IntervalType>(i)));
            form->addRow("", add);
        }
    }
}

qint32
IntervalsPage::saveClicked()
{
    int discovery = 0;

    // now update discovery !
    for(int i=0; i< checkBoxes.count(); i++)
        if (checkBoxes[i]->isChecked())
            discovery += RideFileInterval::intervalTypeBits(static_cast<RideFileInterval::IntervalType>(i+user+1));

    // new value returned
    appsettings->setCValue(context->athlete->cyclist, GC_DISCOVERY, discovery);

    // return change !
    if (b4.discovery != discovery) return CONFIG_DISCOVERY;
    else return 0;
}

///
/// MeasuresConfigPage
///
MeasuresConfigPage::MeasuresConfigPage(QWidget *parent, Context *context) :
    QWidget(parent), context(context)
{
    // get config
    measures = new Measures();

    // create all the widgets
    QLabel *mlabel = new QLabel(HLO + tr("Groups") + HLC);
    measuresTable = new QTreeWidget(this);
    measuresTable->headerItem()->setText(0, tr("Symbol"));
    measuresTable->headerItem()->setText(1, tr("Name"));
    measuresTable->setColumnCount(2);
    basicTreeWidgetStyle(measuresTable);
    measuresTable->setItemDelegateForColumn(0, &meNameDelegate);
    measuresTable->setItemDelegateForColumn(1, &meSymbolDelegate);

    ActionButtonBox *measuresActions = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);

    meFiFactorDelegate.setDecimals(5);
    meFiFactorDelegate.setSingleStep(0.1);

    mflabel = new QLabel(QString(HLO) + HLC);
    measuresFieldsTable = new QTreeWidget(this);
    measuresFieldsTable->headerItem()->setText(0, tr("Symbol"));
    measuresFieldsTable->headerItem()->setText(1, tr("Name"));
    measuresFieldsTable->headerItem()->setText(2, tr("Metric Units"));
    measuresFieldsTable->headerItem()->setText(3, tr("Imperial Units"));
    measuresFieldsTable->headerItem()->setText(4, tr("Units Factor"));
    measuresFieldsTable->headerItem()->setText(5, tr("CSV Headers"));
    measuresFieldsTable->setColumnCount(6);
    basicTreeWidgetStyle(measuresFieldsTable);
    measuresFieldsTable->setItemDelegateForColumn(0, &meFiNameDelegate);
    measuresFieldsTable->setItemDelegateForColumn(1, &meFiSymbolDelegate);
    measuresFieldsTable->setItemDelegateForColumn(4, &meFiFactorDelegate);
    measuresFieldsTable->setItemDelegateForColumn(5, &meFiHeaderDelegate);

    ActionButtonBox *measureFieldsActions = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);

    QLabel *warningLabel = new QLabel(tr("Saved changes take effect after restart"));
    QHBoxLayout *xr = new QHBoxLayout();
    xr->addStretch();
    xr->addWidget(warningLabel);

    // lay it out
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mlabel);
    mainLayout->addWidget(measuresTable);
    mainLayout->addWidget(measuresActions);
    mainLayout->addWidget(mflabel);
    mainLayout->addWidget(measuresFieldsTable);
    mainLayout->addWidget(measureFieldsActions);
    mainLayout->addLayout(xr);

    connect(measuresTable, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(measuresSelected()));
    connect(measuresActions, &ActionButtonBox::addRequested, this, &MeasuresConfigPage::addMeasuresClicked);
    connect(measuresActions, &ActionButtonBox::deleteRequested, this, &MeasuresConfigPage::removeMeasuresClicked);
    connect(measureFieldsActions, &ActionButtonBox::addRequested, this, &MeasuresConfigPage::addMeasuresFieldClicked);
    connect(measureFieldsActions, &ActionButtonBox::deleteRequested, this, &MeasuresConfigPage::removeMeasuresFieldClicked);

    refreshMeasuresTable();
}

MeasuresConfigPage::~MeasuresConfigPage()
{
    if (measures != nullptr) delete measures;
}

qint32
MeasuresConfigPage::saveClicked()
{
    measures->saveConfig();
    return 0;
}

void
MeasuresConfigPage::refreshMeasuresTable()
{
    disconnect(measuresTable->model(), &QAbstractItemModel::dataChanged, this, &MeasuresConfigPage::measureChanged);
    // remove existing rows
    measuresTable->clear();

    // add a row for each measures group
    foreach (MeasuresGroup* group, measures->getGroups()) {
        QTreeWidgetItem *add = new QTreeWidgetItem(measuresTable->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);
        add->setData(0, Qt::DisplayRole, group->getSymbol());
        add->setData(1, Qt::DisplayRole, group->getName());

        measuresTable->setCurrentItem(add); // select the last added
    }
    measuresSelected();
    connect(measuresTable->model(), &QAbstractItemModel::dataChanged, this, &MeasuresConfigPage::measureChanged);
}

void MeasuresConfigPage::measuresSelected()
{
    // lets find the one we have selected...
    int row = measuresTable->invisibleRootItem()->indexOfChild(measuresTable->currentItem());
    if (row < 0) return; // nothing selected

    // update measures series table to reflect the selection
    refreshMeasuresFieldsTable();
}


void
MeasuresConfigPage::measureChanged
(const QModelIndex &topLeft)
{
    QTreeWidgetItem *item = measuresTable->currentItem();
    if (item == nullptr) {
        return;
    }

    MeasuresGroup *group = measures->getGroup(measuresTable->invisibleRootItem()->indexOfChild(item));
    switch (topLeft.column()) {
    case 0:
        group->setSymbol(topLeft.data(Qt::DisplayRole).toString());
        break;
    case 1:
        group->setName(topLeft.data(Qt::DisplayRole).toString());
        break;
    default:
        break;
    }
}


void
MeasuresConfigPage::resetMeasuresClicked()
{
    // Are you sure ?
    QMessageBox msgBox;
    msgBox.setText(tr("Are you sure you want to remove Measures customizations and reset to default configuration?"));
    msgBox.setInformativeText(tr("This action takes effect immediately and cannot be reverted"));
    QPushButton *resetButton = msgBox.addButton(tr("Reset"),QMessageBox::YesRole);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();

    // nope, don't want to
    if(msgBox.clickedButton() != resetButton) return;

    QFile::remove(QDir(gcroot).canonicalPath() + "/measures.ini");
    delete measures;
    measures = new Measures();
    refreshMeasuresTable();
}


void
MeasuresConfigPage::addMeasuresClicked
()
{
    QString token = tr("New");
    for (int i = 0; measuresTable->findItems(token, Qt::MatchExactly, 0).count() > 0 || measuresTable->findItems(token, Qt::MatchExactly, 1).count() > 0; i++) {
        token = tr("New (%1)").arg(i + 1);
    }
    measures->addGroup(new MeasuresGroup(token, token, QStringList(), QStringList(), QStringList(), QStringList(), QList<double>(), QList<QStringList>()));
    refreshMeasuresTable();
}


void
MeasuresConfigPage::removeMeasuresClicked
()
{
    // lets find the one we have selected...
    int row = measuresTable->invisibleRootItem()->indexOfChild(measuresTable->currentItem());
    if (row < 0) {
        return;
    }
    measures->removeGroup(row);
    refreshMeasuresTable();
}

void
MeasuresConfigPage::refreshMeasuresFieldsTable()
{
    disconnect(measuresFieldsTable->model(), &QAbstractItemModel::dataChanged, this, &MeasuresConfigPage::measureFieldChanged);

    // find the current Measures Group
    MeasuresGroup* group = measures->getGroup(measuresTable->invisibleRootItem()->indexOfChild(measuresTable->currentItem()));
    if (group == nullptr) return; // just in case...

    // remove existing rows
    measuresFieldsTable->clear();

    mflabel->setText(HLO + tr("Fields in Group <i>%1</i>").arg(group->getName()) + HLC);

    // lets populate
    for (int i=0; i<group->getFieldSymbols().count(); i++) {
        QTreeWidgetItem *add = new QTreeWidgetItem(measuresFieldsTable->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);
        MeasuresField field = group->getField(i);
        add->setData(0, Qt::DisplayRole, field.symbol);
        add->setData(1, Qt::DisplayRole, field.name);
        add->setData(2, Qt::DisplayRole, field.metricUnits);
        add->setData(3, Qt::DisplayRole, field.imperialUnits);
        add->setData(4, Qt::DisplayRole, QString::number(field.unitsFactor));
        add->setData(5, Qt::DisplayRole, field.headers.join(","));

        measuresFieldsTable->setCurrentItem(add); // select the last added
    }

    connect(measuresFieldsTable->model(), &QAbstractItemModel::dataChanged, this, &MeasuresConfigPage::measureFieldChanged);
}


void
MeasuresConfigPage::measureFieldChanged
(const QModelIndex &topLeft)
{
    if (measuresTable->currentItem() == nullptr) {
        return;
    }
    MeasuresGroup* group = measures->getGroup(measuresTable->invisibleRootItem()->indexOfChild(measuresTable->currentItem()));
    if (group == nullptr) {
        return;
    }
    int row = measuresFieldsTable->invisibleRootItem()->indexOfChild(measuresFieldsTable->currentItem());
    if (row < 0) {
        return;
    }

    // find the current Measures Group
    MeasuresField field = group->getField(row);
    switch (topLeft.column()) {
    case 0:
        field.symbol = topLeft.data(Qt::DisplayRole).toString();
        break;
    case 1:
        field.name = topLeft.data(Qt::DisplayRole).toString();
        break;
    case 2:
        field.metricUnits = topLeft.data(Qt::DisplayRole).toString();
        break;
    case 3:
        field.imperialUnits = topLeft.data(Qt::DisplayRole).toString();
        break;
    case 4:
        field.unitsFactor = topLeft.data(Qt::DisplayRole).toDouble();
        break;
    case 5:
        field.headers = topLeft.data(Qt::DisplayRole).toString().split(',');
        break;
    default:
        break;
    }
    group->setField(row, field);
}


void
MeasuresConfigPage::addMeasuresFieldClicked()
{
    // lets find the one we have selected...
    int index = measuresTable->currentIndex().row();
    if (index < 0) {
        return;
    }

    MeasuresGroup *group = measures->getGroup(measuresTable->invisibleRootItem()->indexOfChild(measuresTable->currentItem()));
    if (group == nullptr) {
        return;
    }

    QString token = tr("New");
    for (int i = 0; measuresFieldsTable->findItems(token, Qt::MatchExactly, 0).count() > 0 || measuresFieldsTable->findItems(token, Qt::MatchExactly, 1).count() > 0; i++) {
        token = tr("New (%1)").arg(i + 1);
    }
    MeasuresField field;
    field.symbol = token;
    field.name = token;
    group->addField(field);
    refreshMeasuresFieldsTable();
}

void
MeasuresConfigPage::removeMeasuresFieldClicked()
{
    // lets find the one we have selected...
    int row = measuresFieldsTable->invisibleRootItem()->indexOfChild(measuresFieldsTable->currentItem());
    if (row < 0) return; // nothing selected

    // find the current Measures Group
    MeasuresGroup* group = measures->getGroup(measuresTable->invisibleRootItem()->indexOfChild(measuresTable->currentItem()));

    group->removeField(row);
    refreshMeasuresFieldsTable();
}
