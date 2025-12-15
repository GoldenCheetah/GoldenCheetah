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
#include "Seasons.h"
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
#include "AthleteBackup.h"


//
// Passwords page
//
CredentialsPage::CredentialsPage(Context *context) : context(context)
{
    setBackgroundRole(QPalette::Base);
    QGridLayout *mainLayout = new QGridLayout(this);

    accounts = new QTreeWidget;
    accounts->headerItem()->setText(0, tr("Service"));
    accounts->headerItem()->setText(1, tr("Description"));
    accounts->setColumnCount(2);
    accounts->setColumnWidth(0, 175 * dpiXFactor);
    accounts->setSelectionMode(QAbstractItemView::SingleSelection);
    //fields->setUniformRowHeights(true);
    accounts->setIndentation(0);
    accounts->setAlternatingRowColors(true);

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::EditGroup | ActionButtonBox::AddDeleteGroup);

    mainLayout->addWidget(accounts, 0,0);
    mainLayout->addWidget(actionButtons, 1,0);

    // list accounts...
    resetList();

    // connect up slots
    actionButtons->defaultConnect(accounts);
    connect(actionButtons, &ActionButtonBox::addRequested, this, &CredentialsPage::addClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &CredentialsPage::deleteClicked);
    connect(actionButtons, &ActionButtonBox::editRequested, this, &CredentialsPage::editClicked);
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
// Wheel Size Calculator
//

WheelSizeCalculator::WheelSizeCalculator
(QWidget *parent)
: QDialog(parent)
{
    setWindowTitle(tr("Wheel Size Calculator"));

    rimSizeCombo = new QComboBox();
    rimSizeCombo->addItems(WheelSize::RIM_SIZES);

    tireSizeCombo = new QComboBox();
    tireSizeCombo->addItems(WheelSize::TIRE_SIZES);

    wheelSizeLabel = new QLabel("---");
    QFont font(wheelSizeLabel->font());
    font.setWeight(QFont::Black);
    wheelSizeLabel->setFont(font);
    wheelSizeLabel->setEnabled(false);

    buttonBox = new QDialogButtonBox(  QDialogButtonBox::Apply
                                     | QDialogButtonBox::Discard);
    buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

    QFormLayout *form = newQFormLayout();
    form->addRow(tr("Rim Size"), rimSizeCombo);
    form->addRow(tr("Tire Size"), tireSizeCombo);
    form->addRow(tr("Wheel Size"), wheelSizeLabel);
    form->addItem(new QSpacerItem(1, 10 * dpiYFactor));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(form);
    mainLayout->addWidget(buttonBox);

    connect(rimSizeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(calc()));
    connect(tireSizeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(calc()));
    connect(buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttonBox->button(QDialogButtonBox::Discard), SIGNAL(clicked()), this, SLOT(reject()));
}

void
WheelSizeCalculator::calc
()
{
    if (rimSizeCombo->currentIndex() > 0 && tireSizeCombo->currentIndex() > 0) {
        wheelSize = WheelSize::calcPerimeter(rimSizeCombo->currentIndex(), tireSizeCombo->currentIndex());
        wheelSizeLabel->setText(QString::number(wheelSize) + " " + tr("mm"));
        wheelSizeLabel->setEnabled(true);
        buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
    } else {
        wheelSizeLabel->setText("---");
        buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
        wheelSizeLabel->setEnabled(false);
    }
}

int
WheelSizeCalculator::getWheelSize
() const
{
    return wheelSize;
}


//
// About me
//
AboutRiderPage::AboutRiderPage(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    metricUnits = GlobalContext::context()->useMetricUnits;

    nickname = new QLineEdit(this);
    nickname->setText(appsettings->cvalue(context->athlete->cyclist, GC_NICKNAME, "").toString());
    if (nickname->text() == "0") nickname->setText("");

    QLocale locale;

    dob = new QDateEdit(this);
    dob->setDate(appsettings->cvalue(context->athlete->cyclist, GC_DOB).toDate());
    dob->setCalendarPopup(true);
    dob->setDisplayFormat(locale.dateFormat(QLocale::LongFormat));

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
    int newHeight = std::min(avatar.width(), avatar.height());
    avatar = avatar.copy(0, 0, newHeight, newHeight);

    avatarButton = new QPushButton(this);
    avatarButton->setContentsMargins(0,0,0,0);
    avatarButton->setFlat(true);
    avatarButton->setIcon(avatar.scaledToHeight(140, Qt::SmoothTransformation));
    avatarButton->setIconSize(QSize(140,140));
    avatarButton->setFixedHeight(140);
    avatarButton->setFixedWidth(140);
    QRegion region(0, 0, avatarButton->width() - 1, avatarButton->height() - 1, QRegion::Ellipse);
    avatarButton->setMask(region);

    //
    // Crank length - only used by PfPv chart (should move there!)
    //
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
    int wheelSize = appsettings->cvalue(context->athlete->cyclist, GC_WHEELSIZE, 2100).toInt();

    wheelSizeEdit = new QSpinBox();
    wheelSizeEdit->setRange(1, 9999);
    wheelSizeEdit->setSingleStep(1);
    wheelSizeEdit->setSuffix(" " + tr("mm"));
    wheelSizeEdit->setValue(wheelSize);

    wheelSizeCalculatorButton = new QPushButton(tr("Calculate..."));
    std::u32string calculatorGlyph = U"🖩";
    if (wheelSizeCalculatorButton->fontMetrics().inFontUcs4(calculatorGlyph[0])) {
        wheelSizeCalculatorButton->setText(QString::fromUcs4(calculatorGlyph.c_str()));
    }

    QFormLayout *form = newQFormLayout();

    QHBoxLayout *wheelSizeLayout = new QHBoxLayout();
    wheelSizeLayout->addWidget(wheelSizeEdit, form->fieldGrowthPolicy() == QFormLayout::AllNonFixedFieldsGrow ? 1 : 0);
    wheelSizeLayout->addWidget(wheelSizeCalculatorButton, 0);

    form->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    form->addRow(tr("Nickname"), nickname);
    form->addRow(tr("Date of Birth"), dob);
    form->addRow(tr("Sex"), sex);
    form->addRow(tr("Height"), height);
    form->addRow(tr("Weight"), weight);
    form->addItem(new QSpacerItem(1, 15 * dpiYFactor));
    form->addRow("", new QLabel(tr("Bike")));
    form->addRow(tr("Crank Length"), crankLengthCombo);
    form->addRow(tr("Wheelsize"), wheelSizeLayout);

    QVBoxLayout *avatarLayout = new QVBoxLayout();
    avatarLayout->addWidget(avatarButton);
    avatarLayout->addStretch();

    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->addLayout(avatarLayout);
    mainLayout->addSpacing(20 * dpiXFactor);
    mainLayout->addLayout(centerLayout(form));

    QLabel *athleteLabel = new QLabel(context->athlete->cyclist);
    athleteLabel->setAlignment(Qt::AlignCenter);
    QFont athleteFont = athleteLabel->font();
    athleteFont.setBold(true);
    athleteFont.setPointSize(athleteFont.pointSize() * 1.3);
    athleteLabel->setFont(athleteFont);

    QVBoxLayout *mainMainLayout = new QVBoxLayout(this);
    mainLayout->addSpacing(10 * dpiYFactor);
    mainMainLayout->addWidget(athleteLabel);
    mainMainLayout->addSpacing(30 * dpiXFactor);
    mainMainLayout->addLayout(mainLayout);

    // save initial values for things we care about
    // note we don't worry about age or sex at this point
    // since they are not used, nor the W'bal tau used in
    // the realtime code. This is limited to stuff we
    // care about tracking as it is used by metrics
    b4.weight = appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT).toDouble();
    b4.height = appsettings->cvalue(context->athlete->cyclist, GC_HEIGHT).toDouble();
    b4.wheel = wheelSize;
    b4.crank = crankLengthCombo->currentIndex();

    connect(wheelSizeCalculatorButton, SIGNAL(clicked()), this, SLOT(openWheelSizeCalculator()));
    connect(avatarButton, SIGNAL(clicked()), this, SLOT(chooseAvatar()));
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
        int s = std::min(avatar.width(), avatar.height());
        avatar = avatar.copy((avatar.width() - s) / 2, (avatar.height() - s) / 2, s, s);
        avatarButton->setIcon(avatar.scaledToHeight(140, Qt::SmoothTransformation));
        avatarButton->setIconSize(QSize(140,140));
    }
}

void
AboutRiderPage::openWheelSizeCalculator
()
{
    WheelSizeCalculator wsDialog(this);

    int dialogRet = wsDialog.exec();
    if (dialogRet == QDialog::Accepted) {
        wheelSizeEdit->setValue(wsDialog.getWheelSize());
    }
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
    appsettings->setCValue(context->athlete->cyclist, GC_WHEELSIZE, wheelSizeEdit->value());

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
    if (b4.wheel != wheelSizeEdit->value() ||
        b4.crank != crankLengthCombo->currentIndex() )
        state += CONFIG_GENERAL;

    return state;
}

AboutModelPage::AboutModelPage(Context *context) : context(context)
{
    //
    // W'bal Tau
    //
    wbaltau = new QSpinBox(this);
    wbaltau->setMinimum(30);
    wbaltau->setMaximum(1200);
    wbaltau->setSingleStep(10);
    wbaltau->setSuffix(" " + tr("s"));
    wbaltau->setValue(appsettings->cvalue(context->athlete->cyclist, GC_WBALTAU, defaultWbaltau).toInt());

    //
    // Performance manager
    //

    // get config or set to defaults
    int perfManSTSVal = appsettings->cvalue(context->athlete->cyclist, GC_STS_DAYS, defaultSTSavg).toInt();
    int perfManLTSVal = appsettings->cvalue(context->athlete->cyclist, GC_LTS_DAYS, defaultLTSavg).toInt();

    perfManSTSavg = new QSpinBox();
    perfManSTSavg->setRange(1, 21);
    perfManSTSavg->setSuffix(" " + tr("days"));
    perfManSTSavg->setValue(perfManSTSVal);

    perfManLTSavg = new QSpinBox();
    perfManLTSavg->setRange(7, 56);
    perfManLTSavg->setSuffix(" " + tr("days"));
    perfManLTSavg->setValue(perfManLTSVal);

    showSBToday = new QCheckBox(tr("PMC Stress Balance Today"), this);
    showSBToday->setChecked(appsettings->cvalue(context->athlete->cyclist, GC_SB_TODAY, defaultSBToday).toInt());

    resetButton = new QPushButton(tr("Restore Defaults"));

    QFormLayout *form = newQFormLayout(this);

    form->addRow(tr("W'bal tau"), wbaltau);
    form->addRow(tr("Short Term Stress (STS) average"), perfManSTSavg);
    form->addRow(tr("Long Term Stress (LTS) average"), perfManLTSavg);
    form->addRow("", showSBToday);
    form->addItem(new QSpacerItem(1, 15 * dpiYFactor));
    form->addRow("", resetButton);

    // save initial values for things we care about
    // note we don't worry about age or sex at this point
    // since they are not used, nor the W'bal tau used in
    // the realtime code. This is limited to stuff we
    // care about tracking as it is used by metrics
    b4.lts = perfManLTSVal;
    b4.sts = perfManSTSVal;

    connect(resetButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
    connect(wbaltau, SIGNAL(valueChanged(int)), this, SLOT(anyChanged()));
    connect(perfManSTSavg, SIGNAL(valueChanged(int)), this, SLOT(anyChanged()));
    connect(perfManLTSavg, SIGNAL(valueChanged(int)), this, SLOT(anyChanged()));
    connect(showSBToday, SIGNAL(toggled(bool)), this, SLOT(anyChanged()));

    anyChanged();
}

qint32
AboutModelPage::saveClicked()
{
    // W'bal Tau
    appsettings->setCValue(context->athlete->cyclist, GC_WBALTAU, wbaltau->value());

    // Performance Manager
    appsettings->setCValue(context->athlete->cyclist, GC_STS_DAYS, perfManSTSavg->value());
    appsettings->setCValue(context->athlete->cyclist, GC_LTS_DAYS, perfManLTSavg->value());
    appsettings->setCValue(context->athlete->cyclist, GC_SB_TODAY, (int) showSBToday->isChecked());

    qint32 state=0;

    // PMC constants changed ?
    if(b4.lts != perfManLTSavg->value() || b4.sts != perfManSTSavg->value())
        state += CONFIG_PMC;

    return state;
}


void
AboutModelPage::restoreDefaults
()
{
    wbaltau->setValue(defaultWbaltau);
    perfManSTSavg->setValue(defaultSTSavg);
    perfManLTSavg->setValue(defaultLTSavg);
    showSBToday->setChecked(defaultSBToday);
}


void
AboutModelPage::anyChanged
()
{
    resetButton->setEnabled(   wbaltau->value() != defaultWbaltau
                            || perfManSTSavg->value() != defaultSTSavg
                            || perfManLTSavg->value() != defaultLTSavg
                            || showSBToday->isChecked() != defaultSBToday);
}


BackupPage::BackupPage
(Context *context)
: context(context)
{
    //
    // Auto Backup
    //
    // Selecting the storage folder folder of the Local File Store
    autoBackupFolder = new DirectoryPathWidget();
    autoBackupFolder->setPlaceholderText(tr("Enter a directory"));
    autoBackupFolder->setPath(appsettings->cvalue(context->athlete->cyclist, GC_AUTOBACKUP_FOLDER, "").toString());

    autoBackupPeriod = new QSpinBox();
    autoBackupPeriod->setMinimum(0);
    autoBackupPeriod->setMaximum(9999);
    autoBackupPeriod->setSingleStep(1);
    autoBackupPeriod->setValue(appsettings->cvalue(context->athlete->cyclist, GC_AUTOBACKUP_PERIOD, 0).toInt());
    autoBackupPeriod->setPrefix(tr("every") + " ");
    autoBackupPeriod->setSuffix(" " + tr("times"));
    autoBackupPeriod->setSpecialValueText(tr("never"));

    QPushButton *backupNow = new QPushButton(tr("Backup now"));

    QFormLayout *form = newQFormLayout(this);
    form->addRow(tr("Auto Backup Folder"), autoBackupFolder);
    form->addRow(tr("Auto Backup after closing the athlete"), autoBackupPeriod);
    form->addItem(new QSpacerItem(1, 15 * dpiYFactor));
    form->addRow("", backupNow);

    connect(backupNow, SIGNAL(clicked()), this, SLOT(backupNow()));
}

void
BackupPage::backupNow
()
{
    AthleteBackup backup(context->athlete->home->root());
    backup.backupImmediate();
}

qint32
BackupPage::saveClicked()
{
    // Auto Backup
    appsettings->setCValue(context->athlete->cyclist, GC_AUTOBACKUP_FOLDER, autoBackupFolder->getPath());
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

    QString dateTimetext = tr("From Date - Time");
    QStringList fieldNames = measuresGroup->getFieldNames();

    QString commenttext = tr("Comment");

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);

    // Measures
    measuresTree = new QTreeWidget;
    measuresTree->headerItem()->setText(0, dateTimetext);

    measuresTree->setItemDelegateForColumn(0, &dateTimeDelegate);
    int k = 0;
    for (const QString &fieldName : fieldNames) {
        measuresTree->headerItem()->setText(k + 1, fieldName);
        DoubleSpinBoxEditDelegate *ed = new DoubleSpinBoxEditDelegate;
        ed->setRange(0.0, 9999.99);
        ed->setDecimals(2);
        ed->setSuffix(measuresGroup->getFieldUnits(k, metricUnits));
        valueDelegates << ed;
        measuresTree->setItemDelegateForColumn(k + 1, ed);
        k++;
    }
    measuresTree->setItemDelegateForColumn(k + 2, &sourceDelegate);
    measuresTree->setItemDelegateForColumn(k + 3, &origSourceDelegate);

    measuresTree->headerItem()->setText(k+1, commenttext);
    measuresTree->headerItem()->setText(k+2, tr("Source"));
    measuresTree->headerItem()->setText(k+3, tr("Original Source"));
    measuresTree->setColumnCount(k+4);
    basicTreeWidgetStyle(measuresTree);

    // get a copy of group measures if the file exists
    measures = QList<Measure>(measuresGroup->measures());
    std::reverse(measures.begin(), measures.end());

    // setup measuresTree
    for (int i = 0; i < measures.count(); i++) {
        QTreeWidgetItem *add = new QTreeWidgetItem(measuresTree->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);
        fillItemFromMeasures(i, add);
    }

    QVBoxLayout *all = new QVBoxLayout(this);
    all->addWidget(measuresTree);
    all->addWidget(actionButtons);

    actionButtons->defaultConnect(measuresTree);
    connect(actionButtons, &ActionButtonBox::addRequested, this, &MeasuresPage::addClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &MeasuresPage::deleteClicked);
    connect(measuresTree->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(rangeChanged(const QModelIndex&)));

    // save initial values for things we care about
    b4.fingerprint = 0;
    foreach (Measure measure, measures) {
        b4.fingerprint += measure.getFingerprint();
    }
}


