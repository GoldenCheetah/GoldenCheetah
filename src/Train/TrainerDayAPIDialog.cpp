#include "TrainerDayAPIDialog.h"

#include <QDebug>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyleFactory>
#include <QStyle>

#include "TrainerDayAPIQuery.h"
#include "ErgFilePlot.h"
#include "PowerZonesWidget.h"
#include "PowerInfoWidget.h"
#include "ErgOverview.h"
#include "Shy.h"
#include "TrainDB.h"
#include "Secrets.h"

#if !defined(GC_TRAINERDAY_API_PAGESIZE)
#define GC_TRAINERDAY_API_PAGESIZE 25
#endif


#if defined(GC_WANT_TRAINERDAY_API)
TrainerDayAPIDialog::TrainerDayAPIDialog
(Context *context, QWidget *parent)
: QWidget(parent), context(context)
{
    fe = new FilterEditor();
    fe->setPlaceholderText(tr("Query TrainerDay..."));
    fe->setClearButtonEnabled(true);
    fe->setFilterCommands(trainerDayAPIQueryCommands());

    search = new QPushButton(tr("Search"));
    search->setAutoDefault(false);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    prev = new QPushButton("<");
    prev->setEnabled(false);
    prev->setAutoDefault(false);
    next = new QPushButton(">");
    next->setEnabled(false);
    next->setAutoDefault(false);
    pageLabel = new QLabel();
    selectAll = new QCheckBox(tr("Select all"));
    selectAll->setTristate(true);
    selectAll->setEnabled(false);
    selectAll->setCheckState(Qt::Unchecked);
    allowReimport = new QCheckBox(tr("Allow overwriting of existing workouts"));
    allowReimport->setCheckState(Qt::Unchecked);
    imp = new QPushButton(tr("Import selected"));
    imp->setAutoDefault(false);
    imp->setEnabled(false);

    buttonLayout->addWidget(prev);
    buttonLayout->addWidget(next);
    buttonLayout->addWidget(pageLabel);
    buttonLayout->addStretch();
    buttonLayout->addWidget(selectAll);
    buttonLayout->addWidget(allowReimport);
    buttonLayout->addStretch();
    buttonLayout->addWidget(imp);

    connect(fe, SIGNAL(textChanged(const QString&)), this, SLOT(resetPagination()));
    connect(fe, SIGNAL(returnPressed()), this, SLOT(newQuery()));
    connect(search, SIGNAL(clicked()), this, SLOT(newQuery()));
    connect(next, SIGNAL(clicked()), this, SLOT(nextPage()));
    connect(prev, SIGNAL(clicked()), this, SLOT(prevPage()));
    connect(imp, SIGNAL(clicked()), this, SLOT(importSelected()));
    connect(selectAll, SIGNAL(clicked(bool)), this, SLOT(selectAllClicked(bool)));
    connect(allowReimport, SIGNAL(clicked(bool)), this, SLOT(setBoxesCheckable(bool)));

    QWidget *workoutDisplay = new QWidget();
    QVBoxLayout *l = new QVBoxLayout();
    QLabel *startLabel = new QLabel(tr("Download workouts from <a href='https://trainerday.com'>TrainerDay</a>"));
    startLabel->setOpenExternalLinks(true);
    l->addWidget(startLabel);
    workoutDisplay->setLayout(l);

    workoutArea = new QScrollArea();
    workoutArea->setWidgetResizable(true);
    workoutArea->setWidget(workoutDisplay);

    int row = 0;
    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(fe, row, 0);
    layout->addWidget(search, row, 1);
    ++row;
    layout->addWidget(workoutArea, row, 0, 1, 2);
    layout->setRowStretch(row, 5);
    ++row;
    layout->addLayout(buttonLayout, row, 0, 1, 2);
    ++row;
    layout->setColumnStretch(0, 1);
}


TrainerDayAPIDialog::~TrainerDayAPIDialog
()
{
    clearErgFiles();
}
#else
TrainerDayAPIDialog::TrainerDayAPIDialog
(Context *context, QWidget *parent)
: QWidget(parent), context(context)
{
    qWarning() << __func__ << ": GC_WANT_TRAINERDAY_API is not defined - disabling functionality";
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(new QLabel(tr("TrainerDay workout querying is disabled in this build.")));
}


TrainerDayAPIDialog::~TrainerDayAPIDialog
()
{
}
#endif


void
TrainerDayAPIDialog::newQuery
()
{
    currentPage = 0;
    executeQuery();
}


