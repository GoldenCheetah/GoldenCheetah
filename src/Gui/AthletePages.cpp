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

#include "Athlete.h"
#include <QtGui>
#include <QIntValidator>

#include <assert.h>
#include <algorithm>

#include "AthletePages.h"
#include "Units.h"
#include "Settings.h"
#include "UserMetricParser.h"
#include "Units.h"
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
#ifdef GC_WANT_PYTHON
#include "PythonEmbed.h"
#include "FixPySettings.h"
#endif
#ifdef GC_HAS_CLOUD_DB
#include "CloudDBUserMetric.h"
#endif
#include "MainWindow.h"

//
// Passwords page
//
CredentialsPage::CredentialsPage(Context *context) : context(context)
{
    setBackgroundRole(QPalette::Base);
    QGridLayout *mainLayout = new QGridLayout(this);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
    editButton = new QPushButton(tr("Edit"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
#endif
    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2 *dpiXFactor);
    actionButtons->addStretch();
    actionButtons->addWidget(editButton);
    actionButtons->addStretch();
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);

    accounts = new QTreeWidget;
    accounts->headerItem()->setText(0, tr("Service"));
    accounts->headerItem()->setText(1, tr("Description"));
    accounts->setColumnCount(2);
    accounts->setColumnWidth(0, 175 * dpiXFactor);
    accounts->setSelectionMode(QAbstractItemView::SingleSelection);
    //fields->setUniformRowHeights(true);
    accounts->setIndentation(0);

    mainLayout->addWidget(accounts, 0,0);
    mainLayout->addLayout(actionButtons, 1,0);

    // list accounts...
    resetList();

    // connect up slots
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(editButton, SIGNAL(clicked()), this, SLOT(editClicked()));
}

void
CredentialsPage::resetList()
{
    // clear whats there
    while(accounts->invisibleRootItem()->childCount() > 0) {
        QTreeWidgetItem *take = accounts->invisibleRootItem()->takeChild(0);
        delete take;
    }

    // re-add
    int index=0;
    foreach (const QString name, CloudServiceFactory::instance().serviceNames()) {

        const CloudService *s = CloudServiceFactory::instance().service(name);

        // skip inactive accounts
        if (appsettings->cvalue(context->athlete->cyclist, s->activeSettingName(), false).toBool() == false) continue;

        QTreeWidgetItem *add = new QTreeWidgetItem;
        add->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
        add->setText(0, s->uiName());
        add->setTextAlignment(1, Qt::AlignLeft | Qt::AlignVCenter);
        add->setText(1, s->description());
        add->setText(2, s->id());

        accounts->invisibleRootItem()->insertChild(index++, add);
        accounts->hideColumn(2);
    }
}

qint32
CredentialsPage::saveClicked()
{

    return 0;
}

void
CredentialsPage::addClicked()
{
    // just run the add cloud wizard
    AddCloudWizard *wizard = new AddCloudWizard(context);

    // when the wizard closes we need to raise back - or else we get hidden
    //XXX connect(wizard, SIGNAL(finished(int)), configdialog_ptr, SLOT(raise()));
    wizard->exec();

    // update the account list
    resetList();
}

void
CredentialsPage::deleteClicked()
{
    // delete current
    if (accounts->selectedItems().count() == 0) return;

    // does it exist?
    const CloudService *service = CloudServiceFactory::instance().service(accounts->selectedItems().first()->text(2));
    if (service) {

        // set it inactive
        appsettings->setCValue(context->athlete->cyclist, service->activeSettingName(), false);
        appsettings->setCValue(context->athlete->cyclist, service->syncOnStartupSettingName(), false);
        appsettings->setCValue(context->athlete->cyclist, service->syncOnImportSettingName(), false);

        // reset
        resetList();
    }
}

void
CredentialsPage::editClicked()
{
    // edit current
    if (accounts->selectedItems().count() == 0) return;

    // edit the details
    AddCloudWizard *edit = new AddCloudWizard(context, accounts->selectedItems().first()->text(2));

    // when the wizard closes we need to raise back - or else we get hidden
    //XXX connect(edit, SIGNAL(finished(int)), configdialog_ptr, SLOT(raise()));
    edit->exec();

}

//
// About me
//
AboutRiderPage::AboutRiderPage(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    metricUnits = GlobalContext::context()->useMetricUnits;

    QVBoxLayout *all = new QVBoxLayout(this);
    QGridLayout *grid = new QGridLayout;

    QLabel *nicklabel = new QLabel(tr("Nickname"));
    QLabel *doblabel = new QLabel(tr("Date of Birth"));
    QLabel *sexlabel = new QLabel(tr("Sex"));
    weightlabel = new QLabel(tr("Weight"));
    heightlabel = new QLabel(tr("Height"));

    nickname = new QLineEdit(this);
    nickname->setText(appsettings->cvalue(context->athlete->cyclist, GC_NICKNAME, "").toString());
    if (nickname->text() == "0") nickname->setText("");

    dob = new QDateEdit(this);
    dob->setDate(appsettings->cvalue(context->athlete->cyclist, GC_DOB).toDate());
    dob->setCalendarPopup(true);
    dob->setDisplayFormat("yyyy/MM/dd");

    sex = new QComboBox(this);
    sex->addItem(tr("Male"));
    sex->addItem(tr("Female"));

    // we set to 0 or 1 for male or female since this
    // is language independent (and for once the code is easier!)
    sex->setCurrentIndex(appsettings->cvalue(context->athlete->cyclist, GC_SEX).toInt());

    weight = new QDoubleSpinBox(this);
    weight->setMaximum(999.9);
    weight->setMinimum(0.0);
    weight->setDecimals(1);
    weight->setValue(appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT).toDouble() * (metricUnits ? 1.0 : LB_PER_KG));
    weight->setSuffix(metricUnits ? tr(" kg") : tr(" lb"));

    height = new QDoubleSpinBox(this);
    height->setMaximum(999.9);
    height->setMinimum(0.0);
    height->setDecimals(1);
    height->setValue(appsettings->cvalue(context->athlete->cyclist, GC_HEIGHT).toDouble() * (metricUnits ? 100.0 : 100.0/CM_PER_INCH));
    height->setSuffix(metricUnits ? tr(" cm") : tr(" in"));

    if (QFileInfo(context->athlete->home->config().canonicalPath() + "/" + "avatar.png").exists())
        avatar = QPixmap(context->athlete->home->config().canonicalPath() + "/" + "avatar.png");
    else
        avatar = QPixmap(":/images/noavatar.png");

    avatarButton = new QPushButton(this);
    avatarButton->setContentsMargins(0,0,0,0);
    avatarButton->setFlat(true);
    avatarButton->setIcon(avatar.scaled(140,140));
    avatarButton->setIconSize(QSize(140,140));
    avatarButton->setFixedHeight(140);
    avatarButton->setFixedWidth(140);

    //
    // Crank length - only used by PfPv chart (should move there!)
    //
    QLabel *crankLengthLabel = new QLabel(tr("Crank Length"));
    QVariant crankLength = appsettings->cvalue(context->athlete->cyclist, GC_CRANKLENGTH, "175");
    crankLengthCombo = new QComboBox();
    crankLengthCombo->addItem("130");
    crankLengthCombo->addItem("135");
    crankLengthCombo->addItem("140");
    crankLengthCombo->addItem("145");
    crankLengthCombo->addItem("150");
    crankLengthCombo->addItem("155");
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
    crankLengthCombo->addItem("190");
    crankLengthCombo->addItem("195");
    crankLengthCombo->addItem("200");
    crankLengthCombo->addItem("205");
    crankLengthCombo->addItem("210");
    crankLengthCombo->addItem("215");
    crankLengthCombo->addItem("220");
    crankLengthCombo->setCurrentText(crankLength.toString());

    //
    // Wheel size
    //
    QLabel *wheelSizeLabel = new QLabel(tr("Wheelsize"), this);
    int wheelSize = appsettings->cvalue(context->athlete->cyclist, GC_WHEELSIZE, 2100).toInt();

    rimSizeCombo = new QComboBox();
    rimSizeCombo->addItems(WheelSize::RIM_SIZES);

    tireSizeCombo = new QComboBox();
    tireSizeCombo->addItems(WheelSize::TIRE_SIZES);


    wheelSizeEdit = new QLineEdit(QString("%1").arg(wheelSize),this);
    wheelSizeEdit->setInputMask("0000");
    wheelSizeEdit->setFixedWidth(40 *dpiXFactor);

    QLabel *wheelSizeUnitLabel = new QLabel(tr("mm"), this);

    QHBoxLayout *wheelSizeLayout = new QHBoxLayout();
    wheelSizeLayout->addWidget(rimSizeCombo);
    wheelSizeLayout->addWidget(tireSizeCombo);
    wheelSizeLayout->addWidget(wheelSizeEdit);
    wheelSizeLayout->addWidget(wheelSizeUnitLabel);

    connect(rimSizeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(calcWheelSize()));
    connect(tireSizeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(calcWheelSize()));
    connect(wheelSizeEdit, SIGNAL(textEdited(QString)), this, SLOT(resetWheelSize()));

    Qt::Alignment alignment = Qt::AlignLeft|Qt::AlignVCenter;

    grid->addWidget(nicklabel, 0, 0, alignment);
    grid->addWidget(doblabel, 1, 0, alignment);
    grid->addWidget(sexlabel, 2, 0, alignment);
    grid->addWidget(heightlabel, 3, 0, alignment);
    grid->addWidget(weightlabel, 4, 0, alignment);

    grid->addWidget(nickname, 0, 1, alignment);
    grid->addWidget(dob, 1, 1, alignment);
    grid->addWidget(sex, 2, 1, alignment);
    grid->addWidget(height, 3, 1, alignment);
    grid->addWidget(weight, 4, 1, alignment);

    grid->addWidget(crankLengthLabel, 5, 0, alignment);
    grid->addWidget(crankLengthCombo, 5, 1, alignment);
    grid->addWidget(wheelSizeLabel, 6, 0, alignment);
    grid->addLayout(wheelSizeLayout, 6, 1, 1, 2, alignment);

    grid->addWidget(avatarButton, 0, 1, 4, 2, Qt::AlignRight|Qt::AlignVCenter);
    all->addLayout(grid);
    all->addStretch();

    // save initial values for things we care about
    // note we don't worry about age or sex at this point
    // since they are not used, nor the W'bal tau used in
    // the realtime code. This is limited to stuff we
    // care about tracking as it is used by metrics
    b4.weight = appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT).toDouble();
    b4.height = appsettings->cvalue(context->athlete->cyclist, GC_HEIGHT).toDouble();
    b4.wheel = wheelSize;
    b4.crank = crankLengthCombo->currentIndex();

    connect (avatarButton, SIGNAL(clicked()), this, SLOT(chooseAvatar()));
}

void
AboutRiderPage::unitChanged(int currentIndex)
{
    if (currentIndex == 0) {
        metricUnits = true;
        weight->setValue(weight->value() / LB_PER_KG);
        height->setValue(height->value() * CM_PER_INCH);
    } else {
        metricUnits = false;
        weight->setValue(weight->value() * LB_PER_KG);
        height->setValue(height->value() / CM_PER_INCH);
    }
    weight->setSuffix(metricUnits ? tr(" kg") : tr(" lb"));
    height->setSuffix(metricUnits ? tr(" cm") : tr(" in"));
}

void
AboutRiderPage::chooseAvatar()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Choose Picture"),
                            "", tr("Images") + " (*.png *.jpg *.bmp)");
    if (filename != "") {

        avatar = QPixmap(filename);
        avatarButton->setIcon(avatar.scaled(140,140));
        avatarButton->setIconSize(QSize(140,140));
    }
}

void
AboutRiderPage::calcWheelSize()
{
   int diameter = WheelSize::calcPerimeter(rimSizeCombo->currentIndex(), tireSizeCombo->currentIndex());
   if (diameter>0)
        wheelSizeEdit->setText(QString("%1").arg(diameter));
}

void
AboutRiderPage::resetWheelSize()
{
   rimSizeCombo->setCurrentIndex(0);
   tireSizeCombo->setCurrentIndex(0);
}

qint32
AboutRiderPage::saveClicked()
{
    appsettings->setCValue(context->athlete->cyclist, GC_NICKNAME, nickname->text());
    appsettings->setCValue(context->athlete->cyclist, GC_DOB, dob->date());


    appsettings->setCValue(context->athlete->cyclist, GC_SEX, sex->currentIndex());
    avatar.save(context->athlete->home->config().canonicalPath() + "/" + "avatar.png", "PNG");
    appsettings->setCValue(context->athlete->cyclist, GC_WEIGHT, weight->value() * (metricUnits ? 1.0 : KG_PER_LB));
    appsettings->setCValue(context->athlete->cyclist, GC_HEIGHT, height->value() * (metricUnits ? 1.0/100.0 : CM_PER_INCH/100.0));

    appsettings->setCValue(context->athlete->cyclist, GC_CRANKLENGTH, crankLengthCombo->currentText());
    appsettings->setCValue(context->athlete->cyclist, GC_WHEELSIZE, wheelSizeEdit->text().toInt());

    qint32 state=0;

    // default weight changed ?
    if (b4.weight != appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT).toDouble()) {
        state += CONFIG_ATHLETE;
    }

    // default height changed ?
    if (b4.height != appsettings->cvalue(context->athlete->cyclist, GC_HEIGHT).toDouble()) {
        state += CONFIG_ATHLETE;
    }

    // general stuff changed ?
    if (b4.wheel != wheelSizeEdit->text().toInt() ||
        b4.crank != crankLengthCombo->currentIndex() )
        state += CONFIG_GENERAL;

    return state;
}