MeasuresPage::~MeasuresPage
()
{
    for (DoubleSpinBoxEditDelegate *ed : valueDelegates) {
        delete ed;
    }
    valueDelegates.clear();
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
MeasuresPage::addClicked()
{
    QDialog dialog;

    QDialogButtonBox *buttonBox = new QDialogButtonBox(  QDialogButtonBox::Apply
                                                       | QDialogButtonBox::Discard);
    connect(buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), &dialog, SLOT(accept()));
    connect(buttonBox->button(QDialogButtonBox::Discard), SIGNAL(clicked()), &dialog, SLOT(reject()));

    QFormLayout *form = newQFormLayout(&dialog);

    QDateTime now = QDateTime::currentDateTime();
    QTime nowT = now.time();
    nowT.setHMS(nowT.hour(), nowT.minute(), nowT.second(), 0);
    now.setTime(nowT);
    QDateTimeEdit *dtEdit = new QDateTimeEdit(now);
    dtEdit->setCalendarPopup(true);
    form->addRow(tr("Start Date"), dtEdit);
    QList<double> unitsFactors = measuresGroup->getFieldUnitsFactors();

    QStringList fieldNames = measuresGroup->getFieldNames();
    QList<QLabel*> valuesLabel;
    QList<QDoubleSpinBox*> valuesEdit;
    int k = 0;
    for (QString &fieldName : fieldNames) {
        const double unitsFactor = (metricUnits ? 1.0 : unitsFactors[k]);
        valuesLabel << new QLabel(fieldName);
        valuesEdit << new QDoubleSpinBox(this);
        valuesEdit[k]->setMaximum(9999.99);
        valuesEdit[k]->setMinimum(0.0);
        valuesEdit[k]->setDecimals(2);
        if (measures.count() > 0) {
            valuesEdit[k]->setValue(measures[0].values[k] * unitsFactor);
        } else {
            valuesEdit[k]->setValue(0.0);
        }
        valuesEdit[k]->setSuffix(QString(" %1").arg(measuresGroup->getFieldUnits(k, metricUnits)));

        form->addRow(valuesLabel[k], valuesEdit[k]);

        k++;
    }

    QLineEdit *commentEdit = new QLineEdit();
    form->addRow(tr("Comment"), commentEdit);

    form->addRow(buttonBox);

    int dialogRet = dialog.exec();
    if (dialogRet != QDialog::Accepted) {
        return;
    }

    int i = 0;
    int rnum = 0;
    while (i < measures.count() && measures[i].when != dtEdit->dateTime()) {
        if (measures[i].when > dtEdit->dateTime()) {
            ++rnum;
        }
        ++i;
    }
    QTreeWidgetItem *add;
    if (i == measures.count()) {
        add = new QTreeWidgetItem();
        add->setFlags(add->flags() | Qt::ItemIsEditable);
        Measure measure;
        measure.source = Measure::Manual;
        measures.insert(rnum, measure);
        measuresTree->invisibleRootItem()->insertChild(rnum, add);
    } else {
        rnum = i;
        add = measuresTree->invisibleRootItem()->child(rnum);
    }
    measures[rnum].when = dtEdit->dateTime();
    for (k = 0; k < valuesEdit.count(); ++k) {
        const double unitsFactor = (metricUnits ? 1.0 : unitsFactors[k]);
        measures[rnum].values[k] = valuesEdit[k]->value() / unitsFactor;
    }
    measures[rnum].comment = commentEdit->text();
    fillItemFromMeasures(rnum, add);
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
MeasuresPage::rangeChanged(const QModelIndex &topLeft)
{
    // Ignore first and last two columns
    if (   topLeft.column() == 0
        || topLeft.column() >= topLeft.model()->columnCount() - 2) {
        return;
    }
    QDateTime dt = topLeft.siblingAtColumn(0).data(Qt::UserRole).toDateTime();
    int col = topLeft.column();
    int rnum = 0;
    while (rnum < measures.count() && measures[rnum].when != dt) {
        ++rnum;
    }
    // No matching measure found
    if (rnum == measures.count()) {
        return;
    }
    QList<double> unitsFactors = measuresGroup->getFieldUnitsFactors();
    if (col > 0 && col <= valueDelegates.count()) {
        const double unitsFactor = (metricUnits ? 1.0 : unitsFactors[col - 1]);
        measures[rnum].values[col - 1] = topLeft.data(Qt::DisplayRole).toDouble() / unitsFactor;
    } else if (col == valueDelegates.count() + 1) {
        measures[rnum].comment = topLeft.data(Qt::DisplayRole).toString();
    }
    // Set measures source to manual
    if (measures[rnum].source != Measure::Manual) {
        measures[rnum].source = Measure::Manual;
        QModelIndex sourceIndex = topLeft.siblingAtColumn(topLeft.model()->columnCount() - 2);
        measuresTree->model()->setData(sourceIndex, measures[rnum].getSourceDescription(), Qt::DisplayRole);
    }
}


void
MeasuresPage::fillItemFromMeasures
(int rnum, QTreeWidgetItem *item) const
{
    item->setData(0, Qt::DisplayRole, measures[rnum].when.toString(tr("MMM d, yyyy - hh:mm:ss")));
    item->setData(0, Qt::UserRole, measures[rnum].when);
    int k;
    QList<double> unitsFactors = measuresGroup->getFieldUnitsFactors();
    for (k = 0; k < unitsFactors.count(); ++k) {
        const double unitsFactor = (metricUnits ? 1.0 : unitsFactors[k]);
        item->setData(k + 1, Qt::DisplayRole, measures[rnum].values[k] * unitsFactor);
    }
    item->setData(k + 1, Qt::DisplayRole, measures[rnum].comment);
    item->setData(k + 2, Qt::DisplayRole, measures[rnum].getSourceDescription());
    item->setData(k + 3, Qt::DisplayRole, measures[rnum].originalSource);
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

        // which model to use?
        appsettings->setCValue(context->athlete->cyclist, zones[i]->useCPModelSetting(), cpPage[i]->useModel->currentIndex());

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

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);
    actionButtons->setMaxViewItems(10);

    zoneFromDelegate.setRange(0, 1000);
    zoneFromDelegate.setSingleStep(1);
    zoneFromDelegate.setSuffix(tr("%"));
    zoneFromDelegate.setShowSuffixOnEdit(true);
    zoneFromDelegate.setShowSuffixOnDisplay(true);

    scheme = new QTreeWidget;
    scheme->setAlternatingRowColors(true);
    scheme->headerItem()->setText(0, tr("Short"));
    scheme->headerItem()->setText(1, tr("Long"));
    scheme->headerItem()->setText(2, tr("Percent of CP"));
    scheme->setColumnCount(3);
    scheme->setItemDelegateForColumn(2, &zoneFromDelegate);
    basicTreeWidgetStyle(scheme);

    // setup list
    for (int i = 0; i < zones->getScheme().nzones_default; i++) {
        QTreeWidgetItem *add = new QTreeWidgetItem(scheme->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        add->setData(0, Qt::DisplayRole, zones->getScheme().zone_default_name[i]);
        add->setData(1, Qt::DisplayRole, zones->getScheme().zone_default_desc[i]);
        add->setData(2, Qt::DisplayRole, zones->getScheme().zone_default[i]);
    }

    mainLayout->addWidget(scheme);
    mainLayout->addWidget(actionButtons);

    // button connect
    actionButtons->defaultConnect(scheme);
    connect(actionButtons, &ActionButtonBox::addRequested, this, &SchemePage::addClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &SchemePage::deleteClicked);
}


void
SchemePage::addClicked()
{
    int index = scheme->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    scheme->invisibleRootItem()->insertChild(index, add);

    // Short
    QString text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setData(0, Qt::DisplayRole, text);

    // long
    text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setData(1, Qt::DisplayRole, text);
    add->setData(2, Qt::DisplayRole, 100);
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
        add.name = scheme->invisibleRootItem()->child(i)->data(0, Qt::DisplayRole).toString();
        add.desc = scheme->invisibleRootItem()->child(i)->data(1, Qt::DisplayRole).toString();
        add.lo = scheme->invisibleRootItem()->child(i)->data(2, Qt::DisplayRole).toInt();
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



#define CPPAGE_DEFAULT_CP 250
#define CPPAGE_DEFAULT_FACTOR_AETP 0.85
#define CPPAGE_DEFAULT_WPRIME 20000
#define CPPAGE_DEFAULT_PMAX 1000

#define CPPAGE_RANGES_COL_RNUM 0
#define CPPAGE_RANGES_COL_STARTDATE 1
#define CPPAGE_RANGES_COL_CP 2
#define CPPAGE_RANGES_COL_AETP 3
#define CPPAGE_RANGES_COL_FTP 4
#define CPPAGE_RANGES_COL_WPRIME 5
#define CPPAGE_RANGES_COL_PMAX 6
#define CPPAGE_RANGES_COL_MODELFIT 7
#define CPPAGE_RANGES_COL_EST_DEVIATION 8
#define CPPAGE_RANGES_COL_EST_OFFSET 9
#define CPPAGE_RANGES_COL_EST_CP 10
#define CPPAGE_RANGES_COL_EST_FTP 11
#define CPPAGE_RANGES_COL_EST_WPRIME 12
#define CPPAGE_RANGES_COL_EST_PMAX 13
#define CPPAGE_RANGES_COUNT_COL 14

#define CPPAGE_RANGES_EST_MATCH 0
#define CPPAGE_RANGES_EST_DEVIATE 1
#define CPPAGE_RANGES_EST_NA 2

#define CPPAGE_EST_MODEL_NONE 0
#define CPPAGE_EST_MODEL_CP2 1
#define CPPAGE_EST_MODEL_CP3 2
#define CPPAGE_EST_MODEL_EXT 3

#define CPPAGE_EST_TOLERANCE_CP 5
#define CPPAGE_EST_TOLERANCE_WPRIME 1000
#define CPPAGE_EST_TOLERANCE_PMAX 10

#define CPPAGE_INFO_AETP 0
#define CPPAGE_INFO_FTP_CP 1
#define CPPAGE_INFO_MODEL_FTP 2
#define CPPAGE_INFO_MODEL_PMAX 3
#define CPPAGE_INFO_MODEL_CP2 4
#define CPPAGE_INFO_MODEL_CP3 5
#define CPPAGE_LABEL_STARTDATE 6
#define CPPAGE_LABEL_CP 7
#define CPPAGE_LABEL_AETP 8
#define CPPAGE_LABEL_FTP 9
#define CPPAGE_LABEL_WPRIME 10
#define CPPAGE_LABEL_PMAX 11

CPPage::CPPage(Context *context, Zones *zones_, SchemePage *schemePage) :
               context(context), zones_(zones_), schemePage(schemePage)
{
    active = false;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10 *dpiXFactor);

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);
    reviewButton = actionButtons->addButton(tr("Review..."), ActionButtonBox::Right);
    reviewButton->setVisible(false);
    newZoneRequired = actionButtons->addButton(tr("Changed power estimates are available"), ActionButtonBox::Left);
    newZoneRequired->setFlat(true);
    newZoneRequired->setVisible(false);

    ActionButtonBox *zoneActionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);
    zoneActionButtons->setMaxViewItems(10);
    defaultButton = zoneActionButtons->addButton(tr("Def"), ActionButtonBox::Right);
    defaultButton->hide();

    useModel = new QComboBox(this);
    useModel->addItem(tr("Manual"), "manual");
    useModel->addItem(tr("Semi-Automatic (CP2)"), "cp2");
    useModel->addItem(tr("Semi-Automatic (CP3)"), "cp3");
    useModel->addItem(tr("Semi-Automatic (Extended)"), "ext");

     // Use CP for FTP
    useCPForFTPCombo = new QComboBox(this);
    useCPForFTPCombo->addItem(tr("Use CP for all metrics"));
    useCPForFTPCombo->addItem(tr("Use FTP for Coggan metrics"));

    b4.modelIdx = appsettings->cvalue(context->athlete->cyclist, zones_->useCPModelSetting(), CPPAGE_EST_MODEL_NONE).toInt();
    useModel->setCurrentIndex(b4.modelIdx);

    b4.cpforftp = appsettings->cvalue(context->athlete->cyclist, zones_->useCPforFTPSetting(), 0).toInt() ? 1 : 0;
    useCPForFTPCombo->setCurrentIndex(b4.cpforftp);

    QHBoxLayout *addLayout = new QHBoxLayout;
    addLayout->addStretch();
    addLayout->addWidget(useModel);
    addLayout->addWidget(useCPForFTPCombo);

    dateDelegate.setCalendarPopup(true);

    cpDelegate.setRange(50, 500);
    cpDelegate.setSingleStep(5);
    cpDelegate.setSuffix(tr("W"));
    cpDelegate.setShowSuffixOnEdit(true);
    cpDelegate.setShowSuffixOnDisplay(true);

    aetDelegate.setRange(50, 500);
    aetDelegate.setSingleStep(5);
    aetDelegate.setSuffix(tr("W"));
    aetDelegate.setShowSuffixOnEdit(true);
    aetDelegate.setShowSuffixOnDisplay(true);

    ftpDelegate.setRange(50, 500);
    ftpDelegate.setSingleStep(5);
    ftpDelegate.setSuffix(tr("W"));
    ftpDelegate.setShowSuffixOnEdit(true);
    ftpDelegate.setShowSuffixOnDisplay(true);

    wDelegate.setRange(1, 100000);
    wDelegate.setSingleStep(100);
    wDelegate.setSuffix(tr("J"));
    wDelegate.setShowSuffixOnEdit(true);
    wDelegate.setShowSuffixOnDisplay(true);

    pmaxDelegate.setRange(100, 3000);
    pmaxDelegate.setSingleStep(10);
    pmaxDelegate.setSuffix(tr("W"));
    pmaxDelegate.setShowSuffixOnEdit(true);
    pmaxDelegate.setShowSuffixOnDisplay(true);

    ranges = new QTreeWidget();
    ranges->headerItem()->setText(CPPAGE_RANGES_COL_RNUM, "_rnum");
    ranges->headerItem()->setText(CPPAGE_RANGES_COL_STARTDATE, tr("Start Date"));
    ranges->headerItem()->setText(CPPAGE_RANGES_COL_CP, tr("Critical Power"));
    ranges->headerItem()->setText(CPPAGE_RANGES_COL_AETP, tr("AeTP"));
    ranges->headerItem()->setText(CPPAGE_RANGES_COL_FTP, tr("FTP"));
    ranges->headerItem()->setText(CPPAGE_RANGES_COL_WPRIME, tr("W'"));
    ranges->headerItem()->setText(CPPAGE_RANGES_COL_PMAX, tr("Pmax"));
    ranges->headerItem()->setText(CPPAGE_RANGES_COL_MODELFIT, tr("Model Fit"));
    ranges->headerItem()->setText(CPPAGE_RANGES_COL_EST_DEVIATION, "_deviation");
    ranges->headerItem()->setText(CPPAGE_RANGES_COL_EST_OFFSET, "_offset");
    ranges->headerItem()->setText(CPPAGE_RANGES_COL_EST_CP, "_cp");
    ranges->headerItem()->setText(CPPAGE_RANGES_COL_EST_FTP, "_ftp");
    ranges->headerItem()->setText(CPPAGE_RANGES_COL_EST_WPRIME, "_wprime");
    ranges->headerItem()->setText(CPPAGE_RANGES_COL_EST_PMAX, "_pmax");
    ranges->setColumnCount(CPPAGE_RANGES_COUNT_COL);
    ranges->setColumnHidden(CPPAGE_RANGES_COL_RNUM, true);
    ranges->setColumnHidden(CPPAGE_RANGES_COL_EST_DEVIATION, true);
    ranges->setColumnHidden(CPPAGE_RANGES_COL_EST_OFFSET, true);
    ranges->setColumnHidden(CPPAGE_RANGES_COL_EST_CP, true);
    ranges->setColumnHidden(CPPAGE_RANGES_COL_EST_FTP, true);
    ranges->setColumnHidden(CPPAGE_RANGES_COL_EST_WPRIME, true);
    ranges->setColumnHidden(CPPAGE_RANGES_COL_EST_PMAX, true);
    ranges->setItemDelegateForColumn(CPPAGE_RANGES_COL_STARTDATE, &dateDelegate);
    ranges->setItemDelegateForColumn(CPPAGE_RANGES_COL_CP, &cpDelegate);
    ranges->setItemDelegateForColumn(CPPAGE_RANGES_COL_AETP, &aetDelegate);
    ranges->setItemDelegateForColumn(CPPAGE_RANGES_COL_FTP, &ftpDelegate);
    ranges->setItemDelegateForColumn(CPPAGE_RANGES_COL_WPRIME, &wDelegate);
    ranges->setItemDelegateForColumn(CPPAGE_RANGES_COL_PMAX, &pmaxDelegate);
    ranges->setItemDelegateForColumn(CPPAGE_RANGES_COL_MODELFIT, &statusDelegate);
    ranges->header()->setSectionResizeMode(CPPAGE_RANGES_COL_STARTDATE, QHeaderView::ResizeToContents);
    ranges->header()->setSectionResizeMode(CPPAGE_RANGES_COL_CP, QHeaderView::ResizeToContents);
    ranges->header()->setSectionResizeMode(CPPAGE_RANGES_COL_AETP, QHeaderView::ResizeToContents);
    ranges->header()->setSectionResizeMode(CPPAGE_RANGES_COL_FTP, QHeaderView::ResizeToContents);
    ranges->header()->setSectionResizeMode(CPPAGE_RANGES_COL_WPRIME, QHeaderView::ResizeToContents);
    ranges->header()->setSectionResizeMode(CPPAGE_RANGES_COL_PMAX, QHeaderView::ResizeToContents);
    ranges->header()->setSectionResizeMode(CPPAGE_RANGES_COL_MODELFIT, QHeaderView::ResizeToContents);
    basicTreeWidgetStyle(ranges);

    zoneFromDelegate.setRange(0, 1000);
    zoneFromDelegate.setSingleStep(1);
    zoneFromDelegate.setSuffix(tr("W"));
    zoneFromDelegate.setShowSuffixOnEdit(true);
    zoneFromDelegate.setShowSuffixOnDisplay(true);

    zones = new QTreeWidget;
    zones->headerItem()->setText(0, tr("Short"));
    zones->headerItem()->setText(1, tr("Long"));
    zones->headerItem()->setText(2, tr("From Watts"));
    zones->setColumnCount(3);
    zones->setItemDelegateForColumn(2, &zoneFromDelegate);
    basicTreeWidgetStyle(zones);

    mainLayout->addLayout(addLayout);
    mainLayout->addWidget(ranges);
    mainLayout->addWidget(actionButtons);
    mainLayout->addWidget(zones);
    mainLayout->addWidget(zoneActionButtons);

    initializeRanges(zones_->getRangeSize() - 1);

    // button connect
    actionButtons->defaultConnect(ranges);
    connect(newZoneRequired, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(actionButtons, &ActionButtonBox::addRequested, this, &CPPage::addClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &CPPage::deleteClicked);
    connect(reviewButton, SIGNAL(clicked()), this, SLOT(review()));
    zoneActionButtons->defaultConnect(zones);
    connect(zoneActionButtons, &ActionButtonBox::addRequested, this, &CPPage::addZoneClicked);
    connect(zoneActionButtons, &ActionButtonBox::deleteRequested, this, &CPPage::deleteZoneClicked);
    connect(defaultButton, SIGNAL(clicked()), this, SLOT(defaultClicked()));
    connect(useCPForFTPCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(reInitializeRanges()));
    connect(useModel, SIGNAL(currentIndexChanged(int)), this, SLOT(reInitializeRanges()));
    connect(ranges, SIGNAL(itemSelectionChanged()), this, SLOT(rangeSelectionChanged()));
    connect(zones, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(zonesChanged()));
    connect(zones->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(zonesChanged()));
}


