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
#include <QEventLoop>
#include <QXmlInputSource>
#include <QXmlSimpleReader>

CloudDBChartClient::CloudDBChartClient()
{
    g_nam = new QNetworkAccessManager(this);
    g_cache = new QNetworkDiskCache(this);
    QDir cacheDir(QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation).at(0));
    cacheDir.cdUp();
    g_cacheDir = QString(cacheDir.absolutePath()+"/GoldenCheetahCloudDB");
    g_cache->setCacheDirectory(g_cacheDir);
    QStorageInfo storageInfo(cacheDir.absolutePath());
    // cache shall be 100 MB - that fits for approx. 1000 charts
    // but we reserve only if there 5 time the space avaiable, if not the default is used
    qint64 cacheSize = 104857600; // cache shall be 100 MB - that fits for approx. 1000 charts / bu
    if (storageInfo.bytesAvailable() > 5* cacheSize) {
       g_cache->setMaximumCacheSize(cacheSize);
    }
    g_nam->setCache(g_cache);

    // general handling for sslErrors
    connect(g_nam, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this,
            SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));

    // common definitions used

    g_chart_url_base = g_chart_url_header = QString("https://%1.appspot.com").arg(GC_CLOUD_DB_APP_NAME);

    g_chart_url_base.append("/v1/chart/");
    g_chart_url_header.append("/v1/chartheader");

    g_header_content_type = QVariant("application/json");

    g_header_basic_auth = "Basic ";
    g_header_basic_auth.append(GC_CLOUD_DB_BASIC_AUTH);

}
CloudDBChartClient::~CloudDBChartClient() {
    delete g_nam;
}


int CloudDBChartClient::postChart(ChartAPIv1 chart) {

    // check if Athlete ID is filled

    if (chart.Header.CreatorId.isEmpty()) return CloudDBCommon::APIresponseOthers;

    // default where it may be necessary
    if (chart.Header.Language.isEmpty()) chart.Header.Language = "en";

    // first create the JSON object
    // only a subset of fields is required for POST
    QJsonObject json_header;
    json_header["name"] = chart.Header.Name;
    json_header["description"] = chart.Header.Description;
    json_header["language"] = chart.Header.Language;
    json_header["gcversion"] = chart.Header.GcVersion;
    json_header["creatorid"] = chart.Header.CreatorId;

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

    QUrl url = QUrl(g_chart_url_base);
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, g_header_content_type);
    request.setRawHeader("Authorization", g_header_basic_auth);
    g_reply = g_nam->post(request, document.toJson());

    // blocking request
    QEventLoop loop;
    connect(g_reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (g_reply->error() != QNetworkReply::NoError) {

        return processReplyStatusCodes(g_reply);

    };

    return CloudDBCommon::APIresponseCreated;

}

