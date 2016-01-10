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
    g_cache->setCacheDirectory(QString(cacheDir.absolutePath()+"/GoldenCheetahCloudDB"));
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


bool
CloudDBChartClient::postChart(ChartAPIv1 chart) {

    // check if Athlete ID is filled

    if (chart.CreatorId.isEmpty()) return false;

    // first create the JSON object
    // only a subset of fields is required for POST
    QJsonObject json;
    json.insert("name", QJsonValue(chart.Name));
    json.insert("description", chart.Description);
    json.insert("language", chart.Language);
    json.insert("gcversion", chart.GcVersion);
    json.insert("chartxml", chart.ChartXML);
    QString image;
    image.append(chart.Image.toBase64());
    json.insert("image", image);
    json.insert("creatorid", chart.CreatorId);
    json.insert("creatornick", chart.CreatorNick);
    json.insert("creatoremail", chart.CreatorEmail);
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

        QVariant statusCode = g_reply->attribute( QNetworkRequest::HttpStatusCodeAttribute );
        if ( !statusCode.isValid() ) {
            QMessageBox::warning(0, tr("CloudDB"), QString(tr("Technical problem uploading the chart - please try again later")));
        } else {

            QMessageBox::warning(0, tr("CloudDB"), QString(tr("Technical problem uploading the chart - Error Code: %1").arg(statusCode.toInt())));
        }
        return false;
    };

    return true;

}

bool CloudDBChartClient::getChartByID(qint64 id, ChartAPIv1 *chart) {

    QString u = g_chart_url_base;
    u.append(QString::number(id, 10));
    QUrl url = QUrl(u);
    QNetworkRequest request;
    request.setUrl(url);
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

            QVariant statusCode = g_reply->attribute( QNetworkRequest::HttpStatusCodeAttribute );
            if ( !statusCode.isValid() ) {
                QMessageBox::warning(0, tr("CloudDB"), QString(tr("Technical problem reading chart details - please try again later")));
            } else {

                QMessageBox::warning(0, tr("CloudDB"), QString(tr("Technical problem reading chart details - Error Code: %1").arg(statusCode.toInt())));
            }
            return false;
        };

    };
    QByteArray result = g_reply->readAll();
    QList<ChartAPIv1>* charts = new QList<ChartAPIv1>;
    unmarshallAPIv1(result, charts);
    if (charts->size() > 0) {
        *chart = charts->value(0);
        charts->clear();
        delete charts;
        return true;
    }
    delete charts;
    return false;
}

bool CloudDBChartClient::getAllChartHeader(QList<ChartAPIHeaderV1> *chartHeader) {

    QString u = g_chart_url_header;
    QUrl url(u);
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, g_header_content_type);
    request.setRawHeader("Authorization", g_header_basic_auth);
    g_reply = g_nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(g_reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (g_reply->error() != QNetworkReply::NoError) {

        QVariant statusCode = g_reply->attribute( QNetworkRequest::HttpStatusCodeAttribute );
        if ( !statusCode.isValid() ) {
           QMessageBox::warning(0, tr("CloudDB"), QString(tr("Technical problem retrieving chart list - please try again later")));
        } else {
           QMessageBox::warning(0, tr("CloudDB"), QString(tr("Technical problem retrieving chart list - Error Code: %1").arg(QString::number(statusCode.toInt()))));
        }
        return false;
    };

    // result
    QByteArray result = g_reply->readAll();
    unmarshallAPIHeaderV1(result, chartHeader);

    return true;

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
        charts->append(chart);

    } else if (document.isArray()) {
        QJsonArray array(document.array());
        for (int i = 0; i< array.size(); i++) {
            QJsonValue value = array.at(i);
            if (value.isObject()) {
                ChartAPIv1 chart;
                QJsonObject object = value.toObject();
                unmarshallAPIv1Object(&object, &chart);
                charts->append(chart);
            }
        }
    }

    return true;
}