qint32
CPPage::saveClicked()
{
    if (   b4.cpforftp != useCPForFTPCombo->currentIndex()
        || b4.modelIdx != useModel->currentIndex()) {
        return CONFIG_ZONES;
    } else {
        return 0;
    }
}


void
CPPage::initializeRanges
(int selectIndex)
{
    disconnect(ranges->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QList<int>&)),
               this, SLOT(rangeChanged(const QModelIndex&, const QModelIndex&, const QList<int>&)));

    ranges->blockSignals(true);
    ranges->clear();
    ranges->blockSignals(false);

    bool useCPForFTP = (useCPForFTPCombo->currentIndex() == 0 ? true : false);
    ranges->setColumnHidden(CPPAGE_RANGES_COL_FTP, useCPForFTP);

    // setup list of ranges
    QTreeWidgetItem *selectedItem = nullptr;
    int selectIndex2 = std::min(selectIndex, zones_->getRangeSize() - 1);
    for (int i = 0; i < zones_->getRangeSize(); i++) {
        QTreeWidgetItem *add = new QTreeWidgetItem(ranges->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);
        if (i == selectIndex2) {
            selectedItem = add;
        }

        // Embolden ranges with manually configured zones
        QFont font;
        font.setWeight(zones_->getZoneRange(i).zonesSetFromCP ? QFont::Normal : QFont::Black);

        add->setData(CPPAGE_RANGES_COL_RNUM, Qt::DisplayRole, i);
        add->setData(CPPAGE_RANGES_COL_STARTDATE, Qt::DisplayRole, zones_->getStartDate(i));
        add->setFont(CPPAGE_RANGES_COL_STARTDATE, font);
        add->setData(CPPAGE_RANGES_COL_CP, Qt::DisplayRole, zones_->getCP(i));
        add->setFont(CPPAGE_RANGES_COL_CP, font);
        add->setData(CPPAGE_RANGES_COL_AETP, Qt::DisplayRole, zones_->getAeT(i));
        add->setFont(CPPAGE_RANGES_COL_AETP, font);
        add->setData(CPPAGE_RANGES_COL_FTP, Qt::DisplayRole, zones_->getFTP(i));
        add->setFont(CPPAGE_RANGES_COL_FTP, font);
        add->setData(CPPAGE_RANGES_COL_WPRIME, Qt::DisplayRole, zones_->getWprime(i));
        add->setFont(CPPAGE_RANGES_COL_WPRIME, font);
        add->setData(CPPAGE_RANGES_COL_PMAX, Qt::DisplayRole, zones_->getPmax(i));
        add->setFont(CPPAGE_RANGES_COL_PMAX, font);
        add->setFont(CPPAGE_RANGES_COL_MODELFIT, font);

        if (useModel->currentIndex() != CPPAGE_EST_MODEL_NONE) {
            ranges->setColumnHidden(CPPAGE_RANGES_COL_MODELFIT, false);
            int cp = 0;
            int aetp = 0;
            int ftp = 0;
            int wprime = 0;
            int pmax = 0;
            int estOffset = 0;
            bool defaults = false;
            if (getValuesFor(zones_->getStartDate(i), false, cp, aetp, ftp, wprime, pmax, estOffset, defaults)) {
                add->setData(CPPAGE_RANGES_COL_EST_OFFSET, Qt::DisplayRole, estOffset);
                add->setData(CPPAGE_RANGES_COL_EST_CP, Qt::DisplayRole, cp);
                add->setData(CPPAGE_RANGES_COL_EST_FTP, Qt::DisplayRole, ftp);
                add->setData(CPPAGE_RANGES_COL_EST_WPRIME, Qt::DisplayRole, wprime);
                add->setData(CPPAGE_RANGES_COL_EST_PMAX, Qt::DisplayRole, pmax);
            } else {
                add->setData(CPPAGE_RANGES_COL_EST_CP, Qt::DisplayRole, 0);
            }
        } else {
            ranges->setColumnHidden(CPPAGE_RANGES_COL_MODELFIT, true);
        }
        setEstimateStatus(add);
    }
    ranges->sortByColumn(CPPAGE_RANGES_COL_RNUM, Qt::DescendingOrder);

    if (selectedItem != nullptr) {
        ranges->setCurrentItem(selectedItem);
        ranges->scrollTo(ranges->currentIndex());
    }
    rangeSelectionChanged();

    newZoneRequired->setVisible(needsNewRange());

    connect(ranges->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QList<int>&)),
            this, SLOT(rangeChanged(const QModelIndex&, const QModelIndex&, const QList<int>&)));
}


void
CPPage::reInitializeRanges
()
{
    int rnum = -1;
    if (ranges->currentItem() != nullptr) {
        rnum = ranges->currentItem()->data(CPPAGE_RANGES_COL_RNUM, Qt::DisplayRole).toInt();
    }
    initializeRanges(rnum);
}


