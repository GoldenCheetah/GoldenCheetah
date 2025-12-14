/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2024 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "NewAthleteWizard.h"
#include "MainWindow.h"
#include "Zones.h"
#include "HrZones.h"
#include "PaceZones.h"
#include "GcUpgrade.h"
#include "Athlete.h"
#include "Colors.h"

#include <QFormLayout>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>


////////////////////////////////////////////////////////////////////////////////
// NewAthleteWizard

NewAthleteWizard::NewAthleteWizard
(const QDir &home, QWidget *parent)
: QWizard(parent), home(home)
{
    setWindowTitle(tr("Welcome to Goldencheetah"));
    setMinimumSize(800 * dpiXFactor, 650 * dpiYFactor);

#ifdef Q_OS_MAC
    setWizardStyle(QWizard::MacStyle);
    QPixmap bgImage = QPixmap(":/images/gc-blank.png");
    setPixmap(QWizard::BackgroundPixmap, bgImage);
#else
    setWizardStyle(QWizard::ModernStyle);
    QPixmap bgImage = QPixmap(":/images/gc.png");
    setPixmap(QWizard::LogoPixmap, bgImage.scaledToHeight(120 * dpiYFactor, Qt::SmoothTransformation));
#endif

    setPage(PageUser, new NewAthletePageUser(home));
    setPage(PagePerformance, new NewAthletePagePerformance());

    setStartId(PageUser);
}


QString
NewAthleteWizard::getName
() const
{
    return field("user.name").toString().trimmed();
}