void
CloudDBChartClient::unmarshallAPIv1Object(QJsonObject* object, ChartAPIv1* chart) {

    chart->Id = object->value("id").toDouble();
    chart->Name = object->value("name").toString();
    chart->Description = object->value("description").toString();
    chart->Language = object->value("language").toString();
    chart->GcVersion = object->value("gcversion").toString();
    chart->ChartXML = object->value("chartxml").toString();
    QString imageString = object->value("image").toString();
    QByteArray ba;
    ba.append(imageString);
    chart->Image = QByteArray::fromBase64(ba);
    chart->LastChanged = QDateTime::fromString(object->value("lastChange").toString(), "yyyy-MM-ddTHH:mm:ssZ");
    chart->CreatorId = object->value("creatorId").toString();
    chart->CreatorNick = object->value("creatorNick").toString();
    chart->CreatorEmail = object->value("creatorEmail").toString();
    chart->Curated = object->value("curated").toBool();
    chart->Deleted = object->value("deleted").toBool();

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
        unmarshallAPIHeaderV1Object(&object, &chartHeader);
        charts->append(chartHeader);

    } else if (document.isArray()) {
        QJsonArray array(document.array());
        for (int i = 0; i< array.size(); i++) {
            QJsonValue value = array.at(i);
            if (value.isObject()) {
                ChartAPIHeaderV1 chartHeader;
                QJsonObject object = value.toObject();
                unmarshallAPIHeaderV1Object(&object, &chartHeader);
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
    chartHeader->GcVersion = object->value("gcversion").toString();
    chartHeader->LastChanged = QDateTime::fromString(object->value("lastChange").toString(), "yyyy-MM-ddTHH:mm:ssZ");
    chartHeader->Curated = object->value("curated").toBool();
    chartHeader->Deleted = object->value("deleted").toBool();

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
   g_filterActive = true; // we always start with "curated" only

   g_stepSize = 10;

   showing = new QLabel;
   showingTextTemplate = tr("Showing %1 to %2 of %3 charts");
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

   curatedOnly = new QCheckBox(tr("Curated Only"));
   curatedOnly->setChecked(true);
   filterLabel = new QLabel(tr("Filter"));
   filter = new QLineEdit;

   connect(curatedOnly, SIGNAL(toggled(bool)), this, SLOT(curatedToggled(bool)));

   QHBoxLayout *filterLayout = new QHBoxLayout;
   filterLayout->addWidget(curatedOnly);
   filterLayout->addStretch();
   filterLayout->addWidget(filterLabel);
   filterLayout->addWidget(filter);

   // TODO add Filtering
   filterLabel->setVisible(false);
   filter->setVisible(false);

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

void
CloudDBChartImportDialog::initialize() {
    if (g_currentHeaderList->isEmpty()) {
        if (!g_client->getAllChartHeader(g_fullHeaderList)) {
            QMessageBox::warning(0, tr("CloudDB"), QString(tr("Technical problem reading the charts - please try again later")));
            return;
        }
        g_currentIndex = 0;
        g_stepSize = 10;
        // we always start with curated Only and no Filter
        applyFilterAndCurated(curatedOnly->isChecked(), "");
        // get the Charts Header from CloudDB first
        getCurrentPresets(g_currentIndex, g_stepSize);
        updateDialogWithCurrentPresets();
    }
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
    for (int i = index; i< index+count && i<g_currentHeaderList->count(); i++) {
        if (g_client->getChartByID(g_currentHeaderList->at(i).Id, chart)) {

            ChartImportUIStructure preset;
            preset.name = chart->Name;
            preset.description = chart->Description;
            preset.creatorNick = chart->CreatorNick;
            preset.image.loadFromData(chart->Image,"JPG");
            QXmlInputSource source;
            source.setData(chart->ChartXML);
            QXmlSimpleReader xmlReader;
            LTMChartParser handler;
            xmlReader.setContentHandler(&handler);
            xmlReader.setErrorHandler(&handler);
            xmlReader.parse( source );
            preset.ltmSettings = handler.getSettings().at(0); //only one LTMSettings Object is stored

            g_currentPresets->append(preset);
        }
    }
    delete chart;
    // enable UI again
    resetToStart->setEnabled(true);
    nextSet->setEnabled(true);
    prevSet->setEnabled(true);
    closeButton->setEnabled(true);
    addAndCloseButton->setEnabled(true);

}



void
CloudDBChartImportDialog::updateDialogWithCurrentPresets()
{
    // clean current
    tableWidget->clearContents();
    tableWidget->setRowCount(0);

    int lastIndex = g_currentIndex+g_stepSize;
    if (lastIndex > g_currentHeaderList->size()) { lastIndex = g_currentHeaderList->size(); }

    showing->setText(QString(showingTextTemplate)
                     .arg(QString::number(g_currentIndex+1))
                     .arg(QString::number(lastIndex))
                     .arg(QString::number(g_currentHeaderList->size())));

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

        QString cellText = QString(tr("<center><b><big><b>%1</b><br><i>Creator %2</i></big></center><br>%3"))
                .arg(encodeHTML(preset.name))
                .arg(encodeHTML(preset.creatorNick))
                .arg(encodeHTML(preset.description));

        QTextEdit *formattedText = new QTextEdit;
        formattedText->setHtml(cellText);
        formattedText->setReadOnly(true);
        tableWidget->setCellWidget(i,1, formattedText);
    }
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
    updateDialogWithCurrentPresets();

}

void
CloudDBChartImportDialog::nextSetClicked()  {

    g_currentIndex += g_stepSize;
    if (g_currentIndex >= g_currentHeaderList->size()) {
        g_currentIndex = g_currentHeaderList->size() - g_stepSize;
    }
    if (g_currentIndex < 0) g_currentIndex = 0;
    getCurrentPresets(g_currentIndex, g_stepSize);
    updateDialogWithCurrentPresets();

}

void
CloudDBChartImportDialog::prevSetClicked()  {

    g_currentIndex -= g_stepSize;
    if (g_currentIndex < 0) g_currentIndex = 0;
    getCurrentPresets(g_currentIndex, g_stepSize);
    updateDialogWithCurrentPresets();

}

void
CloudDBChartImportDialog::curatedToggled(bool checked)  {
   applyFilterAndCurated(checked, "");
   getCurrentPresets(g_currentIndex, g_stepSize);
   updateDialogWithCurrentPresets();
}

void
CloudDBChartImportDialog::applyFilterAndCurated(bool curated, QString filter) {

    g_currentHeaderList->clear();
    if (curated) {
        for (int i = 0; i< g_fullHeaderList->size(); i++) {
            if (g_fullHeaderList->at(i).Curated) {
                g_currentHeaderList->append(g_fullHeaderList->at(i));
            }
        }
    } else {
        for (int i = 0; i< g_fullHeaderList->size(); i++) {
           g_currentHeaderList->append(g_fullHeaderList->at(i));
        }
    }
    g_currentIndex = 0;
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
   name->setText(tr("<Chart Name>"));

   QLabel *nickLabel = new QLabel(tr("Nickname"));
   nickName = new QLineEdit();
   nickName->setMaxLength(40); // reasonable for displayo
   nickName->setText(appsettings->cvalue(athlete, GC_CLOUDDB_NICKNAME, "").toString());
   // regexp: validate / only chars and 0-9 - at least 5 chars long
   QRegExp nick_rx("^[a-zA-Z0-9_]{5,40}$");
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
   versionString.append(" : " + data.GcVersion);
   gcVersionString = new QLabel(versionString);
   QLabel* creatorIdLabel = new QLabel(tr("Creator UUid"));
   creatorId = new QLabel(data.CreatorId);

   QGridLayout *detailsLayout = new QGridLayout;
   detailsLayout->addWidget(chartName, 0, 0, Qt::AlignLeft);
   detailsLayout->addWidget(name, 0, 1, 1, 3);

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
   description->setText(tr("<Enter the description of the chart here>"));

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
    if (nickName->text().isEmpty() || !nickNameOk) {
        QMessageBox::warning(0, tr("Export Chart to CloudDB"), QString(tr("Please enter a nickname !")));
        return;
    }
    if (email->text().isEmpty() || !emailOk) {
        QMessageBox::warning(0, tr("Export Chart to ClouDB"), QString(tr("Please enter a valid e-mail address !")));
        return;
    }

    if (QMessageBox::question(0, tr("CloudDB"), QString(tr("Do you want to publish this chart definition to CloudDB"))) != QMessageBox::Yes) return;

    data.Name = name->text();
    data.Description = description->toPlainText();
    data.CreatorEmail = email->text();
    data.CreatorNick = nickName->text();

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
        QMessageBox::warning(0, tr("Export Chart to CloudDB"), QString(tr("Please enter a nickname !")));
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
        QMessageBox::warning(0, tr("Export Chart to CloudDB"), QString(tr("Please enter a valid e-mail address !")));
    } else {
        emailOk = false;
    }
}

void
CloudDBChartPublishDialog::emailEditingFinished()  {
    // validator check passed
    emailOk = true;
}