void
CPPage::rangeChanged
(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    Q_UNUSED(bottomRight)

    if (   roles.length() > 0
        && ! roles.contains(Qt::DisplayRole)
        && ! roles.contains(Qt::EditRole)) {
        return;
    }
    zones_->setScheme(schemePage->getScheme());
    QModelIndex rnumIdx = topLeft.siblingAtColumn(CPPAGE_RANGES_COL_RNUM);
    int rnum = rnumIdx.data(Qt::DisplayRole).toInt();
    switch (topLeft.column()) {
    case CPPAGE_RANGES_COL_STARTDATE: {
        QDate date = topLeft.data().toDate();
        zones_->setStartDate(rnum, date);
        if (useModel->currentIndex() != CPPAGE_EST_MODEL_NONE) {
            review();
            setEstimateStatus(ranges->itemFromIndex(topLeft));
        }
        break;
    }
    case CPPAGE_RANGES_COL_CP: {
        int cp = topLeft.data().toInt();
        zones_->setCP(rnum, cp);
        if (useCPForFTPCombo->currentIndex() == 0) {
            zones_->setFTP(rnum, cp);
        }
        setEstimateStatus(ranges->itemFromIndex(topLeft));
        break;
    }
    case CPPAGE_RANGES_COL_AETP:
        zones_->setAeT(rnum, topLeft.data().toInt());
        setEstimateStatus(ranges->itemFromIndex(topLeft));
        break;
    case CPPAGE_RANGES_COL_FTP:
        zones_->setFTP(rnum, topLeft.data().toInt());
        setEstimateStatus(ranges->itemFromIndex(topLeft));
        break;
    case CPPAGE_RANGES_COL_WPRIME: {
        int wp = topLeft.data().toInt();
        wp = wp ? wp : 20000;
        if (wp < 1000) {
            wp *= 1000; // entered in kJ we want joules
        }
        zones_->setWprime(rnum, wp);
        setEstimateStatus(ranges->itemFromIndex(topLeft));
        break;
    }
    case CPPAGE_RANGES_COL_PMAX: {
        int pmax = topLeft.data().toInt();
        pmax = pmax ? pmax : 1000;
        zones_->setPmax(rnum, pmax);
        setEstimateStatus(ranges->itemFromIndex(topLeft));
        break;
    }
    default:
        break;
    }
    if (useModel->currentIndex() == CPPAGE_EST_MODEL_NONE) {
        reviewButton->setVisible(false);
    } else if (ranges->currentItem() != nullptr && ranges->indexFromItem(ranges->currentItem()).row() == topLeft.row()) {
        reviewButton->setVisible(ranges->currentItem()->data(CPPAGE_RANGES_COL_EST_DEVIATION, Qt::DisplayRole).toInt() == CPPAGE_RANGES_EST_DEVIATE);
        newZoneRequired->setVisible(needsNewRange());
    }
}


void
CPPage::review
()
{
    bool preActive = active;
    active = false;
    QTreeWidgetItem *item = ranges->currentItem();
    if (item != nullptr) {
        QDate date = item->data(CPPAGE_RANGES_COL_STARTDATE, Qt::DisplayRole).toDate();
        int curCp = item->data(CPPAGE_RANGES_COL_CP, Qt::DisplayRole).toInt();
        int curAetp = item->data(CPPAGE_RANGES_COL_AETP, Qt::DisplayRole).toInt();
        int curFtp = item->data(CPPAGE_RANGES_COL_FTP, Qt::DisplayRole).toInt();
        int curWprime = item->data(CPPAGE_RANGES_COL_WPRIME, Qt::DisplayRole).toInt();
        int curPmax = item->data(CPPAGE_RANGES_COL_PMAX, Qt::DisplayRole).toInt();
        QModelIndex modelIndex = ranges->indexFromItem(item);
        int estCp = 0;
        int estAetp = 0;
        int estFtp = 0;
        int estWprime = 0;
        int estPmax = 0;
        int estOffset = 0;
        bool defaults = false;
        if (   getValuesFor(date, false, estCp, estAetp, estFtp, estWprime, estPmax, estOffset, defaults)
            && (   estCp != curCp
                || estFtp != curFtp
                || estWprime != curWprime
                || estPmax != curPmax)) {
            QLocale locale;
            QDialog dialog;
            dialog.setWindowTitle(tr("Review range starting on %1").arg(date.toString(locale.dateFormat(QLocale::ShortFormat))));

            QCheckBox *cpAccept = nullptr;
            QCheckBox *aetpAccept = nullptr;
            QCheckBox *ftpAccept = nullptr;
            QCheckBox *wprimeAccept = nullptr;
            QCheckBox *pmaxAccept = nullptr;

            QDialogButtonBox *buttonBox = new QDialogButtonBox(  QDialogButtonBox::Apply
                                                               | QDialogButtonBox::Discard);
            connect(buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), &dialog, SLOT(accept()));
            connect(buttonBox->button(QDialogButtonBox::Discard), SIGNAL(clicked()), &dialog, SLOT(reject()));

            int row = 0;
            QGridLayout *grid = new QGridLayout();
            grid->setSpacing(10 * dpiXFactor);
            grid->addWidget(new QLabel(tr("Current")), row, 2, Qt::AlignCenter);
            grid->addWidget(new QLabel(tr("Estimate")), row, 4, Qt::AlignCenter);
            grid->addWidget(new QLabel(tr("Accept")), row, 5, Qt::AlignCenter);
            ++row;
            mkReviewRow(grid, row++, CPPAGE_LABEL_CP, tr("W"), curCp, estCp, cpAccept);
            QString infoTextAetp = "";
            if (curAetp != estAetp) {
                infoTextAetp = getText(CPPAGE_INFO_AETP, CPPAGE_DEFAULT_FACTOR_AETP * 100);
            }
            mkReviewRow(grid, row++, CPPAGE_LABEL_AETP, tr("W"), curAetp, estAetp, aetpAccept, infoTextAetp);
            if (estFtp != curFtp || useCPForFTPCombo->currentIndex() == 1) {
                QString infoTextFtp = "";
                if (useCPForFTPCombo->currentIndex() == 0) {
                    infoTextFtp += getText(CPPAGE_INFO_FTP_CP);
                } else if (! modelHasFtp()) {
                    infoTextFtp += getText(CPPAGE_INFO_MODEL_FTP);
                }
                mkReviewRow(grid, row++, CPPAGE_LABEL_FTP, tr("W"), curFtp, estFtp, ftpAccept, infoTextFtp);
            }
            mkReviewRow(grid, row++, CPPAGE_LABEL_WPRIME, tr("J"), curWprime, estWprime, wprimeAccept);
            QString infoTextPmax = "";
            if (! modelHasPmax()) {
                infoTextPmax += getText(CPPAGE_INFO_MODEL_PMAX, estPmax);
            }
            mkReviewRow(grid, row++, CPPAGE_LABEL_PMAX, tr("W"), curPmax, estPmax, pmaxAccept, infoTextPmax);

            connectReviewDialogApplyButton(QVector<QCheckBox*>({cpAccept, aetpAccept, ftpAccept, wprimeAccept, pmaxAccept}), buttonBox->button(QDialogButtonBox::Apply));
            aetpAccept->setChecked(Qt::Unchecked);

            QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
            mainLayout->addLayout(grid);
            mainLayout->addStretch();
            mainLayout->addSpacing(30 * dpiYFactor);
            mainLayout->addWidget(buttonBox);

            if (dialog.exec() == QDialog::Accepted) {
                if (cpAccept->checkState() == Qt::Checked) {
                    setRangeData(modelIndex, CPPAGE_RANGES_COL_CP, estCp);
                    if (useCPForFTPCombo->currentIndex() == 0) {
                        setRangeData(modelIndex, CPPAGE_RANGES_COL_FTP, estFtp);
                    }
                }
                if (aetpAccept->checkState() == Qt::Checked) {
                    setRangeData(modelIndex, CPPAGE_RANGES_COL_AETP, estAetp);
                }
                if (ftpAccept != nullptr && ftpAccept->checkState() == Qt::Checked) {
                    setRangeData(modelIndex, CPPAGE_RANGES_COL_FTP, estFtp);
                }
                if (wprimeAccept->checkState() == Qt::Checked) {
                    setRangeData(modelIndex, CPPAGE_RANGES_COL_WPRIME, estWprime);
                }
                if (pmaxAccept->checkState() == Qt::Checked) {
                    setRangeData(modelIndex, CPPAGE_RANGES_COL_PMAX, estPmax);
                }
            }
            setRangeData(modelIndex, CPPAGE_RANGES_COL_EST_CP, estCp);
            setRangeData(modelIndex, CPPAGE_RANGES_COL_EST_FTP, estFtp);
            setRangeData(modelIndex, CPPAGE_RANGES_COL_EST_WPRIME, estWprime);
            setRangeData(modelIndex, CPPAGE_RANGES_COL_EST_PMAX, estPmax);
            setRangeData(modelIndex, CPPAGE_RANGES_COL_EST_OFFSET, estOffset);
        }
    }
    active = preActive;
}


bool
CPPage::getValuesFor
(const QDate &date, bool allowDefaults, int &cp, int &aetp, int &ftp, int &wprime, int &pmax, int &estOffset, bool &defaults, QDate *startDate) const
{
    estOffset = 0;
    defaults = false;
    if (useModel->currentIndex() != CPPAGE_EST_MODEL_NONE) {
        PDEstimate pde = context->athlete->getPDEstimateClosestFor(date, useModel->currentData().toString(), false, zones_->sport());
        cp = pde.CP;
        aetp = pde.CP * CPPAGE_DEFAULT_FACTOR_AETP;
        ftp = int(pde.FTP) > 0 ? pde.FTP : cp;
        if (useCPForFTPCombo->currentIndex() == 0) {
            ftp = cp;
        }
        wprime = int(pde.WPrime) > 0 ? pde.WPrime : CPPAGE_DEFAULT_WPRIME;
        pmax = int(pde.PMax) > 0 ? pde.PMax : CPPAGE_DEFAULT_PMAX;
        if (pde.from <= date && date <= pde.to) {
            estOffset = 0;
        } else if (date > pde.to) {
            estOffset = pde.to.daysTo(date);
        } else if (date < pde.from) {
            estOffset = pde.from.daysTo(date);
        }
        if (startDate != nullptr) {
            *startDate = pde.from;
        }
    }

    bool ret = false;
    if (cp > 0) {
        ret = true;
    } else if (allowDefaults) {
        QTreeWidgetItem *sourceItem = nullptr;
        if (ranges->currentItem() != nullptr) {
            sourceItem = ranges->currentItem();
        } else if (ranges->invisibleRootItem()->childCount() > 0) {
            sourceItem = ranges->invisibleRootItem()->child(0);
        }

        if (sourceItem != nullptr) {
            cp = sourceItem->data(CPPAGE_RANGES_COL_CP, Qt::DisplayRole).toInt();
            aetp = sourceItem->data(CPPAGE_RANGES_COL_AETP, Qt::DisplayRole).toInt();
            ftp = sourceItem->data(CPPAGE_RANGES_COL_FTP, Qt::DisplayRole).toInt();
            wprime = sourceItem->data(CPPAGE_RANGES_COL_WPRIME, Qt::DisplayRole).toInt();
            pmax = sourceItem->data(CPPAGE_RANGES_COL_PMAX, Qt::DisplayRole).toInt();
        } else {
            cp = CPPAGE_DEFAULT_CP;
            aetp = cp * CPPAGE_DEFAULT_FACTOR_AETP;
            ftp = cp;
            wprime = CPPAGE_DEFAULT_WPRIME;
            pmax = CPPAGE_DEFAULT_PMAX;
        }
        estOffset = 0;
        defaults = true;
        if (startDate != nullptr) {
            *startDate = QDate();
        }
        ret = true;
    }
    return ret;
}


void
CPPage::setEstimateStatus
(QTreeWidgetItem *item)
{
    bool preActive = active;
    active = true;

    QModelIndex modelIndex = ranges->indexFromItem(item);

    int cp = item->data(CPPAGE_RANGES_COL_CP, Qt::DisplayRole).toInt();
    int ftp = item->data(CPPAGE_RANGES_COL_FTP, Qt::DisplayRole).toInt();
    int wprime = item->data(CPPAGE_RANGES_COL_WPRIME, Qt::DisplayRole).toInt();
    int pmax = item->data(CPPAGE_RANGES_COL_PMAX, Qt::DisplayRole).toInt();
    int estCp = item->data(CPPAGE_RANGES_COL_EST_CP, Qt::DisplayRole).toInt();
    int estFtp = item->data(CPPAGE_RANGES_COL_EST_FTP, Qt::DisplayRole).toInt();
    int estWprime = item->data(CPPAGE_RANGES_COL_EST_WPRIME, Qt::DisplayRole).toInt();
    int estPmax = item->data(CPPAGE_RANGES_COL_EST_PMAX, Qt::DisplayRole).toInt();
    int estOffset = item->data(CPPAGE_RANGES_COL_EST_OFFSET, Qt::DisplayRole).toInt();
    QString fitText;
    QString toolTipText = "<center>";
    if (estCp > 0) {
        if (   (   useModel->currentIndex() == CPPAGE_EST_MODEL_CP2
                && std::abs(cp - estCp) <= CPPAGE_EST_TOLERANCE_CP
                && std::abs(wprime - estWprime) <= CPPAGE_EST_TOLERANCE_WPRIME)
            || (   useModel->currentIndex() == CPPAGE_EST_MODEL_CP3
                && std::abs(cp - estCp) <= CPPAGE_EST_TOLERANCE_CP
                && std::abs(wprime - estWprime) <= CPPAGE_EST_TOLERANCE_WPRIME
                && std::abs(pmax - estPmax) <= CPPAGE_EST_TOLERANCE_PMAX)
            || (   useModel->currentIndex() == CPPAGE_EST_MODEL_EXT
                && std::abs(cp - estCp) <= CPPAGE_EST_TOLERANCE_CP
                && std::abs(wprime - estWprime) <= CPPAGE_EST_TOLERANCE_WPRIME
                && std::abs(ftp - estFtp) <= CPPAGE_EST_TOLERANCE_CP
                && std::abs(pmax - estPmax) <= CPPAGE_EST_TOLERANCE_PMAX)) {
            setRangeData(modelIndex, CPPAGE_RANGES_COL_EST_DEVIATION, CPPAGE_RANGES_EST_MATCH);
            fitText = tr("🗹");
            toolTipText += tr("Estimate and settings <b>match</b>");
        } else {
            setRangeData(modelIndex, CPPAGE_RANGES_COL_EST_DEVIATION, CPPAGE_RANGES_EST_DEVIATE);
            fitText = tr("☐");
            toolTipText += tr("Estimate and settings <b>differ</b>");
        }
        if (estOffset != 0) {
            fitText += " ";
            fitText += tr("⏲");
            toolTipText += "<br><small>";
            if (estOffset < 0) {
                toolTipText += tr("Range is %1 days older than closest estimate").arg(-1 * estOffset);
            } else if (estOffset > 0) {
                toolTipText += tr("Range is %1 days younger than closest estimate").arg(estOffset);
            }
            toolTipText += "</small>";
        }
    } else {
        setRangeData(modelIndex, CPPAGE_RANGES_COL_EST_DEVIATION, CPPAGE_RANGES_EST_NA);
        fitText = tr("∅");
        toolTipText += tr("No estimate available");
    }
    toolTipText += "</center>";
    setRangeData(modelIndex, CPPAGE_RANGES_COL_MODELFIT, fitText);
    item->setToolTip(CPPAGE_RANGES_COL_MODELFIT, toolTipText);
    active = preActive;
}


