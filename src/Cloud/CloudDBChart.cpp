/*
 * Copyright (c) 2015 Joern Rischmueller (joern.rm@gmail.com)
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

#include "CloudDBChart.h"
#include "CloudDBCommon.h"
#include "CloudDBStatus.h"

#include "LTMChartParser.h"
#include "GcUpgrade.h"

#include <QtGlobal>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QDesktopWidget>

CloudDBChartClient::CloudDBChartClient()
{
    g_nam = new QNetworkAccessManager(this);
    QDir cacheDir(QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation).at(0));
    cacheDir.cdUp();
    g_cacheDir = QString(cacheDir.absolutePath()+"/GoldenCheetahCloudDB");

    // general handling for sslErrors
    connect(g_nam, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this,
            SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));

    // common definitions used

    g_chart_url_base = g_chart_url_header = g_chartcuration_url_base = CloudDBCommon::cloudDBBaseURL;

    g_chart_url_base.append("chart/");
    g_chart_url_header.append("chartheader");
    g_chartcuration_url_base.append("chartcuration/");

}
CloudDBChartClient::~CloudDBChartClient() {
    delete g_nam;
}


bool
CloudDBChartClient::postChart(ChartAPIv1 chart) {

    // check if Athlete ID is filled

    if (chart.Header.CreatorId.isEmpty()) return CloudDBCommon::APIresponseOthers;

    // default where it may be necessary
    if (chart.Header.Language.isEmpty()) chart.Header.Language = "en";

    // first create the JSON object
    // only a subset of fields is required for POST
    QJsonObject json_header;
    CloudDBCommon::marshallAPIHeaderV1Object(json_header, chart.Header);

    QJsonObject json;
    json["header"] = json_header;
    json["chartxml"] = chart.ChartXML;
    QString image;
    image.append(chart.Image.toBase64());
    json["image"] = image;
    json["creatornick"] = chart.CreatorNick;
    json["creatoremail"] = chart.CreatorEmail;
    QJsonDocument document;
    document.setObject(json);

    QNetworkRequest request;
    CloudDBCommon::prepareRequest(request, g_chart_url_base);
    g_reply = g_nam->post(request, document.toJson());

    // wait for reply (synchronously) and process error codes as necessary
    if (!CloudDBCommon::replyReceivedAndOk(g_reply)) return false;

    QByteArray result = g_reply->readAll();
    result.replace(QByteArray("\""), QByteArray(""));
    qint64 id = result.toLongLong();
    if (id > 0) {
        chart.Header.Id = id;
        writeChartCache(&chart);
    }

    return true;

}

bool
CloudDBChartClient::putChart(ChartAPIv1 chart) {

    // we assume all field are filled properly / not further check or modification

    // first create the JSON object / all fields are required for PUT / only LastChanged i
    QJsonObject json_header;
    CloudDBCommon::marshallAPIHeaderV1Object(json_header, chart.Header);

    QJsonObject json;
    json["header"] = json_header;
    json["chartxml"] = chart.ChartXML;
    QString image;
    image.append(chart.Image.toBase64());
    json["image"] = image;
    json["creatornick"] = chart.CreatorNick;
    json["creatoremail"] = chart.CreatorEmail;
    QJsonDocument document;
    document.setObject(json);

    QNetworkRequest request;
    CloudDBCommon::prepareRequest(request, g_chart_url_base);
    g_reply = g_nam->put(request, document.toJson());

    // wait for reply (synchronously) and process error codes as necessary
    if (!CloudDBCommon::replyReceivedAndOk(g_reply)) return false;

    // update cache
    writeChartCache(&chart);
    return true;

}


bool
CloudDBChartClient::getChartByID(qint64 id, ChartAPIv1 *chart) {

    // read from Cache first
    if (readChartCache(id, chart)) return CloudDBCommon::APIresponseOk;

    // now from GAE
    QNetworkRequest request;
    CloudDBCommon::prepareRequest(request, g_chart_url_base+QString::number(id, 10));
    g_reply = g_nam->get(request);

    // wait for reply (synchronously) and process error codes as necessary
    if (!CloudDBCommon::replyReceivedAndOk(g_reply)) return false;

    // call successfull
    QByteArray result = g_reply->readAll();
    QList<ChartAPIv1>* charts = new QList<ChartAPIv1>;
    unmarshallAPIv1(result, charts);
    if (charts->size() > 0) {
        *chart = charts->value(0);
        writeChartCache(chart);
        charts->clear();
        delete charts;
        return true;
    }
    delete charts;
    return false;
}

bool
CloudDBChartClient::deleteChartByID(qint64 id) {

    QNetworkRequest request;
    CloudDBCommon::prepareRequest(request, g_chart_url_base+QString::number(id, 10));
    g_reply = g_nam->deleteResource(request);

    // wait for reply (synchronously) and process error codes as necessary
    if (!CloudDBCommon::replyReceivedAndOk(g_reply)) return false;

    deleteChartCache(id);
    return true;
}


bool
CloudDBChartClient::curateChartByID(qint64 id, bool newStatus) {

    QUrlQuery query;
    query.addQueryItem("newStatus", (newStatus ? "true": "false"));
    QNetworkRequest request;
    CloudDBCommon::prepareRequest(request, g_chartcuration_url_base+QString::number(id, 10), &query);

    // add a payload to "PUT" even though it's not processed
    g_reply = g_nam->put(request, "{ \"id\": \"dummy\"");

    // wait for reply (synchronously) and process error codes as necessary
    if (!CloudDBCommon::replyReceivedAndOk(g_reply)) return false;

    deleteChartCache(id);
    return true;
}


bool
CloudDBChartClient::getAllChartHeader(QList<CommonAPIHeaderV1>* header) {
    bool request_ok = CloudDBHeader::getAllCachedHeader(header, CloudDBHeader::CloudDB_Chart, g_cacheDir, g_chart_url_header, g_nam, g_reply);
    if (request_ok && header->size()>0) {
        cleanChartCache(header);
    }
    return request_ok;
}


//
// Trap SSL errors
//
void
CloudDBChartClient::sslErrors(QNetworkReply* reply ,QList<QSslError> errors)
{
    CloudDBCommon::sslErrors(reply, errors);
}

// Internal Methods


bool
CloudDBChartClient::writeChartCache(ChartAPIv1 * chart) {

    // make sure the subdir exists
    QDir cacheDir(g_cacheDir);
    if (cacheDir.exists()) {
        cacheDir.mkdir("charts");
    } else {
        return false;
    }
    QFile file(g_cacheDir+"/charts/"+QString::number(chart->Header.Id)+".dat");
    if (file.exists()) {
       // overwrite data
       file.resize(0);
    }

    if (!file.open(QIODevice::WriteOnly)) return false;
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_4_6);
    // track a version to be able change data structure
    out << chart_magic_string;
    out << chart_cache_version;
    out << chart->Header.LastChanged; // start
    out << chart->Header.CreatorId;
    out << chart->Header.Curated;
    out << chart->Header.Deleted;
    out << chart->Header.Description;
    out << chart->Header.GcVersion;
    out << chart->Header.Id;
    out << chart->Header.Language;
    out << chart->Header.Name;
    out << chart->ChartXML;
    out << chart->CreatorEmail;
    out << chart->CreatorNick;
    out << chart->Image;

    file.close();
    return true;
}



bool CloudDBChartClient::readChartCache(qint64 id, ChartAPIv1 *chart) {

    QFile file(g_cacheDir+"/charts/"+QString::number(id)+".dat");
    if (!file.open(QIODevice::ReadOnly)) return false;
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_4_6);
    // track a version to be able change data structure
    int magic_string;
    int version;

    in >> magic_string;
    if (magic_string != chart_magic_string) {
        // wrong file format / close and exit
        file.close();
        return false;
    }

    in >> version;
    if (version != chart_cache_version) {
       // change of version, delete old cache entry
       file.remove();
       return false;
    }

    in >> chart->Header.LastChanged; // start
    in >> chart->Header.CreatorId;
    in >> chart->Header.Curated;
    in >> chart->Header.Deleted;
    in >> chart->Header.Description;
    in >> chart->Header.GcVersion;
    in >> chart->Header.Id;
    in >> chart->Header.Language;
    in >> chart->Header.Name;
    in >> chart->ChartXML;
    in >> chart->CreatorEmail;
    in >> chart->CreatorNick;
    in >> chart->Image;

    file.close();
    return true;

}

void
CloudDBChartClient::deleteChartCache(qint64 id) {

    QFile file(g_cacheDir+"/charts/"+QString::number(id)+".dat");
    if (file.exists()) {
       file.remove();
    }

}

void
CloudDBChartClient::cleanChartCache(QList<CommonAPIHeaderV1> *objectHeader) {
    // remove Deleted Entries from Cache
    int deleted = 0;
    QMutableListIterator<CommonAPIHeaderV1> it(*objectHeader);
    while (it.hasNext()) {
        CommonAPIHeaderV1 header = it.next();
        if (header.Deleted) {
            deleteChartCache(header.Id);
            it.remove();
            deleted ++;
        }
    }
    if (deleted > 0) {
        // store cache for next time
        CloudDBHeader::writeHeaderCache(objectHeader, CloudDBHeader::CloudDB_Chart, g_cacheDir);
    }

}

bool
CloudDBChartClient::unmarshallAPIv1(QByteArray json, QList<ChartAPIv1> *charts) {

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(json, &parseError);

    // all these things should not happen and we have not valid object to return
    if (parseError.error != QJsonParseError::NoError || document.isEmpty() || document.isNull()) {
        return false;
    }

    // do we have a single object or an array ?
    if (document.isObject()) {
        ChartAPIv1 chart;
        QJsonObject object = document.object();
        unmarshallAPIv1Object(&object, &chart);
        QJsonObject header = object["header"].toObject();
        CloudDBCommon::unmarshallAPIHeaderV1Object(&header, &chart.Header);
        charts->append(chart);

    } else if (document.isArray()) {
        QJsonArray array(document.array());
        for (int i = 0; i< array.size(); i++) {
            QJsonValue value = array.at(i);
            if (value.isObject()) {
                ChartAPIv1 chart;
                QJsonObject object = value.toObject();
                unmarshallAPIv1Object(&object, &chart);
                QJsonObject header = object["header"].toObject();
                CloudDBCommon::unmarshallAPIHeaderV1Object(&header, &chart.Header);
                charts->append(chart);
            }
        }
    }

    return true;
}

void
CloudDBChartClient::unmarshallAPIv1Object(QJsonObject* object, ChartAPIv1* chart) {

    chart->ChartXML = object->value("chartxml").toString();
    QString imageString = object->value("image").toString();
    QByteArray ba;
    ba.append(imageString);
    chart->Image = QByteArray::fromBase64(ba);
    chart->CreatorNick = object->value("creatorNick").toString();
    chart->CreatorEmail = object->value("creatorEmail").toString();

}




//------------------------------------------------------------------------------------------------------------
//  Dialog to show and retrieve Shared Charts
//------------------------------------------------------------------------------------------------------------

// define size if image preview at one place
static const int chartImageWidth = 320;
static const int chartImageHeight = 240;

CloudDBChartListDialog::CloudDBChartListDialog() : const_stepSize(5)
{
   g_client = new CloudDBChartClient();
   g_currentHeaderList = new QList<CommonAPIHeaderV1>;
   g_fullHeaderList = new QList<CommonAPIHeaderV1>;
   g_currentPresets = new QList<ChartWorkingStructure>;
   g_textFilterActive = false;
   g_networkrequestactive = false; // don't allow Dialog to close while we are retrieving data

   showing = new QLabel;
   showingTextTemplate = tr("Showing %1 to %2 of %3 charts / Total on CloudDB %4");
   resetToStart = new QPushButton(tr("First"));
   nextSet = new QPushButton(tr("Next %1").arg(QString::number(const_stepSize)));
   prevSet = new QPushButton(tr("Prev %1").arg(QString::number(const_stepSize)));
   resetToStart->setEnabled(true);
   nextSet->setDefault(true);
   nextSet->setEnabled(true);
   prevSet->setEnabled(true);

   connect(resetToStart, SIGNAL(clicked()), this, SLOT(resetToStartClicked()));
   connect(nextSet, SIGNAL(clicked()), this, SLOT(nextSetClicked()));
   connect(prevSet, SIGNAL(clicked()), this, SLOT(prevSetClicked()));

   showingLayout = new QHBoxLayout;
   showingLayout->addWidget(showing);
   showingLayout->addStretch();
   showingLayout->addWidget(resetToStart);
   showingLayout->addWidget(prevSet);
   showingLayout->addWidget(nextSet);

   ownChartsOnly = new QCheckBox(tr("My Charts"));
   ownChartsOnly->setChecked(false);

   connect(ownChartsOnly, SIGNAL(toggled(bool)), this, SLOT(ownChartsToggled(bool)));

   curationStateCombo = new QComboBox();
   curationStateCombo->addItem(tr("All"));
   curationStateCombo->addItem(tr("Curated Only"));
   curationStateCombo->addItem(tr("Uncurated Only"));

   connect(curationStateCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(curationStateFilterChanged(int)));

   langCombo = new QComboBox();
   langCombo->addItem(tr("Any Language"));
   foreach (QString lang, CloudDBCommon::cloudDBLangs) {
       langCombo->addItem(lang);
   }

   textFilterApply = new QPushButton(tr("Search Keyword"));
   textFilterApply->setFixedWidth(180); // To allow proper translation
   textFilter = new QLineEdit;
   g_textFilterActive = false;

   connect(textFilter, SIGNAL(editingFinished()), this, SLOT(textFilterEditingFinished()));
   connect(langCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(languageFilterChanged(int)));
   connect(textFilterApply, SIGNAL(clicked()), this, SLOT(toggleTextFilterApply()));

   filterLayout = new QHBoxLayout;
   filterLayout->addWidget(ownChartsOnly);
   filterLayout->addStretch();
   filterLayout->addWidget(curationStateCombo);
   filterLayout->addStretch();
   filterLayout->addWidget(langCombo);
   filterLayout->addStretch();
   filterLayout->addWidget(textFilterApply);
   filterLayout->addWidget(textFilter);

   textFilter->setVisible(true);

   tableWidget = new QTableWidget(0, 2);
   tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
   tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
   tableWidget->setContentsMargins(0,0,0,0);
   tableWidget->horizontalHeader()->setStretchLastSection(true);
   tableWidget->horizontalHeader()->setVisible(false);
   tableWidget->verticalHeader()->setVisible(false);
   tableWidget->setShowGrid(true);

   // the preview shall have a dedicated size
   tableWidget->setColumnWidth(0, chartImageWidth);

   connect(tableWidget, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(cellDoubleClicked(int,int)));

   // UserGet Role
   addAndCloseUserGetButton = new QPushButton(tr("Add selected chart to library"));
   closeUserGetButton = new QPushButton(tr("Close without selection"));

   addAndCloseUserGetButton->setEnabled(true);
   closeUserGetButton->setEnabled(true);
   closeUserGetButton->setDefault(true);

   connect(addAndCloseUserGetButton, SIGNAL(clicked()), this, SLOT(addAndCloseClicked()));
   connect(closeUserGetButton, SIGNAL(clicked()), this, SLOT(closeClicked()));

   buttonUserGetLayout = new QHBoxLayout;
   buttonUserGetLayout->addWidget(addAndCloseUserGetButton);
   buttonUserGetLayout->addStretch();
   buttonUserGetLayout->addWidget(closeUserGetButton);

   // UserEdit Role
   deleteUserEditButton = new QPushButton(tr("Delete selected chart"));
   editUserEditButton = new QPushButton(tr("Edit selected chart"));
   closeUserEditButton = new QPushButton(tr("Close"));

   connect(deleteUserEditButton, SIGNAL(clicked()), this, SLOT(deleteUserEdit()));
   connect(editUserEditButton, SIGNAL(clicked()), this, SLOT(editUserEdit()));
   connect(closeUserEditButton, SIGNAL(clicked()), this, SLOT(closeClicked()));

   buttonUserEditLayout = new QHBoxLayout;
   buttonUserEditLayout->addWidget(deleteUserEditButton);
   buttonUserEditLayout->addWidget(editUserEditButton);
   buttonUserEditLayout->addStretch();
   buttonUserEditLayout->addWidget(closeUserEditButton);

   // CuratorEdit Role
   curateCuratorEditButton = new QPushButton(tr("Set selected chart 'Curated'"));
   editCuratorEditButton = new QPushButton(tr("Edit selected chart"));
   deleteCuratorEditButton = new QPushButton(tr("Delete selected chart"));
   closeCuratorButton = new QPushButton(tr("Close"));

   connect(curateCuratorEditButton, SIGNAL(clicked()), this, SLOT(curateCuratorEdit()));
   connect(editCuratorEditButton, SIGNAL(clicked()), this, SLOT(editCuratorEdit()));
   connect(deleteCuratorEditButton, SIGNAL(clicked()), this, SLOT(deleteCuratorEdit()));
   connect(closeCuratorButton, SIGNAL(clicked()), this, SLOT(closeClicked()));

   buttonCuratorEditLayout = new QHBoxLayout;
   buttonCuratorEditLayout->addWidget(curateCuratorEditButton);
   buttonCuratorEditLayout->addWidget(editCuratorEditButton);
   buttonCuratorEditLayout->addWidget(deleteCuratorEditButton);
   buttonCuratorEditLayout->addStretch();
   buttonCuratorEditLayout->addWidget(closeCuratorButton);

   // prepare the main layouts - with different buttons layouts
   mainLayout = new QVBoxLayout;
   mainLayout->addLayout(showingLayout);
   mainLayout->addLayout(filterLayout);
   mainLayout->addWidget(tableWidget);

   mainLayout->addLayout(buttonUserGetLayout);
   mainLayout->addLayout(buttonUserEditLayout);
   mainLayout->addLayout(buttonCuratorEditLayout);

   setMinimumHeight(500);
   setMinimumWidth(700);
   setLayout(mainLayout);

}

CloudDBChartListDialog::~CloudDBChartListDialog() {
    delete g_client;
    delete g_currentHeaderList;
    delete g_fullHeaderList;
    delete g_currentPresets;
}

// block DialogWindow close while networkrequest is processed
void
CloudDBChartListDialog::closeEvent(QCloseEvent* event) {

     if (g_networkrequestactive) {
         event->ignore();
         return;
     }
     QDialog::closeEvent(event);
}

bool
CloudDBChartListDialog::prepareData(QString athlete, CloudDBCommon::UserRole role) {

    g_role = role;
    // and now initialize the dialog
    setVisibleButtonsForRole();

    if (g_role == CloudDBCommon::UserEdit) {
        ownChartsOnly->setChecked(true);
        ownChartsOnly->setEnabled(false);
        setWindowTitle(tr("Chart maintenance - Edit or Delete your Charts"));
    } else if (role == CloudDBCommon::CuratorEdit) {
        ownChartsOnly->setChecked(false);
        curationStateCombo->setCurrentIndex(2); // start with uncurated
        setWindowTitle(tr("Curator chart maintenance - Curate, Edit or Delete Charts"));
    } else {
        setWindowTitle(tr("Select a Chart"));
    }

    if (CloudDBHeader::isStaleChartHeader()) {
        if (!refreshStaleChartHeader()) return false;
        CloudDBHeader::setChartHeaderStale(false);
    }
    g_currentAthleteId = appsettings->cvalue(athlete, GC_ATHLETE_ID, "").toString();
    applyAllFilters();
    return true;
}


void
CloudDBChartListDialog::updateCurrentPresets(int index, int count) {

    // while getting the data (which may take of few seconds), disable the UI
    resetToStart->setEnabled(false);
    nextSet->setEnabled(false);
    prevSet->setEnabled(false);
    closeUserGetButton->setEnabled(false);
    addAndCloseUserGetButton->setEnabled(false);
    curationStateCombo->setEnabled(false);
    ownChartsOnly->setEnabled(false);
    textFilterApply->setEnabled(false);
    langCombo->setEnabled(false);
    deleteUserEditButton->setEnabled(false);
    editUserEditButton->setEnabled(false);
    closeUserEditButton->setEnabled(false);
    curateCuratorEditButton->setEnabled(false);
    editCuratorEditButton->setEnabled(false);
    deleteCuratorEditButton->setEnabled(false);
    closeCuratorButton->setEnabled(false);

    // now get the presets
    g_currentPresets->clear();
    ChartAPIv1* chart = new ChartAPIv1;
    g_networkrequestactive = true;
    bool noError = true;
    for (int i = index; i< index+count && i<g_currentHeaderList->count() && noError; i++) {
        if (g_client->getChartByID(g_currentHeaderList->at(i).Id, chart)) {

            ChartWorkingStructure preset;
            preset.id = chart->Header.Id;
            preset.name = chart->Header.Name;
            preset.description = chart->Header.Description;
            preset.language = chart->Header.Language;
            preset.createdAt = chart->Header.LastChanged;
            preset.image.loadFromData(chart->Image);
            QXmlInputSource source;
            source.setData(chart->ChartXML);
            QXmlSimpleReader xmlReader;
            LTMChartParser handler;
            xmlReader.setContentHandler(&handler);
            xmlReader.setErrorHandler(&handler);
            xmlReader.parse( source );
            // in case of corrupt data, settings may be empty
            if (handler.getSettings().size()>0) {
                preset.ltmSettings = handler.getSettings().at(0); //only one LTMSettings Object is stored
                preset.validLTMSettings = true;
            } else {
                preset.validLTMSettings = false;
            }
            preset.creatorNick = chart->CreatorNick;
            g_currentPresets->append(preset);
        } else {
            noError = false;
        }
    }
    g_networkrequestactive = false;
    delete chart;

    // update table with current list
    tableWidget->clearContents();
    tableWidget->setRowCount(0);

    int chartCount = (g_currentHeaderList->size() == 0) ? 0 : g_currentIndex+1;
    int lastIndex = (g_currentIndex+const_stepSize > g_currentHeaderList->size()) ? g_currentHeaderList->size() : g_currentIndex+const_stepSize;
    showing->setText(QString(showingTextTemplate)
                     .arg(QString::number(chartCount))
                     .arg(QString::number(lastIndex))
                     .arg(QString::number(g_currentHeaderList->size()))
                     .arg(QString::number(g_fullHeaderList->size())));

    for (int i = 0; i< g_currentPresets->size(); i++ ) {
        tableWidget->insertRow(i);
        ChartWorkingStructure preset =  g_currentPresets->at(i);

        QTableWidgetItem *newPxItem = new QTableWidgetItem("");
        newPxItem->setData(Qt::DecorationRole, QVariant(preset.image.scaled(chartImageWidth, chartImageHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        newPxItem->setSizeHint(QSize(chartImageWidth, chartImageHeight));
        newPxItem->setFlags(newPxItem->flags() & ~Qt::ItemIsEditable);
        tableWidget->setItem(i, 0, newPxItem);
        tableWidget->item(i,0)->setBackgroundColor(Qt::darkGray);
        tableWidget->setRowHeight(i, chartImageHeight+20);

        QString cellText = QString(tr("<h3>%1</h3><h4>Last Edited At: %2 - Creator: %3</h4>%4"))
                .arg(encodeHTML(preset.name))
                .arg(preset.createdAt.date().toString(Qt::ISODate))
                .arg(encodeHTML(preset.creatorNick))
                .arg(encodeHTML(preset.description));

        QTextEdit *formattedText = new QTextEdit;
        formattedText->setHtml(cellText);
        formattedText->setReadOnly(true);
        tableWidget->setCellWidget(i,1, formattedText);
    }


    // enable UI again
    resetToStart->setEnabled(true);
    nextSet->setEnabled(true);
    prevSet->setEnabled(true);
    closeUserGetButton->setEnabled(true);
    addAndCloseUserGetButton->setEnabled(true);
    curationStateCombo->setEnabled(true);
    ownChartsOnly->setEnabled(true);
    textFilterApply->setEnabled(true);
    langCombo->setEnabled(true);
    deleteUserEditButton->setEnabled(true);
    editUserEditButton->setEnabled(true);
    closeUserEditButton->setEnabled(true);
    curateCuratorEditButton->setEnabled(true);
    editCuratorEditButton->setEnabled(true);
    deleteCuratorEditButton->setEnabled(true);
    closeCuratorButton->setEnabled(true);

    // role dependent UI settings
    if (g_role == CloudDBCommon::UserEdit) {
        ownChartsOnly->setEnabled(false);
    }

}

void
CloudDBChartListDialog::setVisibleButtonsForRole() {
    if (g_role == CloudDBCommon::UserEdit) {
        deleteUserEditButton->setVisible(true);
        editUserEditButton->setVisible(true);
        closeUserEditButton->setVisible(true);
        curateCuratorEditButton->setVisible(false);
        editCuratorEditButton->setVisible(false);
        deleteCuratorEditButton->setVisible(false);
        closeCuratorButton->setVisible(false);
        closeUserGetButton->setVisible(false);
        addAndCloseUserGetButton->setVisible(false);
    } else if (g_role == CloudDBCommon::CuratorEdit) {
        deleteUserEditButton->setVisible(false);
        editUserEditButton->setVisible(false);
        closeUserEditButton->setVisible(false);
        curateCuratorEditButton->setVisible(true);
        editCuratorEditButton->setVisible(true);
        deleteCuratorEditButton->setVisible(true);
        closeCuratorButton->setVisible(true);
        closeUserGetButton->setVisible(false);
        addAndCloseUserGetButton->setVisible(false);
    } else {
        deleteUserEditButton->setVisible(false);
        editUserEditButton->setVisible(false);
        closeUserEditButton->setVisible(false);
        curateCuratorEditButton->setVisible(false);
        editCuratorEditButton->setVisible(false);
        deleteCuratorEditButton->setVisible(false);
        closeCuratorButton->setVisible(false);
        closeUserGetButton->setVisible(true);
        addAndCloseUserGetButton->setVisible(true);
    }

}

bool
CloudDBChartListDialog::refreshStaleChartHeader() {

    g_fullHeaderList->clear();
    g_currentHeaderList->clear();
    g_currentIndex = 0;
    g_networkrequestactive = true;
    if (!g_client->getAllChartHeader(g_fullHeaderList)) {
        g_networkrequestactive = false;
        return false;
    }

    g_networkrequestactive = false;
    return true;
}



void
CloudDBChartListDialog::addAndCloseClicked() {

    // check if an item of the table is selected

    if (tableWidget->selectedItems().size()>0)
    {
        // the selectionMode allows only 1 item to be selected at a time
        QTableWidgetItem* s = tableWidget->selectedItems().at(0);
        if (s->row() >= 0 && s->row() <= g_currentPresets->count()) {
            if (g_currentPresets->at(s->row()).validLTMSettings) {
                g_selected = g_currentPresets->at(s->row()).ltmSettings;
                g_selected.name = g_selected.title = g_currentPresets->at(s->row()).name;
            } else {
                QMessageBox::warning(0, tr("CloudDB"), QString(tr("Chart definition is corrupt - please choose a different chart")));
            }
            accept();
        }
    }
}


void
CloudDBChartListDialog::closeClicked()  {
    reject();
}


void
CloudDBChartListDialog::resetToStartClicked()  {

    if (g_currentIndex == 0) return;

    g_currentIndex = 0;
    updateCurrentPresets(g_currentIndex, const_stepSize);

}

void
CloudDBChartListDialog::nextSetClicked()  {

    g_currentIndex += const_stepSize;
    if (g_currentIndex >= g_currentHeaderList->size()) {
        g_currentIndex = g_currentHeaderList->size() - const_stepSize;
    }
    if (g_currentIndex < 0) g_currentIndex = 0;
    updateCurrentPresets(g_currentIndex, const_stepSize);

}

void
CloudDBChartListDialog::prevSetClicked()  {

    g_currentIndex -= const_stepSize;
    if (g_currentIndex < 0) g_currentIndex = 0;
    updateCurrentPresets(g_currentIndex, const_stepSize);

}

void
CloudDBChartListDialog::curateCuratorEdit() {

    if (tableWidget->selectedItems().size()>0)
    {
        // the selectionMode allows only 1 item to be selected at a time
        QTableWidgetItem* s = tableWidget->selectedItems().at(0);
        if (s->row() >= 0 && s->row() <= g_currentPresets->count()) {
            qint64 id = g_currentPresets->at(s->row()).id;
            if (!g_client->curateChartByID(id, true)) {
                return;
            }
            // refresh header buffer
            refreshStaleChartHeader();

            // curated chart appears on top of the list / and needs to be filtered
            applyAllFilters();
        }
    }

}
void
CloudDBChartListDialog::deleteCuratorEdit(){
   // currently same like User
   deleteUserEdit();
}
void
CloudDBChartListDialog::editCuratorEdit(){
    // currently same like User
    editUserEdit();
}

void
CloudDBChartListDialog::deleteUserEdit(){

    if (tableWidget->selectedItems().size()>0)
    {
        // the selectionMode allows only 1 item to be selected at a time
        QTableWidgetItem* s = tableWidget->selectedItems().at(0);
        if (s->row() >= 0 && s->row() <= g_currentPresets->count()) {
            qint64 id = g_currentPresets->at(s->row()).id;
            if (!g_client->deleteChartByID(id)) {
                return;
            }
            // set stale for subsequent list dialog calls
            CloudDBHeader::setChartHeaderStale(true);

            // remove deleted chart from both lists
            QMutableListIterator<CommonAPIHeaderV1> it1(*g_currentHeaderList);
            while (it1.hasNext()) {
                if (it1.next().Id == id) {
                    it1.remove();
                    break; // there is just one equal entry
                }
            }
            QMutableListIterator<CommonAPIHeaderV1> it2(*g_fullHeaderList);
            while (it2.hasNext()) {
                if (it2.next().Id == id) {
                    it2.remove();
                    break; // there is just one equal entry
                }
            }
            if (g_currentIndex >= g_currentHeaderList->size()) {
                g_currentIndex = g_currentHeaderList->size() - const_stepSize;
            }
            if (g_currentIndex < 0) g_currentIndex = 0;
            updateCurrentPresets(g_currentIndex, const_stepSize);
        }
    }
}

void
CloudDBChartListDialog::editUserEdit(){

    if (tableWidget->selectedItems().size()>0)
    {
        // the selectionMode allows only 1 item to be selected at a time
        QTableWidgetItem* s = tableWidget->selectedItems().at(0);
        if (s->row() >= 0 && s->row() <= g_currentPresets->count()) {
            qint64 id = g_currentPresets->at(s->row()).id;
            ChartAPIv1 chart;
            if (!g_client->getChartByID(id, &chart)) {
                return;
            }

            // now complete the chart with for the user manually added fields
            CloudDBChartObjectDialog dialog(chart, "", true);
            if (dialog.exec() == QDialog::Accepted) {
                if (g_client->putChart(dialog.getChart())) {
                    refreshStaleChartHeader();
                } else {
                    return;
                }
            }
        }

        // updated chart appears on top of the list / and needs to be filtered
        applyAllFilters();

    }
}

void
CloudDBChartListDialog::curationStateFilterChanged(int)  {
   applyAllFilters();
}

void
CloudDBChartListDialog::ownChartsToggled(bool)  {
   applyAllFilters();
}

void
CloudDBChartListDialog::toggleTextFilterApply()  {
    if (g_textFilterActive) {
        g_textFilterActive = false;
        textFilterApply->setText(tr("Search Keyword"));
        applyAllFilters();
    } else {
        g_textFilterActive = true;
        textFilterApply->setText(tr("Reset Search"));
        applyAllFilters();
    }
}

void
CloudDBChartListDialog::languageFilterChanged(int) {
    applyAllFilters();
}


void
CloudDBChartListDialog::applyAllFilters() {

    QStringList searchList;
    g_currentHeaderList->clear();
    if (!textFilter->text().isEmpty()) {
        // split by any whitespace
        searchList = textFilter->text().split(QRegExp("\\s+"), QString::SkipEmptyParts);
    }
    foreach (CommonAPIHeaderV1 chart, *g_fullHeaderList) {

        // list does not contain any deleted chart id's

        int curationState = curationStateCombo->currentIndex();
        // check curated first
        if (curationState == 0 ||
                (curationState == 1 && chart.Curated) ||
                (curationState == 2 && !chart.Curated ) ) {

            //check own chart only
            if (!ownChartsOnly->isChecked() ||
                    (ownChartsOnly->isChecked() && chart.CreatorId == g_currentAthleteId)) {

                // then check language (observe that we have an additional entry "all" in the combo !
                if (langCombo->currentIndex() == 0 ||
                        (langCombo->currentIndex() > 0 && CloudDBCommon::cloudDBLangsIds[langCombo->currentIndex()-1] == chart.Language )) {

                    // at last check text filter
                    if (g_textFilterActive) {
                        // search with filter Keywords
                        QString chartInfo = chart.Name + " " + chart.Description;
                        foreach (QString search, searchList) {
                            if (chartInfo.contains(search, Qt::CaseInsensitive)) {
                                g_currentHeaderList->append(chart);
                                break;
                            }
                        }
                    } else {
                        g_currentHeaderList->append(chart);
                    }
                }
            }

        }
    }
    g_currentIndex = 0;

    // now get the data
    updateCurrentPresets(g_currentIndex, const_stepSize);

}


void
CloudDBChartListDialog::textFilterEditingFinished() {
    if (g_textFilterActive) {
        applyAllFilters();
    }
}

void
CloudDBChartListDialog::cellDoubleClicked(int row, int /*column */) {
    ChartAPIv1* chart = new ChartAPIv1;
    if (row >= 0 && row < g_currentHeaderList->size() ) {
        g_networkrequestactive = true;
        if (g_client->getChartByID(g_currentHeaderList->at(row).Id, chart)) {
            CloudDBChartShowPictureDialog showImage(chart->Image);
            showImage.exec();

        }
    }
    g_networkrequestactive = false;
    delete chart;
}