void
NewAthleteWizard::done
(int result)
{
    if (result == QDialog::Accepted) {
        bool useMetricUnits = (field("user.unit") == 0);
        QString name = field("user.name").toString().trimmed();
        if (! home.exists(name)) {
            if (home.mkdir(name)) {
                QDir athleteDir = QDir(home.canonicalPath() + '/' + name);
                AthleteDirectoryStructure *athleteHome = new AthleteDirectoryStructure(athleteDir);

                // create the sub-Dirs here
                athleteHome->createAllSubdirs();

                // new Athlete/new Directories - no Upgrade required
                appsettings->initializeQSettingsNewAthlete(home.canonicalPath(), name);
                appsettings->setCValue(name, GC_UPGRADE_FOLDER_SUCCESS, true);

                // set the version under which the Athlete is created - to avoid unneccary upgrade execution
                appsettings->setCValue(name, GC_VERSION_USED, QVariant(VERSION_LATEST));

                // nice sidebars please!
                appsettings->setCValue(name, GC_QSETTINGS_ATHLETE_LAYOUT + QString("splitter/LTM/hide"), true);
                appsettings->setCValue(name, GC_QSETTINGS_ATHLETE_LAYOUT + QString("splitter/LTM/hide/0"), false);
                appsettings->setCValue(name, GC_QSETTINGS_ATHLETE_LAYOUT + QString("splitter/LTM/hide/1"), false);
                appsettings->setCValue(name, GC_QSETTINGS_ATHLETE_LAYOUT + QString("splitter/LTM/hide/2"), false);
                appsettings->setCValue(name, GC_QSETTINGS_ATHLETE_LAYOUT + QString("splitter/LTM/hide/3"), true);
                appsettings->setCValue(name, GC_QSETTINGS_ATHLETE_LAYOUT + QString("splitter/analysis/hide"), true);
                appsettings->setCValue(name, GC_QSETTINGS_ATHLETE_LAYOUT + QString("splitter/analysis/hide/0"), false);
                appsettings->setCValue(name, GC_QSETTINGS_ATHLETE_LAYOUT + QString("splitter/analysis/hide/1"), true);
                appsettings->setCValue(name, GC_QSETTINGS_ATHLETE_LAYOUT + QString("splitter/analysis/hide/2"), false);
                appsettings->setCValue(name, GC_QSETTINGS_ATHLETE_LAYOUT + QString("splitter/analysis/hide/3"), true);
                appsettings->setCValue(name, GC_QSETTINGS_ATHLETE_LAYOUT + QString("splitter/train/hide"), true);
                appsettings->setCValue(name, GC_QSETTINGS_ATHLETE_LAYOUT + QString("splitter/train/hide/0"), false);
                appsettings->setCValue(name, GC_QSETTINGS_ATHLETE_LAYOUT + QString("splitter/train/hide/1"), false);
                appsettings->setCValue(name, GC_QSETTINGS_ATHLETE_LAYOUT + QString("splitter/train/hide/2"), false);
                appsettings->setCValue(name, GC_QSETTINGS_ATHLETE_LAYOUT + QString("splitter/train/hide/3"), false);

                // lets setup!
                if (useMetricUnits) {
                    appsettings->setValue(GC_UNIT, GC_UNIT_METRIC);
                } else {
                    appsettings->setValue(GC_UNIT, GC_UNIT_IMPERIAL);
                }

                appsettings->setCValue(name, GC_DOB, field("user.dob").toDate());
                appsettings->setCValue(name, GC_WEIGHT, field("user.weight").toDouble() * (useMetricUnits ? 1.0 : KG_PER_LB));
                appsettings->setCValue(name, GC_HEIGHT, field("user.height").toDouble() * (useMetricUnits ? 1.0 / 100.0 : CM_PER_INCH / 100.0));
                appsettings->setCValue(name, GC_WBALTAU, field("perf.wbaltau").toInt());
                appsettings->setCValue(name, GC_SEX, field("user.sex").toInt());
                appsettings->setCValue(name, GC_BIO, field("user.bio").toString());

                QString avatarFilename = field("user.avatar").toString();
                if (! avatarFilename.isEmpty()) {
                    QPixmap avatar(avatarFilename);
                    int s = std::min(avatar.width(), avatar.height());
                    avatar = avatar.copy((avatar.width() - s) / 2, (avatar.height() - s) / 2, s, s);
                    avatar.scaledToHeight(140, Qt::SmoothTransformation).save(athleteHome->config().canonicalPath() + "/" + "avatar.png", "PNG");
                }

                // Setup Power Zones
                Zones zones;
                zones.addZoneRange(field("user.dob").toDate(),
                                   field("perf.cp").toInt(),
                                   0,
                                   field("perf.cp").toInt(),
                                   field("perf.w").toInt(),
                                   field("perf.pmax").toInt());
                zones.write(athleteHome->config().canonicalPath());

                // HR Zones too!
                HrZones hrzones;
                hrzones.addHrZoneRange(field("user.dob").toDate(),
                                       field("perf.lthr").toInt(),
                                       0,
                                       field("perf.resthr").toInt(),
                                       field("perf.maxhr").toInt());
                hrzones.write(athleteHome->config().canonicalPath());

                // Pace Zones for Run
                PaceZones rnPaceZones(false);
                rnPaceZones.addZoneRange(field("user.dob").toDate(),
                                         rnPaceZones.kphFromTime(field("perf.cvRn").toTime(), useMetricUnits),
                                         0);
                rnPaceZones.write(athleteHome->config().canonicalPath());

                // Pace Zones for Run
                PaceZones swPaceZones(true);
                swPaceZones.addZoneRange(field("user.dob").toDate(),
                                         swPaceZones.kphFromTime(field("perf.cvSw").toTime(), useMetricUnits),
                                         0);
                swPaceZones.write(athleteHome->config().canonicalPath());

                appsettings->syncQSettingsAllAthletes();

                // If template athlete was selected, copy xml files
                if (field("perf.template").toInt()) {
                    QDir templateDir = QDir(home.canonicalPath() + "/" + field("perf.template").toString());
                    AthleteDirectoryStructure *templateHome = new AthleteDirectoryStructure(templateDir);
                    foreach(QString fileName, templateHome->config().entryList(QStringList() << "*.xml", QDir::Files)) {
                        QFile::copy(templateHome->config().canonicalPath() + "/" + fileName,
                                    athleteHome->config().canonicalPath() + "/" + fileName);
                    }
                }
            } else {
                QMessageBox::critical(0, tr("Fatal Error"), tr("Can't create new directory ") + home.canonicalPath() + "/" + name, "OK");
            }
        } else {
            QMessageBox::critical(0, tr("Fatal Error"), tr("Athlete already exists ")  + name, "OK");
        }
    }
    QWizard::done(result);
}