void
TrainerDayAPIDialog::executeQuery
()
{
#if defined(GC_WANT_TRAINERDAY_API)
    selectAll->setEnabled(false);
    imp->setEnabled(false);
    clearErgFiles();
    bool ok = false;
    QString msg;
    QUrlQuery query = parseTrainerDayAPIQuery(fe->text().trimmed(), ok, msg);
    if (ok) {
        getWorkouts(query);
        if (lastOk) {
            next->setEnabled(lastWorkouts.size() == GC_TRAINERDAY_API_PAGESIZE); // Hack since TrainerDay doesn't report the number of pages / number of overall results
            prev->setEnabled(currentPage > 0);
            pageLabel->setText(tr("Page %1").arg(currentPage + 1));
        } else {
            ok = lastOk;
            msg = lastMsg;
            pageLabel->setText("");
        }
    } else {
        if (msg.length() == 0) {
            msg = tr("Syntax error in query '%1'").arg(fe->text().trimmed());
            showError(msg);
            pageLabel->setText("");
        }
    }
    if (! ok) {
        showError(msg);
        resetPagination();
    }
    groupBoxClicked();
#endif
}


void
TrainerDayAPIDialog::nextPage
()
{
    ++currentPage;
    executeQuery();
}


void
TrainerDayAPIDialog::prevPage
()
{
    if (currentPage > 0) {
        --currentPage;
        executeQuery();
    }
}


void
TrainerDayAPIDialog::importSelected
()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    window()->setEnabled(false);

    trainDB->startLUW();
    for (int i = 0; i < groupBoxes.size(); ++i) {
        if (groupBoxes[i]->isChecked()) {
            QString filename = buildFilepath(lastWorkouts[i].first);
            QFile out(filename);
            if (out.open(QIODevice::WriteOnly) == true) {
                QTextStream output(&out);
                output << lastWorkouts[i].second;
                out.close();

                if (trainDB->importWorkout(filename, *ergFiles[i], allowReimport->isChecked() ? ImportMode::insertOrUpdate : ImportMode::insert)) {
                    setBoxState(i, TrainerDayAPIDialogState::importNew);
                } else {
                    setBoxState(i, TrainerDayAPIDialogState::importFailed);
                }
            } else {
                setBoxState(i, TrainerDayAPIDialogState::importFailed);
            }
        }
    }
    trainDB->endLUW();

    groupBoxClicked();
    QApplication::restoreOverrideCursor();
    window()->setEnabled(true);
}


void
TrainerDayAPIDialog::getWorkouts
(const QUrlQuery &query)
{
#if defined(GC_WANT_TRAINERDAY_API)
    lastWorkouts.clear();
    lastOk = true;
    lastMsg = "";
    QEventLoop eventLoop;
    QNetworkAccessManager networkMgr;
    networkMgr.setTransferTimeout();

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(parseWorkoutResults(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));

    QUrlQuery q(query);
    q.addQueryItem("pageIndex", QString::number(currentPage));
    QUrl url("https://api.trainerday.com/api/v1/workouts/find");
    url.setQuery(q);
    QNetworkRequest request = QNetworkRequest(url);
    request.setRawHeader("x-api-key", GC_TRAINERDAY_API_KEY);
    networkMgr.get(request);

    QApplication::setOverrideCursor(Qt::WaitCursor);
    window()->setEnabled(false);

    eventLoop.exec();

    QApplication::restoreOverrideCursor();
    window()->setEnabled(true);
#else
    Q_UNUSED(query)
#endif
}