QString
CloudDBChartListDialog::encodeHTML ( const QString& encodeMe )
{
    QString temp;

    for (int index(0); index < encodeMe.size(); index++)
    {
        QChar character(encodeMe.at(index));

        switch (character.unicode())
        {
        case '&':
            temp += "&amp;"; break;

        case '\'':
            temp += "&apos;"; break;

        case '"':
            temp += "&quot;"; break;

        case '<':
            temp += "&lt;"; break;

        case '>':
            temp += "&gt;"; break;

        default:
            temp += character;
            break;
        }
    }

    return temp;
}

CloudDBChartShowPictureDialog::CloudDBChartShowPictureDialog(QByteArray imageData) : imageData(imageData) {

    QRect rec = QApplication::desktop()->screenGeometry();
    imageLabel = new QLabel();
    imageLabel->setMinimumSize(150,100);
    chartImage.loadFromData(imageData);
    QSize size = chartImage.size();
    int w = rec.width()/2;
    int h = (qreal)size.width()*w/size.height();
    imageLabel->setPixmap(chartImage.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    okButton = new QPushButton(tr("Ok"));
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);

    connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(imageLabel);
    layout->addLayout(buttonLayout);

    setLayout(layout);

}

CloudDBChartShowPictureDialog::~CloudDBChartShowPictureDialog() {
    delete imageLabel;
    delete okButton;
}