////////////////////////////////////////////////////////////////////////////////
// NewAthletePageUser

NewAthletePageUser::NewAthletePageUser
(const QDir &home, QWidget *parent)
: QWizardPage(parent), home(home)
{
    setTitle(tr("Create a new athlete"));
    setSubTitle(tr("Specify basic information about the new athlete."));

    QPixmap avatar(":/images/noavatar.png");
    avatarButton = new QPushButton();
    avatarButton->setContentsMargins(0, 0, 0, 0);
    avatarButton->setFlat(true);
    avatarButton->setIcon(avatar.scaled(140, 140, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    avatarButton->setIconSize(QSize(140, 140));
    avatarButton->setFixedHeight(140);
    avatarButton->setFixedWidth(140);
    QRegion region(0, 0, avatarButton->width() - 1, avatarButton->height() - 1, QRegion::Ellipse);
    avatarButton->setMask(region);
    QLineEdit *avatarFilename = new QLineEdit(this);
    avatarFilename->setVisible(false);
    registerField("user.avatar", avatarFilename);

    name = new QLineEdit();
    name->setPlaceholderText(tr("Unique name (mandatory)"));
    registerField("user.name*", name);

    QLocale locale;

    dob = new QDateEdit();
    dob->setCalendarPopup(true);
    dob->setDisplayFormat(locale.dateFormat(QLocale::LongFormat));
    registerField("user.dob", dob);

    sex = new QComboBox();
    sex->addItem(tr("Male"));
    sex->addItem(tr("Female"));
    registerField("user.sex", sex);

    unitCombo = new QComboBox();
    unitCombo->addItem(tr("Metric"));
    unitCombo->addItem(tr("Imperial"));
    registerField("user.unit", unitCombo);

    weight = new QDoubleSpinBox();
    weight->setMaximum(600.0);
    weight->setMinimum(20.0);
    weight->setDecimals(1);
    weight->setSuffix(" " + (unitCombo->currentIndex() == 0 ? tr("kg") : tr("lb")));
    weight->setValue(75); // default
    registerField("user.weight", weight, "value", SIGNAL(valueChanged(double)));

    height = new QDoubleSpinBox();
    height->setMaximum(250.0);
    height->setMinimum(30.0);
    height->setDecimals(1);
    height->setSuffix(" " + (unitCombo->currentIndex() == 0 ? tr("cm") : tr("in")));
    height->setValue(175); // default
    registerField("user.height", height, "value", SIGNAL(valueChanged(double)));

    bio = new QTextEdit();
    registerField("user.bio", bio, "plainText", SIGNAL(textChanged()));

    connect(avatarButton, SIGNAL(clicked()), this, SLOT(chooseAvatar()));
    connect(name, SIGNAL(textChanged(const QString&)), this, SLOT(nameChanged()));
    connect(unitCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(unitChanged(int)));

    QFormLayout *form = newQFormLayout();
    form->addRow(tr("Athlete Name") + "<sup>*</sup>", name);
    form->addRow(tr("Date of Birth"), dob);
    form->addRow(tr("Sex"), sex);
    form->addRow(tr("Units"), unitCombo);
    form->addRow(tr("Weight"), weight);
    form->addRow(tr("Height"), height);
    form->addRow(tr("Bio"), bio);

    QVBoxLayout *avatarLayout = new QVBoxLayout();
    avatarLayout->addWidget(avatarButton);
    avatarLayout->addStretch();

    QHBoxLayout *dialogLayout = new QHBoxLayout(this);
    dialogLayout->addLayout(avatarLayout);
    dialogLayout->addSpacing(20 * dpiXFactor);
    dialogLayout->addLayout(form);

    allowNext = isComplete();
}


int
NewAthletePageUser::nextId
() const
{
    return NewAthleteWizard::PagePerformance;
}


bool
NewAthletePageUser::isComplete
() const
{
    QString trimmedName = name->text().trimmed();
    return ! trimmedName.isEmpty() && ! home.exists(trimmedName);
}


void
NewAthletePageUser::chooseAvatar()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Choose Picture"),
                            "", tr("Images") + " (*.png *.jpg *.bmp)");
    if (filename != "") {
        QPixmap avatar(filename);
        int s = std::min(avatar.width(), avatar.height());
        avatar = avatar.copy((avatar.width() - s) / 2, (avatar.height() - s) / 2, s, s);
        avatarButton->setIcon(avatar.scaledToHeight(140, Qt::SmoothTransformation));
        avatarButton->setIconSize(QSize(140, 140));
        setField("user.avatar", filename);
    }
}