void
CPPage::setRangeData
(QModelIndex modelIndex, int column, QVariant data)
{
    ranges->model()->setData(modelIndex.siblingAtColumn(column), data, Qt::DisplayRole);
}


bool
CPPage::needsNewRange
() const
{
    zones_->setScheme(schemePage->getScheme());
    if (useModel->currentIndex() == CPPAGE_EST_MODEL_NONE) {
        return false;
    }

    QDate today = QDate::currentDate();
    int todayEstCp = 0;
    int todayEstAetp = 0;
    int todayEstFtp = 0;
    int todayEstWprime = 0;
    int todayEstPmax = 0;
    int todayEstEstOffset = 0;
    bool defaults = false;
    QDate startDate;
    bool todayOk = getValuesFor(today, false, todayEstCp, todayEstAetp, todayEstFtp, todayEstWprime, todayEstPmax, todayEstEstOffset, defaults, &startDate);

    int activeRange = zones_->whichRange(today);
    if (activeRange < 0) {
        return todayOk;
    }

    int currentSetCp = zones_->getZoneRange(activeRange).cp;
    int currentSetFtp = zones_->getZoneRange(activeRange).ftp;
    int currentSetWprime = zones_->getZoneRange(activeRange).wprime;
    int currentSetPmax = zones_->getZoneRange(activeRange).pmax;
    QDate currentSetStart = zones_->getZoneRange(activeRange).begin;

    return    todayOk
           && startDate > currentSetStart
           && ! (   (   useModel->currentIndex() == CPPAGE_EST_MODEL_CP2
                     && std::abs(todayEstCp - currentSetCp) <= CPPAGE_EST_TOLERANCE_CP
                     && std::abs(todayEstWprime - currentSetWprime) <= CPPAGE_EST_TOLERANCE_WPRIME)
                 || (   useModel->currentIndex() == CPPAGE_EST_MODEL_CP3
                     && std::abs(todayEstCp - currentSetCp) <= CPPAGE_EST_TOLERANCE_CP
                     && std::abs(todayEstWprime - currentSetWprime) <= CPPAGE_EST_TOLERANCE_WPRIME
                     && std::abs(todayEstPmax - currentSetPmax) <= CPPAGE_EST_TOLERANCE_PMAX)
                 || (   useModel->currentIndex() == CPPAGE_EST_MODEL_EXT
                     && std::abs(todayEstCp - currentSetCp) <= CPPAGE_EST_TOLERANCE_CP
                     && std::abs(todayEstWprime - currentSetWprime) <= CPPAGE_EST_TOLERANCE_WPRIME
                     && std::abs(todayEstFtp - currentSetFtp) <= CPPAGE_EST_TOLERANCE_CP
                     && std::abs(todayEstPmax - currentSetPmax) <= CPPAGE_EST_TOLERANCE_PMAX));
}


void
CPPage::addClicked()
{
    // get current scheme
    zones_->setScheme(schemePage->getScheme());

    QDate date = QDate::currentDate();
    int cp = 0;
    int aetp = 0;
    int ftp = 0;
    int wprime = 0;
    int pmax = 0;
    int estOffset = 0;
    bool defaults = false;
    QDate startDate;
    bool ok = getValuesFor(date, true, cp, aetp, ftp, wprime, pmax, estOffset, defaults, &startDate);
    if (ok && startDate.isValid() && startDate < date) {
        date = startDate;
        estOffset = 0;
    }
    if (ok && ! defaults && estOffset == 0) {
        QString text;
        if (useModel->currentData().toString() == "cp2") {
            text = getText(CPPAGE_INFO_MODEL_CP2, pmax);
        } else if (useModel->currentData().toString() == "cp3") {
            text = getText(CPPAGE_INFO_MODEL_CP3);
        }
        text += getText(CPPAGE_INFO_AETP, CPPAGE_DEFAULT_FACTOR_AETP * 100);
        QMessageBox::information(this, tr("New range from estimate"), text);
    } else if (! addDialogManual(date, cp, aetp, wprime, ftp, pmax)) {
        return;
    }

    int rnum = zones_->addZoneRange(date, cp, aetp, ftp, wprime, pmax);

    initializeRanges(rnum);
}


void
CPPage::deleteClicked()
{
    if (ranges->currentItem()) {
        int rnum = ranges->currentItem()->data(CPPAGE_RANGES_COL_RNUM, Qt::DisplayRole).toInt();
        zones_->deleteRange(rnum);
        initializeRanges(rnum);
    }
}

void
CPPage::defaultClicked()
{
    if (ranges->currentItem()) {
        int rnum = ranges->currentItem()->data(CPPAGE_RANGES_COL_RNUM, Qt::DisplayRole).toInt();
        ZoneRange current = zones_->getZoneRange(rnum);

        // unbold
        QFont font;
        font.setWeight(QFont::Normal);
        ranges->currentItem()->setFont(CPPAGE_RANGES_COL_STARTDATE, font);
        ranges->currentItem()->setFont(CPPAGE_RANGES_COL_CP, font);
        ranges->currentItem()->setFont(CPPAGE_RANGES_COL_AETP, font);
        ranges->currentItem()->setFont(CPPAGE_RANGES_COL_FTP, font);
        ranges->currentItem()->setFont(CPPAGE_RANGES_COL_WPRIME, font);
        ranges->currentItem()->setFont(CPPAGE_RANGES_COL_PMAX, font);
        ranges->currentItem()->setFont(CPPAGE_RANGES_COL_MODELFIT, font);

        // set the range to use defaults on the scheme page
        zones_->setScheme(schemePage->getScheme());
        zones_->setZonesFromCP(rnum);

        // hide the default button since we are now using defaults
        defaultButton->hide();

        // update the zones display
        rangeSelectionChanged();
    }
}


void
CPPage::rangeSelectionChanged()
{
    active = true;

    // wipe away current contents of zones
    zones->clear();

    // fill with current details
    QTreeWidgetItem *item = ranges->currentItem();
    if (item != nullptr) {
        reviewButton->setVisible(   useModel->currentIndex() != CPPAGE_EST_MODEL_NONE
                                && item->data(CPPAGE_RANGES_COL_EST_DEVIATION, Qt::DisplayRole).toInt() == CPPAGE_RANGES_EST_DEVIATE);

        int rnum = ranges->currentItem()->data(CPPAGE_RANGES_COL_RNUM, Qt::DisplayRole).toInt();
        ZoneRange current = zones_->getZoneRange(rnum);

        if (current.zonesSetFromCP) {
            // reapply the scheme in case it has been changed
            zones_->setScheme(schemePage->getScheme());
            zones_->setZonesFromCP(rnum);
            current = zones_->getZoneRange(rnum);

            defaultButton->hide();
        } else {
            defaultButton->show();
        }

        for (int i=0; i< current.zones.count(); i++) {
            QTreeWidgetItem *add = new QTreeWidgetItem(zones->invisibleRootItem());
            add->setFlags(add->flags() | Qt::ItemIsEditable);

            add->setData(0, Qt::DisplayRole, current.zones[i].name);
            add->setData(1, Qt::DisplayRole, current.zones[i].desc);
            add->setData(2, Qt::DisplayRole, current.zones[i].lo);
        }
    } else {
        reviewButton->setVisible(false);
        zones->clear();
    }

    active = false;
}

void
CPPage::addZoneClicked()
{
    // no range selected
    if (!ranges->currentItem()) return;

    active = true;
    int rnum = zones->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    zones->invisibleRootItem()->insertChild(rnum, add);

    // Short
    QString text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setData(0, Qt::DisplayRole, text);

    // long
    text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setData(1, Qt::DisplayRole, text);

    add->setData(2, Qt::DisplayRole, 100);
    active = false;

    zonesChanged();
}

void
CPPage::deleteZoneClicked()
{
    // range and zone must be selected
    if (ranges->currentItem() == nullptr || zones->currentItem() == nullptr) {
        return;
    }

    active = true;
    int index = zones->invisibleRootItem()->indexOfChild(zones->currentItem());
    delete zones->invisibleRootItem()->takeChild(index);
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
            int rnum = ranges->currentItem()->data(CPPAGE_RANGES_COL_RNUM, Qt::DisplayRole).toInt();
            ZoneRange current = zones_->getZoneRange(rnum);

            // embolden that range on the list to show it has been edited
            QFont font;
            font.setWeight(QFont::Black);
            ranges->currentItem()->setFont(CPPAGE_RANGES_COL_STARTDATE, font);
            ranges->currentItem()->setFont(CPPAGE_RANGES_COL_CP, font);
            ranges->currentItem()->setFont(CPPAGE_RANGES_COL_AETP, font);
            ranges->currentItem()->setFont(CPPAGE_RANGES_COL_FTP, font);
            ranges->currentItem()->setFont(CPPAGE_RANGES_COL_WPRIME, font);
            ranges->currentItem()->setFont(CPPAGE_RANGES_COL_PMAX, font);
            ranges->currentItem()->setFont(CPPAGE_RANGES_COL_MODELFIT, font);

            // show the default button to undo
            defaultButton->show();

            // we manually edited so save in full
            current.zonesSetFromCP = false;

            // create the new zoneinfos for this range
            QList<ZoneInfo> zoneinfos;
            for (int i=0; i< zones->invisibleRootItem()->childCount(); i++) {
                QTreeWidgetItem *item = zones->invisibleRootItem()->child(i);
                zoneinfos << ZoneInfo(item->data(0, Qt::DisplayRole).toString(),
                                      item->data(1, Qt::DisplayRole).toString(),
                                      item->data(2, Qt::DisplayRole).toInt(),
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
            zones_->setZoneRange(rnum, current);
        }
    }
}


void
CPPage::mkReviewRow
(QGridLayout *grid, int row, int labelId, const QString &unit, int cur, int est, QCheckBox *&accept, const QString &infoText) const
{
    QFormLayout dummy;

    accept = new QCheckBox();
    accept->setVisible(cur != est);

    QLineEdit *current = new QLineEdit(QString("%1 %2").arg(cur).arg(unit));
    current->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    current->setAlignment(Qt::AlignCenter);
    current->setReadOnly(true);
    current->setEnabled(false);

    if (cur != est) {
        QLabel *relation = new QLabel();
        relation->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        if (cur < est) {
            relation->setText("↗");
        } else if (cur > est) {
            relation->setText("↘");
        }
        QLineEdit *estimate = new QLineEdit(QString("%1 %2").arg(est).arg(unit));
        estimate->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        estimate->setAlignment(Qt::AlignCenter);
        estimate->setReadOnly(true);
        estimate->setEnabled(true);

        grid->addWidget(current, row, 2);
        grid->addWidget(relation, row, 3);
        grid->addWidget(estimate, row, 4);

        connect(
            accept, &QCheckBox::toggled, this,
            [this, current, estimate](bool checked) {
                current->setEnabled(! checked);
                estimate->setEnabled(checked);
            }
        );
    } else {
        grid->addWidget(current, row, 2, 1, 3);
    }

    grid->addWidget(mkInfoLabel(labelId, infoText), row, 0, dummy.labelAlignment());
    grid->addWidget(accept, row, 5, Qt::AlignCenter);

    accept->setCheckState(cur != est ? Qt::Checked : Qt::Unchecked);
}


void
CPPage::connectReviewDialogApplyButton
(QVector<QCheckBox*> checkboxes, QPushButton *applyButton) const
{
    bool hasChecked = false;
    for (int i = 0; i < checkboxes.size(); ++i) {
        if (checkboxes[i] != nullptr) {
            hasChecked |= checkboxes[i]->isChecked();
            connect(
                checkboxes[i], &QCheckBox::toggled, this,
                [this, checkboxes, applyButton](bool checked) {
                    bool anyChecked = checked;
                    int j = 0;
                    while (! anyChecked && j < checkboxes.size()) {
                        if (checkboxes[j] != nullptr && checkboxes[j]->isChecked()) {
                            anyChecked = true;
                        }
                        ++j;
                    }
                    applyButton->setEnabled(anyChecked);
                }
            );
        }
    }
    applyButton->setEnabled(hasChecked);
}


bool
CPPage::modelHasFtp
() const
{
    return useModel->currentIndex() == CPPAGE_EST_MODEL_EXT;
}


bool
CPPage::modelHasPmax
() const
{
    return    useModel->currentIndex() == CPPAGE_EST_MODEL_CP3
           || useModel->currentIndex() == CPPAGE_EST_MODEL_EXT;
}


QWidget*
CPPage::mkInfoLabel
(int labelId, const QString &infoText) const
{
    QWidget *ret = nullptr;
    if (infoText.isEmpty()) {
        ret = new QLabel(getText(labelId));
    } else {
        ret = new QWidget();

        QLabel *info = new QLabel();
        info->setPixmap(QPixmap(":images/toolbar/info.png"));
        info->setToolTip(infoText);

        QHBoxLayout *labelLayout = new QHBoxLayout(ret);
        labelLayout->setContentsMargins(0, 0, 0, 0);
        labelLayout->addWidget(new QLabel(getText(labelId)));
        labelLayout->addWidget(info);
    }
    return ret;
}