void
CloudDBChartShowPictureDialog::okClicked() {
    accept();
}


void
CloudDBChartShowPictureDialog::resizeEvent(QResizeEvent *) {

    int w = imageLabel->width();
    int h = imageLabel->height();

    // set a scaled pixmap to a w x h window keeping its aspect ratio
    imageLabel->setPixmap(chartImage.scaled(w,h,Qt::KeepAspectRatio, Qt::SmoothTransformation));

}


//------------------------------------------------------------------------------------------------------------
//  Dialog for publishing Chart Details
//------------------------------------------------------------------------------------------------------------


CloudDBChartObjectDialog::CloudDBChartObjectDialog(ChartAPIv1 data, QString athlete, bool update) : data(data), athlete(athlete), update(update) {

   QLabel *chartName = new QLabel(tr("Chart Name"));
   name = new QLineEdit();
   nameDefault = tr("<Chart Name>");
   name->setMaxLength(50);
   if (update) {
       name->setText(data.Header.Name);
   } else {
       name->setText(nameDefault);
   }
   QRegExp name_rx("^.{5,50}$");
   QValidator *name_validator = new QRegExpValidator(name_rx, this);
   name->setValidator(name_validator);
   nameOk = false;

   QLabel* langLabel = new QLabel(tr("Language"));
   langCombo = new QComboBox();
   foreach (QString lang, CloudDBCommon::cloudDBLangs) {
       langCombo->addItem(lang);
   }
   if (update) {
       data.Header.Language = CloudDBCommon::cloudDBLangsIds.at(langCombo->currentIndex());
       int index = CloudDBCommon::cloudDBLangs.indexOf(data.Header.Language);
       langCombo->setCurrentIndex( index<0 ? 0 : index);
   }

   connect(name, SIGNAL(textChanged(QString)), this, SLOT(nameTextChanged(QString)));
   connect(name, SIGNAL(editingFinished()), this, SLOT(nameEditingFinished()));

   QLabel *nickLabel = new QLabel(tr("Nickname"));
   nickName = new QLineEdit();
   nickName->setMaxLength(50); // reasonable for displayo
   if (update) {
       nickName->setText(data.CreatorNick);
   } else {
       nickName->setText(appsettings->cvalue(athlete, GC_CLOUDDB_NICKNAME, "").toString());
   }
   // regexp: validate / only chars and 0-9 - at least 5 chars long
   QRegExp nick_rx("^[a-zA-Z0-9_]{5,50}$");
   QValidator *nick_validator = new QRegExpValidator(nick_rx, this);
   nickName->setValidator(nick_validator);
   nickNameOk = !nickName->text().isEmpty(); // nickname from properties is ok when loaded

   connect(nickName, SIGNAL(textChanged(QString)), this, SLOT(nickNameTextChanged(QString)));
   connect(nickName, SIGNAL(editingFinished()), this, SLOT(nickNameEditingFinished()));

   QLabel *emailLabel = new QLabel(tr("E-Mail"));
   email = new QLineEdit();
   email->setMaxLength(100);
   if (update) {
       email->setText(data.CreatorEmail);
   } else {

       email->setText(appsettings->cvalue(athlete, GC_CLOUDDB_EMAIL, "").toString());
   }
   // regexp: simple e-mail validation / also allow long domain types
   QRegExp email_rx("^.+@[a-zA-Z_]+\\.[a-zA-Z]{2,10}$");
   QValidator *email_validator = new QRegExpValidator(email_rx, this);
   email->setValidator(email_validator);
   emailOk = !nickName->text().isEmpty(); // email from properties is ok when loaded

   connect(email, SIGNAL(textChanged(QString)), this, SLOT(emailTextChanged(QString)));
   connect(email, SIGNAL(editingFinished()), this, SLOT(emailEditingFinished()));

   QLabel* gcVersionLabel = new QLabel(tr("Version Details"));
   QString versionString = VERSION_STRING;
   versionString.append(" : " + data.Header.GcVersion);
   gcVersionString = new QLabel(versionString);
   QLabel* creatorIdLabel = new QLabel(tr("Creator UUid"));
   creatorId = new QLabel(data.Header.CreatorId);

   QGridLayout *detailsLayout = new QGridLayout;
   detailsLayout->addWidget(chartName, 0, 0, Qt::AlignLeft);
   detailsLayout->addWidget(name, 0, 1);
   detailsLayout->addWidget(langLabel, 0, 2, Qt::AlignLeft);
   detailsLayout->addWidget(langCombo, 0, 3, Qt::AlignLeft);

   detailsLayout->addWidget(nickLabel, 1, 0, Qt::AlignLeft);
   detailsLayout->addWidget(nickName, 1, 1);
   detailsLayout->addWidget(emailLabel, 1, 2, Qt::AlignLeft);
   detailsLayout->addWidget(email, 1, 3);

   detailsLayout->addWidget(gcVersionLabel, 2, 0, Qt::AlignLeft);
   detailsLayout->addWidget(gcVersionString, 2, 1);
   detailsLayout->addWidget(creatorIdLabel, 2, 2, Qt::AlignLeft);
   detailsLayout->addWidget(creatorId, 2, 3);

   description = new QTextEdit();
   description->setAcceptRichText(false);
   descriptionDefault = tr("<Enter the description of the chart here>");
   description->setText(descriptionDefault);
   if (update) {
       description->setText(data.Header.Description);
   } else {
       description->setText(descriptionDefault);
   }
   QPixmap *chartImage = new QPixmap();
   chartImage->loadFromData(data.Image);
   image = new QLabel();
   image->setPixmap(chartImage->scaled(chartImageWidth*2, chartImageHeight*2, Qt::KeepAspectRatio, Qt::SmoothTransformation));

   publishButton = new QPushButton(tr("Publish"), this);
   cancelButton = new QPushButton(tr("Cancel"), this);

   publishButton->setEnabled(true);

   connect(publishButton, SIGNAL(clicked()), this, SLOT(publishClicked()));
   connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));

   QHBoxLayout *buttonLayout = new QHBoxLayout;
   buttonLayout->addWidget(cancelButton);
   buttonLayout->addStretch();
   buttonLayout->addWidget(publishButton);

   QVBoxLayout *mainLayout = new QVBoxLayout(this);
   mainLayout->addLayout(detailsLayout);
   mainLayout->addWidget(description);
   mainLayout->addWidget(image);
   mainLayout->addLayout(buttonLayout);

   setLayout(mainLayout);

}