void
NewAthletePageUser::nameChanged
()
{
    if (allowNext != isComplete()) {
        allowNext = ! allowNext;
        emit completeChanged();
    }
}


void
NewAthletePageUser::unitChanged
(int idx)
{
    if (idx == 0) {
        weight->setValue(weight->value() / LB_PER_KG);
        weight->setSuffix(" " + tr("kg"));

        height->setValue(height->value() * CM_PER_INCH);
        height->setSuffix(" " + tr("cm"));
    } else {
        weight->setValue(weight->value() * LB_PER_KG);
        weight->setSuffix(" " + tr("lb"));

        height->setValue(height->value() / CM_PER_INCH);
        height->setSuffix(" " + tr("in"));
    }
}


////////////////////////////////////////////////////////////////////////////////
// NewAthletePagePerformance

NewAthletePagePerformance::NewAthletePagePerformance
(QWidget *parent)
: QWizardPage(parent)
{
    setTitle(tr("Create a new athlete"));
    setSubTitle(tr("Provide performance-related data about the athlete. If unsure, it is usually fine to start with the defaults."));

    cp = new QSpinBox();
    cp->setMinimum(100); // juniors may average 100w for an hour, lower values might be seen for para-juniors (?)
    cp->setMaximum(500); // thats over 6w/kg for a 80kg rider, anything higher is physiologically unlikely
    cp->setSingleStep(5);      // for those that insist on using the spinners, make it a bit quicker
    cp->setSuffix(" " + tr("W"));
    cp->setValue(250);   // seems like a 'sensible' default for those that 'don't know' ?
    registerField("perf.cp", cp);

    w = new QSpinBox();
    w->setMinimum(0);
    w->setMaximum(100000);
    w->setSingleStep(100);
    w->setSuffix(" " + tr("J"));
    w->setValue(20000); // default to 20kj
    registerField("perf.w", w);

    pmax = new QSpinBox();
    pmax->setMinimum(0);
    pmax->setMaximum(3000);
    pmax->setSingleStep(5);
    pmax->setSuffix(" " + tr("W"));
    pmax->setValue(1000); // default to 1000W
    registerField("perf.pmax", pmax);

    wbaltau = new QSpinBox();
    wbaltau->setMinimum(30);
    wbaltau->setMaximum(1200);
    wbaltau->setSingleStep(10);
    wbaltau->setSuffix(" " + tr("s"));
    wbaltau->setValue(300); // default to 300s
    registerField("perf.wbaltau", wbaltau);

    resthr = new QSpinBox();
    resthr->setMinimum(30);
    resthr->setMaximum(100);
    resthr->setSingleStep(1);
    resthr->setSuffix(" " + tr("bpm"));
    resthr->setValue(60);
    registerField("perf.resthr", resthr);

    lthr = new QSpinBox();
    lthr->setMinimum(80);
    lthr->setMaximum(220);
    lthr->setSingleStep(1);
    lthr->setSuffix(" " + tr("bpm"));
    lthr->setValue(165);
    registerField("perf.lthr", lthr);

    maxhr = new QSpinBox();
    maxhr->setMinimum(150);
    maxhr->setMaximum(250);
    maxhr->setSingleStep(1);
    maxhr->setSuffix(" " + tr("bpm"));
    maxhr->setValue(190);
    registerField("perf.maxhr", maxhr);

    templateCombo = new QComboBox();
    templateCombo->addItem(tr("None")); // No template athlete, use default
    // get a list of all athletes
    QStringListIterator i(QDir(gcroot).entryList(QDir::Dirs | QDir::NoDotAndDotDot));
    while (i.hasNext()) {
        QString name = i.next();
        SKIP_QTWE_CACHE  // skip Folder Names created by QTWebEngine on Windows
        templateCombo->addItem(name);
    }
    templateCombo->setEnabled(templateCombo->count() > 1);
    registerField("perf.template", templateCombo);

    // cvRn default is a nice round number comparable to CP default
    cvRn = new QTimeEdit(QTime::fromString("04:00", "mm:ss"));
    cvRn->setMinimumTime(QTime::fromString("01:00", "mm:ss"));
    cvRn->setMaximumTime(QTime::fromString("20:00", "mm:ss"));
    cvRn->setDisplayFormat("mm:ss '" + (useMetricUnits ? tr("min/km") : tr("min/mile")) + "'");
    registerField("perf.cvRn", cvRn);

    // cvSw default is in 4-1 relation to cvRun
    cvSw = new QTimeEdit(QTime::fromString("01:36", "mm:ss"));
    cvSw->setMinimumTime(QTime::fromString("01:00", "mm:ss"));
    cvSw->setMaximumTime(QTime::fromString("20:00", "mm:ss"));
    cvSw->setDisplayFormat("mm:ss '" + (useMetricUnits ? tr("min/100m") : tr("min/100yd")) +"'");
    registerField("perf.cvSw", cvSw);

    QFormLayout *form = newQFormLayout(this);
    form->addRow(tr("Maximum Power (PMax)"), pmax);
    form->addRow(tr("Critical Power (CP) / FTP"), cp);
    form->addRow(tr("Anaerobic Capacity (W')"), w);
    form->addRow(tr("W'bal Tau"), wbaltau);
    form->addRow(tr("Resting Heartrate"), resthr);
    form->addRow(tr("Lactate Heartrate"), lthr);
    form->addRow(tr("Maximum Heartrate"), maxhr);
    form->addRow(tr("Critical Velocity (CV) Run"), cvRn);
    form->addRow(tr("Critical Velocity (CV) Swim"), cvSw);
    form->addRow(tr("Use athlete's perspectives as template"), templateCombo);
}