AboutModelPage::AboutModelPage(Context *context) : context(context)
{
    QVBoxLayout *all = new QVBoxLayout(this);
    QGridLayout *grid = new QGridLayout;

    //
    // W'bal Tau
    //
    wbaltaulabel = new QLabel(tr("W'bal tau (s)"));
    wbaltau = new QSpinBox(this);
    wbaltau->setMinimum(30);
    wbaltau->setMaximum(1200);
    wbaltau->setSingleStep(10);
    wbaltau->setValue(appsettings->cvalue(context->athlete->cyclist, GC_WBALTAU, 300).toInt());

    //
    // Performance manager
    //

    perfManSTSLabel = new QLabel(tr("STS average (days)"));
    perfManLTSLabel = new QLabel(tr("LTS average (days)"));
    perfManSTSavgValidator = new QIntValidator(1,21,this);
    perfManLTSavgValidator = new QIntValidator(7,56,this);

    // get config or set to defaults
    QVariant perfManSTSVal = appsettings->cvalue(context->athlete->cyclist, GC_STS_DAYS);
    if (perfManSTSVal.isNull() || perfManSTSVal.toInt() == 0) perfManSTSVal = 7;
    QVariant perfManLTSVal = appsettings->cvalue(context->athlete->cyclist, GC_LTS_DAYS);
    if (perfManLTSVal.isNull() || perfManLTSVal.toInt() == 0) perfManLTSVal = 42;

    perfManSTSavg = new QLineEdit(perfManSTSVal.toString(),this);
    perfManSTSavg->setValidator(perfManSTSavgValidator);
    perfManLTSavg = new QLineEdit(perfManLTSVal.toString(),this);
    perfManLTSavg->setValidator(perfManLTSavgValidator);

    showSBToday = new QCheckBox(tr("PMC Stress Balance Today"), this);
    showSBToday->setChecked(appsettings->cvalue(context->athlete->cyclist, GC_SB_TODAY).toInt());

    Qt::Alignment alignment = Qt::AlignLeft|Qt::AlignVCenter;

    grid->addWidget(wbaltaulabel, 9, 0, alignment);
    grid->addWidget(wbaltau, 9, 1, alignment);

    grid->addWidget(perfManSTSLabel, 10, 0, alignment);
    grid->addWidget(perfManSTSavg, 10, 1, alignment);
    grid->addWidget(perfManLTSLabel, 11, 0, alignment);
    grid->addWidget(perfManLTSavg, 11, 1, alignment);
    grid->addWidget(showSBToday, 12, 1, alignment);

    all->addLayout(grid);
    all->addStretch();

    // save initial values for things we care about
    // note we don't worry about age or sex at this point
    // since they are not used, nor the W'bal tau used in
    // the realtime code. This is limited to stuff we
    // care about tracking as it is used by metrics
    b4.lts = perfManLTSVal.toInt();
    b4.sts = perfManSTSVal.toInt();
}

qint32
AboutModelPage::saveClicked()
{
    // W'bal Tau
    appsettings->setCValue(context->athlete->cyclist, GC_WBALTAU, wbaltau->value());

    // Performance Manager
    appsettings->setCValue(context->athlete->cyclist, GC_STS_DAYS, perfManSTSavg->text());
    appsettings->setCValue(context->athlete->cyclist, GC_LTS_DAYS, perfManLTSavg->text());
    appsettings->setCValue(context->athlete->cyclist, GC_SB_TODAY, (int) showSBToday->isChecked());

    qint32 state=0;

    // PMC constants changed ?
    if(b4.lts != perfManLTSavg->text().toInt() || b4.sts != perfManSTSavg->text().toInt())
        state += CONFIG_PMC;

    return state;
}

BackupPage::BackupPage(Context *context) : context(context)
{
    QVBoxLayout *all = new QVBoxLayout(this);
    QGridLayout *grid = new QGridLayout;

    //
    // Auto Backup
    //
    // Selecting the storage folder folder of the Local File Store
    QLabel *autoBackupFolderLabel = new QLabel(tr("Auto Backup Folder"));
    autoBackupFolder = new QLineEdit(this);
    autoBackupFolder->setText(appsettings->cvalue(context->athlete->cyclist, GC_AUTOBACKUP_FOLDER, "").toString());
    autoBackupFolderBrowse = new QPushButton(tr("Browse"));
    connect(autoBackupFolderBrowse, SIGNAL(clicked()), this, SLOT(chooseAutoBackupFolder()));
    autoBackupPeriod = new QSpinBox(this);
    autoBackupPeriod->setMinimum(0);
    autoBackupPeriod->setMaximum(9999);
    autoBackupPeriod->setSingleStep(1);
    QLabel *autoBackupPeriodLabel = new QLabel(tr("Auto Backup execution every"));
    autoBackupPeriod->setValue(appsettings->cvalue(context->athlete->cyclist, GC_AUTOBACKUP_PERIOD, 0).toInt());
    QLabel *autoBackupUnitLabel = new QLabel(tr("times the athlete is closed - 0 means never"));
    QHBoxLayout *backupInput = new QHBoxLayout();
    backupInput->addWidget(autoBackupPeriod);
    //backupInput->addStretch();
    backupInput->addWidget(autoBackupUnitLabel);

    Qt::Alignment alignment = Qt::AlignLeft|Qt::AlignVCenter;

    grid->addWidget(autoBackupFolderLabel, 7,0, alignment);
    grid->addWidget(autoBackupFolder, 7, 1, alignment);
    grid->addWidget(autoBackupFolderBrowse, 7, 2, alignment);
    grid->addWidget(autoBackupPeriodLabel, 8, 0,alignment);
    grid->addLayout(backupInput, 8, 1, alignment);

    all->addLayout(grid);
    all->addStretch();
}

void BackupPage::chooseAutoBackupFolder()
{
    // did the user type something ? if not, get it from the Settings
    QString path = autoBackupFolder->text();
    if (path == "") path = appsettings->cvalue(context->athlete->cyclist, GC_AUTOBACKUP_FOLDER, "").toString();
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose Backup Directory"),
                            path, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir != "") autoBackupFolder->setText(dir);  //only overwrite current dir, if a new was selected

}

qint32
BackupPage::saveClicked()
{
    // Auto Backup
    appsettings->setCValue(context->athlete->cyclist, GC_AUTOBACKUP_FOLDER, autoBackupFolder->text());
    appsettings->setCValue(context->athlete->cyclist, GC_AUTOBACKUP_PERIOD, autoBackupPeriod->value());
    return 0;
}

//
// Time Dependent Measures
//
MeasuresPage::MeasuresPage(QWidget *parent, Context *context, MeasuresGroup *measuresGroup) : QWidget(parent), context(context), measuresGroup(measuresGroup)
{
    metricUnits = GlobalContext::context()->useMetricUnits;
    QList<double> unitsFactors = measuresGroup->getFieldUnitsFactors();

    QVBoxLayout *all = new QVBoxLayout(this);
    QGridLayout *measuresGrid = new QGridLayout;
    Qt::Alignment alignment = Qt::AlignLeft|Qt::AlignVCenter;

    QLabel* seperatorText = new QLabel(tr("Time dependent %1 measures").arg(measuresGroup->getName()));
    all->addWidget(seperatorText);

    QString dateTimetext = tr("From Date - Time");
    dateLabel = new QLabel(dateTimetext);
    dateTimeEdit = new QDateTimeEdit;
    dateTimeEdit->setDateTime(QDateTime::currentDateTime());
    dateTimeEdit->setCalendarPopup(true);
    dateTimeEdit->setDisplayFormat(tr("MMM d, yyyy - hh:mm:ss"));

    measuresGrid->addWidget(dateLabel, 1, 0, alignment);
    measuresGrid->addWidget(dateTimeEdit, 1, 1, alignment);

    QStringList fieldNames = measuresGroup->getFieldNames();
    valuesLabel = QVector<QLabel*>(fieldNames.count());
    valuesEdit = QVector<QDoubleSpinBox*>(fieldNames.count());
    int k = 0;
    foreach (QString fieldName, fieldNames) {

        valuesLabel[k] = new QLabel(fieldName);
        valuesEdit[k] = new QDoubleSpinBox(this);
        valuesEdit[k]->setMaximum(9999.99);
        valuesEdit[k]->setMinimum(0.0);
        valuesEdit[k]->setDecimals(2);
        valuesEdit[k]->setValue(0.0);
        valuesEdit[k]->setSuffix(QString(" %1").arg(measuresGroup->getFieldUnits(k, metricUnits)));

        measuresGrid->addWidget(valuesLabel[k], k+2, 0, alignment);
        measuresGrid->addWidget(valuesEdit[k], k+2, 1, alignment);

        k++;
    }

    QString commenttext = tr("Comment");
    commentlabel = new QLabel(commenttext);
    comment = new QLineEdit(this);
    comment->setText("");

    measuresGrid->addWidget(commentlabel, k+2, 0, alignment);
    measuresGrid->addWidget(comment, k+2, 1, alignment);

    all->addLayout(measuresGrid);


    // Buttons
    updateButton = new QPushButton(tr("Update"));
    updateButton->hide();
    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    updateButton->setText(tr("Update"));
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
#endif

    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2 *dpiXFactor);
    actionButtons->addStretch();
    actionButtons->addWidget(updateButton);
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);
    all->addLayout(actionButtons);

    // Measures
    measuresTree = new QTreeWidget;
    measuresTree->headerItem()->setText(0, dateTimetext);
    measuresTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    k = 0;
    foreach (QString fieldName, fieldNames) {
        measuresTree->headerItem()->setText(k+1, fieldName);
        measuresTree->header()->setSectionResizeMode(k+1, QHeaderView::ResizeToContents);
        k++;
    }

    measuresTree->headerItem()->setText(k+1, commenttext);
    measuresTree->header()->setSectionResizeMode(k+1, QHeaderView::ResizeToContents);
    measuresTree->headerItem()->setText(k+2, tr("Source"));
    measuresTree->header()->setSectionResizeMode(k+2, QHeaderView::ResizeToContents);
    measuresTree->headerItem()->setText(k+3, tr("Original Source"));
    measuresTree->setColumnCount(k+4);
    measuresTree->setSelectionMode(QAbstractItemView::SingleSelection);
    measuresTree->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    measuresTree->setUniformRowHeights(true);
    measuresTree->setIndentation(0);

    // get a copy of group measures if the file exists
    measures = QList<Measure>(measuresGroup->measures());
    std::reverse(measures.begin(), measures.end());

    // setup measuresTree
    for (int i=0; i<measures.count(); i++) {
        QTreeWidgetItem *add = new QTreeWidgetItem(measuresTree->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);
        // date & time
        add->setText(0, measures[i].when.toString(tr("MMM d, yyyy - hh:mm:ss")));
        // measures data
        k = 0;
        foreach (QString fieldName, fieldNames) {
            const double unitsFactor = (metricUnits ? 1.0 : unitsFactors[k]);
            add->setText(k+1, QString("%1").arg(measures[i].values[k]*unitsFactor, 0, 'f', 2));
            k++;
        }

        add->setText(k+1, measures[i].comment);
        // source
        add->setText(k+2, measures[i].getSourceDescription());
        add->setText(k+3, measures[i].originalSource);
    }

    all->addWidget(measuresTree);

    // set default edit values to newest measures (if one exists)
    if (measures.count() > 0) {
        for (int k = 0; k < fieldNames.count(); k++) {
            const double unitsFactor = (metricUnits ? 1.0 : unitsFactors[k]);
            valuesEdit[k]->setValue(measures[0].values[k]*unitsFactor);
        }
    }

    // edit connect
    connect(dateTimeEdit, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(rangeEdited()));
    for (int k = 0; k < fieldNames.count(); k++)
        connect(valuesEdit[k], SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));
    connect(comment, SIGNAL(textEdited(QString)), this, SLOT(rangeEdited()));

    // button connect
    connect(updateButton, SIGNAL(clicked()), this, SLOT(addOReditClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addOReditClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));

    // list selection connect
    connect(measuresTree, SIGNAL(itemSelectionChanged()), this, SLOT(rangeSelectionChanged()));

    // save initial values for things we care about
    b4.fingerprint = 0;
    foreach (Measure measure, measures) {
        b4.fingerprint += measure.getFingerprint();
    }
}

void
MeasuresPage::unitChanged(int currentIndex)
{
    metricUnits = (currentIndex == 0);
    QList<double> unitsFactors = measuresGroup->getFieldUnitsFactors();

    // update edit fields
    for (int k = 0; k < valuesEdit.count(); k++) {
        const double unitsFactor = (metricUnits ? 1.0/unitsFactors[k] : unitsFactors[k]);
        valuesEdit[k]->setValue(valuesEdit[k]->value() * unitsFactor);
        valuesEdit[k]->setSuffix(QString(" %1").arg(measuresGroup->getFieldUnits(k, metricUnits)));
    }

    // update measuresTree
    for (int i=0; i<measuresTree->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *edit = measuresTree->invisibleRootItem()->child(i);

        for (int k = 0; k < valuesEdit.count(); k++) {
            const double unitsFactor = (metricUnits ? 1.0 : unitsFactors[k]);
            edit->setText(k+1, QString("%1").arg(measures[i].values[k]*unitsFactor, 0, 'f', 2));
        }
    }
}

qint32
MeasuresPage::saveClicked()
{
    qint32 state=0;

    // Measures changed ?
    unsigned long fingerprint = 0;
    foreach (Measure measure, measures) {
        fingerprint += measure.getFingerprint();
    }
    if (fingerprint != b4.fingerprint) {
        // store in athlete
        std::reverse(measures.begin(), measures.end());
        measuresGroup->setMeasures(measures);
        // now save data away if we actually got something !
        measuresGroup->write();
        state += CONFIG_ATHLETE;
    }

    return state;
}