QString
CPPage::getText
(int id, int value) const
{
    switch (id) {
    case CPPAGE_INFO_AETP:
        return tr("The proposed value for AeTP is a very rough estimate, assuming %1 % of CP. Usually it is determined by a<ul><li>Metabolic test</li><li>Lactate ramp test</li><li>Run / Cycling ‘conversational’ test</li><li>Run Decoupling test</li></ul>").arg(value);
    case CPPAGE_INFO_FTP_CP:
        return tr("Updating FTP internally to match CP");
    case CPPAGE_INFO_MODEL_FTP:
    case CPPAGE_INFO_MODEL_CP3:
        return tr("Your selected model does not deliver values for<ul><li>FTP, using CP instead</li></ul>");
    case CPPAGE_INFO_MODEL_PMAX:
        return tr("Your selected model does not deliver values for PMax, using default value of %1 W instead").arg(value);
    case CPPAGE_INFO_MODEL_CP2:
        return tr("Your selected model does not deliver values for<ul><li>FTP, using CP instead</li><li>PMax, assuming a default of %1 W</li></ul>").arg(value);
    case CPPAGE_LABEL_STARTDATE:
        return tr("Start Date");
    case CPPAGE_LABEL_CP:
        return tr("Critical Power (CP)");
    case CPPAGE_LABEL_AETP:
        return tr("Aerobic Threshold Power (AeTP)");
    case CPPAGE_LABEL_FTP:
        return tr("Functional Threshold Power (FTP)");
    case CPPAGE_LABEL_WPRIME:
        return tr("Anaerobic Work Capacity (W')");
    case CPPAGE_LABEL_PMAX:
        return tr("Maximum Power (PMax)");
    default:
        return QString("UNKNOWN MESSAGE %1").arg(id);
    }
}


bool
CPPage::addDialogManual
(QDate &date, int &cp, int &aetp, int &wprime, int &ftp, int &pmax) const
{
    QDialog dialog;
    dialog.setWindowTitle(tr("Manual range"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(  QDialogButtonBox::Apply
                                                       | QDialogButtonBox::Discard);
    connect(buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), &dialog, SLOT(accept()));
    connect(buttonBox->button(QDialogButtonBox::Discard), SIGNAL(clicked()), &dialog, SLOT(reject()));

    QDateEdit *dateEdit = new QDateEdit(date);
    dateEdit->setCalendarPopup(true);

    QSpinBox *cpEdit = new QSpinBox();
    cpEdit->setRange(1, 999);
    cpEdit->setSingleStep(1);
    cpEdit->setSuffix(" " + tr("W"));
    cpEdit->setValue(cp);

    QSpinBox *aetpEdit = new QSpinBox();
    aetpEdit->setRange(1, 999);
    aetpEdit->setSingleStep(1);
    aetpEdit->setSuffix(" " + tr("W"));
    aetpEdit->setValue(aetp);

    QSpinBox *ftpEdit = nullptr;
    if (useCPForFTPCombo->currentIndex() != 0) {
        ftpEdit = new QSpinBox();
        ftpEdit->setRange(1, 999);
        ftpEdit->setSingleStep(1);
        ftpEdit->setSuffix(" " + tr("W"));
        ftpEdit->setValue(ftp);
    }

    QSpinBox *wprimeEdit = new QSpinBox();
    wprimeEdit->setRange(1, 99999);
    wprimeEdit->setSingleStep(100);
    wprimeEdit->setSuffix(" " + tr("J"));
    wprimeEdit->setValue(wprime);

    QSpinBox *pmaxEdit = new QSpinBox();
    pmaxEdit->setRange(1, 9999);
    pmaxEdit->setSingleStep(10);
    pmaxEdit->setSuffix(" " + tr("W"));
    pmaxEdit->setValue(pmax);

    QFormLayout *form = newQFormLayout(&dialog);
    form->addRow(getText(CPPAGE_LABEL_STARTDATE), dateEdit);
    form->addRow(getText(CPPAGE_LABEL_CP), cpEdit);
    form->addRow(getText(CPPAGE_LABEL_AETP), aetpEdit);
    if (ftpEdit != nullptr) {
        form->addRow(getText(CPPAGE_LABEL_FTP), ftpEdit);
    }
    form->addRow(getText(CPPAGE_LABEL_WPRIME), wprimeEdit);
    form->addRow(getText(CPPAGE_LABEL_PMAX), pmaxEdit);
    form->addItem(new QSpacerItem(1, 30 * dpiYFactor));
    form->addRow(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        date = dateEdit->date();
        cp = cpEdit->value();
        aetp = aetpEdit->value();
        ftp = ftpEdit != nullptr ? ftpEdit->value() : cp;
        wprime = wprimeEdit->value();
        pmax = pmaxEdit->value();
        return true;
    } else {
        return false;
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

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);
    actionButtons->setMaxViewItems(10);

    ltDelegate.setRange(0, 1000);
    ltDelegate.setSuffix(tr("%"));
    ltDelegate.setShowSuffixOnEdit(true);
    ltDelegate.setShowSuffixOnDisplay(true);

    trimpkDelegate.setRange(0, 10);
    trimpkDelegate.setSingleStep(0.1);
    trimpkDelegate.setDecimals(2);
    trimpkDelegate.setShowSuffixOnEdit(false);
    trimpkDelegate.setShowSuffixOnDisplay(false);

    scheme = new QTreeWidget;
    scheme->headerItem()->setText(0, tr("Short"));
    scheme->headerItem()->setText(1, tr("Long"));
    scheme->headerItem()->setText(2, tr("Percent of LT"));
    scheme->headerItem()->setText(3, tr("Trimp k"));
    scheme->setColumnCount(4);
    scheme->setItemDelegateForColumn(2, &ltDelegate);
    scheme->setItemDelegateForColumn(3, &trimpkDelegate);
    basicTreeWidgetStyle(scheme);

    // setup list
    for (int i=0; i< hrZones->getScheme().nzones_default; i++) {
        QTreeWidgetItem *add = new QTreeWidgetItem(scheme->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // tab name
        add->setData(0, Qt::DisplayRole, hrZones->getScheme().zone_default_name[i]);
        // field name
        add->setData(1, Qt::DisplayRole, hrZones->getScheme().zone_default_desc[i]);

        // low
        add->setData(2, Qt::DisplayRole, hrZones->getScheme().zone_default[i]);

        // trimp
        add->setData(3, Qt::DisplayRole, hrZones->getScheme().zone_default_trimp[i]);
    }

    mainLayout->addWidget(scheme);
    mainLayout->addWidget(actionButtons);

    // button connect
    actionButtons->defaultConnect(scheme);
    connect(actionButtons, &ActionButtonBox::addRequested, this, &HrSchemePage::addClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &HrSchemePage::deleteClicked);
}


void
HrSchemePage::addClicked()
{
    int index = scheme->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);
    scheme->invisibleRootItem()->insertChild(index, add);

    // Short
    QString text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setData(0, Qt::DisplayRole, text);

    // long
    text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setData(1, Qt::DisplayRole, text);

    // lo
    add->setData(2, Qt::DisplayRole, 100);

    // trimp
    add->setData(3, Qt::DisplayRole, 1);
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
        add.name = scheme->invisibleRootItem()->child(i)->data(0, Qt::DisplayRole).toString();
        add.desc = scheme->invisibleRootItem()->child(i)->data(1, Qt::DisplayRole).toString();
        add.lo = scheme->invisibleRootItem()->child(i)->data(2, Qt::DisplayRole).toInt();
        add.trimp = scheme->invisibleRootItem()->child(i)->data(3, Qt::DisplayRole).toDouble();
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

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10 *dpiXFactor);

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);

    ActionButtonBox *zoneActionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);
    zoneActionButtons->setMaxViewItems(10);
    defaultButton = zoneActionButtons->addButton(tr("Def"), ActionButtonBox::Right);
    defaultButton->hide();

    dateDelegate.setCalendarPopup(true);

    ltDelegate.setRange(1, 240);
    ltDelegate.setSuffix(tr("bpm"));
    ltDelegate.setShowSuffixOnEdit(true);
    ltDelegate.setShowSuffixOnDisplay(true);

    aetDelegate.setRange(0, 240);
    aetDelegate.setSuffix(tr("bpm"));
    aetDelegate.setShowSuffixOnEdit(true);
    aetDelegate.setShowSuffixOnDisplay(true);

    restHrDelegate.setRange(0, 240);
    restHrDelegate.setSuffix(tr("bpm"));
    restHrDelegate.setShowSuffixOnEdit(true);
    restHrDelegate.setShowSuffixOnDisplay(true);

    maxHrDelegate.setRange(0, 240);
    maxHrDelegate.setSuffix(tr("bpm"));
    maxHrDelegate.setShowSuffixOnEdit(true);
    maxHrDelegate.setShowSuffixOnDisplay(true);

    ranges = new QTreeWidget;
    ranges->headerItem()->setText(0, tr("Start Date"));
    ranges->headerItem()->setText(1, tr("Lactate Threshold"));
    ranges->headerItem()->setText(2, tr("Aerobic Threshold"));
    ranges->headerItem()->setText(3, tr("Rest HR"));
    ranges->headerItem()->setText(4, tr("Max HR"));
    ranges->headerItem()->setText(5, "_rnum");
    ranges->setColumnCount(6);
    ranges->setColumnHidden(5, true);
    ranges->setItemDelegateForColumn(0, &dateDelegate);
    ranges->setItemDelegateForColumn(1, &ltDelegate);
    ranges->setItemDelegateForColumn(2, &aetDelegate);
    ranges->setItemDelegateForColumn(3, &restHrDelegate);
    ranges->setItemDelegateForColumn(4, &maxHrDelegate);
    basicTreeWidgetStyle(ranges);

    // setup list of ranges
    for (int i = 0; i < hrZones->getRangeSize(); i++) {
        QTreeWidgetItem *add = new QTreeWidgetItem(ranges->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // Embolden ranges with manually configured zones
        QFont font;
        font.setWeight(hrZones->getHrZoneRange(i).hrZonesSetFromLT ?
                       QFont::Normal : QFont::Black);

        add->setData(0, Qt::DisplayRole, hrZones->getStartDate(i));
        add->setFont(0, font);
        add->setData(1, Qt::DisplayRole, hrZones->getLT(i));
        add->setFont(1, font);
        add->setData(2, Qt::DisplayRole, hrZones->getAeT(i));
        add->setFont(2, font);
        add->setData(3, Qt::DisplayRole, hrZones->getRestHr(i));
        add->setFont(3, font);
        add->setData(4, Qt::DisplayRole, hrZones->getMaxHr(i));
        add->setFont(4, font);
        add->setData(5, Qt::DisplayRole, i);
    }
    ranges->sortByColumn(5, Qt::DescendingOrder);

    zoneLoDelegate.setRange(0, 240);
    zoneLoDelegate.setSuffix(tr("bpm"));
    zoneLoDelegate.setShowSuffixOnEdit(true);
    zoneLoDelegate.setShowSuffixOnDisplay(true);

    zoneTrimpDelegate.setRange(0, 10);
    zoneTrimpDelegate.setSingleStep(0.1);
    zoneTrimpDelegate.setDecimals(2);
    zoneTrimpDelegate.setShowSuffixOnEdit(false);
    zoneTrimpDelegate.setShowSuffixOnDisplay(false);

    zones = new QTreeWidget;
    zones->headerItem()->setText(0, tr("Short"));
    zones->headerItem()->setText(1, tr("Long"));
    zones->headerItem()->setText(2, tr("From BPM"));
    zones->headerItem()->setText(3, tr("Trimp k"));
    zones->setColumnCount(4);
    zones->setItemDelegateForColumn(2, &zoneLoDelegate);
    zones->setItemDelegateForColumn(3, &zoneTrimpDelegate);
    basicTreeWidgetStyle(zones);

    mainLayout->addWidget(ranges);
    mainLayout->addWidget(actionButtons);
    mainLayout->addWidget(zones);
    mainLayout->addWidget(zoneActionButtons);

    // button connect
    actionButtons->defaultConnect(ranges);
    connect(actionButtons, &ActionButtonBox::addRequested, this, &LTPage::addClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &LTPage::deleteClicked);
    zoneActionButtons->defaultConnect(zones);
    connect(zoneActionButtons, &ActionButtonBox::addRequested, this, &LTPage::addZoneClicked);
    connect(zoneActionButtons, &ActionButtonBox::deleteRequested, this, &LTPage::deleteZoneClicked);
    connect(defaultButton, SIGNAL(clicked()), this, SLOT(defaultClicked()));

    connect(ranges, SIGNAL(itemSelectionChanged()), this, SLOT(rangeSelectionChanged()));
    connect(ranges->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(rangeChanged(const QModelIndex&)));
    connect(zones, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(zonesChanged()));
    connect(zones->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(zonesChanged()));

    if (ranges->invisibleRootItem()->childCount() > 0) {
        ranges->setCurrentItem(ranges->invisibleRootItem()->child(0));
    }
}