CloudDBChartObjectDialog::~CloudDBChartObjectDialog() {

}

void
CloudDBChartObjectDialog::publishClicked() {

    // check data consistency

    if (name->text().isEmpty() || name->text() == nameDefault || !nameOk) {
        QMessageBox::warning(0, tr("Export Chart to ClouDB"), QString(tr("Please enter a valid chart name!")));
        return;
    }

    if (nickName->text().isEmpty() || !nickNameOk) {
        QMessageBox::warning(0, tr("Export Chart to CloudDB"), QString(tr("Please enter a nickname!")));
        return;
    }
    if (email->text().isEmpty() || !emailOk) {
        QMessageBox::warning(0, tr("Export Chart to ClouDB"), QString(tr("Please enter a valid e-mail address!")));
        return;
    }

    if (description->toPlainText().isEmpty() || description->toPlainText() == descriptionDefault) {
        QMessageBox::warning(0, tr("Export Chart to ClouDB"), QString(tr("Please enter a sensible chart description!")));
        return;
    }

    if (QMessageBox::question(0, tr("CloudDB"), QString(tr("Do you want to publish this chart definition to CloudDB"))) != QMessageBox::Yes) return;

    data.Header.Name = name->text();
    data.Header.Description = description->toPlainText();
    data.CreatorEmail = email->text();
    data.CreatorNick = nickName->text();
    data.Header.Language = CloudDBCommon::cloudDBLangsIds.at(langCombo->currentIndex());

    if (!update) {
        appsettings->setCValue(athlete, GC_CLOUDDB_NICKNAME, data.CreatorNick);
        appsettings->setCValue(athlete, GC_CLOUDDB_EMAIL, data.CreatorEmail);
    }
    accept();
}