void
MeasuresPage::addOReditClicked()
{
    int index;
    QTreeWidgetItem *add;
    Measure addMeasure;
    QString dateTimeTxt = dateTimeEdit->dateTime().toString(tr("MMM d, yyyy - hh:mm:ss"));

    // if an entry for this date & time already exists, edit item otherwise add new
    QList<QTreeWidgetItem*> matches = measuresTree->findItems(dateTimeTxt, Qt::MatchExactly, 0);
    if (matches.count() > 0) {
        // edit existing
        add = matches[0];
        index = measuresTree->invisibleRootItem()->indexOfChild(matches[0]);
        measures.removeAt(index);
    } else {
        // add new
        index = measures.count();
        for (int i = 0; i < measures.count(); i++) {
            if (dateTimeEdit->dateTime() > measures[i].when) {
                index = i;
                break;
            }
        }
        add = new QTreeWidgetItem;
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);
        measuresTree->invisibleRootItem()->insertChild(index, add);
    }

    addMeasure.when = dateTimeEdit->dateTime();
    QList<double> unitsFactors = measuresGroup->getFieldUnitsFactors();
    for (int k = 0; k < valuesEdit.count(); k++) {
        const double unitsFactor = (metricUnits ? 1.0 : unitsFactors[k]);
        addMeasure.values[k] = valuesEdit[k]->value() / unitsFactor;
    }
    addMeasure.comment = comment->text();
    addMeasure.source = Measure::Manual;
    addMeasure.originalSource = "";
    measures.insert(index, addMeasure);

    // date and time
    add->setText(0, dateTimeTxt);
    // values
    int k;
    for (k = 0; k < valuesEdit.count(); k++)
        add->setText(k+1, QString("%1").arg(valuesEdit[k]->value(), 0, 'f', 2));
    add->setText(k+1, QString("%1").arg(comment->text()));
    add->setText(k+2, QString("%1").arg(tr("Manual entry")));
    add->setText(k+3, ""); // Original Source

    updateButton->hide();
}

void
MeasuresPage::deleteClicked()
{
    if (measuresTree->currentItem()) {
        int index = measuresTree->invisibleRootItem()->indexOfChild(measuresTree->currentItem());
        delete measuresTree->invisibleRootItem()->takeChild(index);
        measures.removeAt(index);
    }
}

void
MeasuresPage::rangeEdited()
{
    if (measuresTree->currentItem()) {
        int index = measuresTree->invisibleRootItem()->indexOfChild(measuresTree->currentItem());

        QDateTime dateTime = dateTimeEdit->dateTime();
        QDateTime odateTime = measures[index].when;

        bool valuesChanged = false;
        for (int k = 0; !valuesChanged && k < valuesEdit.count(); k++)
            if (valuesEdit[k]->value() != measures[index].values[k])
                valuesChanged = true;
        QString ncomment = comment->text();
        QString ocomment = measures[index].comment;

        if (dateTime == odateTime && (valuesChanged || ncomment != ocomment))
            updateButton->show();
        else
            updateButton->hide();
    }
}

void
MeasuresPage::rangeSelectionChanged()
{
    // fill with current details
    if (measuresTree->currentItem()) {

        int index = measuresTree->invisibleRootItem()->indexOfChild(measuresTree->currentItem());
        Measure current = measures[index];

        dateTimeEdit->setDateTime(current.when);
        QList<double> unitsFactors = measuresGroup->getFieldUnitsFactors();
        for (int k = 0; k < valuesEdit.count(); k++) {
            const double unitsFactor = (metricUnits ? 1.0 : unitsFactors[k]);
            valuesEdit[k]->setValue(current.values[k]*unitsFactor);
        }
        comment->setText(current.comment);

        updateButton->hide();
    }
}

//
// Power Zone Config page
//
ZonePage::ZonePage(Context *context) : context(context)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *hlayout = new QHBoxLayout;

    sportLabel = new QLabel(tr("Sport"));
    sportCombo = new QComboBox();
    hlayout->addStretch();
    hlayout->addWidget(sportLabel);
    hlayout->addWidget(sportCombo);
    hlayout->addStretch();
    layout->addLayout(hlayout);
    connect(sportCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSport()));
    tabs = new QTabWidget(this);
    layout->addWidget(tabs);

    foreach (QString sport, GlobalContext::context()->rideMetadata->sports()) {
        QString i = RideFile::sportTag(sport);

        // Add sport to combo
        sportCombo->addItem(sport, i);

        // get current config by reading it in (leave mainwindow zones alone)
        zones[i] = new Zones(i);
        QFile zonesFile(context->athlete->home->config().canonicalPath() + "/" + zones[i]->fileName());
        if (zonesFile.exists()) {
            zones[i]->read(zonesFile);
            zonesFile.close();
            b4Fingerprint[i] = zones[i]->getFingerprint(); // remember original state
        }

        // setup maintenance pages using current config
        schemePage[i] = new SchemePage(zones[i]);
        cpPage[i] = new CPPage(context, zones[i], schemePage[i]);
    }
    sportCombo->setCurrentIndex(0);

    // finish setup for the default sport
    changeSport();
}

ZonePage::~ZonePage()
{
    foreach (Zones* zone, zones) delete zone;
}

void
ZonePage::changeSport()
{
    // change tabs according to the selected sport
    QString i = sportCombo->currentData().toString();
    tabs->clear();
    tabs->addTab(cpPage[i], tr("Critical Power"));
    tabs->addTab(schemePage[i], tr("Default"));
}


qint32
ZonePage::saveClicked()
{
    qint32 changed = 0;
    qint32 cppageChanged = 0;
    // write
    foreach (QString i, zones.keys()) {
        zones[i]->setScheme(schemePage[i]->getScheme());
        zones[i]->write(context->athlete->home->config());

        // re-read Zones in case it changed
        QFile zonesFile(context->athlete->home->config().canonicalPath() + "/" + context->athlete->zones_[i]->fileName());
        context->athlete->zones_[i]->read(zonesFile);
        if (i != "Bike" && context->athlete->zones_[i]->getRangeSize() == 0) { // No Power zones
            // Start with Cycling Power zones for backward compatibilty
            QFile zonesFile(context->athlete->home->config().canonicalPath() + "/" + Zones().fileName());
            if (zonesFile.exists()) context->athlete->zones_[i]->read(zonesFile);
        }

        // use CP for FTP?
        appsettings->setCValue(context->athlete->cyclist, zones[i]->useCPforFTPSetting(), cpPage[i]->useCPForFTPCombo->currentIndex());


        // did cp for ftp change ?
        cppageChanged |= cpPage[i]->saveClicked();

        // did we change ?
        if (zones[i]->getFingerprint() != b4Fingerprint[i])
            changed = CONFIG_ZONES;
    }
    return changed | cppageChanged;
}

SchemePage::SchemePage(Zones* zones) : zones(zones)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
#endif
    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2 *dpiXFactor);
    actionButtons->addStretch();
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);

    scheme = new QTreeWidget;
    scheme->headerItem()->setText(0, tr("Short"));
    scheme->headerItem()->setText(1, tr("Long"));
    scheme->headerItem()->setText(2, tr("Percent of CP"));
    scheme->setColumnCount(3);
    scheme->setSelectionMode(QAbstractItemView::SingleSelection);
    scheme->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    scheme->setUniformRowHeights(true);
    scheme->setIndentation(0);
    //scheme->header()->resizeSection(0,90);
    //scheme->header()->resizeSection(1,200);
    //scheme->header()->resizeSection(2,80);

    // setup list
    for (int i=0; i< zones->getScheme().nzones_default; i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(scheme->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // tab name
        add->setText(0, zones->getScheme().zone_default_name[i]);
        // field name
        add->setText(1, zones->getScheme().zone_default_desc[i]);

        // low
        QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
        loedit->setMinimum(0);
        loedit->setMaximum(1000);
        loedit->setSingleStep(1.0);
        loedit->setDecimals(0);
        loedit->setValue(zones->getScheme().zone_default[i]);
        scheme->setItemWidget(add, 2, loedit);
    }

    mainLayout->addWidget(scheme);
    mainLayout->addLayout(actionButtons);

    // button connect
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
}

void
SchemePage::addClicked()
{
    // are we at maximum already?
    if (scheme->invisibleRootItem()->childCount() == 10) {
        QMessageBox err;
        err.setText(tr("Maximum of 10 zones reached."));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    int index = scheme->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
    loedit->setMinimum(0);
    loedit->setMaximum(1000);
    loedit->setSingleStep(1.0);
    loedit->setDecimals(0);
    loedit->setValue(100);

    scheme->invisibleRootItem()->insertChild(index, add);
    scheme->setItemWidget(add, 2, loedit);

    // Short
    QString text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);

    // long
    text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(1, text);
}

void
SchemePage::renameClicked()
{
    // which one is selected?
    if (scheme->currentItem()) scheme->editItem(scheme->currentItem(), 0);
}

void
SchemePage::deleteClicked()
{
    if (scheme->currentItem()) {
        int index = scheme->invisibleRootItem()->indexOfChild(scheme->currentItem());
        delete scheme->invisibleRootItem()->takeChild(index);
    }
}

// just for sorting
struct schemeitem {
    QString name, desc;
    int lo;
    double trimp;
    bool operator<(schemeitem right) const { return lo < right.lo; }
};

ZoneScheme
SchemePage::getScheme()
{
    // read the scheme widget and return a scheme object
    QList<schemeitem> table;
    ZoneScheme results;

    // read back the details from the table
    for (int i=0; i<scheme->invisibleRootItem()->childCount(); i++) {

        schemeitem add;
        add.name = scheme->invisibleRootItem()->child(i)->text(0);
        add.desc = scheme->invisibleRootItem()->child(i)->text(1);
        add.lo = ((QDoubleSpinBox *)(scheme->itemWidget(scheme->invisibleRootItem()->child(i), 2)))->value();
        table.append(add);
    }

    // sort the list into ascending order
    std::sort(table.begin(),table.end());

    // now update the results
    results.nzones_default = 0;
    foreach(schemeitem zone, table) {
        results.nzones_default++;
        results.zone_default.append(zone.lo);
        results.zone_default_is_pct.append(true);
        results.zone_default_name.append(zone.name);
        results.zone_default_desc.append(zone.desc);
    }

    return results;
}


