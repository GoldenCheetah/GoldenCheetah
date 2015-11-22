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

#include "Secrets.h"
#include "ChartExchange.h"
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

ChartExchangeClient::ChartExchangeClient()
{

    g_nam = new QNetworkAccessManager(this);

    // general handling for sslErrors
    connect(g_nam, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this,
            SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));

    // check if SSL is available - if not - message and end
    if (!QSslSocket::supportsSsl()) {
        noSSLlib = true;
    }

    // common definitions used

    g_chart_url_base = QString("https://%1.appspot.com").arg(GC_CLOUD_DB_APP_NAME);
    g_chart_url_base.append("/v1/chart/");

    g_header_content_type = QVariant("application/json");

    g_header_basic_auth = "Basic ";
    g_header_basic_auth.append(GC_CLOUD_DB_BASIC_AUTH);

}
ChartExchangeClient::~ChartExchangeClient() {
    delete g_nam;
}


bool
ChartExchangeClient::postChart(ChartAPIv1 chart) {

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
        if ( !statusCode.isValid() )
        return false;

        int httpstatus = statusCode.toInt();

        switch (httpstatus) {

        case 402:
            // out of quota
            break;
        default:
            break;
            // out of quota
        }

        // TODO Add error Handling

    };

    return true;




}

ChartAPIv1
ChartExchangeClient::getChartByID(qint64 id) {

    QString u = g_chart_url_base;
    u.append(QString::number(id, 10));
    QUrl url = QUrl(u);
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

        // TODO Add error Handling

    };

    // result
    QByteArray result = g_reply->readAll();
    QList<ChartAPIv1>* charts = new QList<ChartAPIv1>;
    unmarshallAPIv1(result, charts);
    ChartAPIv1 chart;
    if (charts->size() > 0) {
        chart = charts->at(0);
    }

    return chart;
}

QList<ChartAPIv1>*
ChartExchangeClient::getChartsByDate(QDateTime changedAfter) {

    QString u = g_chart_url_base;
    QUrl url(u);
    // TODO - changedAfter ?
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

        // TODO Add error Handling

    };

    // result
    QByteArray result = g_reply->readAll();
    QList<ChartAPIv1>* charts = new QList<ChartAPIv1>;
    unmarshallAPIv1(result, charts);

    return charts;

}


