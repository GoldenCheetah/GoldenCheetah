/*
 * Copyright (c) 2009 Eric Murray (ericm@lne.com)
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2025 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "ManualActivityWizard.h"

#include <QtGui>
#include <QMessageBox>
#include <QSplitter>
#include <QStyleFactory>
#include <QScrollBar>

#include <string.h>
#include <errno.h>
#include <cmath>

#include "Context.h"
#include "TrainDB.h"
#include "Colors.h"
#include "Athlete.h"
#include "RideCache.h"
#include "RideItem.h"
#include "Settings.h"
#include "RideMetadata.h"
#include "Units.h"
#include "HelpWhatsThis.h"

#define MANDATORY " *"
#define TRADEMARK "<sup>TM</sup>"
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
#define HLO "<h4>"
#define HLC "</h4>"

static QString activityBasename(const QDateTime &dt);
static QString activityFilename(const QDateTime &dt, bool plan, Context *context);


////////////////////////////////////////////////////////////////////////////////
// ManualActivityWizard

ManualActivityWizard::ManualActivityWizard
(Context *context, bool plan, QWidget *parent)
: QWizard(parent), context(context), plan(plan)
{
    if (plan) {
        setWindowTitle(tr("Add a Planned Activity"));
    } else {
        setWindowTitle(tr("Create a Completed Activity"));
    }
    setMinimumSize(800 * dpiXFactor, 650 * dpiYFactor);
    setModal(true);

#ifdef Q_OS_MAC
    setWizardStyle(QWizard::MacStyle);
#else
    setWizardStyle(QWizard::ModernStyle);
#endif
    setPixmap(ICON_TYPE, svgAsColoredPixmap(":images/material/summit.svg", QSize(ICON_SIZE * dpiXFactor, ICON_SIZE * dpiYFactor), ICON_MARGIN * dpiXFactor, ICON_COLOR));

    setPage(PageBasics, new ManualActivityPageBasics(context, plan));
    setPage(PageWorkout, new ManualActivityPageWorkout(context));
    setPage(PageMetrics, new ManualActivityPageMetrics(context, plan));
    setPage(PageSummary, new ManualActivityPageSummary(plan));

    setStartId(PageBasics);
}


void
ManualActivityWizard::done
(int result)
{
    int finalResult = result;
    if (result == QDialog::Accepted) {
        QString sport = field("sport").toString().trimmed();
        appsettings->setValue(GC_BIKESCOREDAYS, field("estimationDays").toInt());
        int eb = field("estimateBy").toInt();
        appsettings->setValue(GC_BIKESCOREMODE, eb == 0 ? "time" : (eb == 1 ? "dist" : "manual"));

        RideFile rideFile;

        QDateTime rideDateTime = QDateTime(field("activityDate").toDate(), field("activityTime").toTime());
        rideFile.setStartTime(rideDateTime);
        rideFile.setRecIntSecs(0.00);
        rideFile.setDeviceType("Manual");
        rideFile.setFileFormat("GoldenCheetah Json");

        field2TagString(rideFile, "sport", "Sport");
        field2TagString(rideFile, "subSport", "SubSport");
        field2TagString(rideFile, "workoutCode", "Workout Code");
        field2TagInt(rideFile, "rpe", "RPE");
        field2TagString(rideFile, "objective", "Objective");
        field2TagString(rideFile, "notes", "Notes");
        field2TagString(rideFile, "woFilename", "WorkoutFilename");
        field2TagString(rideFile, "woTitle", "Route");

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
            field2MetricDouble(rideFile, "realDistance", "total_distance");
            field2MetricInt(rideFile, "realDuration", "workout_time");
            field2MetricInt(rideFile, "realDuration", "time_riding");
        }
        field2MetricInt(rideFile, "averageHr", "average_hr");
        field2MetricInt(rideFile, "averageCadence", "average_cad");
        field2MetricInt(rideFile, "averagePower", "average_power");
        field2MetricInt(rideFile, "work", "total_work");
        field2MetricInt(rideFile, "bikeStress", "coggan_tss");
        field2MetricInt(rideFile, "bikeScore", "skiba_bike_score");
        field2MetricInt(rideFile, "swimScore", "swimscore");
        field2MetricInt(rideFile, "triScore", "triscore");
        field2MetricInt(rideFile, "woElevationGain", "elevation_gain");
        field2MetricInt(rideFile, "woIsoPower", "coggan_np");
        field2MetricInt(rideFile, "woXPower", "skiba_xpower");

        // process linked defaults
        GlobalContext::context()->rideMetadata->setLinkedDefaults(&rideFile);

        // what should the filename be?
        QString basename = activityBasename(rideDateTime);
        QFile out(activityFilename(rideDateTime, plan, context));
        if (RideFileFactory::instance().writeRideFile(context, &rideFile, out, "json")) {
            // refresh metric db etc
            context->athlete->addRide(basename + ".json", true, true, false, plan);
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


void
ManualActivityWizard::field2MetricDouble
(RideFile &rideFile, const QString &fieldName, const QString &metricName) const
{
    double value = field(fieldName).toDouble();
    if (value > 0) {
        QMap<QString,QString> values;
        values.insert("value", QString::number(value));
        rideFile.metricOverrides.insert(metricName, values);
    }
}


void
ManualActivityWizard::field2MetricInt
(RideFile &rideFile, const QString &fieldName, const QString &metricName) const
{
    int value = field(fieldName).toInt();
    if (value > 0) {
        QMap<QString,QString> values;
        values.insert("value", QString::number(value));
        rideFile.metricOverrides.insert(metricName, values);
    }
}


void
ManualActivityWizard::field2TagString
(RideFile &rideFile, const QString &fieldName, const QString &tagName) const
{
    if (! field(fieldName).toString().trimmed().isEmpty()) {
        rideFile.setTag(tagName, field(fieldName).toString().trimmed());
    }
}


void
ManualActivityWizard::field2TagInt
(RideFile &rideFile, const QString &fieldName, const QString &tagName) const
{
    if (field(fieldName).toInt() > 0) {
        rideFile.setTag(tagName, QString::number(field(fieldName).toInt()));
    }
}


////////////////////////////////////////////////////////////////////////////////
// ManualActivityPageBasics

ManualActivityPageBasics::ManualActivityPageBasics
(Context *context, bool plan, QWidget *parent)
: QWizardPage(parent), context(context), plan(plan)
{
    setTitle(tr("General Information"));
    if (plan) {
        setSubTitle(tr("Plan your upcoming activity by entering the basic details for this session. On the next pages, you can either manually log your performance metrics or select a workout for train mode."));
    } else {
        setSubTitle(tr("Log your activity by entering the basic details of the session. Once finished, you can add your performance metrics on the next page."));
    }

    bool useMetricUnits = GlobalContext::context()->useMetricUnits;
    bool metricSwimPace = appsettings->value(this, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
    QLocale locale;

    QDateEdit *dateEdit = new QDateEdit();
    dateEdit->setDisplayFormat(locale.dateFormat(QLocale::ShortFormat));
    if (plan) {
        dateEdit->setMinimumDate(QDate::currentDate());
        dateEdit->setMaximumDate(QDate(2099, 31, 12));
    } else {
        dateEdit->setMinimumDate(QDate(2000, 1, 1));
        dateEdit->setMaximumDate(QDate::currentDate());
    }
    dateEdit->setCalendarPopup(true);

    QTimeEdit *timeEdit = new QTimeEdit();
    timeEdit->setDisplayFormat(locale.timeFormat(QLocale::ShortFormat));

    duplicateActivityLabel = new QLabel(tr("An activity already exists for this date and time. Continuing will overwrite the existing activity."));
    duplicateActivityLabel->setWordWrap(true);
    duplicateActivityLabel->setVisible(false);
    duplicateActivityLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QLineEdit *sportEdit = new QLineEdit();

    QLabel *subSportLabel = new QLabel(tr("Sub Sport"));
    QLineEdit *subSportEdit = new QLineEdit();

    QLabel *workoutCodeLabel = new QLabel(tr("Workout Code"));
    QLineEdit *workoutCodeEdit = new QLineEdit();

    QLabel *rpeLabel = new QLabel(tr("RPE"));
    QComboBox *rpeEdit = new QComboBox();
    rpeEdit->addItem("0 - " + tr("Rest"), QColor(Qt::lightGray));
    rpeEdit->addItem("1 - " + tr("Very, very easy"), QColor(Qt::lightGray));
    rpeEdit->addItem("2 - " + tr("Easy"), QColor(Qt::darkGreen));
    rpeEdit->addItem("3 - " + tr("Moderate"), QColor(Qt::darkGreen));
    rpeEdit->addItem("4 - " + tr("Somewhat hard"), QColor(Qt::darkGreen));
    rpeEdit->addItem("5 - " + tr("Hard"), QColor(Qt::darkYellow));
    rpeEdit->addItem("6 - " + tr("Hard+"), QColor(Qt::darkYellow));
    rpeEdit->addItem("7 - " + tr("Very hard"), QColor(Qt::darkYellow));
    rpeEdit->addItem("8 - " + tr("Very hard+"), QColor(Qt::darkRed));
    rpeEdit->addItem("9 - " + tr("Very hard++"), QColor(Qt::darkRed));
    rpeEdit->addItem("10 - " + tr("Maximum"), QColor(Qt::red));
    ColorDelegate *rpeDelegate = new ColorDelegate();
    rpeEdit->setItemDelegate(rpeDelegate);

    QLabel *notesLabel = new QLabel(tr("Notes"));
    QTextEdit *notesEdit = new QTextEdit();
    // rich text hangs 'fontd' for some users
    notesEdit->setAcceptRichText(false);

    QLabel *woTypeLabel = new QLabel(tr("Type") + MANDATORY);
    QComboBox *woTypeEdit = new QComboBox();
    woTypeEdit->addItem(tr("Workout (Train mode)"));
    woTypeEdit->addItem(tr("Manual Entry"));
    woTypeEdit->setCurrentIndex(1);
    if (plan) {

#if QT_VERSION >= 0x060000
        connect(woTypeEdit, &QComboBox::currentIndexChanged, [=](int index) {
#else
        connect(woTypeEdit, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
#endif
            sportEdit->setEnabled(index != 0);
            if (index == 0) {
                sportEdit->setText("Bike");
                emit sportEdit->editingFinished();
            }
        });
    }

    QLabel *objectiveLabel = new QLabel(tr("Objective"));
    QLineEdit *objectiveEdit = new QLineEdit();

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

    subSportLabel->setVisible(! plan);
    subSportEdit->setVisible(! plan);
    workoutCodeLabel->setVisible(! plan);
    workoutCodeEdit->setVisible(! plan);
    rpeLabel->setVisible(! plan);
    rpeEdit->setVisible(! plan);
    woTypeLabel->setVisible(plan);
    woTypeEdit->setVisible(plan);
    objectiveLabel->setVisible(plan);
    objectiveEdit->setVisible(plan);

    connect(dateEdit, &QDateEdit::timeChanged, this, &ManualActivityPageBasics::checkDateTime);
    connect(timeEdit, &QTimeEdit::timeChanged, this, &ManualActivityPageBasics::checkDateTime);
    connect(sportEdit, &QLineEdit::editingFinished, this, &ManualActivityPageBasics::sportsChanged);
    connect(sportEdit, &QLineEdit::textChanged, this, [this]() { emit completeChanged(); });

    registerField("activityDate", dateEdit);
    registerField("activityTime", timeEdit, "time", SIGNAL(timeChanged(QTime)));
    registerField("sport*", sportEdit);
    registerField("subSport", subSportEdit);
    registerField("workoutCode", workoutCodeEdit);
    registerField("rpe", rpeEdit);
    registerField("objective", objectiveEdit);
    registerField("notes", notesEdit, "plainText", SIGNAL(textChanged()));
    registerField("woType", woTypeEdit);

    QFormLayout *form = newQFormLayout();
    form->addRow(tr("Date") + MANDATORY, dateEdit);
    form->addRow(tr("Time") + MANDATORY, timeEdit);
    form->addRow("", duplicateActivityLabel);
    form->addRow(woTypeLabel, woTypeEdit);
    form->addRow(tr("Sport") + MANDATORY, sportEdit);
    form->addRow(subSportLabel, subSportEdit);
    form->addRow(workoutCodeLabel, workoutCodeEdit);
    form->addRow(rpeLabel, rpeEdit);
    form->addRow(objectiveLabel, objectiveEdit);
    form->addRow(notesLabel, notesEdit);

    QWidget *scrollWidget = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->addWidget(centerLayoutInWidget(form, false));
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);

    QVBoxLayout *all = new QVBoxLayout();
    all->addWidget(scrollArea);
    setLayout(all);
}


void
ManualActivityPageBasics::initializePage
()
{
    setField("activityDate", QDate::currentDate());
    if (plan) {
        setField("activityTime", QTime(16, 0, 0)); // Planned: 16:00 by default
    } else {
        setField("activityTime", QTime::currentTime().addSecs(-4 * 3600)); // Completed: 4 hours ago by default
    }
    if (plan) {
        setField("woType", 0);
    }
}


int
ManualActivityPageBasics::nextId
() const
{
    return (plan && field("woType").toInt() == 0) ? ManualActivityWizard::PageWorkout : ManualActivityWizard::PageMetrics;
}


bool
ManualActivityPageBasics::isComplete
() const
{
    return field("sport").toString().trimmed().size() > 0;
}


void
ManualActivityPageBasics::checkDateTime
()
{
    QDateTime dt(field("activityDate").toDate(), field("activityTime").toTime());
    QFile file(activityFilename(dt, plan, context));
    duplicateActivityLabel->setVisible(file.exists());
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
// ManualActivityPageWorkout

ManualActivityPageWorkout::ManualActivityPageWorkout
(Context *context, QWidget *parent)
: QWizardPage(parent), context(context)
{
    setTitle(tr("Workout for Train Mode"));
    setSubTitle(tr("Browse and filter workouts based on your goals and preferences. Choose the one that best suits your needs to guide your upcoming session."));

    workoutFilterBox = new WorkoutFilterBox();

    workoutModel = trainDB->getWorkoutModel();

    sortModel = new MultiFilterProxyModel(this);
    sortModel->setSourceModel(workoutModel);
    workoutModel->setParent(this);
    sortModel->setDynamicSortFilter(true);
    sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    sortModel->sort(TdbWorkoutModelIdx::sortdummy, Qt::AscendingOrder); //sort by sortdummy-field

    workoutTree = new QTreeView();
    workoutTree->setModel(sortModel);

    // hide unwanted columns and header
    for (int i = 0; i < workoutTree->header()->count(); i++) {
        workoutTree->setColumnHidden(i, true);
    }
    workoutTree->setColumnHidden(TdbWorkoutModelIdx::displayname, false); // show displayname
    workoutTree->header()->hide();
    workoutTree->setFrameStyle(QFrame::NoFrame);
    workoutTree->setAlternatingRowColors(false);
    workoutTree->setEditTriggers(QAbstractItemView::NoEditTriggers); // read-only
    workoutTree->expandAll();
    workoutTree->header()->setCascadingSectionResizes(true); // easier to resize this way
    workoutTree->setContextMenuPolicy(Qt::CustomContextMenu);
    workoutTree->header()->setStretchLastSection(true);
    workoutTree->header()->setMinimumSectionSize(0);
    workoutTree->header()->setFocusPolicy(Qt::NoFocus);
    workoutTree->setFrameStyle(QFrame::NoFrame);
    workoutTree->setRootIsDecorated(false);
#ifdef Q_OS_MAC
    workoutTree->header()->setSortIndicatorShown(false); // blue looks nasty
    workoutTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
#ifdef Q_OS_WIN
    QStyle *xde = QStyleFactory::create(OS_STYLE);
    workoutTree->verticalScrollBar()->setStyle(xde);
#endif

    backupWorkout = context->workout; // ergFilePlot->setData is not sufficient for showing a workout, ErgFilePlot also
                                      // relies on context->workout therefore keeping a backup of the original workout in
                                      // the context so it can be reset on destruction of this page (yes, this is ugly)
    context->workout = nullptr;
    ergFilePlot = new ErgFilePlot(context);
    ergFilePlot->setShowColorZones(2);
    ergFilePlot->setShowTooltip(1);

    QWidget *ergFileWrapperWidget = new QWidget();
    QHBoxLayout *ergFileWrapperLayout = new QHBoxLayout(ergFileWrapperWidget);
    ergFileWrapperLayout->addWidget(ergFilePlot);

    int zonerange = context->athlete->zones("Bike")->whichRange(QDateTime::currentDateTime().date());
    QList<QColor> zoneColors;
    if (zonerange != -1) {
        int numZones = context->athlete->zones("Bike")->numZones(zonerange);
        for (int j = 0; j < numZones; ++j) {
            zoneColors << zoneColor(j, numZones);
        }
    }
    infoWidget = new InfoWidget(zoneColors, context->athlete->zones("Bike")->getZoneDescriptions(zonerange), false, false);

    QWidget *detailsWrapperWidget = new QWidget();
    QHBoxLayout *detailsWrapperLayout = new QHBoxLayout(detailsWrapperWidget);
    detailsWrapperLayout->addWidget(infoWidget);

    QWidget *detailsScrollWidget = new QWidget();
    QVBoxLayout *detailsScrollLayout = new QVBoxLayout(detailsScrollWidget);
    detailsScrollLayout->addWidget(infoWidget);
    detailsScrollLayout->addStretch(100);
    QScrollArea *detailsScrollArea = new QScrollArea();
    detailsScrollArea->setFrameShape(QFrame::NoFrame);
    detailsScrollArea->setWidget(detailsScrollWidget);
    detailsScrollArea->setWidgetResizable(true);

    QTabWidget *ergTabs = new QTabWidget();
    ergTabs->addTab(detailsScrollArea, tr("Metrics"));
    ergTabs->addTab(ergFileWrapperWidget, tr("Chart"));

    QLabel *noWorkoutLabel = new QLabel(tr("Select a workout to view its details and continue to the next page."));
    noWorkoutLabel->setWordWrap(true);
    noWorkoutLabel->setAlignment(Qt::AlignCenter);

    QLabel *noDataLabel = new QLabel(tr("No details are available for this workout."));
    noDataLabel->setWordWrap(true);
    noDataLabel->setAlignment(Qt::AlignCenter);

    contentStack = new QStackedWidget();
    contentStack->addWidget(noWorkoutLabel);
    contentStack->addWidget(ergTabs);
    contentStack->addWidget(noDataLabel);
    contentStack->setCurrentIndex(0);

    QLineEdit *woFilename = new QLineEdit();
    QLineEdit *woTitle = new QLineEdit();
    QLineEdit *woFileType = new QLineEdit();
    QSpinBox *woElevationGain = new QSpinBox();
    woElevationGain->setMaximum(10000);
    QSpinBox *woIsoPower = new QSpinBox();
    woIsoPower->setMaximum(10000);
    QSpinBox *woXPower = new QSpinBox();
    woXPower->setMaximum(10000);

    registerField("woFilename*", woFilename);
    registerField("woTitle*", woTitle);
    registerField("woFileType*", woFileType);
    registerField("woElevationGain", woElevationGain);
    registerField("woIsoPower", woIsoPower);
    registerField("woXPower", woXPower);

    connect(workoutFilterBox, &WorkoutFilterBox::workoutFiltersChanged, [=](QList<ModelFilter*> &f) {
        sortModel->setFilters(f);
    });
    connect(workoutFilterBox, &WorkoutFilterBox::workoutFiltersRemoved, [=]() {
        sortModel->removeFilters();
    });
    connect(workoutTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ManualActivityPageWorkout::selectionChanged);

    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(workoutTree);
    splitter->addWidget(contentStack);
    splitter->setHandleWidth(10 * dpiXFactor);
    splitter->setStyleSheet(R"(QSplitter::handle { background: transparent; margin: 0px; })");
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setChildrenCollapsible(false);
    splitter->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    QVBoxLayout *hiddenLayout = new QVBoxLayout();
    hiddenLayout->addWidget(woFilename);
    hiddenLayout->addWidget(woTitle);
    hiddenLayout->addWidget(woFileType);
    hiddenLayout->addWidget(woElevationGain);
    hiddenLayout->addWidget(woIsoPower);
    hiddenLayout->addWidget(woXPower);
    for (int i = 0; i < hiddenLayout->count(); ++i) {
        QLayoutItem *item = hiddenLayout->itemAt(i);
        if (item) {
            QWidget* widget = item->widget();
            if (widget) {
                widget->setVisible(false);
            }
        }
    }

    QVBoxLayout *all = new QVBoxLayout();
    all->addWidget(workoutFilterBox);
    all->addSpacing(10 * dpiYFactor);
    all->addWidget(splitter);
    all->addLayout(hiddenLayout);

    setLayout(all);

    workoutFilterBox->installEventFilter(this);
}


ManualActivityPageWorkout::~ManualActivityPageWorkout
()
{
    context->workout = backupWorkout;
    if (workoutModel != nullptr) {
        delete workoutModel;
    }
    if (ergFile != nullptr) {
        delete ergFile;
    }
}


void
ManualActivityPageWorkout::initializePage
()
{
}


int
ManualActivityPageWorkout::nextId
() const
{
    return ManualActivityWizard::PageMetrics;
}


void
ManualActivityPageWorkout::resetFields
()
{
    setField("woFilename", QString());
    setField("woTitle", QString());
    setField("woFileType", QString());
    setField("woElevationGain", 0);
    setField("woIsoPower", 0);
    setField("woXPower", 0);
    setField("bikeStress", 0);
    setField("bikeScore", 0);
    setField("averagePower", 0);
    setField("realDuration", 0);
    setField("duration", QTime());
    setField("realDistance", 0);
    setField("distance", 0);
}


void
ManualActivityPageWorkout::selectionChanged
()
{
    QModelIndex current = workoutTree->currentIndex();
    if (! current.isValid()) {
        setField("woFilename", QString());
        contentStack->setCurrentIndex(0);
        if (ergFile != nullptr) {
            delete ergFile;
            ergFile = nullptr;
        }
        return;
    }
    resetFields();
    QModelIndex target = sortModel->mapToSource(current);
    QString filename = workoutModel->data(workoutModel->index(target.row(), TdbWorkoutModelIdx::filepath), Qt::DisplayRole).toString();
    QString title = workoutModel->data(workoutModel->index(target.row(), TdbWorkoutModelIdx::displayname), Qt::DisplayRole).toString();
    QString type = workoutModel->data(workoutModel->index(target.row(), TdbWorkoutModelIdx::type), Qt::DisplayRole).toString();
    int avgPower = workoutModel->data(workoutModel->index(target.row(), TdbWorkoutModelIdx::avgPower), Qt::DisplayRole).toInt();
    int bikeStress = workoutModel->data(workoutModel->index(target.row(), TdbWorkoutModelIdx::bikestress), Qt::DisplayRole).toInt();
    int bikeScore = workoutModel->data(workoutModel->index(target.row(), TdbWorkoutModelIdx::bs), Qt::DisplayRole).toInt();
    int elevationGain = workoutModel->data(workoutModel->index(target.row(), TdbWorkoutModelIdx::elevation), Qt::DisplayRole).toInt();
    int isoPower = workoutModel->data(workoutModel->index(target.row(), TdbWorkoutModelIdx::isoPower), Qt::DisplayRole).toInt();
    int xPower = workoutModel->data(workoutModel->index(target.row(), TdbWorkoutModelIdx::xp), Qt::DisplayRole).toInt();
    setField("woFilename", filename);
    setField("woTitle", title);
    setField("woFileType", type);
    setField("woElevationGain", elevationGain);
    setField("woIsoPower", isoPower);
    setField("woXPower", xPower);
    setField("bikeStress", bikeStress);
    setField("bikeScore", bikeScore);
    if (type == "erg") {
        int durationSecs = workoutModel->data(workoutModel->index(target.row(), TdbWorkoutModelIdx::duration), Qt::DisplayRole).toInt() / 1000;
        setField("averagePower", avgPower);
        setField("realDuration", durationSecs);
        setField("duration", QTime(0, 0, 0).addSecs(durationSecs));
    } else if (type == "slp") {
        bool useMetricUnits = GlobalContext::context()->useMetricUnits;
        double distanceKM = workoutModel->data(workoutModel->index(target.row(), TdbWorkoutModelIdx::distance), Qt::DisplayRole).toDouble() / 1000;
        setField("realDistance", distanceKM);
        setField("distance", distanceKM * (useMetricUnits ? 1.0 : MILES_PER_KM));
    }

    if (ergFile != nullptr) {
        delete ergFile;
        ergFile = nullptr;
    }

    if (type != "code") {
        ergFile = new ErgFile(filename, ErgFileFormat::unknown, context);
        if (! ergFile->isValid()) {
            delete ergFile;
            ergFile = nullptr;
        }
    }
    contentStack->setCurrentIndex(ergFile != nullptr ? 1 : 2);
    context->workout = ergFile;
    ergFilePlot->setData(ergFile);
    ergFilePlot->replot();
    infoWidget->ergFileSelected(ergFile);
}


bool
ManualActivityPageWorkout::eventFilter
(QObject *watched, QEvent *event)
{
    if (watched == workoutFilterBox && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            emit workoutFilterBox->returnPressed();
            keyEvent->accept();
            return true;
        }
    }
    return QWizardPage::eventFilter(watched, event);
}


void
ManualActivityPageWorkout::cleanupPage
()
{
    workoutTree->setCurrentIndex(QModelIndex());
    QWizardPage::cleanupPage();
}


////////////////////////////////////////////////////////////////////////////////
// ManualActivityPageMetrics

ManualActivityPageMetrics::ManualActivityPageMetrics
(Context *context, bool plan, QWidget *parent)
: QWizardPage(parent), context(context), plan(plan)
{
    setTitle(tr("Activity Metrics"));

    bool useMetricUnits = GlobalContext::context()->useMetricUnits;
    bool metricSwimPace = appsettings->value(this, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();

    QSpinBox *averageHrEdit = new QSpinBox();
    averageHrEdit->setMinimum(0);
    averageHrEdit->setMaximum(250);
    averageHrEdit->setSuffix(" " + tr("bpm"));

    averagePowerLabel = new QLabel(tr("Average Power"));
    averagePowerEdit = new QSpinBox();
    averagePowerEdit->setMinimum(0);
    averagePowerEdit->setMaximum(2000);
    averagePowerEdit->setSuffix(" " + tr("W"));

    paceIntervals = new QCheckBox(tr("Pace intervals"));

    lapsEditor = new LapsEditorWidget();
    lapsEditor->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    lapsEditor->setMinimumSize(50 * dpiXFactor, 200 * dpiYFactor);

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

    stressHL = new QLabel(HLO + tr("Stress & Workload") + HLC);

    estimateByLabel = new QLabel(tr("Estimate by"));
    estimateByEdit = new QComboBox();
    estimateByEdit->addItem(tr("Duration"));
    estimateByEdit->addItem(tr("Distance"));
    estimateByEdit->addItem(tr("Manually"));
    QString bsMode = appsettings->value(this, GC_BIKESCOREMODE).toString();
    if (bsMode == "time") {
        estimateByEdit->setCurrentIndex(0);
    } else if (bsMode == "dist") {
        estimateByEdit->setCurrentIndex(1);
    } else {
        estimateByEdit->setCurrentIndex(2);
    }

    estimationDaysLabel = new QLabel(tr("Estimate Stress Days"));
    estimationDaysEdit = new QSpinBox();
    estimationDaysEdit->setSingleStep(1);
    estimationDaysEdit->setMinimum(1);
    estimationDaysEdit->setMaximum(999);
    estimationDaysEdit->setValue(appsettings->value(this, GC_BIKESCOREDAYS, "30").toInt());

    workLabel = new QLabel(tr("Work"));
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

    bikeScoreLabel = new QLabel(tr("BikeScore") + TRADEMARK);
    bikeScoreEdit = new QSpinBox();
    bikeScoreEdit->setSingleStep(1);
    bikeScoreEdit->setMinimum(0);
    bikeScoreEdit->setMaximum(999);

    swimScoreLabel = new QLabel(tr("SwimScore") + TRADEMARK);
    swimScoreEdit = new QSpinBox();
    swimScoreEdit->setSingleStep(1);
    swimScoreEdit->setMinimum(0);
    swimScoreEdit->setMaximum(999);

    triScoreLabel = new QLabel(tr("TriScore") + TRADEMARK);
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
    realDistance->setDecimals(15);
    realDistance->setMaximum(10000);
    realDistance->setVisible(false);

    connect(durationEdit, &QTimeEdit::editingFinished, this, &ManualActivityPageMetrics::updateEstimates);
    connect(distanceEdit, &QDoubleSpinBox::editingFinished, this, &ManualActivityPageMetrics::updateEstimates);
    connect(swimDistanceEdit, &QDoubleSpinBox::editingFinished, this, &ManualActivityPageMetrics::updateEstimates);
    connect(paceIntervals, &QCheckBox::toggled, this, &ManualActivityPageMetrics::updateVisibility);
    connect(lapsEditor, &LapsEditorWidget::editingFinished, this, &ManualActivityPageMetrics::updateEstimates);
#if QT_VERSION >= 0x060000
    connect(estimateByEdit, &QComboBox::currentIndexChanged, this, &ManualActivityPageMetrics::updateVisibility);
    connect(estimateByEdit, &QComboBox::currentIndexChanged, this, &ManualActivityPageMetrics::updateEstimates);
    connect(estimationDaysEdit, &QSpinBox::valueChanged, this, &ManualActivityPageMetrics::updateEstimates);
#else
    connect(estimateByEdit, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ManualActivityPageMetrics::updateVisibility);
    connect(estimateByEdit, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ManualActivityPageMetrics::updateEstimates);
    connect(estimationDaysEdit, QOverload<int>::of(&QSpinBox::valueChanged), this, &ManualActivityPageMetrics::updateEstimates);
#endif

    registerField("averageHr", averageHrEdit);
    registerField("averagePower", averagePowerEdit);
    registerField("paceIntervals", paceIntervals);
    registerField("laps", lapsEditor, "dataPoints", SIGNAL(editingFinished()));
    registerField("averageCadence", averageCadenceEdit);
    registerField("distance", distanceEdit, "value", SIGNAL(valueChanged(double)));
    registerField("swimDistance", swimDistanceEdit);
    registerField("duration", durationEdit, "time", SIGNAL(timeChanged(QTime)));
    registerField("estimateBy", estimateByEdit);
    registerField("estimationDays", estimationDaysEdit);
    registerField("work", workEdit);
    registerField("bikeStress", bikeStressEdit);
    registerField("bikeScore", bikeScoreEdit);
    registerField("swimScore", swimScoreEdit);
    registerField("triScore", triScoreEdit);
    registerField("realDuration", realDuration, "value", SIGNAL(valueChanged(double)));
    registerField("realDistance", realDistance, "value", SIGNAL(valueChanged(double)));

    QFormLayout *form1 = newQFormLayout();
    form1->addRow(new QLabel(HLO + tr("Core Training Metrics") + HLC));
    form1->addRow(tr("Average Heartrate"), averageHrEdit);
    form1->addRow(averagePowerLabel, averagePowerEdit);
    form1->addRow(averageCadenceLabel, averageCadenceEdit);
    form1->addRow("", paceIntervals);
    form1->addRow(distanceLabel, distanceEdit);
    form1->addRow(swimDistanceLabel, swimDistanceEdit);
    form1->addRow(durationLabel, durationEdit);

    QFormLayout *form2 = newQFormLayout();
    form2->addRow(stressHL);
    form2->addRow(estimateByLabel, estimateByEdit);
    form2->addRow(estimationDaysLabel, estimationDaysEdit);
    form2->addRow(workLabel, workEdit);
    form2->addRow(bikeStressLabel, bikeStressEdit);
    form2->addRow(bikeScoreLabel, bikeScoreEdit);
    form2->addRow(swimScoreLabel, swimScoreEdit);
    form2->addRow(triScoreLabel, triScoreEdit);
    form2->addRow(realDuration);
    form2->addRow(realDistance);

    QWidget *scrollWidget = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->addWidget(centerLayoutInWidget(form1, false));
    scrollLayout->addWidget(lapsEditor);
    scrollLayout->addWidget(centerLayoutInWidget(form2, false));
    scrollLayout->addStretch(100);
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);

    QVBoxLayout *all = new QVBoxLayout();
    all->addWidget(scrollArea);
    setLayout(all);
}


void
ManualActivityPageMetrics::cleanupPage
()
{
    // Prevent estimateBy and estimationDays from being reset when going back
    QVariant estBy = field("estimateBy");
    QVariant estDays = field("estimationDays");
    QWizardPage::cleanupPage();
    setField("estimateBy", estBy);
    setField("estimationDays", estDays);
}


void
ManualActivityPageMetrics::initializePage
()
{
    if (plan) {
        if (field("woType").toInt() == 0) {
            setSubTitle(tr("Add more details for your upcoming activity by entering the expected values to track your planned performance."));
        } else {
            setSubTitle(tr("Plan the key details for your upcoming activity based on the selected sport. Enter the expected values to track your performance goals."));
        }
    } else {
        setSubTitle(tr("Record the key details of your activity based on the selected sport. Enter the relevant data to track your performance."));
    }
    updateVisibility();
    updateEstimates();
}


int
ManualActivityPageMetrics::nextId
() const
{
    return ManualActivityWizard::PageSummary;
}


void
ManualActivityPageMetrics::updateVisibility
()
{
    bool workoutPlan = plan && field("woType").toInt() == 0;
    QString sport = field("sport").toString().trimmed();
    bool useMetricUnits = GlobalContext::context()->useMetricUnits;
    bool showAveragePower = true;
    bool showAverageCadence = true;
    bool showDistance = true;
    bool showSwimDistance = false;
    bool showDuration = true;
    bool showLapsEditor = false;
    bool showPaceIntervals = false;
    bool manual = estimateByEdit->currentIndex() == 2;
    bool showEstimate = true;
    bool showWork = true;
    bool showBikeStress = false;
    bool showBikeScore = false;
    bool showSwimScore = false;
    bool showTriScore = true;

    if (workoutPlan) {
        QString woFileType = field("woFileType").toString().trimmed();
        if (woFileType == "erg") {
            showAveragePower = false;
            showDistance = false;
            showDuration = false;
            showEstimate = false;
        } else if (woFileType == "slp") {
            showDistance = false;
            showBikeStress = true;
            showBikeScore = true;
        } else {
            showBikeStress = true;
            showBikeScore = true;
        }
        showWork = false;
        showTriScore = false;
    } else if (sport == "") {
        // Hide all conditional fields
        showAveragePower = false;
        showAverageCadence = false;
        showDistance = false;
        showSwimDistance = false;
        showDuration = false;
        showLapsEditor = false;
        showPaceIntervals = false;
        showBikeStress = false;
        showBikeScore = false;
        showSwimScore = false;
        showTriScore = false;
    } else if (sport == "Bike") {
        showBikeStress = true;
        showBikeScore = true;
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
        showSwimScore = true;
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

    estimationDaysEdit->setEnabled(! manual);
    workEdit->setEnabled(manual);
    bikeStressEdit->setEnabled(manual);
    bikeScoreEdit->setEnabled(manual);
    swimScoreEdit->setEnabled(manual);
    triScoreEdit->setEnabled(manual);

    stressHL->setVisible(   showEstimate
                         || showWork
                         || showBikeStress
                         || showBikeScore
                         || showSwimScore
                         || showTriScore);
    estimateByLabel->setVisible(showEstimate);
    estimateByEdit->setVisible(showEstimate);
    estimationDaysLabel->setVisible(showEstimate);
    estimationDaysEdit->setVisible(showEstimate);
    workLabel->setVisible(showWork);
    workEdit->setVisible(showWork);
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
ManualActivityPageMetrics::updateEstimates
()
{
    QString sport = field("sport").toString().trimmed();
    if (   plan
        && field("woType").toInt() == 0
        && field("woFileType").toString().trimmed() == "erg") { // no estimation if planning a ergmode workout
        return;
    }
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

    std::pair<double, double> durationDistance = getDurationDistance();
    double actDuration = durationDistance.first;
    double actDistance = durationDistance.second;
    setField("realDuration", actDuration);
    setField("realDistance", actDistance);
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
ManualActivityPageMetrics::getDurationDistance
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


////////////////////////////////////////////////////////////////////////////////
// ManualActivityPageMetrics

ManualActivityPageSummary::ManualActivityPageSummary
(bool plan, QWidget *parent)
: QWizardPage(parent), plan(plan)
{
    setTitle(tr("Summary"));
    if (plan) {
        setSubTitle(tr("Summary of your upcoming activity. Review the plan to ensure everything is set for your session."));
    } else {
        setSubTitle(tr("Summary of your activity. Review the data to ensure it accurately reflects the session."));
    }
    setFinalPage(true);

    form = newQFormLayout();

    QWidget *scrollWidget = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->addWidget(centerLayoutInWidget(form, false));
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);

    QVBoxLayout *all = new QVBoxLayout();
    all->addWidget(scrollArea);
    setLayout(all);
}


void
ManualActivityPageSummary::cleanupPage
()
{
    while (form->count() > 0) {
        QLayoutItem *item = form->takeAt(0);
        if (! item) {
            continue;
        }
        QWidget *widget = item->widget();
        if (widget) {
            form->removeWidget(widget);
            delete widget;
        }
        delete item;
    }
}


void
ManualActivityPageSummary::initializePage
()
{
    QString sport = field("sport").toString().trimmed();
    bool useMetricUnits = true;
    double metricFactorKM = 1.0;
    double metricFactorM = 1.0;
    if (   (sport == "Run" && ! appsettings->value(this, GC_PACE, GlobalContext::context()->useMetricUnits).toBool())
        || (sport == "Swim" && ! appsettings->value(this, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool())
        || (sport != "Run" && sport != "Swim" && ! GlobalContext::context()->useMetricUnits)) {
        metricFactorKM = MILES_PER_KM;
        metricFactorM = 1.0 / METERS_PER_YARD;
        useMetricUnits = false;
    }

    QLocale locale;
    QDateTime when = QDateTime(field("activityDate").toDate(), field("activityTime").toTime());

    form->addRow(new QLabel(HLO + tr("General Information") + HLC));
    if (plan) {
        addRow(tr("Scheduled for"), locale.toString(when, QLocale::ShortFormat));
    } else {
        addRow(tr("Date"), locale.toString(when, QLocale::ShortFormat));
    }
    addRowString(tr("Sport"), "sport");
    addRowString(tr("SubSport"), "subSport");
    addRowString(tr("Workout Code"), "workoutCode");
    addRowString(tr("Workout Title"), "woTitle");
    if (! plan) {
        switch (field("rpe").toInt()) {
        case 0:
            addRow(tr("RPE"), "0 - " + tr("Rest"));
            break;
        case 1:
            addRow(tr("RPE"), "1 - " + tr("Very, very easy"));
            break;
        case 2:
            addRow(tr("RPE"), "2 - " + tr("Easy"));
            break;
        case 3:
            addRow(tr("RPE"), "3 - " + tr("Moderate"));
            break;
        case 4:
            addRow(tr("RPE"), "4 - " + tr("Somewhat hard"));
            break;
        case 5:
            addRow(tr("RPE"), "5 - " + tr("Hard"));
            break;
        case 6:
            addRow(tr("RPE"), "6 - " + tr("Hard+"));
            break;
        case 7:
            addRow(tr("RPE"), "7 - " + tr("Very hard"));
            break;
        case 8:
            addRow(tr("RPE"), "8 - " + tr("Very hard+"));
            break;
        case 9:
            addRow(tr("RPE"), "9 - " + tr("Very hard++"));
            break;
        case 10:
            addRow(tr("RPE"), "10 - " + tr("Maximum"));
            break;
        default:
            break;
        }
    }
    addRowString(tr("Objective"), "objective");
    addRowString(tr("Notes"), "notes");
    bool hasActMetricsSection = false;
    QLabel *actMetricsHL = new QLabel(HLO + tr("Activity Metrics") + HLC);
    form->addRow(actMetricsHL);
    if (sport == "Swim") {
        hasActMetricsSection |= addRowDouble(tr("Distance"), "realDistance", useMetricUnits ? tr("m") : tr("yd"), metricFactorM * 1000.0);
    } else {
        hasActMetricsSection |= addRowDouble(tr("Distance"), "realDistance", useMetricUnits ? tr("km") : tr("mi"), metricFactorKM);
    }
    int duration = field("realDuration").toInt();
    if (duration > 0) {
        QTime durationTime = QTime(0, 0, 0).addSecs(duration);
        hasActMetricsSection |= addRow(tr("Duration"), durationTime.toString("h:mm:ss"));
    }
    hasActMetricsSection |= addRowInt(tr("Average Heartrate"), "averageHr", tr("bpm"));
    hasActMetricsSection |= addRowInt(tr("Average Cadence"), "averageCadence", tr("rpm"));
    hasActMetricsSection |= addRowInt(tr("Average Power"), "averagePower", tr("W"));
    hasActMetricsSection |= addRowInt(tr("Elevation Gain"), "woElevationGain", useMetricUnits ? tr("m") : tr("yd"), metricFactorM);
    hasActMetricsSection |= addRowInt(tr("IsoPower"), "woIsoPower", tr("W"));
    hasActMetricsSection |= addRowInt(tr("xPower"), "woXPower", tr("W"));
    hasActMetricsSection |= addRowInt(tr("Work"), "work", tr("W"));
    hasActMetricsSection |= addRowInt(tr("BikeStress"), "bikeStress");
    hasActMetricsSection |= addRowInt(tr("BikeScore") + TRADEMARK, "bikeScore");
    hasActMetricsSection |= addRowInt(tr("SwimScore") + TRADEMARK, "swimScore");
    hasActMetricsSection |= addRowInt(tr("TriScore") + TRADEMARK, "triScore");
    actMetricsHL->setVisible(hasActMetricsSection);
}


int
ManualActivityPageSummary::nextId
() const
{
    return ManualActivityWizard::Finalize;
}


bool
ManualActivityPageSummary::addRowString
(const QString &label, const QString &fieldName)
{
    return addRow(label, field(fieldName).toString().trimmed());
}


bool
ManualActivityPageSummary::addRowInt
(const QString &label, const QString &fieldName, const QString &unit, double metricFactor)
{
    double value = field(fieldName).toInt() * metricFactor;
    if (value > 0) {
        QString valueStr = QString::number(value);
        if (! unit.isEmpty()) {
            valueStr += " " + unit;
        }
        return addRow(label, valueStr);
    }
    return false;
}


bool
ManualActivityPageSummary::addRowDouble
(const QString &label, const QString &fieldName, const QString &unit, double metricFactor)
{
    double value = field(fieldName).toDouble() * metricFactor;
    if (value > 0) {
        QString valueStr = QString::number(value);
        if (! unit.isEmpty()) {
            valueStr += " " + unit;
        }
        return addRow(label, valueStr);
    }
    return false;
}


bool
ManualActivityPageSummary::addRow
(const QString &label, const QString &value)
{
    if (! value.isEmpty()) {
        QLabel *valueWidget = new QLabel(value);
        valueWidget->setWordWrap(true);
        valueWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        valueWidget->adjustSize();
        form->addRow(label, valueWidget);
        return true;
    }
    return false;
}


////////////////////////////////////////////////////////////////////////////////
// Helpers & Utilities

static QString
activityBasename
(const QDateTime &dt)
{
    return dt.toString("yyyy_MM_dd_HH_mm_ss");
}


static QString
activityFilename
(const QDateTime &dt, bool plan, Context *context)
{
    QString basename = activityBasename(dt);
    QString filename;
    if (plan) {
        filename = context->athlete->home->planned().canonicalPath() + "/" + basename + ".json";
    } else {
        filename = context->athlete->home->activities().canonicalPath() + "/" + basename + ".json";
    }
    return filename;
}