CPPage::CPPage(Context *context, Zones *zones_, SchemePage *schemePage) :
               context(context), zones_(zones_), schemePage(schemePage)
{
    active = false;

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setSpacing(10 *dpiXFactor);

    updateButton = new QPushButton(tr("Update"));
    updateButton->hide();
    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    addButton->setText(tr("Add"));
    updateButton->setText(tr("Update"));
    deleteButton->setText(tr("Delete"));
#endif
    defaultButton = new QPushButton(tr("Def"));
    defaultButton->hide();

    addZoneButton = new QPushButton(tr("+"));
    deleteZoneButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addZoneButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteZoneButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    addZoneButton->setText(tr("Add"));
    deleteZoneButton->setText(tr("Delete"));
#endif

    QHBoxLayout *zoneButtons = new QHBoxLayout;
    zoneButtons->addStretch();
    zoneButtons->setSpacing(0);
    zoneButtons->addWidget(addZoneButton);
    zoneButtons->addWidget(deleteZoneButton);
    zoneButtons->addWidget(defaultButton);

    QHBoxLayout *addLayout = new QHBoxLayout;
    QLabel *dateLabel = new QLabel(tr("From Date"));
    QLabel *cpLabel = new QLabel(tr("Critical Power"));
    QLabel *aetLabel = new QLabel(tr("AeTP"));
    QLabel *ftpLabel = new QLabel(tr("FTP"));
    QLabel *wLabel = new QLabel(tr("W'"));
    QLabel *pmaxLabel = new QLabel(tr("Pmax"));
    dateEdit = new QDateEdit;
    dateEdit->setDate(QDate::currentDate());
    dateEdit->setCalendarPopup(true);

     // Use CP for FTP
    useCPForFTPCombo = new QComboBox(this);
    useCPForFTPCombo->addItem(tr("Use CP for all metrics"));
    useCPForFTPCombo->addItem(tr("Use FTP for Coggan metrics"));

    b4.cpforftp = appsettings->cvalue(context->athlete->cyclist, zones_->useCPforFTPSetting(), 0).toInt() ? 1 : 0;
    useCPForFTPCombo->setCurrentIndex(b4.cpforftp);

    cpEdit = new QDoubleSpinBox;
    cpEdit->setMinimum(0);
    cpEdit->setMaximum(1000);
    cpEdit->setSingleStep(1.0);
    cpEdit->setDecimals(0);

    aetEdit = new QDoubleSpinBox;
    aetEdit->setMinimum(0);
    aetEdit->setMaximum(1000);
    aetEdit->setSingleStep(1.0);
    aetEdit->setDecimals(0);

    ftpEdit = new QDoubleSpinBox;
    ftpEdit->setMinimum(0);
    ftpEdit->setMaximum(100000);
    ftpEdit->setSingleStep(100);
    ftpEdit->setDecimals(0);

    wEdit = new QDoubleSpinBox;
    wEdit->setMinimum(0);
    wEdit->setMaximum(100000);
    wEdit->setSingleStep(100);
    wEdit->setDecimals(0);

    pmaxEdit = new QDoubleSpinBox;
    pmaxEdit->setMinimum(0);
    pmaxEdit->setMaximum(3000);
    pmaxEdit->setSingleStep(1.0);
    pmaxEdit->setDecimals(0);

    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2 *dpiXFactor);
    actionButtons->addWidget(cpLabel);
    actionButtons->addWidget(cpEdit);


    actionButtons->addWidget(aetLabel);
    actionButtons->addWidget(aetEdit);
    actionButtons->addWidget(ftpLabel);
    actionButtons->addWidget(ftpEdit);

    actionButtons->addWidget(wLabel);
    actionButtons->addWidget(wEdit);
    actionButtons->addWidget(pmaxLabel);
    actionButtons->addWidget(pmaxEdit);
    actionButtons->addStretch();
    actionButtons->addWidget(updateButton);
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);

    addLayout->addWidget(dateLabel);
    addLayout->addWidget(dateEdit);
    addLayout->addStretch();
    addLayout->addWidget(useCPForFTPCombo);

    ranges = new QTreeWidget;
    initializeRanges();

    zones = new QTreeWidget;
    zones->headerItem()->setText(0, tr("Short"));
    zones->headerItem()->setText(1, tr("Long"));
    zones->headerItem()->setText(2, tr("From Watts"));
    zones->setColumnCount(3);
    zones->setSelectionMode(QAbstractItemView::SingleSelection);
    zones->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    zones->setUniformRowHeights(true);
    zones->setIndentation(0);

    mainLayout->addLayout(addLayout, 0,0);
    mainLayout->addLayout(actionButtons, 1,0);
    mainLayout->addWidget(ranges, 2,0);
    mainLayout->addLayout(zoneButtons, 3,0);
    mainLayout->addWidget(zones, 4,0);

    // edit connect
    connect(dateEdit, SIGNAL(dateChanged(QDate)), this, SLOT(rangeEdited()));
    connect(cpEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));
    connect(aetEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));
    connect(ftpEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));
    connect(wEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));
    connect(pmaxEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));
    // button connect
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(updateButton, SIGNAL(clicked()), this, SLOT(editClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(defaultButton, SIGNAL(clicked()), this, SLOT(defaultClicked()));
    connect(addZoneButton, SIGNAL(clicked()), this, SLOT(addZoneClicked()));
    connect(deleteZoneButton, SIGNAL(clicked()), this, SLOT(deleteZoneClicked()));
    connect(useCPForFTPCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(initializeRanges()));
    connect(ranges, SIGNAL(itemSelectionChanged()), this, SLOT(rangeSelectionChanged()));
    connect(zones, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(zonesChanged()));
}

qint32
CPPage::saveClicked()
{
    if (b4.cpforftp != useCPForFTPCombo->currentIndex())
        return CONFIG_ZONES;
    else
        return 0;
}

void
CPPage::initializeRanges() {

    while( int nb = ranges->topLevelItemCount () )
    {
        delete ranges->takeTopLevelItem( nb - 1 );
    }

    int column = 0;
    ranges->headerItem()->setText(column++, tr("From Date"));
    ranges->headerItem()->setText(column++, tr("Critical Power"));
    ranges->headerItem()->setText(column++, tr("AeTP"));
    ranges->headerItem()->setText(column++, tr("FTP"));
    ranges->headerItem()->setText(column++, tr("W'"));
    ranges->headerItem()->setText(column++, tr("Pmax"));

    ranges->setColumnCount(column);

    bool useCPForFTP = (useCPForFTPCombo->currentIndex() == 0 ? true : false);
    ranges->setColumnHidden(3, useCPForFTP);

    ranges->setSelectionMode(QAbstractItemView::SingleSelection);
    ranges->setUniformRowHeights(true);
    ranges->setIndentation(0);

    // setup list of ranges
    for (int i=0; i< zones_->getRangeSize(); i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(ranges->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // Embolden ranges with manually configured zones
        QFont font;
        font.setWeight(zones_->getZoneRange(i).zonesSetFromCP ?
                                        QFont::Normal : QFont::Black);

        int column = 0;
        // date
        add->setText(column, zones_->getStartDate(i).toString(tr("MMM d, yyyy")));
        add->setFont(column++, font);

        // CP
        add->setText(column, QString("%1").arg(zones_->getCP(i)));
        add->setFont(column++, font);

        // AeT
        add->setText(column, QString("%1").arg(zones_->getAeT(i)));
        add->setFont(column++, font);

        // FTP
        add->setText(column, QString("%1").arg(zones_->getFTP(i)));
        add->setFont(column++, font);

        // W'
        add->setText(column, QString("%1").arg(zones_->getWprime(i)));
        add->setFont(column++, font);

        // Pmax
        add->setText(column, QString("%1").arg(zones_->getPmax(i)));
        add->setFont(column++, font);
    }
    for(int i = 0; i < ranges->columnCount(); i++)
        ranges->resizeColumnToContents(i);
}



void
CPPage::addClicked()
{

    // get current scheme
    zones_->setScheme(schemePage->getScheme());

    int cp = cpEdit->value();
    if( cp <= 0 ) {
        QMessageBox err;
        err.setText(tr("Critical Power must be > 0"));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    int wp = wEdit->value() ? wEdit->value() : 20000;
    if (wp < 1000) wp *= 1000; // entered in kJ we want joules

    int pmax = pmaxEdit->value() ? pmaxEdit->value() : 1000;

    int index = zones_->addZoneRange(dateEdit->date(), cpEdit->value(), aetEdit->value(), ftpEdit->value(), wp, pmax);

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() & ~Qt::ItemIsEditable);
    ranges->invisibleRootItem()->insertChild(index, add);

    int column = 0;

    // date
    add->setText(column++, dateEdit->date().toString(tr("MMM d, yyyy")));

    // CP
    add->setText(column++, QString("%1").arg(cpEdit->value()));

    // AeT
    add->setText(column++, QString("%1").arg(aetEdit->value()));

    // FTP
    add->setText(column++, QString("%1").arg(ftpEdit->value()));

    // W'
    add->setText(column++, QString("%1").arg(wp));

    // Pmax
    add->setText(column++, QString("%1").arg(pmax));

}

void
CPPage::editClicked()
{
    // get current scheme
    zones_->setScheme(schemePage->getScheme());

    int cp = cpEdit->value();

    if( cp <= 0 ){
        QMessageBox err;
        err.setText(tr("Critical Power must be > 0"));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    int ftp = ftpEdit->value() ? ftpEdit->value() : cp;
    int wp = wEdit->value() ? wEdit->value() : 20000;
    if (wp < 1000) wp *= 1000; // entered in kJ we want joules

    int pmax = pmaxEdit->value() ? pmaxEdit->value() : 1000;

    QTreeWidgetItem *edit = ranges->selectedItems().at(0);
    int index = ranges->indexOfTopLevelItem(edit);


    int columns = 0;

    // date
    zones_->setStartDate(index, dateEdit->date());
    edit->setText(columns++, dateEdit->date().toString(tr("MMM d, yyyy")));

    // CP
    zones_->setCP(index, cp);
    edit->setText(columns++, QString("%1").arg(cp));

    // AeT
    zones_->setAeT(index, aetEdit->value());
    edit->setText(columns++, QString("%1").arg(aetEdit->value()));

    // show FTP if we use FTP for Coggan Metrics
    zones_->setFTP(index, ftp);
    edit->setText(columns++, QString("%1").arg(ftp));

    // W'
    zones_->setWprime(index, wp);
    edit->setText(columns++, QString("%1").arg(wp));

    // Pmax
    zones_->setPmax(index, pmax);
    edit->setText(columns++, QString("%1").arg(pmax));

}

void
CPPage::deleteClicked()
{
    if (ranges->currentItem()) {
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        delete ranges->invisibleRootItem()->takeChild(index);
        zones_->deleteRange(index);
    }
}

void
CPPage::defaultClicked()
{
    if (ranges->currentItem()) {

        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        ZoneRange current = zones_->getZoneRange(index);

        // unbold
        QFont font;
        font.setWeight(QFont::Normal);
        ranges->currentItem()->setFont(0, font);
        ranges->currentItem()->setFont(1, font);
        ranges->currentItem()->setFont(2, font);


        // set the range to use defaults on the scheme page
        zones_->setScheme(schemePage->getScheme());
        zones_->setZonesFromCP(index);

        // hide the default button since we are now using defaults
        defaultButton->hide();

        // update the zones display
        rangeSelectionChanged();
    }
}

void
CPPage::rangeEdited()
{
    if (ranges->currentItem()) {
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());

        QDate date = dateEdit->date();
        QDate odate = zones_->getStartDate(index);

        int cp = cpEdit->value();
        int ocp = zones_->getCP(index);

        int aet = aetEdit->value();
        int oaet = zones_->getAeT(index);

        int ftp = ftpEdit->value();
        int oftp = zones_->getFTP(index);

        int wp = wEdit->value();
        int owp = zones_->getWprime(index);

        int pmax = pmaxEdit->value();
        int opmax = zones_->getPmax(index);

        if (date != odate || cp != ocp || aet != oaet || ftp != oftp || wp != owp || pmax != opmax)
            updateButton->show();
        else
            updateButton->hide();
    }
}

void
CPPage::rangeSelectionChanged()
{
    active = true;

    // wipe away current contents of zones
    foreach (QTreeWidgetItem *item, zones->invisibleRootItem()->takeChildren()) {
        delete zones->itemWidget(item, 2);
        delete item;
    }

    // fill with current details
    if (ranges->currentItem()) {


        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        ZoneRange current = zones_->getZoneRange(index);

        dateEdit->setDate(zones_->getStartDate(index));
        cpEdit->setValue(zones_->getCP(index));
        aetEdit->setValue(zones_->getAeT(index));
        ftpEdit->setValue(zones_->getFTP(index));
        wEdit->setValue(zones_->getWprime(index));
        pmaxEdit->setValue(zones_->getPmax(index));

        if (current.zonesSetFromCP) {

            // reapply the scheme in case it has been changed
            zones_->setScheme(schemePage->getScheme());
            zones_->setZonesFromCP(index);
            current = zones_->getZoneRange(index);

            defaultButton->hide();

        } else defaultButton->show();

        for (int i=0; i< current.zones.count(); i++) {

            QTreeWidgetItem *add = new QTreeWidgetItem(zones->invisibleRootItem());
            add->setFlags(add->flags() | Qt::ItemIsEditable);

            // tab name
            add->setText(0, current.zones[i].name);
            // field name
            add->setText(1, current.zones[i].desc);

            // low
            QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
            loedit->setMinimum(0);
            loedit->setMaximum(1000);
            loedit->setSingleStep(1.0);
            loedit->setDecimals(0);
            loedit->setValue(current.zones[i].lo);
            zones->setItemWidget(add, 2, loedit);
            connect(loedit, SIGNAL(valueChanged(double)), this, SLOT(zonesChanged()));
        }
        for(int i = 0; i < zones->columnCount(); i++)
            zones->resizeColumnToContents(i);
    }

    active = false;
}

void
CPPage::addZoneClicked()
{
    // no range selected
    if (!ranges->currentItem()) return;

    // are we at maximum already?
    if (zones->invisibleRootItem()->childCount() == 10) {
        QMessageBox err;
        err.setText(tr("Maximum of 10 zones reached."));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    active = true;
    int index = zones->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
    loedit->setMinimum(0);
    loedit->setMaximum(1000);
    loedit->setSingleStep(1.0);
    loedit->setDecimals(0);
    loedit->setValue(100);

    zones->invisibleRootItem()->insertChild(index, add);
    zones->setItemWidget(add, 2, loedit);
    connect(loedit, SIGNAL(editingFinished()), this, SLOT(zonesChanged()));

    // Short
    QString text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);

    // long
    text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(1, text);
    active = false;

    zonesChanged();
}

void
CPPage::deleteZoneClicked()
{
    // no range selected
    if (ranges->invisibleRootItem()->indexOfChild(ranges->currentItem()) == -1)
        return;

    active = true;
    if (zones->currentItem()) {
        int index = zones->invisibleRootItem()->indexOfChild(zones->currentItem());
        delete zones->invisibleRootItem()->takeChild(index);
    }
    active = false;

    zonesChanged();
}

void
CPPage::zonesChanged()
{
    // only take changes when they are not done programmatically
    // the active flag is set when the tree is being modified
    // programmatically, but not when users interact with the widgets
    if (active == false) {
        // get the current zone range
        if (ranges->currentItem()) {

            int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
            ZoneRange current = zones_->getZoneRange(index);

            // embolden that range on the list to show it has been edited
            QFont font;
            font.setWeight(QFont::Black);
            ranges->currentItem()->setFont(0, font);
            ranges->currentItem()->setFont(1, font);
            ranges->currentItem()->setFont(2, font);

            // show the default button to undo
            defaultButton->show();

            // we manually edited so save in full
            current.zonesSetFromCP = false;

            // create the new zoneinfos for this range
            QList<ZoneInfo> zoneinfos;
            for (int i=0; i< zones->invisibleRootItem()->childCount(); i++) {
                QTreeWidgetItem *item = zones->invisibleRootItem()->child(i);
                zoneinfos << ZoneInfo(item->text(0),
                                      item->text(1),
                                      ((QDoubleSpinBox*)zones->itemWidget(item, 2))->value(),
                                      0);
            }

            // now sort the list
            std::sort(zoneinfos.begin(), zoneinfos.end());

            // now fill the highs
            for(int i=0; i<zoneinfos.count(); i++) {
                if (i+1 <zoneinfos.count())
                    zoneinfos[i].hi = zoneinfos[i+1].lo;
                else
                    zoneinfos[i].hi = INT_MAX;
            }
            current.zones = zoneinfos;

            // now replace the current range struct
            zones_->setZoneRange(index, current);
        }
    }
}

//
// Zone Config page
//
HrZonePage::HrZonePage(Context *context) : context(context)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *hlayout = new QHBoxLayout;

    sportLabel = new QLabel(tr("Sport"));
    sportCombo = new QComboBox();
    hlayout->addStretch();
    hlayout->addWidget(sportLabel);
    hlayout->addWidget(sportCombo);
    hlayout->addStretch();
    layout->addLayout(hlayout);
    connect(sportCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSport()));
    tabs = new QTabWidget(this);
    layout->addWidget(tabs);

    foreach (QString sport, GlobalContext::context()->rideMetadata->sports()) {
        QString i = RideFile::sportTag(sport);

        // Add sport to combo
        sportCombo->addItem(sport, i);

        // get current config by reading it in (leave mainwindow zones alone)
        hrZones[i] = new HrZones(i);
        QFile zonesFile(context->athlete->home->config().canonicalPath() + "/" + hrZones[i]->fileName());
        if (zonesFile.exists()) {
            hrZones[i]->read(zonesFile);
            zonesFile.close();
            b4Fingerprint[i] = hrZones[i]->getFingerprint(); // remember original state
        }

        // setup maintenance pages using current config
        schemePage[i] = new HrSchemePage(hrZones[i]);
        ltPage[i] = new LTPage(context, hrZones[i], schemePage[i]);
    }
    sportCombo->setCurrentIndex(0);

    // finish setup for the default sport
    changeSport();
}