void
LTPage::addClicked()
{
    QDialog dialog;

    int lt = 1;
    int aet = 0;
    int restHr = 0;
    int maxHr = 0;

    int rnum = -1;
    if (ranges->currentItem()) {
        rnum = ranges->currentItem()->data(5, Qt::DisplayRole).toInt();
    } else if (ranges->invisibleRootItem()->childCount() > 0) {
        rnum = ranges->invisibleRootItem()->child(0)->data(5, Qt::DisplayRole).toInt();
    }
    if (rnum >= 0) {
        lt = hrZones->getLT(rnum);
        aet = hrZones->getAeT(rnum);
        maxHr = hrZones->getMaxHr(rnum);
        restHr = hrZones->getRestHr(rnum);
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(  QDialogButtonBox::Apply
                                                       | QDialogButtonBox::Discard);
    connect(buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), &dialog, SLOT(accept()));
    connect(buttonBox->button(QDialogButtonBox::Discard), SIGNAL(clicked()), &dialog, SLOT(reject()));

    QDateEdit *dateEdit = new QDateEdit(QDate::currentDate());
    dateEdit->setCalendarPopup(true);

    QSpinBox *ltEdit = new QSpinBox();
    ltEdit->setRange(1, 240);
    ltEdit->setSuffix(" " + tr("bpm"));
    ltEdit->setValue(lt);

    QSpinBox *aetEdit = new QSpinBox();
    aetEdit->setRange(0, 240);
    aetEdit->setSuffix(" " + tr("bpm"));
    aetEdit->setValue(aet);

    QSpinBox *restHrEdit = new QSpinBox();
    restHrEdit->setRange(0, 240);
    restHrEdit->setSuffix(" " + tr("bpm"));
    restHrEdit->setValue(restHr);

    QSpinBox *maxHrEdit = new QSpinBox();
    maxHrEdit->setRange(0, 240);
    maxHrEdit->setSuffix(" " + tr("bpm"));
    maxHrEdit->setValue(maxHr);

    QFormLayout *form = newQFormLayout(&dialog);
    form->addRow(tr("Start Date"), dateEdit);
    form->addRow(tr("Lactate Threshold"), ltEdit);
    form->addRow(tr("Aerobic Threshold"), aetEdit);
    form->addRow(tr("Rest HR"), restHrEdit);
    form->addRow(tr("Max HR"), maxHrEdit);
    form->addRow(buttonBox);

    int dialogRet = dialog.exec();
    if (dialogRet != QDialog::Accepted) {
        return;
    }

    // get current scheme
    hrZones->setScheme(schemePage->getScheme());

    rnum = hrZones->addHrZoneRange(dateEdit->date(), ltEdit->value(), aetEdit->value(), restHrEdit->value(), maxHrEdit->value());

    for (int i = 0; i < ranges->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *item = ranges->invisibleRootItem()->child(i);
        int itemRnum = item->data(5, Qt::DisplayRole).toInt();
        if (itemRnum >= rnum) {
            item->setData(5, Qt::DisplayRole, itemRnum + 1);
        }
    }

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem(ranges->invisibleRootItem());
    add->setFlags(add->flags() | Qt::ItemIsEditable);
    add->setData(0, Qt::DisplayRole, dateEdit->date());
    add->setData(1, Qt::DisplayRole, ltEdit->value());
    add->setData(2, Qt::DisplayRole, aetEdit->value());
    add->setData(3, Qt::DisplayRole, restHrEdit->value());
    add->setData(4, Qt::DisplayRole, maxHrEdit->value());
    add->setData(5, Qt::DisplayRole, rnum),

    ranges->sortByColumn(CPPAGE_RANGES_COL_RNUM, Qt::DescendingOrder);
    ranges->setCurrentItem(add);
}


void
LTPage::deleteClicked()
{
    if (ranges->currentItem()) {
        int rnum = ranges->currentItem()->data(5, Qt::DisplayRole).toInt();

        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        delete ranges->invisibleRootItem()->takeChild(index);
        hrZones->deleteRange(rnum);

        for (int i = 0; i < ranges->invisibleRootItem()->childCount(); i++) {
            QTreeWidgetItem *item = ranges->invisibleRootItem()->child(i);
            int itemRnum = item->data(5, Qt::DisplayRole).toInt();
            if (itemRnum >= rnum) {
                item->setData(5, Qt::DisplayRole, itemRnum - 1);
            }
        }
    }
}

void
LTPage::defaultClicked()
{
    if (ranges->currentItem()) {
        int rnum = ranges->currentItem()->data(5, Qt::DisplayRole).toInt();
        HrZoneRange current = hrZones->getHrZoneRange(rnum);

        // unbold
        QFont font;
        font.setWeight(QFont::Normal);
        ranges->currentItem()->setFont(0, font);
        ranges->currentItem()->setFont(1, font);
        ranges->currentItem()->setFont(2, font);
        ranges->currentItem()->setFont(3, font);
        ranges->currentItem()->setFont(4, font);

        // set the range to use defaults on the scheme page
        hrZones->setScheme(schemePage->getScheme());
        hrZones->setHrZonesFromLT(rnum);

        // hide the default button since we are now using defaults
        defaultButton->hide();

        // update the zones display
        rangeSelectionChanged();
    }
}


void
LTPage::rangeChanged
(const QModelIndex &modelIndex)
{
    hrZones->setScheme(schemePage->getScheme());
    QModelIndex rnumIdx = modelIndex.siblingAtColumn(5);
    int rnum = rnumIdx.data(Qt::DisplayRole).toInt();
    switch (modelIndex.column()) {
    case 0:
        hrZones->setStartDate(rnum, modelIndex.data().toDate());
        break;
    case 1:
        hrZones->setLT(rnum, modelIndex.data().toInt());
        break;
    case 2:
        hrZones->setAeT(rnum, modelIndex.data().toInt());
        break;
    case 3:
        hrZones->setRestHr(rnum, modelIndex.data().toInt());
        break;
    case 4:
        hrZones->setMaxHr(rnum, modelIndex.data().toInt());
        break;
    default:
        break;
    }
    rangeSelectionChanged();
}


void
LTPage::rangeSelectionChanged()
{
    active = true;

    // wipe away current contents of zones
    zones->clear();

    // fill with current details
    if (ranges->currentItem()) {
        int rnum = ranges->currentItem()->data(5, Qt::DisplayRole).toInt();
        HrZoneRange current = hrZones->getHrZoneRange(rnum);

        if (current.hrZonesSetFromLT) {
            // reapply the scheme in case it has been changed
            hrZones->setScheme(schemePage->getScheme());
            hrZones->setHrZonesFromLT(rnum);
            current = hrZones->getHrZoneRange(rnum);

            defaultButton->hide();
        } else {
            defaultButton->show();
        }

        for (int i=0; i< current.zones.count(); i++) {
            QTreeWidgetItem *add = new QTreeWidgetItem(zones->invisibleRootItem());
            add->setFlags(add->flags() | Qt::ItemIsEditable);

            add->setData(0, Qt::DisplayRole, current.zones[i].name);
            add->setData(1, Qt::DisplayRole, current.zones[i].desc);
            add->setData(2, Qt::DisplayRole, current.zones[i].lo);
            add->setData(3, Qt::DisplayRole, current.zones[i].trimp);
        }
    }

    active = false;
}


void
LTPage::addZoneClicked()
{
    // no range selected
    if (!ranges->currentItem()) return;

    active = true;
    int index = zones->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    zones->invisibleRootItem()->insertChild(index, add);

    // Short
    QString text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setData(0, Qt::DisplayRole, text);

    // long
    text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setData(1, Qt::DisplayRole, text);

    add->setData(2, Qt::DisplayRole, 0);
    add->setData(2, Qt::DisplayRole, 0);

    active = false;

    zonesChanged();
}

void
LTPage::deleteZoneClicked()
{
    // range and zone must be selected
    if (ranges->currentItem() == nullptr || zones->currentItem() == nullptr) {
        return;
    }

    active = true;
    int index = zones->invisibleRootItem()->indexOfChild(zones->currentItem());
    delete zones->invisibleRootItem()->takeChild(index);
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
            int rnum = ranges->currentItem()->data(5, Qt::DisplayRole).toInt();
            HrZoneRange current = hrZones->getHrZoneRange(rnum);

            // embolden that range on the list to show it has been edited
            QFont font;
            font.setWeight(QFont::Black);
            ranges->currentItem()->setFont(0, font);
            ranges->currentItem()->setFont(1, font);
            ranges->currentItem()->setFont(2, font);
            ranges->currentItem()->setFont(3, font);
            ranges->currentItem()->setFont(4, font);

            // show the default button to undo
            defaultButton->show();

            // we manually edited so save in full
            current.hrZonesSetFromLT = false;

            // create the new zoneinfos for this range
            QList<HrZoneInfo> zoneinfos;
            for (int i=0; i< zones->invisibleRootItem()->childCount(); i++) {
                QTreeWidgetItem *item = zones->invisibleRootItem()->child(i);
                zoneinfos << HrZoneInfo(item->data(0, Qt::DisplayRole).toString(),
                                        item->data(1, Qt::DisplayRole).toString(),
                                        item->data(2, Qt::DisplayRole).toInt(),
                                        0,
                                        item->data(3, Qt::DisplayRole).toDouble());
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
            hrZones->setHrZoneRange(rnum, current);
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

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);
    actionButtons->setMaxViewItems(10);

    fromDelegate.setRange(0, 1000);
    fromDelegate.setSingleStep(1);
    fromDelegate.setSuffix(tr("%"));
    fromDelegate.setShowSuffixOnEdit(true);
    fromDelegate.setShowSuffixOnDisplay(true);

    scheme = new QTreeWidget;
    scheme->headerItem()->setText(0, tr("Short"));
    scheme->headerItem()->setText(1, tr("Long"));
    scheme->headerItem()->setText(2, tr("Percent of CV"));
    scheme->setColumnCount(3);
    scheme->setItemDelegateForColumn(2, &fromDelegate);
    basicTreeWidgetStyle(scheme);

    // setup list
    for (int i=0; i< paceZones->getScheme().nzones_default; i++) {
        QTreeWidgetItem *add = new QTreeWidgetItem(scheme->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // tab name
        add->setData(0, Qt::DisplayRole, paceZones->getScheme().zone_default_name[i]);
        // field name
        add->setData(1, Qt::DisplayRole, paceZones->getScheme().zone_default_desc[i]);

        // low
        add->setData(2, Qt::DisplayRole, paceZones->getScheme().zone_default[i]);
    }

    mainLayout->addWidget(scheme);
    mainLayout->addWidget(actionButtons);

    // button connect
    actionButtons->defaultConnect(scheme);
    connect(actionButtons, &ActionButtonBox::addRequested, this, &PaceSchemePage::addClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &PaceSchemePage::deleteClicked);
}


void
PaceSchemePage::addClicked()
{
    int index = scheme->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    scheme->invisibleRootItem()->insertChild(index, add);

    // Short
    QString text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setData(0, Qt::DisplayRole, text);

    // long
    text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setData(1, Qt::DisplayRole, text);

    add->setData(2, Qt::DisplayRole, 100);
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
        add.name = scheme->invisibleRootItem()->child(i)->data(0, Qt::DisplayRole).toString();
        add.desc = scheme->invisibleRootItem()->child(i)->data(1, Qt::DisplayRole).toString();
        add.lo = scheme->invisibleRootItem()->child(i)->data(2, Qt::DisplayRole).toInt();
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

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10 *dpiXFactor);

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);

    ActionButtonBox *zoneActionButtons = new ActionButtonBox(ActionButtonBox::AddDeleteGroup);
    zoneActionButtons->setMaxViewItems(10);
    defaultButton = zoneActionButtons->addButton(tr("Def"), ActionButtonBox::Right);
    defaultButton->hide();

    dateDelegate.setCalendarPopup(true);

    cvDelegate.setFormat("mm:ss");
    cvDelegate.setTimeRange(QTime(0, 0, 0), QTime(0, 20, 0));
    cvDelegate.setSuffix(paceZones->paceUnits(metricPace));
    cvDelegate.setShowSuffixOnEdit(true);
    cvDelegate.setShowSuffixOnDisplay(true);

    aetDelegate.setFormat("mm:ss");
    aetDelegate.setTimeRange(QTime(0, 0, 0), QTime(0, 20, 0));
    aetDelegate.setSuffix(paceZones->paceUnits(metricPace));
    aetDelegate.setShowSuffixOnEdit(true);
    aetDelegate.setShowSuffixOnDisplay(true);

    ranges = new QTreeWidget;
    ranges->headerItem()->setText(0, tr("Start Date"));
    ranges->headerItem()->setText(1, tr("Critical Velocity"));
    ranges->headerItem()->setText(2, tr("Aerobic Threshold"));
    ranges->headerItem()->setText(3, "_rnum");
    ranges->setColumnCount(4);
    ranges->setColumnHidden(3, true);
    ranges->setItemDelegateForColumn(0, &dateDelegate);
    ranges->setItemDelegateForColumn(1, &cvDelegate);
    ranges->setItemDelegateForColumn(2, &aetDelegate);
    basicTreeWidgetStyle(ranges);

    // setup list of ranges
    for (int i = 0; i < paceZones->getRangeSize(); i++) {
        QTreeWidgetItem *add = new QTreeWidgetItem(ranges->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // Embolden ranges with manually configured zones
        QFont font;
        font.setWeight(paceZones->getZoneRange(i).zonesSetFromCV ?
                                        QFont::Normal : QFont::Black);

        // date
        add->setData(0, Qt::DisplayRole, paceZones->getStartDate(i));
        add->setFont(0, font);

        // CV
        add->setData(1, Qt::DisplayRole, paceZones->kphToPaceTime(paceZones->getCV(i), metricPace));
        add->setFont(1, font);

        // AeT
        add->setData(2, Qt::DisplayRole, paceZones->kphToPaceTime(paceZones->getAeT(i), metricPace));
        add->setFont(2, font);

        add->setData(3, Qt::DisplayRole, i);
    }
    ranges->sortByColumn(3, Qt::DescendingOrder);

    zoneFromDelegate.setTimeRange(QTime(0, 0, 0), QTime(0, 20, 0));
    zoneFromDelegate.setFormat("mm:ss");
    zoneFromDelegate.setSuffix(paceZones->paceUnits(metricPace));
    zoneFromDelegate.setShowSuffixOnEdit(true);
    zoneFromDelegate.setShowSuffixOnDisplay(true);

    zones = new QTreeWidget;
    zones->headerItem()->setText(0, tr("Short"));
    zones->headerItem()->setText(1, tr("Long"));
    zones->headerItem()->setText(2, tr("From"));
    zones->setColumnCount(3);
    zones->setItemDelegateForColumn(2, &zoneFromDelegate);
    basicTreeWidgetStyle(zones);

    mainLayout->addWidget(ranges);
    mainLayout->addWidget(actionButtons);
    mainLayout->addWidget(zones);
    mainLayout->addWidget(zoneActionButtons);

    // button connect
    actionButtons->defaultConnect(ranges);
    connect(actionButtons, &ActionButtonBox::addRequested, this, &CVPage::addClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &CVPage::deleteClicked);
    zoneActionButtons->defaultConnect(zones);
    connect(zoneActionButtons, &ActionButtonBox::addRequested, this, &CVPage::addZoneClicked);
    connect(zoneActionButtons, &ActionButtonBox::deleteRequested, this, &CVPage::deleteZoneClicked);
    connect(defaultButton, SIGNAL(clicked()), this, SLOT(defaultClicked()));

    connect(ranges, SIGNAL(itemSelectionChanged()), this, SLOT(rangeSelectionChanged()));
    connect(ranges->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QList<int>&)),
            this, SLOT(rangeChanged(const QModelIndex&, const QModelIndex&, const QList<int>&)));
    connect(zones, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(zonesChanged()));

    if (ranges->invisibleRootItem()->childCount() > 0) {
        ranges->setCurrentItem(ranges->invisibleRootItem()->child(0));
    }
}