//
// Trap SSL errors
//
void
ChartExchangeClient::sslErrors(QNetworkReply* reply ,QList<QSslError> errors)
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
ChartExchangeClient::unmarshallAPIv1(QByteArray json, QList<ChartAPIv1> *charts) {

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
ChartExchangeClient::unmarshallAPIv1Object(QJsonObject* object, ChartAPIv1* chart) {

    chart->Id = object->value("id").toDouble();
    chart->Name = object->value("name").toString();
    chart->Description = object->value("description").toString();
    chart->Language = object->value("language").toString();
    chart->GcVersion = object->value("gcversion").toString();
    chart->ChartXML = object->value("chartxml").toString();
    chart->ChartVersion = object->value("chartversion").toInt();
    QString imageString = object->value("image").toString();
    QByteArray ba;
    ba.append(imageString);
    chart->Image = QByteArray::fromBase64(ba);
    chart->LastChanged = QDateTime::fromString(object->value("lastChange").toString(), "yyyy-MM-ddTHH:mm:ssZ");
    chart->CreatorId = object->value("creatorId").toString();
    chart->CreatorNick = object->value("creatorNick").toString();
    chart->CreatorEmail = object->value("creatorEmail").toString();
    chart->Curated = object->value("curated").toBool();

}

//------------------------------------------------------------------------------------------------------------
//  Dialog to show and retrieve Shared Charts
//------------------------------------------------------------------------------------------------------------

// define size if image preview at one place
static const int chartImageWidth = 320;
static const int chartImageHeight = 240;

ChartExchangeRetrieveDialog::ChartExchangeRetrieveDialog() {

   client = new ChartExchangeClient();
   allPresets = getAllSharedPresets();

   setWindowTitle(tr("Select a Chart"));
   setMinimumHeight(500);
   setMinimumWidth(700);

   tableWidget = new QTableWidget(0, 3, this);
   tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
   tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
   QPalette p = tableWidget->palette();
   //TODO set colors for the selection as needed
   tableWidget->setPalette(p);
   tableWidget->setContentsMargins(0,0,0,0);
   tableWidget->horizontalHeader()->setStretchLastSection(true);
   tableWidget->setHorizontalHeaderLabels(QStringList() << tr("Preview") << tr("Name") << tr("Description"));
   tableWidget->verticalHeader()->setVisible(false);


   // the preview shall have a dedicated size
   tableWidget->setColumnWidth(0, chartImageWidth);

   getData();

   selectButton = new QPushButton(tr("Selected"), this);
   cancelButton = new QPushButton(tr("Cancel"), this);

   selectButton->setEnabled(true);
   cancelButton->setEnabled(true);

   connect(selectButton, SIGNAL(clicked()), this, SLOT(selectedClicked()));
   connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
   connect(tableWidget, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(selectedClicked(QTableWidgetItem*)));

   QHBoxLayout *buttonLayout = new QHBoxLayout;
   buttonLayout->addWidget(cancelButton);
   buttonLayout->addStretch();
   buttonLayout->addWidget(selectButton);

   QVBoxLayout *mainLayout = new QVBoxLayout(this);
   mainLayout->addWidget(tableWidget);
   mainLayout->addLayout(buttonLayout);


}

ChartExchangeRetrieveDialog::~ChartExchangeRetrieveDialog() {
    delete client;

}

QList<ChartExchangePresets>*
ChartExchangeRetrieveDialog::getAllSharedPresets() {

   QList<ChartExchangePresets>* presets = new QList<ChartExchangePresets>;

   QList<ChartAPIv1>* allCharts = client->getChartsByDate(QDateTime::currentDateTime());
   for (int i = 0; i<allCharts->length(); i++) {
       ChartAPIv1 chart = allCharts->at(i);
       ChartExchangePresets preset;
       preset.name = chart.Name;
       preset.description = chart.Description;
       preset.image.loadFromData(chart.Image,"JPG");
       QXmlInputSource source;
       source.setData(chart.ChartXML);
       QXmlSimpleReader xmlReader;
       LTMChartParser handler;
       xmlReader.setContentHandler(&handler);
       xmlReader.setErrorHandler(&handler);
       xmlReader.parse( source );
       preset.ltmSettings = handler.getSettings().at(0); //only one LTMSettings Object is stored

       presets->append(preset);
   }
   delete allCharts;

   return presets;

}

void
ChartExchangeRetrieveDialog::getData()
{
    // clean current
    tableWidget->clearContents();
    tableWidget->setRowCount(0);

    for (int i = 0; i< allPresets->size(); i++ ) {
        tableWidget->insertRow(i);
        ChartExchangePresets preset =  allPresets->at(i);

        QTableWidgetItem *newPxItem = new QTableWidgetItem("");
        newPxItem->setData(Qt::DecorationRole, QVariant(preset.image.scaled(chartImageWidth, chartImageHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        newPxItem->setSizeHint(QSize(chartImageWidth, chartImageHeight));
        tableWidget->setItem(i, 0, newPxItem);
        tableWidget->setRowHeight(i, chartImageHeight);

        QTableWidgetItem *newNameItem = new QTableWidgetItem(preset.name);
        tableWidget->setItem(i, 1, newNameItem);

        QTableWidgetItem *newDescriptionItem = new QTableWidgetItem(preset.description);
        tableWidget->setItem(i, 2, newDescriptionItem);

    }
}

void
ChartExchangeRetrieveDialog::selectedClicked() {

    // check if an item of the table is selected

    if (tableWidget->selectedItems().size()>0)
    {
       // the selectionMode allows only 1 item to be selected at a time
       QTableWidgetItem* s = tableWidget->selectedItems().at(0);
       if (s->row() >= 0 && s->row() <= allPresets->count()) {
          selected = allPresets->at(s->row()).ltmSettings;
          selected.name = selected.title = allPresets->at(s->row()).name;
          accept();
       }
    }
}


void
ChartExchangeRetrieveDialog::cancelClicked()  {
    reject();
}

//------------------------------------------------------------------------------------------------------------
//  Dialog for publishing Chart Details
//------------------------------------------------------------------------------------------------------------


ChartExchangePublishDialog::ChartExchangePublishDialog(ChartAPIv1 data) : data(data) {

   QLabel *chartName = new QLabel(tr("Chart Name"));
   name = new QLineEdit();
   name->setText(tr("<Chart Name>"));

   QLabel *nickLabel = new QLabel(tr("Nickname"));
   nickName = new QLineEdit();
   QLabel *emailLabel = new QLabel(tr("E-Mail"));
   email = new QLineEdit();

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
   cancelButton->setEnabled(true);

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


}

ChartExchangePublishDialog::~ChartExchangePublishDialog() {

}


void
ChartExchangePublishDialog::publishClicked() {

    data.Name = name->text();
    data.Description = description->toPlainText();
    data.CreatorEmail = email->text();
    data.CreatorNick = nickName->text();
    accept();

}


void
ChartExchangePublishDialog::cancelClicked()  {
    reject();
}