void
NewAthletePagePerformance::initializePage
()
{
    if (useMetricUnits != (field("user.unit").toInt() == 0)) {
        useMetricUnits = ! useMetricUnits;
        if (useMetricUnits) {
            PaceZones rnPaceZones(false);
            cvRn->setTime(QTime::fromString(rnPaceZones.kphToPaceString(rnPaceZones.kphFromTime(cvRn, false), true), "mm:ss"));
            cvRn->setDisplayFormat("mm:ss 'min/km'");

            PaceZones swPaceZones(true);
            cvSw->setTime(QTime::fromString(swPaceZones.kphToPaceString(swPaceZones.kphFromTime(cvSw, false), true), "mm:ss"));
            cvSw->setDisplayFormat("mm:ss 'min/100m'");
        } else {
            PaceZones rnPaceZones(false);
            cvRn->setTime(QTime::fromString(rnPaceZones.kphToPaceString(rnPaceZones.kphFromTime(cvRn, true), false), "mm:ss"));
            cvRn->setDisplayFormat("mm:ss 'min/mile'");

            PaceZones swPaceZones(true);
            cvSw->setTime(QTime::fromString(swPaceZones.kphToPaceString(swPaceZones.kphFromTime(cvSw, true), false), "mm:ss"));
            cvSw->setDisplayFormat("mm:ss 'min/100yd'");
        }
    }
}


int
NewAthletePagePerformance::nextId
() const
{
    return NewAthleteWizard::Finalize;
}