HrZonePage::~HrZonePage()
{
    foreach (HrZones* hrzones, hrZones) delete hrzones;
}

void
HrZonePage::changeSport()
{
    // change tabs according to the selected sport
    QString i = sportCombo->currentData().toString();
    tabs->clear();
    tabs->addTab(ltPage[i], tr("Lactate Threshold"));
    tabs->addTab(schemePage[i], tr("Default"));
}

qint32
HrZonePage::saveClicked()
{
    qint32 changed = 0;

    // write
    foreach (QString i, hrZones.keys()) {
        hrZones[i]->setScheme(schemePage[i]->getScheme());
        hrZones[i]->write(context->athlete->home->config());

        // reread HR zones
        QFile hrzonesFile(context->athlete->home->config().canonicalPath() + "/" + context->athlete->hrzones_[i]->fileName());
        context->athlete->hrzones_[i]->read(hrzonesFile);
        if (context->athlete->hrzones_[i]->getRangeSize() == 0) { // No HR zones
            // Start with Cycling HR zones for backward compatibilty
            QFile hrzonesFile(context->athlete->home->config().canonicalPath() + "/" + HrZones().fileName());
            if (hrzonesFile.exists()) context->athlete->hrzones_[i]->read(hrzonesFile);
        }

        // did we change ?
        if (hrZones[i]->getFingerprint() != b4Fingerprint[i])
            changed = CONFIG_ZONES;
    }

    return changed;
}

HrSchemePage::HrSchemePage(HrZones *hrZones) : hrZones(hrZones)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5 *dpiXFactor);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
#endif
    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2 *dpiXFactor);
    actionButtons->addStretch();
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);

    scheme = new QTreeWidget;
    scheme->headerItem()->setText(0, tr("Short"));
    scheme->headerItem()->setText(1, tr("Long"));
    scheme->headerItem()->setText(2, tr("Percent of LT"));
    scheme->headerItem()->setText(3, tr("Trimp k"));
    scheme->setColumnCount(4);
    scheme->setSelectionMode(QAbstractItemView::SingleSelection);
    scheme->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    scheme->setUniformRowHeights(true);
    scheme->setIndentation(0);
    //scheme->header()->resizeSection(0,60);
    //scheme->header()->resizeSection(1,180);
    //scheme->header()->resizeSection(2,65);
    //scheme->header()->resizeSection(3,65);

    // setup list
    for (int i=0; i< hrZones->getScheme().nzones_default; i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(scheme->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // tab name
        add->setText(0, hrZones->getScheme().zone_default_name[i]);
        // field name
        add->setText(1, hrZones->getScheme().zone_default_desc[i]);

        // low
        QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
        loedit->setMinimum(0);
        loedit->setMaximum(1000);
        loedit->setSingleStep(1.0);
        loedit->setDecimals(0);
        loedit->setValue(hrZones->getScheme().zone_default[i]);
        scheme->setItemWidget(add, 2, loedit);

        // trimp
        QDoubleSpinBox *trimpedit = new QDoubleSpinBox(this);
        trimpedit->setMinimum(0);
        trimpedit->setMaximum(10);
        trimpedit->setSingleStep(0.1);
        trimpedit->setDecimals(2);
        trimpedit->setValue(hrZones->getScheme().zone_default_trimp[i]);
        scheme->setItemWidget(add, 3, trimpedit);
    }

    mainLayout->addWidget(scheme);
    mainLayout->addLayout(actionButtons);

    // button connect
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
}

void
HrSchemePage::addClicked()
{
    // are we at maximum already?
    if (scheme->invisibleRootItem()->childCount() == 10) {
        QMessageBox err;
        err.setText(tr("Maximum of 10 zones reached."));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    int index = scheme->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
    loedit->setMinimum(0);
    loedit->setMaximum(1000);
    loedit->setSingleStep(1.0);
    loedit->setDecimals(0);
    loedit->setValue(100);

    scheme->invisibleRootItem()->insertChild(index, add);
    scheme->setItemWidget(add, 2, loedit);

    //trimp
    QDoubleSpinBox *trimpedit = new QDoubleSpinBox(this);
    trimpedit->setMinimum(0);
    trimpedit->setMaximum(10);
    trimpedit->setSingleStep(0.1);
    trimpedit->setDecimals(2);
    trimpedit->setValue(1);

    scheme->setItemWidget(add, 3, trimpedit);

    // Short
    QString text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);

    // long
    text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(1, text);
}

void
HrSchemePage::renameClicked()
{
    // which one is selected?
    if (scheme->currentItem()) scheme->editItem(scheme->currentItem(), 0);
}

void
HrSchemePage::deleteClicked()
{
    if (scheme->currentItem()) {
        int index = scheme->invisibleRootItem()->indexOfChild(scheme->currentItem());
        delete scheme->invisibleRootItem()->takeChild(index);
    }
}

HrZoneScheme
HrSchemePage::getScheme()
{
    // read the scheme widget and return a scheme object
    QList<schemeitem> table;
    HrZoneScheme results;

    // read back the details from the table
    for (int i=0; i<scheme->invisibleRootItem()->childCount(); i++) {

        schemeitem add;
        add.name = scheme->invisibleRootItem()->child(i)->text(0);
        add.desc = scheme->invisibleRootItem()->child(i)->text(1);
        add.lo = ((QDoubleSpinBox *)(scheme->itemWidget(scheme->invisibleRootItem()->child(i), 2)))->value();
        add.trimp = ((QDoubleSpinBox *)(scheme->itemWidget(scheme->invisibleRootItem()->child(i), 3)))->value();
        table.append(add);
    }

    // sort the list into ascending order
    std::sort(table.begin(),table.end());

    // now update the results
    results.nzones_default = 0;
    foreach(schemeitem zone, table) {
        results.nzones_default++;
        results.zone_default.append(zone.lo);
        results.zone_default_is_pct.append(true);
        results.zone_default_name.append(zone.name);
        results.zone_default_desc.append(zone.desc);
        results.zone_default_trimp.append(zone.trimp);
    }

    return results;
}