void
TrainerDayAPIDialog::parseWorkoutResults
(QNetworkReply *reply)
{
    QWidget *workoutWidget = new QWidget();
    workoutWidget->setProperty("color", GColor(CTRAINPLOTBACKGROUND));
    QVBoxLayout *workoutLayout = new QVBoxLayout(workoutWidget);
    workoutArea->setWidget(workoutWidget);
    if (reply->error() != QNetworkReply::NoError) {
        lastOk = false;
        lastMsg = reply->errorString();
        return;
    }
    QByteArray data = reply->readAll();
    QJsonParseError errorPtr;
    QJsonDocument doc = QJsonDocument::fromJson(data, &errorPtr);
    if (doc.isNull()) {
        lastOk = false;
        lastMsg = "Parsing of workouts failed";
        return;
    }
    QJsonArray root = doc.array();
    int i = 0;
    for (const QJsonValue &wo : root) {
        if (! wo.isObject()) {
            continue;
        }
        QJsonValue segments = wo["segments"];
        if (segments == QJsonValue::Undefined || ! segments.isArray()) {
            continue;
        }
        QJsonDocument segmentsDoc(segments.toArray());
        QByteArray segmentsData = segmentsDoc.toJson(QJsonDocument::Compact);
        QString segmentsHash = QString(QCryptographicHash::hash(segmentsData, QCryptographicHash::Md5).toHex());

        QJsonDocument workoutDoc(wo.toObject());
        QByteArray workout = workoutDoc.toJson(QJsonDocument::Compact);

        ErgFile *ergFile = ErgFile::fromContent2(workout, context);
        if (ergFile->isValid()) {
            ergFile->filename(buildFilename(segmentsHash));
            ergFiles << ergFile;
            ErgFilePlot *ergFilePlot = new ErgFilePlot(context);
            ergFilePlot->setShowColorZones(2);
            ergFilePlot->setShowTooltip(1);
            ergFilePlot->setData(ergFile);
            ergFilePlot->setMinimumHeight(250 * dpiYFactor);
            ergFilePlot->setMaximumHeight(250 * dpiYFactor);
            ergFilePlot->replot();

            QGroupBox *box = new QGroupBox();
            box->setStyleSheet(GCColor::stylesheet(true));
            groupBoxes << box;
            if (trainDB->hasWorkout(buildFilepath(segmentsHash))) {
                setBoxState(i, TrainerDayAPIDialogState::importOld);
            } else if (QFile::exists(buildFilepath(segmentsHash))) {
                setBoxState(i, TrainerDayAPIDialogState::unimportedFile);
            } else {
                setBoxState(i, TrainerDayAPIDialogState::readyForImport);
            }
            connect(box, SIGNAL(clicked(bool)), this, SLOT(groupBoxClicked()));

            QScrollArea *infoArea = new QScrollArea();
            infoArea->setWidgetResizable(true);
            infoArea->setFrameStyle(QFrame::NoFrame);
            infoArea->setMinimumWidth(360 * dpiXFactor);
            infoArea->setMaximumWidth(360 * dpiXFactor);
#ifdef Q_OS_WIN
            QStyle *xde = QStyleFactory::create(OS_STYLE);
            infoArea->verticalScrollBar()->setStyle(xde);
#endif

            ErgOverview *ergOverview = new ErgOverview();
            ergOverview->setContent(ergFile);
            QBoxLayout *eoLayout = new QHBoxLayout();
            eoLayout->setSpacing(0);
            eoLayout->setContentsMargins(0, 0, 0, 0);
            eoLayout->addWidget(ergOverview);
            Shy *eoShy = new Shy(34, ShyType::percent);
            eoShy->setLayout(eoLayout);

            PowerInfoWidget *powerInfo = new PowerInfoWidget();
            powerInfo->setContentsMargins(0, 10, 0, 5);
            powerInfo->setPower(ergFile->minWatts(), ergFile->maxWatts(), ergFile->AP(), ergFile->IsoPower(), ergFile->XP());

            int zonerange = context->athlete->zones("Bike")->whichRange(QDateTime::currentDateTime().date());
            QList<QColor> zoneColors;
            if (zonerange != -1) {
                int numZones = context->athlete->zones("Bike")->numZones(zonerange);
                for (int j = 0; j < numZones; ++j) {
                    zoneColors << zoneColor(j, numZones);
                }
            }
            PowerZonesWidget *powerZones = new PowerZonesWidget(zoneColors, context->athlete->zones("Bike")->getZoneDescriptions(zonerange));
            powerZones->setPowerZones(ergFile->powerZonesPC(), ergFile->dominantZoneInt(), ergFile->duration());
            powerZones->setContentsMargins(0, 5, 0, 10);

            QGridLayout *infoLayout = new QGridLayout();
            int row = 0;
            infoLayout->addWidget(eoShy, row++, 0, 1, -1);
            infoLayout->addWidget(powerInfo, row++, 0, 1, -1);
            infoLayout->addWidget(powerZones, row++, 0, 1, -1);
            infoLayout->setRowStretch(infoLayout->rowCount(), 1);

            QFrame *infoWidget = new QFrame();
            infoWidget->setFrameStyle(QFrame::NoFrame);
            infoWidget->setLayout(infoLayout);
            infoArea->setWidget(infoWidget);

            QFrame *boxFrame = new QFrame();
            boxFrame->setFrameStyle(QFrame::NoFrame);
            QHBoxLayout *boxFrameLayout = new QHBoxLayout();
            boxFrameLayout->addWidget(infoArea);
            boxFrameLayout->addWidget(ergFilePlot);
            boxFrame->setLayout(boxFrameLayout);

            QHBoxLayout *boxLayout = new QHBoxLayout();
            boxLayout->addWidget(boxFrame);
            box->setLayout(boxLayout);

            workoutLayout->addWidget(box);
        } else {
            delete ergFile;
        }

        lastWorkouts << std::make_pair(segmentsHash, workout);
        ++i;
    }
}