int
CloudDBChartClient::getChartByID(qint64 id, ChartAPIv1 *chart) {

    QNetworkRequest request;
    request.setUrl(QUrl(g_chart_url_base+QString::number(id, 10)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, g_header_content_type);
    request.setRawHeader("Authorization", g_header_basic_auth);
    // first try to read from Cache / since Prefer Cache does not work properly - force it !
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);
    g_reply = g_nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(g_reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (g_reply->error() != QNetworkReply::NoError) {

        // not in cache - so read from CloudDB
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        g_reply = g_nam->get(request);

        // blocking request
        QEventLoop loop;
        connect(g_reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();

        if (g_reply->error() != QNetworkReply::NoError) {

            return processReplyStatusCodes(g_reply);

        }
    }
    QByteArray result = g_reply->readAll();
    QList<ChartAPIv1>* charts = new QList<ChartAPIv1>;
    unmarshallAPIv1(result, charts);
    if (charts->size() > 0) {
        *chart = charts->value(0);
        charts->clear();
        delete charts;
        return CloudDBCommon::APIresponseOk;
    }
    delete charts;
    return CloudDBCommon::APIresponseOthers;
}

int CloudDBChartClient::getAllChartHeader(QList<ChartAPIHeaderV1> *chartHeader) {

    QDateTime selectAfter;

    // first check the cache
    chartHeader->clear();
    readHeaderCache(chartHeader);
    if (chartHeader->size()>0) {
        // header are selected from CloudDB sorted - so the first one is always the newest one
        selectAfter = chartHeader->at(0).LastChanged.addSecs(1); // DB has Microseconds - we not - so round up to next full second
    } else {
        // we do not have charts before 2000 :-)
        selectAfter = QDateTime(QDate(2000,01,01));
    }

    // now get the missing data
    QUrlQuery query;
    query.addQueryItem("dateFrom", selectAfter.toString(CloudDBCommon::cloudDBTimeFormat));
    QUrl url(g_chart_url_header);
    url.setQuery(query);
    QNetworkRequest request;
    request.setUrl(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setHeader(QNetworkRequest::ContentTypeHeader, g_header_content_type);
    request.setRawHeader("Authorization", g_header_basic_auth);
    g_reply = g_nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(g_reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (g_reply->error() != QNetworkReply::NoError) {
        return processReplyStatusCodes(g_reply);
    };

    // result (List of Headers and Updated Ids)
    QList<ChartAPIHeaderV1> *latestHeader = new QList<ChartAPIHeaderV1>;
    QByteArray result = g_reply->readAll();
    unmarshallAPIHeaderV1(result, latestHeader);
    QList<quint64> updatedIds;
    foreach (ChartAPIHeaderV1 latest, *latestHeader) {
        updatedIds.append(latest.Id);
    }

    //now merge cache data with selected data

    //first remove duplicate entries from cache which have recently changed/updated on CloudDB
    QMutableListIterator<ChartAPIHeaderV1> it(*chartHeader);
    while (it.hasNext()) {
        quint64 id = it.next().Id;
        if (updatedIds.contains(id) ) {
            // remove from both caches (Chart and ChartHeader)
            g_cache->remove(QUrl(g_chart_url_base+QString::number(id, 10)));
            it.remove();
        }
    }

    //now we have the lastet chart header so add the in the correct sequence / but don't add deleted ones
    ChartAPIHeaderV1 chart;
    while (!latestHeader->isEmpty()) {
        chart = latestHeader->takeLast();
        if (!chart.Deleted) {
            chartHeader->insert(0, chart);
        }
    }
    delete latestHeader;

    // store cache for next time
    writeHeaderCache(chartHeader);

    return CloudDBCommon::APIresponseOk;
}


//
// Trap SSL errors
//
void
CloudDBChartClient::sslErrors(QNetworkReply* reply ,QList<QSslError> errors)
{
    QString errorString = "";
    foreach (const QSslError e, errors ) {
        if (!errorString.isEmpty())
            errorString += ", ";
        errorString += e.errorString();
    }
    QMessageBox::warning(NULL, tr("HTTP"), tr("SSL error(s) has occurred: %1").arg(errorString));
    reply->ignoreSslErrors();
}

// Internal Methods


bool
CloudDBChartClient::writeHeaderCache(QList<ChartAPIHeaderV1>* header) {

   // make sure the subdir exists
   QDir cacheDir (g_cacheDir);
   if (cacheDir.exists()) {
       cacheDir.mkdir("header");
   } else {
       return false;
   }
   QFile file(g_cacheDir+"/header/cache.dat");
   if (!file.open(QIODevice::WriteOnly)) return false;
   QDataStream out(&file);
   out.setVersion(QDataStream::Qt_4_6);
   // track a version to be able change data structure
   out << header_magic_string;
   out << header_cache_version;
   foreach (ChartAPIHeaderV1 h, *header) {
       out << h.Id;
       out << h.Name;
       out << h.Description;
       out << h.Language;
       out << h.GcVersion;
       out << h.LastChanged;
       out << h.CreatorId;
       out << h.Deleted;
       out << h.Curated;
   }
   file.close();
   return true;
}


bool
CloudDBChartClient::readHeaderCache(QList<ChartAPIHeaderV1>* header) {

    QFile file(g_cacheDir+"/header/cache.dat");
    if (!file.open(QIODevice::ReadOnly)) return false;
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_4_6);
    // track a version to be able change data structure
    int magic_string;
    int version;

    in >> magic_string;
    if (magic_string != header_magic_string) {
        // wrong file format / close and exit
        file.close();
        return false;
    }

    in >> version;
    if (version != header_cache_version) {
       // change of version, delete old cach
       file.remove();
       return false;
    }
    ChartAPIHeaderV1 h;
    while (!in.atEnd()) {
        in >> h.Id;
        in >> h.Name;
        in >> h.Description;
        in >> h.Language;
        in >> h.GcVersion;
        in >> h.LastChanged;
        in >> h.CreatorId;
        in >> h.Deleted;
        in >> h.Curated;
        header->append(h);
    }
    file.close();
    return true;


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
        unmarshallAPIHeaderV1Object(&header, &chart.Header);
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
                unmarshallAPIHeaderV1Object(&header, &chart.Header);
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

bool
CloudDBChartClient::unmarshallAPIHeaderV1(QByteArray json, QList<ChartAPIHeaderV1> *charts) {

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(json, &parseError);

    // all these things should not happen and we have not valid object to return
    if (parseError.error != QJsonParseError::NoError || document.isEmpty() || document.isNull()) {
        return false;
    }

    // do we have a single object or an array ?
    if (document.isObject()) {
        ChartAPIHeaderV1 chartHeader;
        QJsonObject object = document.object();
        QJsonObject header = object["header"].toObject();
        unmarshallAPIHeaderV1Object(&header, &chartHeader);
        charts->append(chartHeader);

    } else if (document.isArray()) {
        QJsonArray array(document.array());
        for (int i = 0; i< array.size(); i++) {
            QJsonValue value = array.at(i);
            if (value.isObject()) {
                ChartAPIHeaderV1 chartHeader;
                QJsonObject object = value.toObject();
                QJsonObject header = object["header"].toObject();
                unmarshallAPIHeaderV1Object(&header, &chartHeader);
                charts->append(chartHeader);
            }
        }
    }

    return true;
}

void
CloudDBChartClient::unmarshallAPIHeaderV1Object(QJsonObject* object, ChartAPIHeaderV1* chartHeader) {

    chartHeader->Id = object->value("id").toDouble();
    chartHeader->Name = object->value("name").toString();
    chartHeader->Description = object->value("description").toString();
    chartHeader->Language = object->value("language").toString();
    chartHeader->GcVersion = object->value("gcversion").toString();
    chartHeader->LastChanged = QDateTime::fromString(object->value("lastChange").toString(), CloudDBCommon::cloudDBTimeFormat);
    chartHeader->CreatorId = object->value("creatorId").toString();
    chartHeader->Curated = object->value("curated").toBool();
    chartHeader->Deleted = object->value("deleted").toBool();

}

int CloudDBChartClient::processReplyStatusCodes(QNetworkReply *reply) {

    // PROBLEM - the replies provided are in some of our case not
    // the real HTTP replies - so we do some interpretation to
    // get proper responses to the user
    // main objective is to differentiate "Over Quota" from other problems
    if (reply->error() == QNetworkReply::ServiceUnavailableError) {

        // check for "Over Quota" - checking the body GAE is providing with 2 keywords
        // which should even work when the response text is slighly changed.
        QByteArray body = g_reply->readAll();
        if (body.contains("503") && (body.contains("Quota"))) {
            return CloudDBCommon::APIresponseOverQuota;
        }
    }

    // and here how it should work / jus interpreting what was send through HTTP
    QVariant statusCode = g_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if ( !statusCode.isValid() ) {
        return CloudDBCommon::APIresponseOthers;
    }
    int code = statusCode.toInt();
    switch (code) {
    case CloudDBCommon::APIresponseForbidden :
    case CloudDBCommon::APIresponseOverQuota :
        return code;
    default:
        return CloudDBCommon::APIresponseOthers;
    }
    return CloudDBCommon::APIresponseOthers;

}



//------------------------------------------------------------------------------------------------------------
//  Dialog to show and retrieve Shared Charts
//------------------------------------------------------------------------------------------------------------

// define size if image preview at one place
static const int chartImageWidth = 320;
static const int chartImageHeight = 240;

CloudDBChartImportDialog::CloudDBChartImportDialog() {

   g_client = new CloudDBChartClient();
   g_currentHeaderList = new QList<ChartAPIHeaderV1>;
   g_fullHeaderList = new QList<ChartAPIHeaderV1>;
   g_currentPresets = new QList<ChartImportUIStructure>;
   g_textFilterActive = true; // we always start with "curated" only
   g_networkrequestactive = false; // don't allow Dialog to close while we are retrieving data

   g_stepSize = 10;

   showing = new QLabel;
   showingTextTemplate = tr("Showing %1 to %2 of %3 charts / Total on CloudDB %4");
   resetToStart = new QPushButton(tr("First"));
   nextSet = new QPushButton(tr("Next %1").arg(QString::number(g_stepSize)));
   prevSet = new QPushButton(tr("Prev %1").arg(QString::number(g_stepSize)));
   resetToStart->setEnabled(true);
   nextSet->setDefault(true);
   nextSet->setEnabled(true);
   prevSet->setEnabled(true);

   connect(resetToStart, SIGNAL(clicked()), this, SLOT(resetToStartClicked()));
   connect(nextSet, SIGNAL(clicked()), this, SLOT(nextSetClicked()));
   connect(prevSet, SIGNAL(clicked()), this, SLOT(prevSetClicked()));

   QHBoxLayout *showingLayout = new QHBoxLayout;
   showingLayout->addWidget(showing);
   showingLayout->addStretch();
   showingLayout->addWidget(resetToStart);
   showingLayout->addWidget(prevSet);
   showingLayout->addWidget(nextSet);

   curatedOnly = new QCheckBox(tr("Curated"));
   curatedOnly->setChecked(true);

   connect(curatedOnly, SIGNAL(toggled(bool)), this, SLOT(curatedToggled(bool)));

   ownChartsOnly = new QCheckBox(tr("My Charts"));
   ownChartsOnly->setChecked(false);

   connect(ownChartsOnly, SIGNAL(toggled(bool)), this, SLOT(ownChartsToggled(bool)));

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

   QHBoxLayout *filterLayout = new QHBoxLayout;
   filterLayout->addWidget(curatedOnly);
   filterLayout->addWidget(ownChartsOnly);
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

   addAndCloseButton = new QPushButton(tr("Add selected chart to library"));
   closeButton = new QPushButton(tr("Close without selection"));

   addAndCloseButton->setEnabled(true);
   closeButton->setEnabled(true);
   closeButton->setDefault(true);

   connect(addAndCloseButton, SIGNAL(clicked()), this, SLOT(addAndCloseClicked()));
   connect(closeButton, SIGNAL(clicked()), this, SLOT(closeClicked()));

   QHBoxLayout *buttonLayout = new QHBoxLayout;
   buttonLayout->addWidget(addAndCloseButton);
   buttonLayout->addStretch();
   buttonLayout->addWidget(closeButton);

   QVBoxLayout *mainLayout = new QVBoxLayout;
   mainLayout->addLayout(showingLayout);
   mainLayout->addLayout(filterLayout);
   mainLayout->addWidget(tableWidget);
   mainLayout->addLayout(buttonLayout);

   setLayout(mainLayout);
   setWindowTitle(tr("Select a Chart"));
   setMinimumHeight(500);
   setMinimumWidth(700);

}

CloudDBChartImportDialog::~CloudDBChartImportDialog() {
    delete g_client;
    delete g_currentHeaderList;
    delete g_fullHeaderList;
    delete g_currentPresets;
}

// block DialogWindow close while networkrequest is processed
void
CloudDBChartImportDialog::closeEvent(QCloseEvent* event) {

     if (g_networkrequestactive) {
         event->ignore();
         return;
     }
     QDialog::closeEvent(event);
}

bool
CloudDBChartImportDialog::initialize(QString athlete) {

    if (CloudDBDataStatus::isStaleChartHeader()) {
        g_fullHeaderList->clear();
        g_currentHeaderList->clear();
        g_currentIndex = 0;
        g_stepSize = 10;
        g_networkrequestactive = true;
        int response = g_client->getAllChartHeader(g_fullHeaderList);
        if (response != CloudDBCommon::APIresponseOk) {
            switch (response) {
            case CloudDBCommon::APIresponseOverQuota:
                QMessageBox::warning(0, tr("CloudDB"), QString(tr("Usage has exceeded the free quota - please try again later.")));
                break;
            default:
                QMessageBox::warning(0, tr("CloudDB"), QString(tr("Technical problem reading the charts - please try again later")));
            }
            g_networkrequestactive = false;
            return false;
        }

        g_networkrequestactive = false;
        CloudDBDataStatus::setChartHeaderStale(false);
        // we always start with curated Only / All Charts / and no Filter
        applyAllFilters();
    }
    g_currentAthleteId = appsettings->cvalue(athlete, GC_ATHLETE_ID, "").toString();
    return true;
}

void
CloudDBChartImportDialog::getCurrentPresets(int index, int count) {

    // while getting the data (which may take of few seconds), disable the UI
    resetToStart->setEnabled(false);
    nextSet->setEnabled(false);
    prevSet->setEnabled(false);
    closeButton->setEnabled(false);
    addAndCloseButton->setEnabled(false);

    // now get the presets
    g_currentPresets->clear();
    ChartAPIv1* chart = new ChartAPIv1;
    g_networkrequestactive = true;
    bool noError = true;
    int response;
    for (int i = index; i< index+count && i<g_currentHeaderList->count() && noError; i++) {
        response = g_client->getChartByID(g_currentHeaderList->at(i).Id, chart);
        if (response == CloudDBCommon::APIresponseOk) {

            ChartImportUIStructure preset;
            preset.name = chart->Header.Name;
            preset.description = chart->Header.Description;
            preset.language = chart->Header.Language;
            preset.createdAt = chart->Header.LastChanged;
            preset.image.loadFromData(chart->Image,"JPG");
            QXmlInputSource source;
            source.setData(chart->ChartXML);
            QXmlSimpleReader xmlReader;
            LTMChartParser handler;
            xmlReader.setContentHandler(&handler);
            xmlReader.setErrorHandler(&handler);
            xmlReader.parse( source );
            preset.ltmSettings = handler.getSettings().at(0); //only one LTMSettings Object is stored
            preset.creatorNick = chart->CreatorNick;
            g_currentPresets->append(preset);
        } else {
            switch (response) {
            case CloudDBCommon::APIresponseOverQuota:
                QMessageBox::warning(0, tr("CloudDB"), QString(tr("Usage has exceeded the free quota - please try again later.")));
                break;
            default:
                QMessageBox::warning(0, tr("CloudDB"), QString(tr("Technical problem reading the charts - please try again later")));
            }
            noError = false;
        }
    }
    g_networkrequestactive = false;
    delete chart;

    // update table with current list
    tableWidget->clearContents();
    tableWidget->setRowCount(0);

    int chartCount = (g_currentHeaderList->size() == 0) ? 0 : g_currentIndex+1;
    int lastIndex = (g_currentIndex+g_stepSize > g_currentHeaderList->size()) ? g_currentHeaderList->size() : g_currentIndex+g_stepSize;
    showing->setText(QString(showingTextTemplate)
                     .arg(QString::number(chartCount))
                     .arg(QString::number(lastIndex))
                     .arg(QString::number(g_currentHeaderList->size()))
                     .arg(QString::number(g_fullHeaderList->size())));

    for (int i = 0; i< g_currentPresets->size(); i++ ) {
        tableWidget->insertRow(i);
        ChartImportUIStructure preset =  g_currentPresets->at(i);

        QTableWidgetItem *newPxItem = new QTableWidgetItem("");
        newPxItem->setData(Qt::DecorationRole, QVariant(preset.image.scaled(chartImageWidth, chartImageHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        newPxItem->setSizeHint(QSize(chartImageWidth, chartImageHeight));
        newPxItem->setFlags(newPxItem->flags() & ~Qt::ItemIsEditable);
        tableWidget->setItem(i, 0, newPxItem);
        tableWidget->item(i,0)->setBackgroundColor(Qt::darkGray);
        tableWidget->setRowHeight(i, chartImageHeight+20);

        QString cellText = QString(tr("<h3>%1</h3><h4>Created At: %2 - Creator: %3</h4>%4"))
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
    closeButton->setEnabled(true);
    addAndCloseButton->setEnabled(true);

}


void
CloudDBChartImportDialog::addAndCloseClicked() {

    // check if an item of the table is selected

    if (tableWidget->selectedItems().size()>0)
    {
       // the selectionMode allows only 1 item to be selected at a time
       QTableWidgetItem* s = tableWidget->selectedItems().at(0);
       if (s->row() >= 0 && s->row() <= g_currentPresets->count()) {
          g_selected = g_currentPresets->at(s->row()).ltmSettings;
          g_selected.name = g_selected.title = g_currentPresets->at(s->row()).name;
          accept();
       }
    }
}


void
CloudDBChartImportDialog::closeClicked()  {
    reject();
}


void
CloudDBChartImportDialog::resetToStartClicked()  {

    if (g_currentIndex == 0) return;

    g_currentIndex = 0;
    getCurrentPresets(g_currentIndex, g_stepSize);

}

void
CloudDBChartImportDialog::nextSetClicked()  {

    g_currentIndex += g_stepSize;
    if (g_currentIndex >= g_currentHeaderList->size()) {
        g_currentIndex = g_currentHeaderList->size() - g_stepSize;
    }
    if (g_currentIndex < 0) g_currentIndex = 0;
    getCurrentPresets(g_currentIndex, g_stepSize);

}

void
CloudDBChartImportDialog::prevSetClicked()  {

    g_currentIndex -= g_stepSize;
    if (g_currentIndex < 0) g_currentIndex = 0;
    getCurrentPresets(g_currentIndex, g_stepSize);

}

void
CloudDBChartImportDialog::curatedToggled(bool)  {
   applyAllFilters();
}

void
CloudDBChartImportDialog::ownChartsToggled(bool)  {
   applyAllFilters();
}

void
CloudDBChartImportDialog::toggleTextFilterApply()  {
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
CloudDBChartImportDialog::languageFilterChanged(int) {
    applyAllFilters();
}


void
CloudDBChartImportDialog::applyAllFilters() {

    QStringList searchList;
    g_currentHeaderList->clear();
    if (!textFilter->text().isEmpty()) {
        // split by any whitespace
        searchList = textFilter->text().split(QRegExp("\\s+"), QString::SkipEmptyParts);
    }
    foreach (ChartAPIHeaderV1 chart, *g_fullHeaderList) {

        // check curated first
        if (!curatedOnly->isChecked() ||
                (curatedOnly->isChecked() && chart.Curated)) {

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
    getCurrentPresets(g_currentIndex, g_stepSize);

}


void CloudDBChartImportDialog::textFilterEditingFinished() {
    if (g_textFilterActive) {
        applyAllFilters();
    }
}

QString
CloudDBChartImportDialog::encodeHTML ( const QString& encodeMe )
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

//------------------------------------------------------------------------------------------------------------
//  Dialog for publishing Chart Details
//------------------------------------------------------------------------------------------------------------


CloudDBChartPublishDialog::CloudDBChartPublishDialog(ChartAPIv1 data, QString athlete) : data(data), athlete(athlete) {

   QLabel *chartName = new QLabel(tr("Chart Name"));
   name = new QLineEdit();
   nameDefault = tr("<Chart Name>");
   name->setMaxLength(50);
   name->setText(nameDefault);
   QRegExp name_rx("^.{5,50}$");
   QValidator *name_validator = new QRegExpValidator(name_rx, this);
   name->setValidator(name_validator);
   nameOk = false;

   QLabel* langLabel = new QLabel(tr("Language"));
   langCombo = new QComboBox();
   foreach (QString lang, CloudDBCommon::cloudDBLangs) {
       langCombo->addItem(lang);
   }

   connect(name, SIGNAL(textChanged(QString)), this, SLOT(nameTextChanged(QString)));
   connect(name, SIGNAL(editingFinished()), this, SLOT(nameEditingFinished()));

   QLabel *nickLabel = new QLabel(tr("Nickname"));
   nickName = new QLineEdit();
   nickName->setMaxLength(50); // reasonable for displayo
   nickName->setText(appsettings->cvalue(athlete, GC_CLOUDDB_NICKNAME, "").toString());
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
   email->setText(appsettings->cvalue(athlete, GC_CLOUDDB_EMAIL, "").toString());
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

CloudDBChartPublishDialog::~CloudDBChartPublishDialog() {

}

void
CloudDBChartPublishDialog::publishClicked() {

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

    appsettings->setCValue(athlete, GC_CLOUDDB_NICKNAME, data.CreatorNick);
    appsettings->setCValue(athlete, GC_CLOUDDB_EMAIL, data.CreatorEmail);
    accept();
}


void
CloudDBChartPublishDialog::cancelClicked()  {
    reject();
}

void
CloudDBChartPublishDialog::nickNameTextChanged(QString text)  {

    if (text.isEmpty()) {
        QMessageBox::warning(0, tr("Export Chart to CloudDB"), QString(tr("Please enter a nickname!")));
    } else {
        nickNameOk = false;
    }
}

void
CloudDBChartPublishDialog::nickNameEditingFinished()  {
    // validator check passed
    nickNameOk = true;
}

void
CloudDBChartPublishDialog::emailTextChanged(QString text)  {

    if (text.isEmpty()) {
        QMessageBox::warning(0, tr("Export Chart to CloudDB"), QString(tr("Please enter a valid e-mail address!")));
    } else {
        emailOk = false;
    }
}

void
CloudDBChartPublishDialog::emailEditingFinished()  {
    // validator check passed
    emailOk = true;
}

void
CloudDBChartPublishDialog::nameTextChanged(QString text)  {

    if (text.isEmpty()) {
        QMessageBox::warning(0, tr("Export Chart to CloudDB"), QString(tr("Please enter a chart name!")));
    } else {
        nameOk = false;
    }
}

void
CloudDBChartPublishDialog::nameEditingFinished()  {
    // validator check passed
    nameOk = true;
}