LTPage::LTPage(Context *context, HrZones *hrZones, HrSchemePage *schemePage) :
               context(context), hrZones(hrZones), schemePage(schemePage)
{
    active = false;

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setSpacing(10 *dpiXFactor);

    updateButton = new QPushButton(tr("Update"));
    updateButton->hide();
    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    updateButton->setFixedSize(60,20);
    deleteButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    updateButton->setText(tr("Update"));
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
#endif
    defaultButton = new QPushButton(tr("Def"));
    defaultButton->hide();

    addZoneButton = new QPushButton(tr("+"));
    deleteZoneButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addZoneButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteZoneButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    addZoneButton->setText(tr("Add"));
    deleteZoneButton->setText(tr("Delete"));
#endif

    QHBoxLayout *zoneButtons = new QHBoxLayout;
    zoneButtons->addStretch();
    zoneButtons->addWidget(addZoneButton);
    zoneButtons->addWidget(deleteZoneButton);
    zoneButtons->addWidget(defaultButton);

    QHBoxLayout *addLayout = new QHBoxLayout;
    QLabel *dateLabel = new QLabel(tr("From Date"));
    dateEdit = new QDateEdit;
    dateEdit->setDate(QDate::currentDate());
    dateEdit->setCalendarPopup(true);

    addLayout->addWidget(dateLabel);
    addLayout->addWidget(dateEdit);
    addLayout->addStretch();

    QHBoxLayout *addLayout2 = new QHBoxLayout;
    QLabel *ltLabel = new QLabel(tr("Lactate Threshold"));
    QLabel *aetLabel = new QLabel(tr("Aerobic Threshold"));
    QLabel *restHrLabel = new QLabel(tr("Rest HR"));
    QLabel *maxHrLabel = new QLabel(tr("Max HR"));

    ltEdit = new QDoubleSpinBox;
    ltEdit->setMinimum(0);
    ltEdit->setMaximum(240);
    ltEdit->setSingleStep(1.0);
    ltEdit->setDecimals(0);

    aetEdit = new QDoubleSpinBox;
    aetEdit->setMinimum(0);
    aetEdit->setMaximum(240);
    aetEdit->setSingleStep(1.0);
    aetEdit->setDecimals(0);

    restHrEdit = new QDoubleSpinBox;
    restHrEdit->setMinimum(0);
    restHrEdit->setMaximum(240);
    restHrEdit->setSingleStep(1.0);
    restHrEdit->setDecimals(0);

    maxHrEdit = new QDoubleSpinBox;
    maxHrEdit->setMinimum(0);
    maxHrEdit->setMaximum(240);
    maxHrEdit->setSingleStep(1.0);
    maxHrEdit->setDecimals(0);

    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2 *dpiXFactor);
    actionButtons->addWidget(ltLabel);
    actionButtons->addWidget(ltEdit);
    actionButtons->addWidget(aetLabel);
    actionButtons->addWidget(aetEdit);
    actionButtons->addWidget(restHrLabel);
    actionButtons->addWidget(restHrEdit);
    actionButtons->addWidget(maxHrLabel);
    actionButtons->addWidget(maxHrEdit);
    actionButtons->addStretch();
    actionButtons->addWidget(updateButton);
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);

    ranges = new QTreeWidget;
    ranges->headerItem()->setText(0, tr("From Date"));
    ranges->headerItem()->setText(1, tr("Lactate Threshold"));
    ranges->headerItem()->setText(2, tr("Aerobic Threshold"));
    ranges->headerItem()->setText(3, tr("Rest HR"));
    ranges->headerItem()->setText(4, tr("Max HR"));
    ranges->setColumnCount(5);
    ranges->setSelectionMode(QAbstractItemView::SingleSelection);
    ranges->setUniformRowHeights(true);
    ranges->setIndentation(0);

    // setup list of ranges
    for (int i=0; i< hrZones->getRangeSize(); i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(ranges->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // Embolden ranges with manually configured zones
        QFont font;
        font.setWeight(hrZones->getHrZoneRange(i).hrZonesSetFromLT ?
                       QFont::Normal : QFont::Black);

        // date
        add->setText(0, hrZones->getStartDate(i).toString(tr("MMM d, yyyy")));
        add->setFont(0, font);

        // LT
        add->setText(1, QString("%1").arg(hrZones->getLT(i)));
        add->setFont(1, font);

        // AeT
        add->setText(2, QString("%1").arg(hrZones->getAeT(i)));
        add->setFont(2, font);

        // Rest HR
        add->setText(3, QString("%1").arg(hrZones->getRestHr(i)));
        add->setFont(3, font);

        // Max HR
        add->setText(4, QString("%1").arg(hrZones->getMaxHr(i)));
        add->setFont(4, font);
    }
    for(int i = 0; i < ranges->columnCount(); i++)
        ranges->resizeColumnToContents(i);

    zones = new QTreeWidget;
    zones->headerItem()->setText(0, tr("Short"));
    zones->headerItem()->setText(1, tr("Long"));
    zones->headerItem()->setText(2, tr("From BPM"));
    zones->headerItem()->setText(3, tr("Trimp k"));
    zones->setColumnCount(4);
    zones->setSelectionMode(QAbstractItemView::SingleSelection);
    zones->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    zones->setUniformRowHeights(true);
    zones->setIndentation(0);

    mainLayout->addLayout(addLayout, 0, 0);
    mainLayout->addLayout(actionButtons, 1, 0);
    mainLayout->addWidget(ranges, 2, 0);
    mainLayout->addLayout(zoneButtons, 3, 0);
    mainLayout->addWidget(zones, 4, 0);

    // edit connect
    connect(dateEdit, SIGNAL(dateChanged(QDate)), this, SLOT(rangeEdited()));
    connect(ltEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));
    connect(aetEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));
    connect(restHrEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));
    connect(maxHrEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));

    // button connect
    connect(updateButton, SIGNAL(clicked()), this, SLOT(editClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(defaultButton, SIGNAL(clicked()), this, SLOT(defaultClicked()));
    connect(addZoneButton, SIGNAL(clicked()), this, SLOT(addZoneClicked()));
    connect(deleteZoneButton, SIGNAL(clicked()), this, SLOT(deleteZoneClicked()));
    connect(ranges, SIGNAL(itemSelectionChanged()), this, SLOT(rangeSelectionChanged()));
    connect(zones, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(zonesChanged()));
}

void
LTPage::addClicked()
{
    if(ltEdit->value() <= 0 ) {
        QMessageBox err;
        err.setText(tr("Lactate Threshold must be > 0"));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    // get current scheme
    hrZones->setScheme(schemePage->getScheme());

    int index = hrZones->addHrZoneRange(dateEdit->date(), ltEdit->value(), aetEdit->value(), restHrEdit->value(), maxHrEdit->value());

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() & ~Qt::ItemIsEditable);
    ranges->invisibleRootItem()->insertChild(index, add);

    // date
    add->setText(0, dateEdit->date().toString(tr("MMM d, yyyy")));

    // LT
    add->setText(1, QString("%1").arg(ltEdit->value()));
    // AeT
    add->setText(2, QString("%1").arg(aetEdit->value()));
    // Rest HR
    add->setText(3, QString("%1").arg(restHrEdit->value()));
    // Max HR
    add->setText(4, QString("%1").arg(maxHrEdit->value()));
}

void
LTPage::editClicked()
{
    if(ltEdit->value() <= 0 ) {
        QMessageBox err;
        err.setText(tr("Lactate Threshold must be > 0"));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    // get current scheme
    hrZones->setScheme(schemePage->getScheme());

    QTreeWidgetItem *edit = ranges->selectedItems().at(0);
    int index = ranges->indexOfTopLevelItem(edit);

    // date
    hrZones->setStartDate(index, dateEdit->date());
    edit->setText(0, dateEdit->date().toString(tr("MMM d, yyyy")));

    // LT
    hrZones->setLT(index, ltEdit->value());
    edit->setText(1, QString("%1").arg(ltEdit->value()));
    // AeT
    hrZones->setAeT(index, aetEdit->value());
    edit->setText(2, QString("%1").arg(aetEdit->value()));
    // Rest HR
    hrZones->setRestHr(index, restHrEdit->value());
    edit->setText(3, QString("%1").arg(restHrEdit->value()));
    // Max HR
    hrZones->setMaxHr(index, maxHrEdit->value());
    edit->setText(4, QString("%1").arg(maxHrEdit->value()));
}

void
LTPage::deleteClicked()
{
    if (ranges->currentItem()) {
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        delete ranges->invisibleRootItem()->takeChild(index);
        hrZones->deleteRange(index);
    }
}

void
LTPage::defaultClicked()
{
    if (ranges->currentItem()) {

        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        HrZoneRange current = hrZones->getHrZoneRange(index);

        // unbold
        QFont font;
        font.setWeight(QFont::Normal);
        ranges->currentItem()->setFont(0, font);
        ranges->currentItem()->setFont(1, font);
        ranges->currentItem()->setFont(2, font);
        ranges->currentItem()->setFont(3, font);


        // set the range to use defaults on the scheme page
        hrZones->setScheme(schemePage->getScheme());
        hrZones->setHrZonesFromLT(index);

        // hide the default button since we are now using defaults
        defaultButton->hide();

        // update the zones display
        rangeSelectionChanged();
    }
}

void
LTPage::rangeEdited()
{
    if (ranges->currentItem()) {
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());

        QDate date = dateEdit->date();
        QDate odate = hrZones->getStartDate(index);

        int lt = ltEdit->value();
        int olt = hrZones->getLT(index);

        int aet = aetEdit->value();
        int oaet = hrZones->getAeT(index);

        int maxhr = maxHrEdit->value();
        int omaxhr = hrZones->getMaxHr(index);

        int resthr = restHrEdit->value();
        int oresthr = hrZones->getRestHr(index);

        if (date != odate || lt != olt || aet != oaet || maxhr != omaxhr || resthr != oresthr)
            updateButton->show();
        else
            updateButton->hide();
    }
}

void
LTPage::rangeSelectionChanged()
{
    active = true;

    // wipe away current contents of zones
    foreach (QTreeWidgetItem *item, zones->invisibleRootItem()->takeChildren()) {
        delete zones->itemWidget(item, 2);
        delete item;
    }

    // fill with current details
    if (ranges->currentItem()) {

        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        HrZoneRange current = hrZones->getHrZoneRange(index);

        dateEdit->setDate(hrZones->getStartDate(index));
        ltEdit->setValue(hrZones->getLT(index));
        aetEdit->setValue(hrZones->getAeT(index));
        maxHrEdit->setValue(hrZones->getMaxHr(index));
        restHrEdit->setValue(hrZones->getRestHr(index));

        if (current.hrZonesSetFromLT) {

            // reapply the scheme in case it has been changed
            hrZones->setScheme(schemePage->getScheme());
            hrZones->setHrZonesFromLT(index);
            current = hrZones->getHrZoneRange(index);

            defaultButton->hide();

        } else defaultButton->show();

        for (int i=0; i< current.zones.count(); i++) {

            QTreeWidgetItem *add = new QTreeWidgetItem(zones->invisibleRootItem());
            add->setFlags(add->flags() | Qt::ItemIsEditable);

            // tab name
            add->setText(0, current.zones[i].name);
            // field name
            add->setText(1, current.zones[i].desc);

            // low
            QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
            loedit->setMinimum(0);
            loedit->setMaximum(1000);
            loedit->setSingleStep(1.0);
            loedit->setDecimals(0);
            loedit->setValue(current.zones[i].lo);
            zones->setItemWidget(add, 2, loedit);
            connect(loedit, SIGNAL(editingFinished()), this, SLOT(zonesChanged()));

            //trimp
            QDoubleSpinBox *trimpedit = new QDoubleSpinBox(this);
            trimpedit->setMinimum(0);
            trimpedit->setMaximum(10);
            trimpedit->setSingleStep(0.1);
            trimpedit->setDecimals(2);
            trimpedit->setValue(current.zones[i].trimp);
            zones->setItemWidget(add, 3, trimpedit);
            connect(trimpedit, SIGNAL(editingFinished()), this, SLOT(zonesChanged()));

        }
        for(int i = 0; i < zones->columnCount(); i++)
            zones->resizeColumnToContents(i);
    }

    active = false;
}

void
LTPage::addZoneClicked()
{
    // no range selected
    if (!ranges->currentItem()) return;

    // are we at maximum already?
    if (zones->invisibleRootItem()->childCount() == 10) {
        QMessageBox err;
        err.setText(tr("Maximum of 10 zones reached."));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    active = true;
    int index = zones->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
    loedit->setMinimum(0);
    loedit->setMaximum(1000);
    loedit->setSingleStep(1.0);
    loedit->setDecimals(0);
    loedit->setValue(100);

    zones->invisibleRootItem()->insertChild(index, add);
    zones->setItemWidget(add, 2, loedit);
    connect(loedit, SIGNAL(editingFinished()), this, SLOT(zonesChanged()));

    //trimp
    QDoubleSpinBox *trimpedit = new QDoubleSpinBox(this);
    trimpedit->setMinimum(0);
    trimpedit->setMaximum(10);
    trimpedit->setSingleStep(0.1);
    trimpedit->setDecimals(2);
    trimpedit->setValue(1);
    zones->setItemWidget(add, 3, trimpedit);
    connect(trimpedit, SIGNAL(editingFinished()), this, SLOT(zonesChanged()));

    // Short
    QString text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);

    // long
    text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(1, text);
    active = false;

    zonesChanged();
}

void
LTPage::deleteZoneClicked()
{
    // no range selected
    if (ranges->invisibleRootItem()->indexOfChild(ranges->currentItem()) == -1)
        return;

    active = true;
    if (zones->currentItem()) {
        int index = zones->invisibleRootItem()->indexOfChild(zones->currentItem());
        delete zones->invisibleRootItem()->takeChild(index);
    }
    active = false;

    zonesChanged();
}

void
LTPage::zonesChanged()
{
    // only take changes when they are not done programmatically
    // the active flag is set when the tree is being modified
    // programmatically, but not when users interact with the widgets
    if (active == false) {
        // get the current zone range
        if (ranges->currentItem()) {

            int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
            HrZoneRange current = hrZones->getHrZoneRange(index);

            // embolden that range on the list to show it has been edited
            QFont font;
            font.setWeight(QFont::Black);
            ranges->currentItem()->setFont(0, font);
            ranges->currentItem()->setFont(1, font);
            ranges->currentItem()->setFont(2, font);
            ranges->currentItem()->setFont(3, font);

            // show the default button to undo
            defaultButton->show();

            // we manually edited so save in full
            current.hrZonesSetFromLT = false;

            // create the new zoneinfos for this range
            QList<HrZoneInfo> zoneinfos;
            for (int i=0; i< zones->invisibleRootItem()->childCount(); i++) {
                QTreeWidgetItem *item = zones->invisibleRootItem()->child(i);
                zoneinfos << HrZoneInfo(item->text(0),
                                        item->text(1),
                                        ((QDoubleSpinBox*)zones->itemWidget(item, 2))->value(),
                                        0, ((QDoubleSpinBox*)zones->itemWidget(item, 3))->value());
            }

            // now sort the list
            std::sort(zoneinfos.begin(), zoneinfos.end());

            // now fill the highs
            for(int i=0; i<zoneinfos.count(); i++) {
                if (i+1 <zoneinfos.count())
                    zoneinfos[i].hi = zoneinfos[i+1].lo;
                else
                    zoneinfos[i].hi = INT_MAX;
            }
            current.zones = zoneinfos;

            // now replace the current range struct
            hrZones->setHrZoneRange(index, current);
        }
    }
}

//
// Pace Zone Config page
//
PaceZonePage::PaceZonePage(Context *context) : context(context)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *hlayout = new QHBoxLayout;

    sportLabel = new QLabel(tr("Sport"));
    sportCombo = new QComboBox();
    sportCombo->addItem(tr("Run"));
    sportCombo->addItem(tr("Swim"));
    sportCombo->setCurrentIndex(0);
    hlayout->addStretch();
    hlayout->addWidget(sportLabel);
    hlayout->addWidget(sportCombo);
    hlayout->addStretch();
    layout->addLayout(hlayout);
    connect(sportCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSport(int)));
    tabs = new QTabWidget(this);
    layout->addWidget(tabs);

    for (int i=0; i < nSports; i++) {
        paceZones[i] = new PaceZones(i > 0);

        // get current config by reading it in (leave mainwindow zones alone)
        QFile zonesFile(context->athlete->home->config().canonicalPath() + "/" + paceZones[i]->fileName());
        if (zonesFile.exists()) {
            paceZones[i]->read(zonesFile);
            zonesFile.close();
            b4Fingerprint[i] = paceZones[i]->getFingerprint(); // remember original state
        }

        // setup maintenance pages using current config
        schemePages[i] = new PaceSchemePage(paceZones[i]);
        cvPages[i] = new CVPage(paceZones[i], schemePages[i]);
    }

    // finish setup for the default sport
    changeSport(sportCombo->currentIndex());
}

PaceZonePage::~PaceZonePage()
{
    for (int i=0; i<nSports; i++) delete paceZones[i];
}

void
PaceZonePage::changeSport(int i)
{
    // change tabs according to the selected sport
    tabs->clear();
    tabs->addTab(cvPages[i], tr("Critical Velocity"));
    tabs->addTab(schemePages[i], tr("Default"));
}

qint32
PaceZonePage::saveClicked()
{
    qint32 changed = 0;
    // write it
    for (int i=0; i < nSports; i++) {
        paceZones[i]->setScheme(schemePages[i]->getScheme());
        paceZones[i]->write(context->athlete->home->config());

        // reread Pace zones
        QFile pacezonesFile(context->athlete->home->config().canonicalPath() + "/" + context->athlete->pacezones_[i]->fileName());
        context->athlete->pacezones_[i]->read(pacezonesFile);

        // did we change ?
        if (paceZones[i]->getFingerprint() != b4Fingerprint[i])
            changed = CONFIG_ZONES;
    }
    return changed;
}

PaceSchemePage::PaceSchemePage(PaceZones* paceZones) : paceZones(paceZones)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
#endif
    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2 *dpiXFactor);
    actionButtons->addStretch();
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);

    scheme = new QTreeWidget;
    scheme->headerItem()->setText(0, tr("Short"));
    scheme->headerItem()->setText(1, tr("Long"));
    scheme->headerItem()->setText(2, tr("Percent of CV"));
    scheme->setColumnCount(3);
    scheme->setSelectionMode(QAbstractItemView::SingleSelection);
    scheme->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    scheme->setUniformRowHeights(true);
    scheme->setIndentation(0);
    //scheme->header()->resizeSection(0,90);
    //scheme->header()->resizeSection(1,200);
    //scheme->header()->resizeSection(2,80);

    // setup list
    for (int i=0; i< paceZones->getScheme().nzones_default; i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(scheme->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // tab name
        add->setText(0, paceZones->getScheme().zone_default_name[i]);
        // field name
        add->setText(1, paceZones->getScheme().zone_default_desc[i]);

        // low
        QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
        loedit->setMinimum(0);
        loedit->setMaximum(1000);
        loedit->setSingleStep(1.0);
        loedit->setDecimals(0);
        loedit->setValue(paceZones->getScheme().zone_default[i]);
        scheme->setItemWidget(add, 2, loedit);
    }

    mainLayout->addWidget(scheme);
    mainLayout->addLayout(actionButtons);

    // button connect
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
}

void
PaceSchemePage::addClicked()
{
    // are we at maximum already?
    if (scheme->invisibleRootItem()->childCount() == 10) {
        QMessageBox err;
        err.setText(tr("Maximum of 10 zones reached."));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    int index = scheme->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
    loedit->setMinimum(0);
    loedit->setMaximum(1000);
    loedit->setSingleStep(1.0);
    loedit->setDecimals(0);
    loedit->setValue(100);

    scheme->invisibleRootItem()->insertChild(index, add);
    scheme->setItemWidget(add, 2, loedit);

    // Short
    QString text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);

    // long
    text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(1, text);
}

void
PaceSchemePage::renameClicked()
{
    // which one is selected?
    if (scheme->currentItem()) scheme->editItem(scheme->currentItem(), 0);
}

void
PaceSchemePage::deleteClicked()
{
    if (scheme->currentItem()) {
        int index = scheme->invisibleRootItem()->indexOfChild(scheme->currentItem());
        delete scheme->invisibleRootItem()->takeChild(index);
    }
}

// just for sorting
struct paceschemeitem {
    QString name, desc;
    int lo;
    double trimp;
    bool operator<(paceschemeitem right) const { return lo < right.lo; }
};

PaceZoneScheme
PaceSchemePage::getScheme()
{
    // read the scheme widget and return a scheme object
    QList<paceschemeitem> table;
    PaceZoneScheme results;

    // read back the details from the table
    for (int i=0; i<scheme->invisibleRootItem()->childCount(); i++) {

        paceschemeitem add;
        add.name = scheme->invisibleRootItem()->child(i)->text(0);
        add.desc = scheme->invisibleRootItem()->child(i)->text(1);
        add.lo = ((QDoubleSpinBox *)(scheme->itemWidget(scheme->invisibleRootItem()->child(i), 2)))->value();
        table.append(add);
    }

    // sort the list into ascending order
    std::sort(table.begin(),table.end());

    // now update the results
    results.nzones_default = 0;
    foreach(paceschemeitem zone, table) {
        results.nzones_default++;
        results.zone_default.append(zone.lo);
        results.zone_default_is_pct.append(true);
        results.zone_default_name.append(zone.name);
        results.zone_default_desc.append(zone.desc);
    }

    return results;
}