void
TrainerDayAPIDialog::resetPagination
()
{
    currentPage = 0;
    prev->setEnabled(false);
    next->setEnabled(false);
    pageLabel->setText("");
}


void
TrainerDayAPIDialog::selectAllClicked
(bool checked)
{
    selectAll->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    setBoxesCheckState(checked);
    groupBoxClicked();
}


void
TrainerDayAPIDialog::groupBoxClicked
()
{
    int checked = 0;
    int unchecked = 0;
    int uncheckable = 0;
    for (QGroupBox *box : groupBoxes) {
        if (box->isCheckable()) {
            if (box->isChecked()) {
                ++checked;
            } else {
                ++unchecked;
            }
        } else {
            ++uncheckable;
        }
    }
    selectAll->setEnabled(checked + unchecked > 0);
    if (checked == 0) {
        selectAll->setCheckState(Qt::Unchecked);
    } else if (unchecked == 0) {
        selectAll->setCheckState(Qt::Checked);
    } else {
        selectAll->setCheckState(Qt::PartiallyChecked);
    }
    imp->setEnabled(checked > 0);
    if (checked + unchecked > 0) {
        imp->setText(tr("Import selected (%1/%2)").arg(checked).arg(checked + unchecked));
    } else {
        imp->setText(tr("Import selected"));
    }
}


void
TrainerDayAPIDialog::setBoxesCheckable
(bool allowReimport)
{
    for (QGroupBox *box : groupBoxes) {
        if (allowReimport) {
            box->setCheckable(true);
        } else {
            box->setCheckable(box->property("imported").toInt() == 0);
        }
    }
    groupBoxClicked();
}


void
TrainerDayAPIDialog::clearErgFiles
()
{
    groupBoxes.clear();
    QWidget *workoutWidget = workoutArea->takeWidget();
    if (workoutWidget != nullptr) {
        delete workoutWidget;
    }
    for (int i = 0; i < ergFiles.length(); ++i) {
        delete ergFiles[i];
    }
    ergFiles.clear();
}


void
TrainerDayAPIDialog::showError
(QString msg)
{
    clearErgFiles();
    workoutArea->setWidget(new QLabel(msg));
}


QString
TrainerDayAPIDialog::buildFilepath
(const QString &hash)
const
{
    return appsettings->value(this, GC_WORKOUTDIR).toString() + "/" + buildFilename(hash);
}


QString
TrainerDayAPIDialog::buildFilename
(const QString &hash)
const
{
    return "trainerday-" + hash + ".erg2";
}


void
TrainerDayAPIDialog::setBoxState
(int idx, TrainerDayAPIDialogState state)
{
    QString workoutTitle = ergFiles[idx]->name().length() > 0 ? ergFiles[idx]->name() : "---";
    QGroupBox *box = groupBoxes[idx];
    switch (state) {
    case TrainerDayAPIDialogState::importNew:
        box->setChecked(false);
        box->setCheckable(allowReimport->isChecked());
        box->setTitle(workoutTitle + " - " + tr("successfully imported"));
        box->setProperty("imported", 1);
        break;
    case TrainerDayAPIDialogState::importOld:
        box->setChecked(false);
        box->setCheckable(allowReimport->isChecked());
        box->setTitle(workoutTitle + " - " + tr("already imported"));
        box->setProperty("imported", 1);
        break;
    case TrainerDayAPIDialogState::importFailed:
        box->setChecked(true);
        box->setCheckable(true);
        box->setTitle(workoutTitle + " - " + tr("import failed"));
        box->setProperty("imported", 0);
        break;
    case TrainerDayAPIDialogState::unimportedFile:
        box->setChecked(false);
        box->setCheckable(allowReimport->isChecked());
        box->setTitle(workoutTitle + " - " + tr("available in filesystem but not imported"));
        box->setProperty("imported", 0);
        break;
    case TrainerDayAPIDialogState::readyForImport:
    default:
        box->setChecked(true);
        box->setCheckable(true);
        box->setTitle(workoutTitle);
        box->setProperty("imported", 0);
        break;
    }
}


void
TrainerDayAPIDialog::setBoxesCheckState
(bool checked)
{
    for (QGroupBox *box : groupBoxes) {
        box->setChecked(checked);
    }
}
