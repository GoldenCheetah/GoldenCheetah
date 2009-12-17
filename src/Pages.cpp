#include <QtGui>
#include <QIntValidator>
#include <assert.h>
#include "Pages.h"
#include "Settings.h"
#include "DeviceTypes.h"
#include "DeviceConfiguration.h"
#include "ANTplusController.h"

ConfigurationPage::ConfigurationPage()
{
    configGroup = new QGroupBox(tr("Golden Cheetah Configuration"));

    boost::shared_ptr<QSettings> settings = GetApplicationSettings();

    langLabel = new QLabel(tr("Language:"));

    langCombo = new QComboBox();
    langCombo->addItem(tr("English"));
    langCombo->addItem(tr("French"));

    QVariant lang = settings->value(GC_LANG);

    if(lang.toString() == "en")
        langCombo->setCurrentIndex(0);
    else if(lang.toString() == "fr")
        langCombo->setCurrentIndex(1);
    else // default : English
        langCombo->setCurrentIndex(0);

    unitLabel = new QLabel(tr("Unit of Measurement:"));

    unitCombo = new QComboBox();
    unitCombo->addItem(tr("Metric"));
    unitCombo->addItem(tr("English"));

    QVariant unit = settings->value(GC_UNIT);

    if(unit.toString() == "Metric")
	unitCombo->setCurrentIndex(0);
    else
	unitCombo->setCurrentIndex(1);

    QLabel *crankLengthLabel = new QLabel(tr("Crank Length:"));

    QVariant crankLength = settings->value(GC_CRANKLENGTH);

    crankLengthCombo = new QComboBox();
    crankLengthCombo->addItem("160");
    crankLengthCombo->addItem("162.5");
    crankLengthCombo->addItem("165");
    crankLengthCombo->addItem("167.5");
    crankLengthCombo->addItem("170");
    crankLengthCombo->addItem("172.5");
    crankLengthCombo->addItem("175");
    crankLengthCombo->addItem("177.5");
    crankLengthCombo->addItem("180");
    crankLengthCombo->addItem("182.5");
    crankLengthCombo->addItem("185");
    if(crankLength.toString() == "160")
	crankLengthCombo->setCurrentIndex(0);
    if(crankLength.toString() == "162.5")
	crankLengthCombo->setCurrentIndex(1);
    if(crankLength.toString() == "165")
	crankLengthCombo->setCurrentIndex(2);
    if(crankLength.toString() == "167.5")
	crankLengthCombo->setCurrentIndex(3);
    if(crankLength.toString() == "170")
	crankLengthCombo->setCurrentIndex(4);
    if(crankLength.toString() == "172.5")
	crankLengthCombo->setCurrentIndex(5);
    if(crankLength.toString() == "175")
	crankLengthCombo->setCurrentIndex(6);
    if(crankLength.toString() == "177.5")
	crankLengthCombo->setCurrentIndex(7);
    if(crankLength.toString() == "180")
	crankLengthCombo->setCurrentIndex(8);
    if(crankLength.toString() == "182.5")
	crankLengthCombo->setCurrentIndex(9);
    if(crankLength.toString() == "185")
	crankLengthCombo->setCurrentIndex(10);

    allRidesAscending = new QCheckBox("Sort ride list ascending.", this);
    QVariant isAscending = settings->value(GC_ALLRIDES_ASCENDING,Qt::Checked); // default is ascending sort
    if(isAscending.toInt() > 0 ){
	allRidesAscending->setCheckState(Qt::Checked);
    } else {
	allRidesAscending->setCheckState(Qt::Unchecked);
    }

    warningLabel = new QLabel(tr("Requires Restart To Take Effect"));

    langLayout = new QHBoxLayout;
    langLayout->addWidget(langLabel);
    langLayout->addWidget(langCombo);

    unitLayout = new QHBoxLayout;
    unitLayout->addWidget(unitLabel);
    unitLayout->addWidget(unitCombo);

    warningLayout = new QHBoxLayout;
    warningLayout->addWidget(warningLabel);

    QHBoxLayout *crankLengthLayout = new QHBoxLayout;
    crankLengthLayout->addWidget(crankLengthLabel);
    crankLengthLayout->addWidget(crankLengthCombo);


    // BikeScore Estimate
    QVariant BSdays = settings->value(GC_BIKESCOREDAYS);
    QVariant BSmode = settings->value(GC_BIKESCOREMODE);

    QGridLayout *bsDaysLayout = new QGridLayout;
    bsModeLayout = new QHBoxLayout;
    QLabel *BSDaysLabel1 = new QLabel(tr("BikeScore Estimate: use rides within last "));
    QLabel *BSDaysLabel2 = new QLabel(tr(" days"));
    BSdaysEdit = new QLineEdit(BSdays.toString(),this);
    BSdaysEdit->setInputMask("009");

    QLabel *BSModeLabel = new QLabel(tr("BikeScore estimate mode: "));
    bsModeCombo = new QComboBox();
    bsModeCombo->addItem("time");
    bsModeCombo->addItem("distance");
    if (BSmode.toString() == "time")
	bsModeCombo->setCurrentIndex(0);
    else
	bsModeCombo->setCurrentIndex(1);

    bsDaysLayout->addWidget(BSDaysLabel1,0,0);
    bsDaysLayout->addWidget(BSdaysEdit,0,1);
    bsDaysLayout->addWidget(BSDaysLabel2,0,2);

    bsModeLayout->addWidget(BSModeLabel);
    bsModeLayout->addWidget(bsModeCombo);




    configLayout = new QVBoxLayout;
    configLayout->addLayout(langLayout);
    configLayout->addLayout(unitLayout);
    configLayout->addWidget(allRidesAscending);
    configLayout->addLayout(crankLengthLayout);
    configLayout->addLayout(bsDaysLayout);
    configLayout->addLayout(bsModeLayout);
    configLayout->addLayout(warningLayout);
    configGroup->setLayout(configLayout);


    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(configGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}

CyclistPage::CyclistPage(const Zones *_zones):
    zones(_zones)
{
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();

    cyclistGroup = new QGroupBox(tr("Cyclist Options"));
    lblThreshold = new QLabel(tr("Critical Power:"));
    txtThreshold = new QLineEdit();

    // the validator will prevent numbers above the upper limit
    // from being entered, but will not prevent non-negative numbers
    // below the lower limit (since it is still plausible a valid
    // entry will result)
    txtThresholdValidator = new QIntValidator(20,999,this);
    txtThreshold->setValidator(txtThresholdValidator);

    btnBack = new QPushButton(this);
    btnBack->setText(tr("Back"));
    btnForward = new QPushButton(this);
    btnForward->setText(tr("Forward"));
    btnDelete = new QPushButton(this);
    btnDelete->setText(tr("Delete Range"));
    checkboxNew = new QCheckBox(this);
    checkboxNew->setText(tr("New Range from Date"));
    btnForward->setEnabled(false);
    txtStartDate = new QLabel("BEGIN");
    txtEndDate = new QLabel("END");
    lblStartDate = new QLabel("Start: ");
    lblStartDate->setAlignment(Qt::AlignRight);
    lblEndDate = new QLabel("End: ");
    lblEndDate->setAlignment(Qt::AlignRight);


    calendar = new QCalendarWidget(this);

    lblCurRange = new QLabel(this);
    lblCurRange->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    lblCurRange->setText(QString("Current Zone Range: %1").arg(currentRange + 1));

    perfManLabel = new QLabel(tr("Performance Manager"));
    perfManStartLabel = new QLabel(tr("Starting LTS"));
    perfManSTSLabel = new QLabel(tr("STS average (days)"));
    perfManLTSLabel = new QLabel(tr("LTS average (days)"));
    perfManStartValidator = new QIntValidator(0,200,this);
    perfManSTSavgValidator = new QIntValidator(1,21,this);
    perfManLTSavgValidator = new QIntValidator(7,56,this);
    QVariant perfManStartVal = settings->value(GC_INITIAL_STS);
    QVariant perfManSTSVal = settings->value(GC_STS_DAYS);
    if (perfManSTSVal.isNull() || perfManSTSVal.toInt() == 0)
	perfManSTSVal = 7;
    QVariant perfManLTSVal = settings->value(GC_LTS_DAYS);
    if (perfManLTSVal.isNull() || perfManLTSVal.toInt() == 0)
	perfManLTSVal = 42;
    perfManStart = new QLineEdit(perfManStartVal.toString(),this);
    perfManStart->setValidator(perfManStartValidator);
    perfManSTSavg = new QLineEdit(perfManSTSVal.toString(),this);
    perfManSTSavg->setValidator(perfManSTSavgValidator);
    perfManLTSavg = new QLineEdit(perfManLTSVal.toString(),this);
    perfManLTSavg->setValidator(perfManLTSavgValidator);


    QDate today = QDate::currentDate();
    calendar->setSelectedDate(today);

    if (zones->getRangeSize() == 0)
        setCurrentRange();
    else
    {
        setCurrentRange(zones->whichRange(today));
    	btnDelete->setEnabled(true);
	checkboxNew->setCheckState(Qt::Unchecked);
    }
    
    int cp = currentRange != -1 ? zones->getCP(currentRange) : 0;
    if (cp > 0)
	setCP(cp);

    //Layout
    powerLayout = new QHBoxLayout();
    powerLayout->addWidget(lblThreshold);
    powerLayout->addWidget(txtThreshold);

    rangeLayout = new QHBoxLayout();
    rangeLayout->addWidget(lblCurRange);
    
    dateRangeLayout = new QHBoxLayout();
    dateRangeLayout->addWidget(lblStartDate);
    dateRangeLayout->addWidget(txtStartDate);
    dateRangeLayout->addWidget(lblEndDate);
    dateRangeLayout->addWidget(txtEndDate);

    zoneLayout = new QHBoxLayout();
    zoneLayout->addWidget(btnBack);
    zoneLayout->addWidget(btnForward);
    zoneLayout->addWidget(btnDelete);
    zoneLayout->addWidget(checkboxNew);

    calendarLayout = new QHBoxLayout();
    calendarLayout->addWidget(calendar);

    // performance manager
    perfManLayout = new QVBoxLayout(); // outer
    perfManStartValLayout = new QHBoxLayout();
    perfManSTSavgLayout = new QHBoxLayout();
    perfManLTSavgLayout = new QHBoxLayout();
    perfManStartValLayout->addWidget(perfManStartLabel);
    perfManStartValLayout->addWidget(perfManStart);
    perfManSTSavgLayout->addWidget(perfManSTSLabel);
    perfManSTSavgLayout->addWidget(perfManSTSavg);
    perfManLTSavgLayout->addWidget(perfManLTSLabel);
    perfManLTSavgLayout->addWidget(perfManLTSavg);
    perfManLayout->addLayout(perfManStartValLayout);
    perfManLayout->addLayout(perfManSTSavgLayout);
    perfManLayout->addLayout(perfManLTSavgLayout);



    cyclistLayout = new QVBoxLayout;
    cyclistLayout->addLayout(powerLayout);
    cyclistLayout->addLayout(rangeLayout);
    cyclistLayout->addLayout(zoneLayout);
    cyclistLayout->addLayout(dateRangeLayout);
    cyclistLayout->addLayout(calendarLayout);
    cyclistLayout->addLayout(perfManLayout);

    cyclistGroup->setLayout(cyclistLayout);

    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(cyclistGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}

QString CyclistPage::getText()
{
    return txtThreshold->text();
}

int CyclistPage::getCP()
{
    int cp = txtThreshold->text().toInt();
    return (
	    (
	     (cp >= txtThresholdValidator->bottom()) &&
	     (cp <= txtThresholdValidator->top())
	     ) ?
	    cp :
	    0
	    );
}

void CyclistPage::setCP(int cp)
{
    txtThreshold->setText(QString("%1").arg(cp));
}

void CyclistPage::setSelectedDate(QDate date)
{
    calendar->setSelectedDate(date);
}

void CyclistPage::setCurrentRange(int range)
{
    int num_ranges = zones->getRangeSize();
    if ((num_ranges == 0) || (range < 0)) {
	btnBack->setEnabled(false);
	btnDelete->setEnabled(false);
	btnForward->setEnabled(false);
	calendar->setEnabled(false);
	checkboxNew->setCheckState(Qt::Checked);
	checkboxNew->setEnabled(false);
	currentRange = -1;
	lblCurRange->setText("no Current Zone Range");
	txtEndDate->setText("undefined");
	txtStartDate->setText("undefined");
	return;
    }

    assert ((range >= 0) && (range < num_ranges));
    currentRange = range;

    // update the labels
    lblCurRange->setText(QString("Current Zone Range: %1").arg(currentRange + 1));

    // update the visibility of the range select buttons
    btnForward->setEnabled(currentRange < num_ranges - 1);
    btnBack->setEnabled(currentRange > 0);

    // if we have ranges to set to, then the calendar must be on
    calendar->setEnabled(true);

    // update the CP display
    setCP(zones->getCP(currentRange));

    // update date limits
    txtStartDate->setText(zones->getStartDateString(currentRange));
    txtEndDate->setText(zones->getEndDateString(currentRange));
}


int CyclistPage::getCurrentRange()
{
    return currentRange;
}


bool CyclistPage::isNewMode()
{
    return (checkboxNew->checkState() == Qt::Checked);
}

DevicePage::DevicePage(QWidget *parent) : QWidget(parent)
{
    DeviceTypes all;
    devices = all.getList();

    nameLabel = new QLabel(tr("Device Name"),this);
    deviceName = new QLineEdit(tr(""), this);

    typeLabel = new QLabel(tr("Device Type"),this);
    typeSelector = new QComboBox(this);

    for (int i=0; i< devices.count(); i++) {
        DeviceType cur = devices.at(i);

        // WARNING: cur.type is what is stored in configuration
        //          do not change this!!
        typeSelector->addItem(cur.name, cur.type);
    }

    specLabel = new QLabel(tr("Device Port"),this);
    specHint = new QLabel();
    profHint = new QLabel();
    deviceSpecifier= new QLineEdit(tr(""), this);

    profLabel = new QLabel(tr("Device Profile"),this);
    deviceProfile = new QLineEdit(tr(""), this);

// THIS CODE IS DISABLED FOR THIS RELEASE XXX
//    isDefaultDownload = new QCheckBox(tr("Default download device"), this);
//    isDefaultRealtime = new QCheckBox(tr("Default realtime device"), this);

    addButton = new QPushButton(tr("Add"),this);
    delButton = new QPushButton(tr("Delete"),this);
    pairButton = new QPushButton(tr("Pair"),this);

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

    leftLayout = new QGridLayout();
    rightLayout = new QVBoxLayout();
    inLayout = new QGridLayout();
    deviceGroup = new QGroupBox(tr("Device Configuration"), this);

    leftLayout->addWidget(nameLabel, 0,0);
    leftLayout->addWidget(deviceName, 0,2);
    leftLayout->setRowMinimumHeight(1,10);
    leftLayout->addWidget(typeLabel, 2,0);
    leftLayout->addWidget(typeSelector, 2,2);
    leftLayout->setRowMinimumHeight(3,10);
    leftLayout->addWidget(specHint, 4,2);
    leftLayout->addWidget(specLabel, 5,0);
    leftLayout->addWidget(deviceSpecifier, 5,2);
    leftLayout->setRowMinimumHeight(6,10);
    leftLayout->addWidget(profHint, 7,2);
    leftLayout->addWidget(profLabel, 8,0);
    leftLayout->addWidget(deviceProfile, 8,2);

    leftLayout->setColumnMinimumWidth(1,10);

// THIS CODE IS DISABLED FOR THIS RELEASE XXX
//    leftLayout->addWidget(isDefaultDownload, 6,1);
//    leftLayout->addWidget(isDefaultRealtime, 8,1);

//    leftLayout->setRowStretch(0, 2);
//    leftLayout->setRowStretch(1, 1);
//    leftLayout->setRowStretch(2, 2);
//    leftLayout->setRowStretch(3, 1);
//    leftLayout->setRowStretch(4, 2);
//    leftLayout->setRowStretch(5, 1);
//    leftLayout->setRowStretch(6, 2);
//    leftLayout->setRowStretch(7, 1);
//    leftLayout->setRowStretch(8, 2);

    rightLayout->addWidget(addButton);
    rightLayout->addSpacing(10);
    rightLayout->addWidget(delButton);
    rightLayout->addStretch();
    rightLayout->addWidget(pairButton);

    inLayout->addItem(leftLayout, 0,0);
    inLayout->addItem(rightLayout, 0,1);
    inLayout->addWidget(deviceList,1,0,1,2);

    deviceGroup->setLayout(inLayout);

    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(deviceGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    // to make sure the default checkboxes have been set appropiately...
    // THIS CODE IS DISABLED IN THIS RELEASE XXX
    // isDefaultRealtime->setEnabled(false);

    setConfigPane();
}

void
DevicePage::setConfigPane()
{
    // depending upon the type of device selected
    // the spec hint tells the user the format they should use
    DeviceTypes Supported;

    // sorry... ;-) obfuscated c++ contest winner 2009
    switch (Supported.getType(typeSelector->itemData(typeSelector->currentIndex()).toInt()).connector) {

    case DEV_ANT:
        specHint->setText("hostname:port");
        profHint->setText("antid 1, antid 2 ...");
        profHint->show();
        pairButton->show();
        profLabel->show();
        deviceProfile->show();
        break;
    case DEV_SERIAL:
#ifdef WIN32
        specHint->setText("COMx");
#else
        specHint->setText("/dev/xxxx");
#endif
        pairButton->hide();
        profHint->hide();
        profLabel->hide();
        deviceProfile->hide();
        break;
    case DEV_TCP:
        specHint->setText("hostname:port");
        pairButton->hide();
        profHint->hide();
        profLabel->hide();
        deviceProfile->hide();
        break;
    }
    //specHint->setTextFormat(Qt::Italic); // mmm need to read the docos
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

void
DevicePage::pairClicked(DeviceConfiguration *dc, QProgressDialog *progress)
{
    ANTplusController ANTplus(0, dc);
    ANTplus.discover(dc, progress);
    deviceProfile->setText(dc->deviceProfile);
}

deviceModel::deviceModel(QObject *parent) : QAbstractTableModel(parent)
{
    this->parent = parent;

    // get current configuration
    DeviceConfigurations all;
    Configuration = all.getList();
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
    return 4;
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
                return lookupType.name;
                }
                break;
            case 2 :
                return Entry.portSpec;
                break;
            case 3 :
                return Entry.deviceProfile;
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
            }
            Configuration.replace(row,p);
                emit(dataChanged(index, index));

            return true;
        }

        return false;
}