CVPage::CVPage(PaceZones* paceZones, PaceSchemePage *schemePage) :
               paceZones(paceZones), schemePage(schemePage)
{
    active = false;

    metricPace = appsettings->value(this, paceZones->paceSetting(), GlobalContext::context()->useMetricUnits).toBool();

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setSpacing(10 *dpiXFactor);

    updateButton = new QPushButton(tr("Update"));
    updateButton->hide();
    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    updateButton->setFixedSize(60,20);
    addButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    updateButton->setText(tr("Update"));
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
#endif
    defaultButton = new QPushButton(tr("Def"));
    defaultButton->hide();

    addZoneButton = new QPushButton(tr("+"));
    deleteZoneButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addZoneButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteZoneButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    addZoneButton->setText(tr("Add"));
    deleteZoneButton->setText(tr("Delete"));
#endif

    QHBoxLayout *zoneButtons = new QHBoxLayout;
    zoneButtons->addStretch();
    zoneButtons->setSpacing(0);
    zoneButtons->addWidget(addZoneButton);
    zoneButtons->addWidget(deleteZoneButton);

    QHBoxLayout *addLayout = new QHBoxLayout;
    QLabel *dateLabel = new QLabel(tr("From Date"));
    QLabel *cpLabel = new QLabel(tr("Critical Velocity"));
    QLabel *aetLabel = new QLabel(tr("Aerobic Threshold"));
    dateEdit = new QDateEdit;
    dateEdit->setDate(QDate::currentDate());
    dateEdit->setCalendarPopup(true);

    cvEdit = new QTimeEdit();
    cvEdit->setMinimumTime(QTime::fromString("00:00", "mm:ss"));
    cvEdit->setMaximumTime(QTime::fromString("20:00", "mm:ss"));
    cvEdit->setDisplayFormat("mm:ss");

    aetEdit = new QTimeEdit();
    aetEdit->setMinimumTime(QTime::fromString("00:00", "mm:ss"));
    aetEdit->setMaximumTime(QTime::fromString("20:00", "mm:ss"));
    aetEdit->setDisplayFormat("mm:ss");

    per = new QLabel(this);
    per->setText(paceZones->paceUnits(metricPace));

    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2 *dpiXFactor);
    actionButtons->addWidget(cpLabel);
    actionButtons->addWidget(cvEdit);
    actionButtons->addWidget(aetLabel);
    actionButtons->addWidget(aetEdit);
    actionButtons->addWidget(per);
    actionButtons->addStretch();
    actionButtons->addWidget(updateButton);
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);
    actionButtons->addWidget(defaultButton);

    addLayout->addWidget(dateLabel);
    addLayout->addWidget(dateEdit);
    addLayout->addStretch();

    ranges = new QTreeWidget;
    ranges->headerItem()->setText(0, tr("From Date"));
    ranges->headerItem()->setText(1, tr("Critical Velocity"));
    ranges->headerItem()->setText(2, tr("Aerobic Threshold"));
    ranges->setColumnCount(3);
    ranges->setSelectionMode(QAbstractItemView::SingleSelection);
    ranges->setUniformRowHeights(true);
    ranges->setIndentation(0);

    // setup list of ranges
    for (int i=0; i< paceZones->getRangeSize(); i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(ranges->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // Embolden ranges with manually configured zones
        QFont font;
        font.setWeight(paceZones->getZoneRange(i).zonesSetFromCV ?
                                        QFont::Normal : QFont::Black);

        // date
        add->setText(0, paceZones->getStartDate(i).toString(tr("MMM d, yyyy")));
        add->setFont(0, font);

        // CV
        double kph = paceZones->getCV(i);
        add->setText(1, QString("%1 %2 %3 %4")
                    .arg(paceZones->kphToPaceString(kph, true))
                    .arg(paceZones->paceUnits(true))
                    .arg(paceZones->kphToPaceString(kph, false))
                    .arg(paceZones->paceUnits(false)));
        add->setFont(1, font);

        // AeT
        kph = paceZones->getAeT(i);
        add->setText(2, QString("%1 %2 %3 %4")
                    .arg(paceZones->kphToPaceString(kph, true))
                    .arg(paceZones->paceUnits(true))
                    .arg(paceZones->kphToPaceString(kph, false))
                    .arg(paceZones->paceUnits(false)));
        add->setFont(2, font);
    }
    for(int i = 0; i < ranges->columnCount(); i++)
        ranges->resizeColumnToContents(i);

    zones = new QTreeWidget;
    zones->headerItem()->setText(0, tr("Short"));
    zones->headerItem()->setText(1, tr("Long"));
    zones->headerItem()->setText(2, tr("From"));
    zones->setColumnCount(3);
    zones->setSelectionMode(QAbstractItemView::SingleSelection);
    zones->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    zones->setUniformRowHeights(true);
    zones->setIndentation(0);

    mainLayout->addLayout(addLayout, 0,0);
    mainLayout->addLayout(actionButtons, 1,0);
    mainLayout->addWidget(ranges, 2,0);
    mainLayout->addLayout(zoneButtons, 3,0);
    mainLayout->addWidget(zones, 4,0);

    // edit connect
    connect(dateEdit, SIGNAL(dateChanged(QDate)), this, SLOT(rangeEdited()));
    connect(cvEdit, SIGNAL(timeChanged(QTime)), this, SLOT(rangeEdited()));
    connect(aetEdit, SIGNAL(timeChanged(QTime)), this, SLOT(rangeEdited()));

    // button connect
    connect(updateButton, SIGNAL(clicked()), this, SLOT(editClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(defaultButton, SIGNAL(clicked()), this, SLOT(defaultClicked()));
    connect(addZoneButton, SIGNAL(clicked()), this, SLOT(addZoneClicked()));
    connect(deleteZoneButton, SIGNAL(clicked()), this, SLOT(deleteZoneClicked()));
    connect(ranges, SIGNAL(itemSelectionChanged()), this, SLOT(rangeSelectionChanged()));
    connect(zones, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(zonesChanged()));
}

void
CVPage::addClicked()
{
    if( paceZones->kphFromTime(cvEdit, metricPace) <= 0 ) {
        QMessageBox err;
        err.setText(tr("Critical Velocity must be > 0"));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    // get current scheme
    paceZones->setScheme(schemePage->getScheme());

    int index = paceZones->addZoneRange(dateEdit->date(), paceZones->kphFromTime(cvEdit, metricPace), paceZones->kphFromTime(aetEdit, metricPace));

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() & ~Qt::ItemIsEditable);
    ranges->invisibleRootItem()->insertChild(index, add);

    // date
    add->setText(0, dateEdit->date().toString(tr("MMM d, yyyy")));

    // CV
    double kph = paceZones->kphFromTime(cvEdit, metricPace);
    add->setText(1, QString("%1 %2 %3 %4")
            .arg(paceZones->kphToPaceString(kph, true))
            .arg(paceZones->paceUnits(true))
            .arg(paceZones->kphToPaceString(kph, false))
            .arg(paceZones->paceUnits(false)));

    // AeT
    kph = paceZones->kphFromTime(aetEdit, metricPace);
    add->setText(2, QString("%1 %2 %3 %4")
            .arg(paceZones->kphToPaceString(kph, true))
            .arg(paceZones->paceUnits(true))
            .arg(paceZones->kphToPaceString(kph, false))
            .arg(paceZones->paceUnits(false)));

}

void
CVPage::editClicked()
{
    if( paceZones->kphFromTime(cvEdit, metricPace) <= 0 ) {
        QMessageBox err;
        err.setText(tr("Critical Velocity must be > 0"));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    // get current scheme
    paceZones->setScheme(schemePage->getScheme());

    QTreeWidgetItem *edit = ranges->selectedItems().at(0);
    int index = ranges->indexOfTopLevelItem(edit);

    // date
    paceZones->setStartDate(index, dateEdit->date());
    edit->setText(0, dateEdit->date().toString(tr("MMM d, yyyy")));

    // CV
    double kph = paceZones->kphFromTime(cvEdit, metricPace);
    paceZones->setCV(index, kph);
    edit->setText(1, QString("%1 %2 %3 %4")
            .arg(paceZones->kphToPaceString(kph, true))
            .arg(paceZones->paceUnits(true))
            .arg(paceZones->kphToPaceString(kph, false))
            .arg(paceZones->paceUnits(false)));

    // AeT
    kph = paceZones->kphFromTime(aetEdit, metricPace);
    paceZones->setAeT(index, kph);
    edit->setText(2, QString("%1 %2 %3 %4")
            .arg(paceZones->kphToPaceString(kph, true))
            .arg(paceZones->paceUnits(true))
            .arg(paceZones->kphToPaceString(kph, false))
            .arg(paceZones->paceUnits(false)));
}

void
CVPage::deleteClicked()
{
    if (ranges->currentItem()) {
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        delete ranges->invisibleRootItem()->takeChild(index);
        paceZones->deleteRange(index);
    }
}

void
CVPage::defaultClicked()
{
    if (ranges->currentItem()) {

        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        PaceZoneRange current = paceZones->getZoneRange(index);

        // unbold
        QFont font;
        font.setWeight(QFont::Normal);
        ranges->currentItem()->setFont(0, font);
        ranges->currentItem()->setFont(1, font);
        ranges->currentItem()->setFont(2, font);


        // set the range to use defaults on the scheme page
        paceZones->setScheme(schemePage->getScheme());
        paceZones->setZonesFromCV(index);

        // hide the default button since we are now using defaults
        defaultButton->hide();

        // update the zones display
        rangeSelectionChanged();
    }
}

void
CVPage::rangeEdited()
{
    if (ranges->currentItem()) {
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());

        QDate date = dateEdit->date();
        QDate odate = paceZones->getStartDate(index);
        QTime cv = cvEdit->time();
        QTime ocv = QTime::fromString(paceZones->kphToPaceString(paceZones->getCV(index), metricPace), "mm:ss");
        QTime aet = aetEdit->time();
        QTime oaet = QTime::fromString(paceZones->kphToPaceString(paceZones->getAeT(index), metricPace), "mm:ss");

        if (date != odate || cv != ocv || aet != oaet)
            updateButton->show();
        else
            updateButton->hide();
    }
}

void
CVPage::rangeSelectionChanged()
{
    active = true;

    // wipe away current contents of zones
    foreach (QTreeWidgetItem *item, zones->invisibleRootItem()->takeChildren()) {
        delete zones->itemWidget(item, 2);
        delete item;
    }

    // fill with current details
    if (ranges->currentItem()) {

        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        PaceZoneRange current = paceZones->getZoneRange(index);
        dateEdit->setDate(paceZones->getStartDate(index));
        cvEdit->setTime(QTime::fromString(paceZones->kphToPaceString(paceZones->getCV(index), metricPace), "mm:ss"));
        aetEdit->setTime(QTime::fromString(paceZones->kphToPaceString(paceZones->getAeT(index), metricPace), "mm:ss"));

        if (current.zonesSetFromCV) {

            // reapply the scheme in case it has been changed
            paceZones->setScheme(schemePage->getScheme());
            paceZones->setZonesFromCV(index);
            current = paceZones->getZoneRange(index);

            defaultButton->hide();

        } else defaultButton->show();

        for (int i=0; i< current.zones.count(); i++) {

            QTreeWidgetItem *add = new QTreeWidgetItem(zones->invisibleRootItem());
            add->setFlags(add->flags() | Qt::ItemIsEditable);

            // tab name
            add->setText(0, current.zones[i].name);
            // field name
            add->setText(1, current.zones[i].desc);

            // low
            QTimeEdit *loedit = new QTimeEdit(QTime::fromString(paceZones->kphToPaceString(current.zones[i].lo, metricPace), "mm:ss"), this);
            loedit->setMinimumTime(QTime::fromString("00:00", "mm:ss"));
            loedit->setMaximumTime(QTime::fromString("20:00", "mm:ss"));
            loedit->setDisplayFormat("mm:ss");
            zones->setItemWidget(add, 2, loedit);
            connect(loedit, SIGNAL(editingFinished()), this, SLOT(zonesChanged()));
        }
        for(int i = 0; i < zones->columnCount(); i++)
            zones->resizeColumnToContents(i);
    }

    active = false;
}

void
CVPage::addZoneClicked()
{
    // no range selected
    if (!ranges->currentItem()) return;

    // are we at maximum already?
    if (zones->invisibleRootItem()->childCount() == 10) {
        QMessageBox err;
        err.setText(tr("Maximum of 10 zones reached."));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    active = true;
    int index = zones->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    QTimeEdit *loedit = new QTimeEdit(QTime::fromString("00:00", "mm:ss"), this);
    loedit->setMinimumTime(QTime::fromString("00:00", "mm:ss"));
    loedit->setMaximumTime(QTime::fromString("20:00", "mm:ss"));
    loedit->setDisplayFormat("mm:ss");

    zones->invisibleRootItem()->insertChild(index, add);
    zones->setItemWidget(add, 2, loedit);
    connect(loedit, SIGNAL(editingFinished()), this, SLOT(zonesChanged()));

    // Short
    QString text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);

    // long
    text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(1, text);
    active = false;

    zonesChanged();
}

void
CVPage::deleteZoneClicked()
{
    // no range selected
    if (ranges->invisibleRootItem()->indexOfChild(ranges->currentItem()) == -1)
        return;

    active = true;
    if (zones->currentItem()) {
        int index = zones->invisibleRootItem()->indexOfChild(zones->currentItem());
        delete zones->invisibleRootItem()->takeChild(index);
    }
    active = false;

    zonesChanged();
}

void
CVPage::zonesChanged()
{
    // only take changes when they are not done programmatically
    // the active flag is set when the tree is being modified
    // programmatically, but not when users interact with the widgets
    if (active == false) {
        // get the current zone range
        if (ranges->currentItem()) {

            int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
            PaceZoneRange current = paceZones->getZoneRange(index);

            // embolden that range on the list to show it has been edited
            QFont font;
            font.setWeight(QFont::Black);
            ranges->currentItem()->setFont(0, font);
            ranges->currentItem()->setFont(1, font);
            ranges->currentItem()->setFont(2, font);

            // show the default button to undo
            defaultButton->show();

            // we manually edited so save in full
            current.zonesSetFromCV = false;

            // create the new zoneinfos for this range
            QList<PaceZoneInfo> zoneinfos;
            for (int i=0; i< zones->invisibleRootItem()->childCount(); i++) {
                QTreeWidgetItem *item = zones->invisibleRootItem()->child(i);
                QTimeEdit *loTimeEdit = (QTimeEdit*)zones->itemWidget(item, 2);
                double kph = loTimeEdit->time() == QTime(0,0,0) ? 0.0 : paceZones->kphFromTime(loTimeEdit, metricPace);
                zoneinfos << PaceZoneInfo(item->text(0),
                                      item->text(1),
                                      kph,
                                      0);
            }

            // now sort the list
            std::sort(zoneinfos.begin(), zoneinfos.end());

            // now fill the highs
            for(int i=0; i<zoneinfos.count(); i++) {
                if (i+1 <zoneinfos.count())
                    zoneinfos[i].hi = zoneinfos[i+1].lo;
                else
                    zoneinfos[i].hi = INT_MAX;
            }
            current.zones = zoneinfos;

            // now replace the current range struct
            paceZones->setZoneRange(index, current);
        }
    }
}

//
// Season Editor
//
SeasonsPage::SeasonsPage(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    QGridLayout *mainLayout = new QGridLayout(this);
    QFormLayout *editLayout = new QFormLayout;
    editLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

    // get the list
    array = context->athlete->seasons->seasons;

    // Edit widgets
    nameEdit = new QLineEdit(this);
    typeEdit = new QComboBox(this);
    foreach(QString t, Season::types) typeEdit->addItem(t);
    typeEdit->setCurrentIndex(0);
    fromEdit = new QDateEdit(this);
    fromEdit->setCalendarPopup(true);

    toEdit = new QDateEdit(this);
    toEdit->setCalendarPopup(true);

    // set form
    editLayout->addRow(new QLabel(tr("Name")), nameEdit);
    editLayout->addRow(new QLabel(tr("Type")), typeEdit);
    editLayout->addRow(new QLabel(tr("From")), fromEdit);
    editLayout->addRow(new QLabel(tr("To")), toEdit);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    upButton = new QToolButton(this);
    downButton = new QToolButton(this);
    upButton->setArrowType(Qt::UpArrow);
    downButton->setArrowType(Qt::DownArrow);
    upButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    downButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    addButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
    upButton = new QPushButton(tr("Up"));
    downButton = new QPushButton(tr("Down"));
#endif

    QVBoxLayout *actionButtons = new QVBoxLayout;
    actionButtons->setSpacing(2 *dpiXFactor);
    actionButtons->addWidget(deleteButton);
    actionButtons->addWidget(upButton);
    actionButtons->addWidget(downButton);
    actionButtons->addStretch();

    seasons = new QTreeWidget;
    seasons->headerItem()->setText(0, tr("Name"));
    seasons->headerItem()->setText(1, tr("Type"));
    seasons->headerItem()->setText(2, tr("From"));
    seasons->headerItem()->setText(3, tr("To"));
    seasons->setColumnCount(4);
    seasons->setSelectionMode(QAbstractItemView::SingleSelection);
    //seasons->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    seasons->setUniformRowHeights(true);
    seasons->setIndentation(0);

#ifdef Q_OS_MAC
    //seasons->header()->resizeSection(0,160); // tab
    //seasons->header()->resizeSection(1,80); // name
    //seasons->header()->resizeSection(2,130); // type
    //seasons->header()->resizeSection(3,130); // values
#else
    //seasons->header()->resizeSection(0,160); // tab
    //seasons->header()->resizeSection(1,80); // name
    //seasons->header()->resizeSection(2,130); // type
    //seasons->header()->resizeSection(3,130); // values
#endif

    foreach(Season season, array) {
        QTreeWidgetItem *add;

        if (season.type == Season::temporary) continue;

        add = new QTreeWidgetItem(seasons->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // tab name
        add->setText(0, season.name);
        // type
        add->setText(1, Season::types[static_cast<int>(season.type)]);
        // from
        add->setText(2, season.getStart().toString(tr("ddd MMM d, yyyy")));
        // to
        add->setText(3, season.getEnd().toString(tr("ddd MMM d, yyyy")));
        // guid -- hidden
        add->setText(4, season.id().toString());

    }
    seasons->setCurrentItem(seasons->invisibleRootItem()->child(0));

    mainLayout->addLayout(editLayout, 0,0);
    mainLayout->addWidget(addButton, 0,1, Qt::AlignTop);
    mainLayout->addWidget(seasons, 1,0);
    mainLayout->addLayout(actionButtons, 1,1);

    // set all the edits to a default value
    clearEdit();

    // connect up slots
    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
}

void
SeasonsPage::clearEdit()
{
    typeEdit->setCurrentIndex(0);
    nameEdit->setText("");
    fromEdit->setDate(QDate::currentDate());
    toEdit->setDate(QDate::currentDate().addMonths(3));
}

void
SeasonsPage::upClicked()
{
    if (seasons->currentItem()) {
        int index = seasons->invisibleRootItem()->indexOfChild(seasons->currentItem());
        if (index == 0) return; // its at the top already

        // movin on up!
        QTreeWidgetItem* moved = seasons->invisibleRootItem()->takeChild(index);
        seasons->invisibleRootItem()->insertChild(index-1, moved);
        seasons->setCurrentItem(moved);

        // and move the array too
        array.move(index, index-1);
    }
}

void
SeasonsPage::downClicked()
{
    if (seasons->currentItem()) {
        int index = seasons->invisibleRootItem()->indexOfChild(seasons->currentItem());
        if (index == (seasons->invisibleRootItem()->childCount()-1)) return; // its at the bottom already

        QTreeWidgetItem* moved = seasons->invisibleRootItem()->takeChild(index);
        seasons->invisibleRootItem()->insertChild(index+1, moved);
        seasons->setCurrentItem(moved);

        array.move(index, index+1);
    }
}

void
SeasonsPage::renameClicked()
{
    // which one is selected?
    if (seasons->currentItem()) seasons->editItem(seasons->currentItem(), 0);
}

void
SeasonsPage::addClicked()
{
    if (nameEdit->text() == "") return; // just ignore it

    // swap if not right way around
    if (fromEdit->date() > toEdit->date()) {
        QDate temp = fromEdit->date();
        fromEdit->setDate(toEdit->date());
        toEdit->setDate(temp);
    }

    QTreeWidgetItem *add = new QTreeWidgetItem(seasons->invisibleRootItem());
    add->setFlags(add->flags() & ~Qt::ItemIsEditable);
    QString id;

    // tab name
    add->setText(0, nameEdit->text());
    // type
    add->setText(1, Season::types[typeEdit->currentIndex()]);
    // from
    add->setText(2, fromEdit->date().toString(tr("ddd MMM d, yyyy")));
    // to
    add->setText(3, toEdit->date().toString(tr("ddd MMM d, yyyy")));
    // guid -- hidden
    add->setText(4, (id=QUuid::createUuid().toString()));

    // now clear the edits
    clearEdit();

    Season addSeason;
    addSeason.setStart(fromEdit->date());
    addSeason.setEnd(toEdit->date());
    addSeason.setName(nameEdit->text());
    addSeason.setType(typeEdit->currentIndex());
    addSeason.setId(QUuid(id));
    array.append(Season());
}

void
SeasonsPage::deleteClicked()
{
    if (seasons->currentItem()) {
        int index = seasons->invisibleRootItem()->indexOfChild(seasons->currentItem());

        // zap!
        delete seasons->invisibleRootItem()->takeChild(index);

        array.removeAt(index);
    }
}

qint32
SeasonsPage::saveClicked()
{
    // get any edits to the names and dates
    // since we don't trap these as they are made
    for(int i=0; i<seasons->invisibleRootItem()->childCount(); i++) {

        QTreeWidgetItem *item = seasons->invisibleRootItem()->child(i);

        array[i].setName(item->text(0));
        array[i].setType(Season::types.indexOf(item->text(1)));
        array[i].setStart(QDate::fromString(item->text(2), "ddd MMM d, yyyy"));
        array[i].setEnd(QDate::fromString(item->text(3), "ddd MMM d, yyyy"));
        array[i].setId(QUuid(item->text(4)));
    }

    // write to disk
    QString file = QString(context->athlete->home->config().canonicalPath() + "/seasons.xml");
    SeasonParser::serialize(file, array);

    // re-read
    context->athlete->seasons->readSeasons();

    return 0;
}


AutoImportPage::AutoImportPage(Context *context) : context(context)
{
    QGridLayout *mainLayout = new QGridLayout(this);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
    browseButton = new QPushButton(tr("Browse"));
#ifndef Q_OS_MAC
    upButton = new QToolButton(this);
    downButton = new QToolButton(this);
    upButton->setArrowType(Qt::UpArrow);
    downButton->setArrowType(Qt::DownArrow);
    upButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    downButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    addButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
    upButton = new QPushButton(tr("Up"));
    downButton = new QPushButton(tr("Down"));
#endif
    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2 *dpiXFactor);
    actionButtons->addWidget(upButton);
    actionButtons->addWidget(downButton);
    actionButtons->addStretch();
    actionButtons->addWidget(browseButton);
    actionButtons->addStretch();
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);

    fields = new QTreeWidget;
    fields->headerItem()->setText(0, tr("Directory"));
    fields->headerItem()->setText(1, tr("Import Rule"));
    fields->setColumnWidth(0,400 *dpiXFactor);
    fields->setColumnWidth(1,100 *dpiXFactor);
    fields->setColumnCount(2);
    fields->setSelectionMode(QAbstractItemView::SingleSelection);
    //fields->setUniformRowHeights(true);
    fields->setIndentation(0);

    fields->setCurrentItem(fields->invisibleRootItem()->child(0));

    mainLayout->addWidget(fields, 0,0);
    mainLayout->addLayout(actionButtons, 1,0);

    context->athlete->autoImportConfig->readConfig();
    QList<RideAutoImportRule> rules = context->athlete->autoImportConfig->getConfig();
    int index = 0;
    foreach (RideAutoImportRule rule, rules) {
        QComboBox *comboButton = new QComboBox(this);
        addRuleTypes(comboButton);
        QTreeWidgetItem *add = new QTreeWidgetItem;
        fields->invisibleRootItem()->insertChild(index, add);
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        add->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
        add->setText(0, rule.getDirectory());

        add->setTextAlignment(1, Qt::AlignHCenter | Qt::AlignVCenter);
        comboButton->setCurrentIndex(rule.getImportRule());
        fields->setItemWidget(add, 1, comboButton);
        index++;
    }

    // connect up slots
    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(browseButton, SIGNAL(clicked()), this, SLOT(browseImportDir()));
}

void
AutoImportPage::upClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
        if (index == 0) return; // its at the top already

        //movin on up!
        QWidget *button = fields->itemWidget(fields->currentItem(),1);
        QComboBox *comboButton = new QComboBox(this);
        addRuleTypes(comboButton);
        comboButton->setCurrentIndex(((QComboBox*)button)->currentIndex());
        QTreeWidgetItem* moved = fields->invisibleRootItem()->takeChild(index);
        fields->invisibleRootItem()->insertChild(index-1, moved);
        fields->setItemWidget(moved, 1, comboButton);
        fields->setCurrentItem(moved);
    }
}