void
CVPage::addClicked()
{
    // get current scheme
    paceZones->setScheme(schemePage->getScheme());

    QDate date = QDate::currentDate();
    QTime cv;
    QTime aet;

    QTreeWidgetItem *sourceItem = nullptr;
    if (ranges->currentItem() != nullptr) {
        sourceItem = ranges->currentItem();
    } else if (ranges->invisibleRootItem()->childCount() > 0) {
        sourceItem = ranges->invisibleRootItem()->child(0);
    }

    if (sourceItem != nullptr) {
        cv = sourceItem->data(1, Qt::DisplayRole).toTime();
        aet = sourceItem->data(2, Qt::DisplayRole).toTime();
    } else {
        if (paceZones->isSwim()) {
            if (metricPace) {
                cv = QTime(0, 1, 36);
                aet = QTime(0, 1, 38);
            } else {
                cv = QTime(0, 1, 28);
                aet = QTime(0, 1, 30);
            }
        } else {
            if (metricPace) {
                cv = QTime(0, 4, 0);
                aet = QTime(0, 4, 27);
            } else {
                cv = QTime(0, 6, 26);
                aet = QTime(0, 7, 9);
            }
        }
    }

    QDialog dialog;
    dialog.setWindowTitle(tr("New range"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(  QDialogButtonBox::Apply
                                                       | QDialogButtonBox::Discard);
    connect(buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), &dialog, SLOT(accept()));
    connect(buttonBox->button(QDialogButtonBox::Discard), SIGNAL(clicked()), &dialog, SLOT(reject()));

    QDateEdit *dateEdit = new QDateEdit(date);
    dateEdit->setCalendarPopup(true);

    QTimeEdit *cvEdit = new QTimeEdit();
    cvEdit->setMinimumTime(QTime(0, 0, 0));
    cvEdit->setMaximumTime(QTime(0, 20, 0));
    cvEdit->setDisplayFormat("mm:ss '" + paceZones->paceUnits(metricPace) + "'");
    cvEdit->setTime(cv);

    QTimeEdit *aetEdit = new QTimeEdit();
    aetEdit->setMinimumTime(QTime(0, 0, 0));
    aetEdit->setMaximumTime(QTime(0, 20, 0));
    aetEdit->setDisplayFormat("mm:ss '" + paceZones->paceUnits(metricPace) + "'");
    aetEdit->setTime(aet);

    QFormLayout *form = newQFormLayout(&dialog);
    form->addRow(tr("Start Date"), dateEdit);
    form->addRow(tr("Critical Velocity (CV)"), cvEdit);
    form->addRow(tr("Aerobic Threshold (AeT)"), aetEdit);
    form->addItem(new QSpacerItem(1, 30 * dpiYFactor));
    form->addRow(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        date = dateEdit->date();
        cv = cvEdit->time();
        aet = aetEdit->time();
    } else {
        return;
    }

    int rnum = paceZones->addZoneRange(date, paceZones->kphFromTime(cv, metricPace), paceZones->kphFromTime(aet, metricPace));
    for (int i = 0; i < ranges->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *item = ranges->invisibleRootItem()->child(i);
        int itemRnum = item->data(3, Qt::DisplayRole).toInt();
        if (itemRnum >= rnum) {
            item->setData(3, Qt::DisplayRole, itemRnum + 1);
        }
    }

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem(ranges->invisibleRootItem());
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    add->setData(0, Qt::DisplayRole, date);
    add->setData(1, Qt::DisplayRole, cv);
    add->setData(2, Qt::DisplayRole, aet);
    add->setData(3, Qt::DisplayRole, rnum);

    ranges->sortByColumn(3, Qt::DescendingOrder);
    ranges->setCurrentItem(add);
}


void
CVPage::deleteClicked()
{
    if (ranges->currentItem()) {
        int rnum = ranges->currentItem()->data(3, Qt::DisplayRole).toInt();
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        delete ranges->invisibleRootItem()->takeChild(index);
        paceZones->deleteRange(rnum);
        for (int i = 0; i < ranges->invisibleRootItem()->childCount(); ++i) {
            QTreeWidgetItem *item = ranges->invisibleRootItem()->child(i);
            int itemRnum = item->data(3, Qt::DisplayRole).toInt();
            if (itemRnum >= rnum) {
                item->setData(3, Qt::DisplayRole, itemRnum - 1);
            }
        }
    }
}

void
CVPage::defaultClicked()
{
    if (ranges->currentItem()) {
        int rnum = ranges->currentItem()->data(3, Qt::DisplayRole).toInt();
        PaceZoneRange current = paceZones->getZoneRange(rnum);

        // unbold
        QFont font;
        font.setWeight(QFont::Normal);
        ranges->currentItem()->setFont(0, font);
        ranges->currentItem()->setFont(1, font);
        ranges->currentItem()->setFont(2, font);

        // set the range to use defaults on the scheme page
        paceZones->setScheme(schemePage->getScheme());
        paceZones->setZonesFromCV(rnum);

        // hide the default button since we are now using defaults
        defaultButton->hide();

        // update the zones display
        rangeSelectionChanged();
    }
}

void
CVPage::rangeChanged
(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    Q_UNUSED(bottomRight)

    if (   roles.length() > 0
        && ! roles.contains(Qt::DisplayRole)
        && ! roles.contains(Qt::EditRole)) {
        return;
    }
    paceZones->setScheme(schemePage->getScheme());
    QModelIndex rnumIdx = topLeft.siblingAtColumn(3);
    int rnum = rnumIdx.data(Qt::DisplayRole).toInt();
    switch (topLeft.column()) {
    case 0:
        paceZones->setStartDate(rnum, topLeft.data(Qt::DisplayRole).toDate());
        break;
    case 1:
        paceZones->setCV(rnum, paceZones->kphFromTime(topLeft.data(Qt::DisplayRole).toTime(), metricPace));
        break;
    case 2:
        paceZones->setAeT(rnum, paceZones->kphFromTime(topLeft.data(Qt::DisplayRole).toTime(), metricPace));
        break;
    default:
        break;
    }
}

void
CVPage::rangeSelectionChanged()
{
    active = true;

    // wipe away current contents of zones
    zones->clear();

    // fill with current details
    if (ranges->currentItem()) {
        int rnum = ranges->currentItem()->data(3, Qt::DisplayRole).toInt();
        PaceZoneRange current = paceZones->getZoneRange(rnum);

        if (current.zonesSetFromCV) {
            // reapply the scheme in case it has been changed
            paceZones->setScheme(schemePage->getScheme());
            paceZones->setZonesFromCV(rnum);
            current = paceZones->getZoneRange(rnum);

            defaultButton->hide();

        } else defaultButton->show();

        for (int i=0; i< current.zones.count(); i++) {
            QTreeWidgetItem *add = new QTreeWidgetItem(zones->invisibleRootItem());
            add->setFlags(add->flags() | Qt::ItemIsEditable);

            // tab name
            add->setData(0, Qt::DisplayRole, current.zones[i].name);
            // field name
            add->setData(1, Qt::DisplayRole, current.zones[i].desc);

            // low
            add->setData(2, Qt::DisplayRole, QTime::fromString(paceZones->kphToPaceString(current.zones[i].lo, metricPace), "mm:ss"));
        }
    }

    active = false;
}

void
CVPage::addZoneClicked()
{
    // no range selected
    if (!ranges->currentItem()) return;

    active = true;
    int index = zones->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);
    zones->invisibleRootItem()->insertChild(index, add);

    // Short
    QString text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setData(0, Qt::DisplayRole, text);

    // long
    text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setData(1, Qt::DisplayRole, text);
    active = false;

    zonesChanged();
}

void
CVPage::deleteZoneClicked()
{
    // no range selected
    if (ranges->currentItem() == nullptr || zones->currentItem() == nullptr) {
        return;
    }

    active = true;
    int index = zones->invisibleRootItem()->indexOfChild(zones->currentItem());
    delete zones->invisibleRootItem()->takeChild(index);
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
            int rnum = ranges->currentItem()->data(3, Qt::DisplayRole).toInt();
            PaceZoneRange current = paceZones->getZoneRange(rnum);

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
                QTime time = item->data(2, Qt::DisplayRole).toTime();
                double kph = time == QTime(0,0,0) ? 0.0 : paceZones->kphFromTime(time, metricPace);
                zoneinfos << PaceZoneInfo(item->data(0, Qt::DisplayRole).toString(),
                                          item->data(1, Qt::DisplayRole).toString(),
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
            paceZones->setZoneRange(rnum, current);
        }
    }
}


//
// Season Editor
//
SeasonsPage::SeasonsPage(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    QGridLayout *mainLayout = new QGridLayout(this);
    QFormLayout *editLayout = newQFormLayout();

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

        if (season.getType() == Season::temporary) continue;

        add = new QTreeWidgetItem(seasons->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // tab name
        add->setText(0, season.getName());
        // type
        add->setText(1, Season::types[static_cast<int>(season.getType())]);
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
    addSeason.setAbsoluteStart(fromEdit->date());
    addSeason.setAbsoluteEnd(toEdit->date());
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
        array[i].setAbsoluteStart(QDate::fromString(item->text(2), "ddd MMM d, yyyy"));
        array[i].setAbsoluteEnd(QDate::fromString(item->text(3), "ddd MMM d, yyyy"));
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
    RideAutoImportRule config;
    QStringList ruleDescriptions = config.getRuleDescriptions();

    QGridLayout *mainLayout = new QGridLayout(this);

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::UpDownGroup | ActionButtonBox::AddDeleteGroup);

    dirDelegate.setMaxWidth(400 * dpiXFactor);
    dirDelegate.setPlaceholderText(tr("Enter a directory"));

    ruleDelegate.addItems(config.getRuleDescriptions());

    fields = new QTreeWidget;
    fields->headerItem()->setText(0, tr("Directory"));
    fields->headerItem()->setText(1, tr("Import Rule"));
    fields->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    fields->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    fields->setColumnCount(2);
    fields->setSelectionMode(QAbstractItemView::SingleSelection);
    fields->setUniformRowHeights(true);
    fields->setIndentation(0);
    fields->setAlternatingRowColors(true);
    fields->setItemDelegateForColumn(0, &dirDelegate);
    fields->setItemDelegateForColumn(1, &ruleDelegate);
    fields->setEditTriggers(  QAbstractItemView::DoubleClicked
                            | QAbstractItemView::SelectedClicked
                            | QAbstractItemView::AnyKeyPressed);
    fields->setCurrentItem(fields->invisibleRootItem()->child(0));

    mainLayout->addWidget(fields, 0,0);
    mainLayout->addWidget(actionButtons, 1,0);

    context->athlete->autoImportConfig->readConfig();
    QList<RideAutoImportRule> rules = context->athlete->autoImportConfig->getConfig();
    int index = 0;
    foreach (RideAutoImportRule rule, rules) {
        QTreeWidgetItem *add = new QTreeWidgetItem;
        fields->invisibleRootItem()->insertChild(index, add);
        add->setFlags(add->flags() | Qt::ItemIsEditable);
        add->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
        add->setTextAlignment(1, Qt::AlignLeft | Qt::AlignVCenter);
        add->setData(0, Qt::DisplayRole, rule.getDirectory());
        add->setData(1, Qt::DisplayRole, rule.getImportRule());
        index++;
    }

    // connect up slots
    actionButtons->defaultConnect(fields);
    connect(actionButtons, &ActionButtonBox::upRequested, this, &AutoImportPage::upClicked);
    connect(actionButtons, &ActionButtonBox::downRequested, this, &AutoImportPage::downClicked);
    connect(actionButtons, &ActionButtonBox::addRequested, this, &AutoImportPage::addClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &AutoImportPage::deleteClicked);
}


void
AutoImportPage::upClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
        if (index == 0)
            return; // its at the top already

        //movin on up!
        QTreeWidgetItem* moved = fields->invisibleRootItem()->takeChild(index);
        fields->invisibleRootItem()->insertChild(index - 1, moved);
        fields->setCurrentItem(moved);
    }
}

void
AutoImportPage::downClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
        if (index == (fields->invisibleRootItem()->childCount() - 1))
            return; // its at the bottom already

        QTreeWidgetItem* moved = fields->invisibleRootItem()->takeChild(index);
        fields->invisibleRootItem()->insertChild(index + 1, moved);
        fields->setCurrentItem(moved);
    }
}


void
AutoImportPage::addClicked()
{
    RideAutoImportRule config;
    QStringList ruleDescriptions = config.getRuleDescriptions();
    int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
    if (index < 0) {
        index = 0;
    }

    QTreeWidgetItem *add = new QTreeWidgetItem;
    fields->invisibleRootItem()->insertChild(index, add);
    add->setFlags(add->flags() | Qt::ItemIsEditable);
    add->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
    add->setTextAlignment(1, Qt::AlignLeft | Qt::AlignVCenter);
    add->setData(0, Qt::DisplayRole, QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    add->setData(1, Qt::DisplayRole, 0);
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
    for(int i = 0; i < fields->invisibleRootItem()->childCount(); i++) {
        RideAutoImportRule rule;
        rule.setDirectory(fields->invisibleRootItem()->child(i)->data(0, Qt::DisplayRole).toString());
        rule.setImportRule(fields->invisibleRootItem()->child(i)->data(1, Qt::DisplayRole).toInt());
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
AutoImportPage::addRuleTypes(QComboBox *p)
{
    RideAutoImportRule config;
    p->addItems(config.getRuleDescriptions());
}
