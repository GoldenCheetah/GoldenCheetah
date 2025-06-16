/*
 * Copyright (c) 2009 Eric Murray (ericm@lne.com)
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#include "ManualRideDialog.h"

#include <QMessageBox>
#include <QSvgRenderer>

#include "Context.h"
#include "Colors.h"
#include "Athlete.h"
#include "RideCache.h"
#include "RideItem.h"
#include "Settings.h"
#include "RideMetadata.h"
#include <string.h>
#include <errno.h>
#include <QtGui>
#include <cmath>
#include "Units.h"
#include "HelpWhatsThis.h"

#define MANDATORY " *"
#define ICON_COLOR QColor("#F79130")
#ifdef Q_OS_MAC
#define ICON_SIZE 250
#define ICON_MARGIN 0
#define ICON_TYPE QWizard::BackgroundPixmap
#else
#define ICON_SIZE 125
#define ICON_MARGIN 5
#define ICON_TYPE QWizard::LogoPixmap
#endif


QPixmap
svgAsColoredPixmap
(const QString &file, const QSize &size, int margin, const QColor &color)
{
    QSvgRenderer renderer(file);
    QPixmap pixmap(size.width(), size.height());
    pixmap.fill(Qt::transparent);

    QRectF renderRect(margin, margin, size.width() - 2 * margin, size.height() - 2 * margin);
    QPainter painter(&pixmap);
    renderer.render(&painter, renderRect);
    painter.end();

    QPainter recolorPainter(&pixmap);
    recolorPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    recolorPainter.fillRect(renderRect, color);
    recolorPainter.end();

    return pixmap;
}


////////////////////////////////////////////////////////////////////////////////
// ManualActivityWizard

ManualActivityWizard::ManualActivityWizard
(Context *context, QWidget *parent)
: QWizard(parent), context(context)
{
    setWindowTitle(tr("Manual Entry"));
    setMinimumSize(800 * dpiXFactor, 650 * dpiYFactor);
    setModal(true);

#ifdef Q_OS_MAC
    setWizardStyle(QWizard::MacStyle);
#else
    setWizardStyle(QWizard::ModernStyle);
#endif
    setPixmap(ICON_TYPE, svgAsColoredPixmap(":images/material/summit.svg", QSize(ICON_SIZE * dpiXFactor, ICON_SIZE * dpiYFactor), ICON_MARGIN * dpiXFactor, ICON_COLOR));

    setPage(PageBasics, new ManualActivityPageBasics(context));
    setPage(PageSpecifics, new ManualActivityPageSpecifics(context));

    setStartId(PageBasics);
}


void
ManualActivityWizard::done
(int result)
{
    int finalResult = result;
    if (result == QDialog::Accepted) {
        appsettings->setValue(GC_BIKESCOREDAYS, field("estimationDays").toInt());
        int eb = field("estimateBy").toInt();
        appsettings->setValue(GC_BIKESCOREMODE, eb == 0 ? "time" : (eb == 1 ? "dist" : "manual"));

        RideFile rideFile;

        QDateTime rideDateTime = QDateTime(field("activityDate").toDate(), field("activityTime").toTime());
        rideFile.setStartTime(rideDateTime);
        rideFile.setRecIntSecs(0.00);
        rideFile.setDeviceType("Manual");
        rideFile.setFileFormat("GoldenCheetah Json");

        QString sport = field("sport").toString().trimmed();
        rideFile.setTag("Sport", sport);
        if (field("subSport").toString().trimmed().size() > 0) {
            rideFile.setTag("SubSport", field("subSport").toString().trimmed());
        }
        if (field("workoutCode").toString().trimmed().size() > 0) {
            rideFile.setTag("Workout Code", field("workoutCode").toString().trimmed());
        }

        if ((sport == "Run" || sport == "Swim") && field("paceIntervals").toBool()) {
            QList<RideFilePoint*> points = field("laps").value<QList<RideFilePoint*>>();
            // get samples from Laps Editor, if available
            if (points.count() > 0) {
                rideFile.setRecIntSecs(1.00);
                for (RideFilePoint *point : points) {
                    rideFile.appendPoint(*point);
                }
                rideFile.fillInIntervals();
            }
        } else {
            double distance = field("realDistance").toDouble();
            if (distance > 0) {
                QMap<QString,QString> values;
                values.insert("value", QString("%1").arg(distance));
                rideFile.metricOverrides.insert("total_distance", values);
            }

            double seconds = field("realDuration").toDouble();
            if (seconds > 0) {
                QMap<QString,QString> values;
                values.insert("value", QString("%1").arg(seconds));
                rideFile.metricOverrides.insert("workout_time", values);
                rideFile.metricOverrides.insert("time_riding", values);
            }
        }
        if (field("averageHr").toInt() > 0) {
            QMap<QString,QString> values;
            values.insert("value", QString("%1").arg(field("averageHr").toInt()));
            rideFile.metricOverrides.insert("average_hr", values);
        }
        if (field("averageCadence").toInt() > 0) {
            QMap<QString,QString> values;
            values.insert("value", QString("%1").arg(field("averageCadence").toInt()));
            rideFile.metricOverrides.insert("average_cad", values);
        }
        if (field("averagePower").toInt() > 0) {
            QMap<QString,QString> values;
            values.insert("value", QString("%1").arg(field("averagePower").toInt()));
            rideFile.metricOverrides.insert("average_power", values);
        }

        if (field("work").toInt() > 0) {
            QMap<QString,QString> values;
            values.insert("value", QString("%1").arg(field("work").toInt()));
            rideFile.metricOverrides.insert("total_work", values);
        }
        if (field("bikeStress").toInt() > 0 && sport == "Bike") {
            QMap<QString,QString> values;
            values.insert("value", QString("%1").arg(field("bikeStress").toInt()));
            rideFile.metricOverrides.insert("coggan_tss", values);
        }
        if (field("bikeScore").toInt() > 0 && sport == "Bike") {
            QMap<QString,QString> values;
            values.insert("value", QString("%1").arg(field("bikeScore").toInt()));
            rideFile.metricOverrides.insert("skiba_bike_score", values);
        }
        if (field("swimScore").toInt() > 0 && sport == "Swim") {
            QMap<QString,QString> values;
            values.insert("value", QString("%1").arg(field("swimScore").toInt()));
            rideFile.metricOverrides.insert("swimscore", values);
        }
        if (field("triScore").toInt() > 0) {
            QMap<QString,QString> values;
            values.insert("value", QString("%1").arg(field("triScore").toInt()));
            rideFile.metricOverrides.insert("triscore", values);
        }

        // process linked defaults
        GlobalContext::context()->rideMetadata->setLinkedDefaults(&rideFile);

        // what should the filename be?
        QChar zero = QLatin1Char('0');
        QString basename = QString("%1_%2_%3_%4_%5_%6")
                                  .arg(rideDateTime.date().year(), 4, 10, zero)
                                  .arg(rideDateTime.date().month(), 2, 10, zero)
                                  .arg(rideDateTime.date().day(), 2, 10, zero)
                                  .arg(rideDateTime.time().hour(), 2, 10, zero)
                                  .arg(rideDateTime.time().minute(), 2, 10, zero)
                                  .arg(rideDateTime.time().second(), 2, 10, zero);
        QString filename = context->athlete->home->activities().canonicalPath() + "/" + basename + ".json";
        QFile out(filename);
        if (RideFileFactory::instance().writeRideFile(context, &rideFile, out, "json")) {
            // refresh metric db etc
            context->athlete->addRide(basename + ".json", true);
        } else {
            // rather than dance around renaming existing rides, this time we will let the user
            // work it out -- they may actually want to keep an existing ride, so we shouldn't
            // rename it silently.
            QMessageBox oops(QMessageBox::Critical, tr("Unable to save"),
                             tr("There is already an activity with the same start time or you do not have permissions to save a file."));
            oops.exec();
            finalResult = QDialog::Rejected;
        }
    }

    QWizard::done(finalResult);
}


////////////////////////////////////////////////////////////////////////////////
// ManualActivityPageBasics

ManualActivityPageBasics::ManualActivityPageBasics
(Context *context, QWidget *parent)
: QWizardPage(parent), context(context)
{
    setTitle(tr("General Information"));
    setSubTitle(tr("Some fields will appear only when relevant to the selected sport. Whenever possible, uploading a recording of your activity is preferred over creating it manually."));

    bool useMetricUnits = GlobalContext::context()->useMetricUnits;
    bool metricSwimPace = appsettings->value(this, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();

    QDateEdit *dateEdit = new QDateEdit();
    dateEdit->setMaximumDate(QDate::currentDate());
    dateEdit->setMinimumDate(QDate(2000, 1, 1));
    dateEdit->setCalendarPopup(true);

    QTimeEdit *timeEdit = new QTimeEdit();
    timeEdit->setDisplayFormat("hh:mm:ss");

    QLineEdit *sportEdit = new QLineEdit();

    QLineEdit *subSportEdit = new QLineEdit();

    QLineEdit *workoutCodeEdit = new QLineEdit();

    QSpinBox *averageHrEdit = new QSpinBox();
    averageHrEdit->setMinimum(0);
    averageHrEdit->setMaximum(250);
    averageHrEdit->setSuffix(" " + tr("bpm"));

    averagePowerLabel = new QLabel("Average Power");

    averagePowerEdit = new QSpinBox();
    averagePowerEdit->setMinimum(0);
    averagePowerEdit->setMaximum(2000);
    averagePowerEdit->setSuffix(" " + tr("W"));

    paceIntervals = new QCheckBox(tr("Pace intervals"));

    lapsEditor = new LapsEditorWidget();

    averageCadenceLabel = new QLabel(tr("Average Cadence"));

    averageCadenceEdit = new QSpinBox();
    averageCadenceEdit->setMinimum(0);
    averageCadenceEdit->setMaximum(500);
    averageCadenceEdit->setSuffix(" " + tr("rpm"));

    distanceLabel = new QLabel(tr("Distance"));

    distanceEdit = new QDoubleSpinBox();
    distanceEdit->setMinimum(0);
    distanceEdit->setMaximum(999);
    distanceEdit->setDecimals(2);
    distanceEdit->setSuffix(" " + (useMetricUnits ? tr("km") : tr("mi")));

    swimDistanceLabel = new QLabel(tr("Swim Distance"));

    swimDistanceEdit = new QSpinBox();
    swimDistanceEdit->setMinimum(0);
    swimDistanceEdit->setMaximum(100000);
    swimDistanceEdit->setSuffix(" " + (metricSwimPace ? tr("m") : tr("yd")));

    durationLabel = new QLabel(tr("Duration"));

    durationEdit = new QTimeEdit();
    durationEdit->setDisplayFormat("hh:mm:ss");

    // Set completer for Sport, SubSport and Workout Code fields
    RideMetadata *rideMetadata = GlobalContext::context()->rideMetadata;
    if (rideMetadata) {
        foreach (FieldDefinition field, rideMetadata->getFields()) {
            if (field.name == "Sport") {
                sportEdit->setCompleter(field.getCompleter(this, context->athlete->rideCache));
            } else if (field.name == "SubSport") {
                subSportEdit->setCompleter(field.getCompleter(this, context->athlete->rideCache));
            } else if (field.name == "Workout Code") {
                workoutCodeEdit->setCompleter(field.getCompleter(this, context->athlete->rideCache));
            }
        }
    }

    connect(sportEdit, &QLineEdit::editingFinished, this, &ManualActivityPageBasics::updateVisibility);
    connect(sportEdit, &QLineEdit::editingFinished, this, &ManualActivityPageBasics::sportsChanged);
    connect(paceIntervals, &QCheckBox::toggled, this, &ManualActivityPageBasics::updateVisibility);

    registerField("activityDate*", dateEdit);
    connect(paceIntervals, &QCheckBox::toggled, this, &ManualActivityPageBasics::updateVisibility);

    registerField("activityDate*", dateEdit);
    registerField("activityTime", timeEdit);
    registerField("sport*", sportEdit);
    registerField("subSport", subSportEdit);
    registerField("workoutCode", workoutCodeEdit);
    registerField("averageHr", averageHrEdit);
    registerField("averagePower", averagePowerEdit);
    registerField("paceIntervals", paceIntervals);
    registerField("laps", lapsEditor, "dataPoints", SIGNAL(editingFinished()));
    registerField("averageCadence", averageCadenceEdit);
    registerField("distance", distanceEdit, "value", SIGNAL(valueChanged(double)));
    registerField("swimDistance", swimDistanceEdit);
    registerField("duration", durationEdit);

    QFormLayout *form = newQFormLayout();
    form->addRow(tr("Date") + MANDATORY, dateEdit);
    form->addRow(tr("Time") + MANDATORY, timeEdit);
    form->addRow(tr("Sport") + MANDATORY, sportEdit);
    form->addRow(tr("Sub Sport"), subSportEdit);
    form->addRow(tr("Workout Code"), workoutCodeEdit);
    form->addRow(tr("Average Heartrate"), averageHrEdit);
    form->addRow(averagePowerLabel, averagePowerEdit);
    form->addRow(averageCadenceLabel, averageCadenceEdit);
    form->addRow("", paceIntervals);
    form->addRow(distanceLabel, distanceEdit);
    form->addRow(swimDistanceLabel, swimDistanceEdit);
    form->addRow(durationLabel, durationEdit);
    form->addRow(lapsEditor);

    QWidget *scrollWidget = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->addWidget(centerLayoutInWidget(form, false));
    scrollLayout->addWidget(lapsEditor);
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);

    QVBoxLayout *all = new QVBoxLayout();
    all->addWidget(scrollArea);
    setLayout(all);

    updateVisibility();
}


void
ManualActivityPageBasics::initializePage
()
{
    setField("activityDate", QDate::currentDate());
    setField("activityTime", QDateTime::currentDateTime().addSecs(-4 * 3600)); // 4 hours ago by default
}


int
ManualActivityPageBasics::nextId
() const
{
    return ManualActivityWizard::PageSpecifics;
}


void
ManualActivityPageBasics::updateVisibility
()
{
    bool useMetricUnits = GlobalContext::context()->useMetricUnits;
    bool showAveragePower = true;
    bool showAverageCadence = true;
    bool showDistance = true;
    bool showSwimDistance = false;
    bool showDuration = true;
    bool showLapsEditor = false;
    bool showPaceIntervals = false;
    QString sport = field("sport").toString().trimmed();

    if (sport == "") {
        // Hide all conditional fields
        showAveragePower = false;
        showAverageCadence = false;
        showDistance = false;
        showSwimDistance = false;
        showDuration = false;
        showLapsEditor = false;
        showPaceIntervals = false;
    } else if (sport == "Bike") {
        // Stick to defaults
    } else if (sport == "Run") {
        useMetricUnits = appsettings->value(this, GC_PACE, GlobalContext::context()->useMetricUnits).toBool();
        showPaceIntervals = true;
        showLapsEditor = paceIntervals->isChecked();
        showDistance = ! showLapsEditor;
        showDuration = ! showLapsEditor;
        lapsEditor->setSwim(false);
    } else if (sport == "Swim") {
        showAveragePower = false;
        showDistance = false;
        showPaceIntervals = true;
        showLapsEditor = paceIntervals->isChecked();
        showSwimDistance = ! showLapsEditor;
        showDuration = ! showLapsEditor;
        lapsEditor->setSwim(true);
    } else if (sport == "Row") {
        // Stick to defaults
    } else if (sport == "Ski") {
        showAveragePower = false;
        showAverageCadence = false;
    } else if (sport == "Gym") {
        showAveragePower = false;
        showAverageCadence = false;
        showDistance = false;
        showSwimDistance = false;
    } else {
        // Stick to defaults
    }

    paceIntervals->setVisible(showPaceIntervals);
    lapsEditor->setVisible(showLapsEditor);

    averageCadenceLabel->setVisible(showAverageCadence);
    averageCadenceEdit->setVisible(showAverageCadence);
    averagePowerLabel->setVisible(showAveragePower);
    averagePowerEdit->setVisible(showAveragePower);
    distanceLabel->setVisible(showDistance);
    distanceEdit->setVisible(showDistance);
    distanceEdit->setSuffix(" " + (useMetricUnits ? tr("km") : tr("mi")));
    swimDistanceLabel->setVisible(showSwimDistance);
    swimDistanceEdit->setVisible(showSwimDistance);
    durationLabel->setVisible(showDuration);
    durationEdit->setVisible(showDuration);
}


void
ManualActivityPageBasics::sportsChanged
()
{
    QString path(":images/material/summit.svg");
    QString sport = field("sport").toString().trimmed();
    if (sport == "Bike") {
        path = ":images/material/bike.svg";
    } else if (sport == "Run") {
        path = ":images/material/run.svg";
    } else if (sport == "Swim") {
        path = ":images/material/swim.svg";
    } else if (sport == "Row") {
        path = ":images/material/rowing.svg";
    } else if (sport == "Ski") {
        path = ":images/material/ski.svg";
    } else if (sport == "Gym") {
        path = ":images/material/weight-lifter.svg";
    } else if (! sport.isEmpty()) {
        path = ":images/material/torch.svg";
    }
    wizard()->setPixmap(ICON_TYPE, svgAsColoredPixmap(path, QSize(ICON_SIZE * dpiXFactor, ICON_SIZE * dpiYFactor), ICON_MARGIN * dpiXFactor, ICON_COLOR));
}


////////////////////////////////////////////////////////////////////////////////
// ManualActivityPageSpecifics

ManualActivityPageSpecifics::ManualActivityPageSpecifics
(Context *context, QWidget *parent)
: QWizardPage(parent), context(context)
{
    setTitle(tr("Stress Information"));
    setSubTitle(tr("Stress values can be estimated or entered manually. Estimates are based on recent activities of the same sport, or your full history if none are found."));
    setFinalPage(true);

    estimateBy = new QComboBox();
    estimateBy->addItem(tr("Duration"));
    estimateBy->addItem(tr("Distance"));
    estimateBy->addItem(tr("Manually"));
    QString bsMode = appsettings->value(this, GC_BIKESCOREMODE).toString();
    if (bsMode == "time") {
        estimateBy->setCurrentIndex(0);
    } else if (bsMode == "dist") {
        estimateBy->setCurrentIndex(1);
    } else {
        estimateBy->setCurrentIndex(2);
    }

    estimationDayEdit = new QSpinBox();
    estimationDayEdit->setSingleStep(1);
    estimationDayEdit->setMinimum(1);
    estimationDayEdit->setMaximum(999);
    estimationDayEdit->setValue(appsettings->value(this, GC_BIKESCOREDAYS, "30").toInt());

    workEdit = new QSpinBox();
    workEdit->setSuffix(" " + tr("kJ"));
    workEdit->setSingleStep(1);
    workEdit->setMinimum(0);
    workEdit->setMaximum(9999);

    bikeStressLabel = new QLabel(tr("BikeStress"));

    bikeStressEdit = new QSpinBox();
    bikeStressEdit->setSingleStep(1);
    bikeStressEdit->setMinimum(0);
    bikeStressEdit->setMaximum(999);

    bikeScoreLabel = new QLabel(tr("BikeScore") + "<sup>TM</sup>");

    bikeScoreEdit = new QSpinBox();
    bikeScoreEdit->setSingleStep(1);
    bikeScoreEdit->setMinimum(0);
    bikeScoreEdit->setMaximum(999);

    swimScoreLabel = new QLabel(tr("SwimScore") + "<sup>TM</sup>");

    swimScoreEdit = new QSpinBox();
    swimScoreEdit->setSingleStep(1);
    swimScoreEdit->setMinimum(0);
    swimScoreEdit->setMaximum(999);

    triScoreLabel = new QLabel(tr("TriScore") + "<sup>TM</sup");

    triScoreEdit = new QSpinBox();
    triScoreEdit->setSingleStep(1);
    triScoreEdit->setMinimum(0);
    triScoreEdit->setMaximum(999);

    QDoubleSpinBox *realDuration = new QDoubleSpinBox();
    realDuration->setMinimum(0);
    realDuration->setMaximum(100000);
    realDuration->setVisible(false);

    QDoubleSpinBox *realDistance = new QDoubleSpinBox();
    realDistance->setMinimum(0);
    realDistance->setMaximum(10000);
    realDistance->setVisible(false);

#if QT_VERSION >= 0x060000
    connect(estimateBy, &QComboBox::currentIndexChanged, this, &ManualActivityPageSpecifics::updateVisibility);
    connect(estimateBy, &QComboBox::currentIndexChanged, this, &ManualActivityPageSpecifics::updateEstimates);
    connect(estimationDayEdit, &QSpinBox::valueChanged, this, &ManualActivityPageSpecifics::updateEstimates);
#else
    connect(estimateBy, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ManualActivityPageSpecifics::updateVisibility);
    connect(estimateBy, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ManualActivityPageSpecifics::updateEstimates);
    connect(estimationDayEdit, QOverload<int>::of(&QSpinBox::valueChanged), this, &ManualActivityPageSpecifics::updateEstimates);
#endif

    registerField("estimateBy", estimateBy);
    registerField("estimationDays", estimationDayEdit);
    registerField("work", workEdit);
    registerField("bikeStress", bikeStressEdit);
    registerField("bikeScore", bikeScoreEdit);
    registerField("swimScore", swimScoreEdit);
    registerField("triScore", triScoreEdit);
    registerField("realDuration", realDuration, "value", SIGNAL(valueChanged(double)));
    registerField("realDistance", realDistance, "value", SIGNAL(valueChanged(double)));

    QFormLayout *form = newQFormLayout();
    form->addRow(tr("Estimate by"), estimateBy);
    form->addRow(tr("Estimate Stress Days"), estimationDayEdit);
    form->addRow(tr("Work"), workEdit);
    form->addRow(bikeStressLabel, bikeStressEdit);
    form->addRow(bikeScoreLabel, bikeScoreEdit);
    form->addRow(swimScoreLabel, swimScoreEdit);
    form->addRow(triScoreLabel, triScoreEdit);
    form->addRow(realDuration);
    form->addRow(realDistance);

    setLayout(centerLayout(form));
}


void
ManualActivityPageSpecifics::cleanupPage
()
{
    // Overriden to prevent "estimate by" and "Estimate Stress days" from being reset when going back
}


void
ManualActivityPageSpecifics::initializePage
()
{
    updateVisibility();
    updateEstimates();
}


int
ManualActivityPageSpecifics::nextId
() const
{
    return ManualActivityWizard::Finalize;
}


void
ManualActivityPageSpecifics::updateVisibility
()
{
    bool manual = estimateBy->currentIndex() == 2;

    bool showBikeStress = false;
    bool showBikeScore = false;
    bool showSwimScore = false;
    bool showTriScore = true;
    QString sport = field("sport").toString().trimmed();
    if (sport == "Bike") {
        showBikeStress = true;
        showBikeScore = true;
    } else if (sport == "Run") {
        // Use defaults
    } else if (sport == "Swim") {
        showSwimScore = true;
    } else if (sport == "Row") {
        // Use defaults
    } else if (sport == "Ski") {
        // Use defaults
    } else if (sport == "Gym") {
        // Use defaults
    }

    workEdit->setEnabled(manual);
    bikeStressEdit->setEnabled(manual);
    bikeScoreEdit->setEnabled(manual);
    swimScoreEdit->setEnabled(manual);
    triScoreEdit->setEnabled(manual);

    estimationDayEdit->setEnabled(! manual);
    bikeStressLabel->setVisible(showBikeStress);
    bikeStressEdit->setVisible(showBikeStress);
    bikeScoreLabel->setVisible(showBikeScore);
    bikeScoreEdit->setVisible(showBikeScore);
    swimScoreLabel->setVisible(showSwimScore);
    swimScoreEdit->setVisible(showSwimScore);
    triScoreLabel->setVisible(showTriScore);
    triScoreEdit->setVisible(showTriScore);
}


void
ManualActivityPageSpecifics::updateEstimates
()
{
    std::pair<double, double> durationDistance = getDurationDistance();
    double actDuration = durationDistance.first;
    double actDistance = durationDistance.second;
    setField("realDuration", actDuration);
    setField("realDistance", actDistance);

    int estimateBy = field("estimateBy").toInt();
    if (estimateBy == 2) { // manual
        return;
    }
    int estimationDays = field("estimationDays").toInt();
    double timeWork = 0.0;
    double distanceWork = 0.0;
    double timeBikeStress = 0.0;
    double distanceBikeStress = 0.0;
    double timeBikeScore = 0.0;
    double distanceBikeScore = 0.0;
    double timeSwimScore = 0.0;
    double distanceSwimScore = 0.0;
    double timeTriScore = 0.0;
    double distanceTriScore = 0.0;

    QString sport = field("sport").toString().trimmed();

    double metricFactor = 1.0;
    if (   (sport == "Run" && ! appsettings->value(this, GC_PACE, GlobalContext::context()->useMetricUnits).toBool())
        || (sport == "Swim" && ! appsettings->value(this, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool())
        || (sport != "Run" && sport != "Swim" && ! GlobalContext::context()->useMetricUnits)) {
        metricFactor = MILES_PER_KM;
    }

    // do we have any rides?
    if (context->athlete->rideCache->rides().count()) {
        // last 'n' days calculation
        double seconds = 0.0;
        double distance = 0.0;
        double work = 0.0;
        double bikeStress = 0.0;
        double bikeScore = 0.0;
        double swimScore = 0.0;
        double triScore = 0.0;
        int rides = 0;

        // fall back to 'all time' calculation
        double totalSeconds = 0.0;
        double totalDistance = 0.0;
        double totalWork = 0.0;
        double totalBikeStress = 0.0;
        double totalBikeScore = 0.0;
        double totalSwimScore = 0.0;
        double totalTriScore = 0.0;

        // iterate over the ride cache
        for (RideItem *ride : context->athlete->rideCache->rides()) {
            if (ride->planned || ride->sport.trimmed() != sport) {
                continue;
            }

            // skip those with no time or distance values (not comparing doubles)
            if (ride->getForSymbol("time_riding") == 0 || ride->getForSymbol("total_distance") == 0) {
                continue;
            }

            // how many days ago was it?
            int daysAgo = ride->dateTime.daysTo(QDateTime::currentDateTime());

            // only use rides in last 'n' days
            if (daysAgo >= 0 && daysAgo < estimationDays) {
                seconds += ride->getForSymbol("time_riding");
                distance += ride->getForSymbol("total_distance");
                work += ride->getForSymbol("total_work");
                bikeStress += ride->getForSymbol("coggan_tss");
                bikeScore += ride->getForSymbol("skiba_bike_score");
                swimScore += ride->getForSymbol("swimscore");
                triScore += ride->getForSymbol("triscore");

                rides++;
            }
            totalSeconds += ride->getForSymbol("time_riding");
            totalDistance += ride->getForSymbol("total_distance");
            totalWork += ride->getForSymbol("total_work");
            totalBikeStress += ride->getForSymbol("coggan_tss");
            totalBikeScore += ride->getForSymbol("skiba_bike_score");
            totalSwimScore += ride->getForSymbol("swimscore");
            totalTriScore += ride->getForSymbol("triscore");
        }

        // total values, not just last 'n' days -- but avoid divide by zero
        totalDistance *= metricFactor;

        timeWork = (totalWork * 3600) / totalSeconds;
        timeBikeStress = (totalBikeStress * 3600) / totalSeconds;
        timeBikeScore = (totalBikeScore * 3600) / totalSeconds;
        timeSwimScore = (totalSwimScore * 3600) / totalSeconds;
        timeTriScore = (totalTriScore * 3600) / totalSeconds;
        distanceWork = totalWork / totalDistance;
        distanceBikeStress = totalBikeStress / totalDistance;
        distanceBikeScore = totalBikeScore / totalDistance;
        distanceSwimScore = totalSwimScore / totalDistance;
        distanceTriScore = totalTriScore / totalDistance;

        // don't use defaults if we have rides in last 'n' days
        if (rides) {
            if (seconds) {
                distance *= metricFactor;
                timeWork = (work * 3600) / seconds;
                timeBikeStress = (bikeStress * 3600) / seconds;
                timeBikeScore = (bikeScore * 3600) / seconds;
                timeSwimScore = (swimScore * 3600) / seconds;
                timeTriScore = (triScore * 3600) / seconds;
            }
            if (distance) {
                distanceWork = work / distance;
                distanceBikeStress = bikeStress / distance;
                distanceBikeScore = bikeScore / distance;
                distanceSwimScore = swimScore / distance;
                distanceTriScore = triScore / distance;
            }
        }
    }

    if (estimateBy == 0) { // by time
        setField("work", actDuration * timeWork / 3600.0);
        setField("bikeStress", actDuration * timeBikeStress / 3600.0);
        setField("bikeScore", actDuration * timeBikeScore / 3600.0);
        setField("swimScore", actDuration * timeSwimScore / 3600.0);
        setField("triScore", actDuration * timeTriScore / 3600.0);
    } else if (estimateBy == 1) { // by distance
        setField("work", actDistance * distanceWork);
        setField("bikeStress", actDistance * distanceBikeStress);
        setField("bikeScore", actDistance * distanceBikeScore);
        setField("swimScore", actDistance * distanceSwimScore);
        setField("triScore", actDistance * distanceTriScore);
    }
}


std::pair<double, double>
ManualActivityPageSpecifics::getDurationDistance
() const
{
    double durationSeconds = 0;
    double distanceKm = 0;
    bool useLaps = field("paceIntervals").toBool();
    if (useLaps) {
        QList<RideFilePoint*> rideFilePoints = field("laps").value<QList<RideFilePoint*>>();
        if (! rideFilePoints.isEmpty()) {
            RideFilePoint *rfp = rideFilePoints.last();
            durationSeconds = rfp->secs;
            distanceKm = rfp->km;
        }
    } else {
        QTime durationField = field("duration").toTime();
        durationSeconds = durationField.hour() * 3600 + durationField.minute() * 60.0 + durationField.second();

        QString sport = field("sport").toString().trimmed();
        bool useMetricUnits = GlobalContext::context()->useMetricUnits;
        if (sport == "Run") {
            useMetricUnits = appsettings->value(this, GC_PACE, GlobalContext::context()->useMetricUnits).toBool();
        } else if (sport == "Swim") {
            useMetricUnits = appsettings->value(this, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
        }
        if (sport == "Swim") {
            distanceKm = field("swimDistance").toInt() / 1000.0;
            if (! useMetricUnits) {
                distanceKm *= METERS_PER_YARD;
            }
        } else {
            distanceKm = field("distance").toDouble();
            if (! useMetricUnits) {
                distanceKm /= MILES_PER_KM;
            }
        }
    }
    return std::make_pair(durationSeconds, distanceKm);
}