void
AutoImportPage::downClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
        if (index == (fields->invisibleRootItem()->childCount()-1)) return; // its at the bottom already

        QWidget *button = fields->itemWidget(fields->currentItem(),1);
        QComboBox *comboButton = new QComboBox(this);
        addRuleTypes(comboButton);
        comboButton->setCurrentIndex(((QComboBox*)button)->currentIndex());
        QTreeWidgetItem* moved = fields->invisibleRootItem()->takeChild(index);
        fields->invisibleRootItem()->insertChild(index+1, moved);
        fields->setItemWidget(moved, 1, comboButton);
        fields->setCurrentItem(moved);
    }
}


void
AutoImportPage::addClicked()
{

    int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
    if (index < 0) index = 0;

    QComboBox *comboButton = new QComboBox(this);
    addRuleTypes(comboButton);

    QTreeWidgetItem *add = new QTreeWidgetItem;
    fields->invisibleRootItem()->insertChild(index, add);
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    add->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
    add->setText(0, tr("Enter directory or press [Browse] to select"));
    add->setTextAlignment(1, Qt::AlignHCenter | Qt::AlignVCenter);
    fields->setItemWidget(add, 1, comboButton);

}

void
AutoImportPage::deleteClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());

        // zap!
        delete fields->invisibleRootItem()->takeChild(index);
    }
}

qint32
AutoImportPage::saveClicked()
{

    rules.clear();
    for(int i=0; i<fields->invisibleRootItem()->childCount(); i++) {

        RideAutoImportRule rule;
        rule.setDirectory(fields->invisibleRootItem()->child(i)->text(0));

        QWidget *button = fields->itemWidget(fields->invisibleRootItem()->child(i),1);
        rule.setImportRule(((QComboBox*)button)->currentIndex());
        rules.append(rule);

    }

    // write to disk
    QString file = QString(context->athlete->home->config().canonicalPath() + "/autoimport.xml");
    RideAutoImportConfigParser::serialize(file, rules);

    // re-read
    context->athlete->autoImportConfig->readConfig();
    return 0;
}

void
AutoImportPage::addRuleTypes(QComboBox *p) {

    RideAutoImportRule config;
    QList<QString> descriptions = config.getRuleDescriptions();

    foreach(QString description, descriptions) {
           p->addItem(description);
    }
}

void
AutoImportPage::browseImportDir()
{
    QStringList selectedDirs;
    if (fields->currentItem()) {
        QFileDialog fileDialog(this);
        fileDialog.setFileMode(QFileDialog::Directory);
        fileDialog.setOptions(QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (fileDialog.exec()) {
            selectedDirs = fileDialog.selectedFiles();
        }
        if (selectedDirs.count() > 0) {
            QString dir = selectedDirs.at(0);
            if (dir != "") {
                fields->currentItem()->setText(0, dir);
            }
        }
    }
}