void
CloudDBChartObjectDialog::cancelClicked()  {
    reject();
}

void
CloudDBChartObjectDialog::nickNameTextChanged(QString text)  {

    if (text.isEmpty()) {
        QMessageBox::warning(0, tr("Export Chart to CloudDB"), QString(tr("Please enter a nickname!")));
    } else {
        nickNameOk = false;
    }
}

void
CloudDBChartObjectDialog::nickNameEditingFinished()  {
    // validator check passed
    nickNameOk = true;
}

void
CloudDBChartObjectDialog::emailTextChanged(QString text)  {

    if (text.isEmpty()) {
        QMessageBox::warning(0, tr("Export Chart to CloudDB"), QString(tr("Please enter a valid e-mail address!")));
    } else {
        emailOk = false;
    }
}

void
CloudDBChartObjectDialog::emailEditingFinished()  {
    // validator check passed
    emailOk = true;
}

void
CloudDBChartObjectDialog::nameTextChanged(QString text)  {

    if (text.isEmpty()) {
        QMessageBox::warning(0, tr("Export Chart to CloudDB"), QString(tr("Please enter a chart name!")));
    } else {
        nameOk = false;
    }
}

void
CloudDBChartObjectDialog::nameEditingFinished()  {
    // validator check passed
    nameOk = true;
}





