/*
 * Copyright (c) 2015 Joern Rischmueller (joern.rm@gmail.com)
 * Copyright (c) 2020 Ale Martinez (amtriathlon@gmail.com)
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

#include "CloudDBUserMetric.h"
#include "CloudDBCommon.h"
#include "CloudDBStatus.h"

#include "UserMetricParser.h"
#include "GcUpgrade.h"
#include "Colors.h"

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
#include <QRegularExpressionValidator>

CloudDBUserMetricClient::CloudDBUserMetricClient()
{
    g_nam = new QNetworkAccessManager(this);
    QDir cacheDir(QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation).at(0));
    cacheDir.cdUp();
    g_cacheDir = QString(cacheDir.absolutePath()+"/GoldenCheetahCloudDB");
    QDir newCacheDir(g_cacheDir);
    if (!newCacheDir.exists()) {
        cacheDir.mkdir("GoldenCheetahCloudDB");
    }

    // general handling for sslErrors
    connect(g_nam, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this,
            SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));

    // common definitions used

    g_usermetric_url_base = g_usermetric_url_header = g_usermetriccuration_url_base = g_usermetricdownloadincr_url_base = CloudDBCommon::cloudDBBaseURL;

    g_usermetric_url_base.append("usermetric/");
    g_usermetric_url_header.append("usermetricheader");
    g_usermetriccuration_url_base.append("usermetriccuration/");
    g_usermetricdownloadincr_url_base.append("usermetricuse/");

}
CloudDBUserMetricClient::~CloudDBUserMetricClient() {
    delete g_nam;
}


bool
CloudDBUserMetricClient::postUserMetric(UserMetricAPIv1 usermetric) {

    // check if Athlete ID is filled

    if (usermetric.Header.CreatorId.isEmpty()) return CloudDBCommon::APIresponseOthers;

    // default where it may be necessary
    if (usermetric.Header.Language.isEmpty()) usermetric.Header.Language = "en";

    // first create the JSON object
    // only a subset of fields is required for POST
    QJsonObject json_header;
    CloudDBCommon::marshallAPIHeaderV1Object(json_header, usermetric.Header);

    QJsonObject json;
    json["header"] = json_header;
    json["metrictxml"] = usermetric.UserMetricXML;
    json["creatornick"] = usermetric.CreatorNick;
    json["creatoremail"] = usermetric.CreatorEmail;
    QJsonDocument document;
    document.setObject(json);

    QNetworkRequest request;
    CloudDBCommon::prepareRequest(request, g_usermetric_url_base);
    g_reply = g_nam->post(request, document.toJson());

    // wait for reply (synchronously) and process error codes as necessary
    if (!CloudDBCommon::replyReceivedAndOk(g_reply)) return false;

    CloudDBHeader::setUserMetricHeaderStale(true);
    return true;

}

bool
CloudDBUserMetricClient::putUserMetric(UserMetricAPIv1 usermetric) {

    // we assume all field are filled properly / not further check or modification

    // first create the JSON object / all fields are required for PUT / only LastChanged i
    QJsonObject json_header;
    CloudDBCommon::marshallAPIHeaderV1Object(json_header, usermetric.Header);

    QJsonObject json;
    json["header"] = json_header;
    json["metrictxml"] = usermetric.UserMetricXML;
    json["creatornick"] = usermetric.CreatorNick;
    json["creatoremail"] = usermetric.CreatorEmail;
    QJsonDocument document;
    document.setObject(json);

    QNetworkRequest request;
    CloudDBCommon::prepareRequest(request, g_usermetric_url_base);
    g_reply = g_nam->put(request, document.toJson());

    // wait for reply (synchronously) and process error codes as necessary
    if (!CloudDBCommon::replyReceivedAndOk(g_reply)) return false;

    // cache is stale
    deleteUserMetricCache(usermetric.Header.Id);
    CloudDBHeader::setUserMetricHeaderStale(true);
    return true;

}


bool
CloudDBUserMetricClient::getUserMetricByID(qint64 id, UserMetricAPIv1 *usermetric) {

    // read from Cache first
    if (readUserMetricCache(id, usermetric)) return CloudDBCommon::APIresponseOk;

    // now from GAE
    QNetworkRequest request;
    CloudDBCommon::prepareRequest(request, g_usermetric_url_base+QString::number(id, 10));
    g_reply = g_nam->get(request);

    // wait for reply (synchronously) and process error codes as necessary
    if (!CloudDBCommon::replyReceivedAndOk(g_reply)) return false;

    // call successfull
    QByteArray result = g_reply->readAll();
    QList<UserMetricAPIv1>* usermetrics = new QList<UserMetricAPIv1>;
    unmarshallAPIv1(result, usermetrics);
    if (usermetrics->size() > 0) {
        *usermetric = usermetrics->value(0);
        writeUserMetricCache(usermetric);
        usermetrics->clear();
        delete usermetrics;
        return true;
    }
    delete usermetrics;
    return false;
}

bool
CloudDBUserMetricClient::deleteUserMetricByID(qint64 id) {

    QNetworkRequest request;
    CloudDBCommon::prepareRequest(request, g_usermetric_url_base+QString::number(id, 10));
    g_reply = g_nam->deleteResource(request);

    // wait for reply (synchronously) and process error codes as necessary
    if (!CloudDBCommon::replyReceivedAndOk(g_reply)) return false;

    deleteUserMetricCache(id);
    return true;
}


bool
CloudDBUserMetricClient::curateUserMetricByID(qint64 id, bool newStatus) {

    QUrlQuery query;
    query.addQueryItem("newStatus", (newStatus ? "true": "false"));
    QNetworkRequest request;
    CloudDBCommon::prepareRequest(request, g_usermetriccuration_url_base+QString::number(id, 10), &query);

    // add a payload to "PUT" even though it's not processed
    g_reply = g_nam->put(request, "{ \"id\": \"dummy\" }");

    // wait for reply (synchronously) and process error codes as necessary
    if (!CloudDBCommon::replyReceivedAndOk(g_reply)) return false;

    deleteUserMetricCache(id);
    return true;
}


bool
CloudDBUserMetricClient::getAllUserMetricHeader(QList<CommonAPIHeaderV1>* header) {
    bool request_ok = CloudDBHeader::getAllCachedHeader(header, CloudDBHeader::CloudDB_UserMetric, g_cacheDir, g_usermetric_url_header, g_nam, g_reply);
    if (request_ok && header->size()>0) {
        cleanUserMetricCache(header);
    }
    return request_ok;
}


//
// Trap SSL errors
//
void
CloudDBUserMetricClient::sslErrors(QNetworkReply* reply ,QList<QSslError> errors)
{
    CloudDBCommon::sslErrors(reply, errors);
}

// Internal Methods


bool
CloudDBUserMetricClient::writeUserMetricCache(UserMetricAPIv1 * usermetric) {

    // make sure the subdir exists
    QDir cacheDir(g_cacheDir);
    if (cacheDir.exists()) {
        cacheDir.mkdir("usermetrics");
    } else {
        return false;
    }
    QFile file(g_cacheDir+"/usermetrics/"+QString::number(usermetric->Header.Id)+".dat");
    if (file.exists()) {
       // overwrite data
       file.resize(0);
    }

    if (!file.open(QIODevice::WriteOnly)) return false;
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_4_6);
    // track a version to be able change data structure
    out << usermetric_magic_string;
    out << usermetric_cache_version;
    out << usermetric->Header.LastChanged; // start
    out << usermetric->Header.CreatorId;
    out << usermetric->Header.Curated;
    out << usermetric->Header.Deleted;
    out << usermetric->Header.Description;
    out << usermetric->Header.GcVersion;
    out << usermetric->Header.Id;
    out << usermetric->Header.Language;
    out << usermetric->Header.Name;
    out << usermetric->UserMetricXML;
    out << usermetric->CreatorEmail;
    out << usermetric->CreatorNick;

    file.close();
    return true;
}



bool CloudDBUserMetricClient::readUserMetricCache(qint64 id, UserMetricAPIv1 *usermetric) {

    QFile file(g_cacheDir+"/usermetrics/"+QString::number(id)+".dat");
    if (!file.open(QIODevice::ReadOnly)) return false;
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_4_6);
    // track a version to be able change data structure
    int magic_string;
    int version;

    in >> magic_string;
    if (magic_string != usermetric_magic_string) {
        // wrong file format / close and exit
        file.close();
        return false;
    }

    in >> version;
    if (version != usermetric_cache_version) {
       // change of version, delete old cache entry
       file.remove();
       return false;
    }

    in >> usermetric->Header.LastChanged; // start
    in >> usermetric->Header.CreatorId;
    in >> usermetric->Header.Curated;
    in >> usermetric->Header.Deleted;
    in >> usermetric->Header.Description;
    in >> usermetric->Header.GcVersion;
    in >> usermetric->Header.Id;
    in >> usermetric->Header.Language;
    in >> usermetric->Header.Name;
    in >> usermetric->UserMetricXML;
    in >> usermetric->CreatorEmail;
    in >> usermetric->CreatorNick;

    file.close();
    return true;

}

void
CloudDBUserMetricClient::deleteUserMetricCache(qint64 id) {

    QFile file(g_cacheDir+"/usermetrics/"+QString::number(id)+".dat");
    if (file.exists()) {
       file.remove();
    }

}

void
CloudDBUserMetricClient::cleanUserMetricCache(QList<CommonAPIHeaderV1> *objectHeader) {
    // remove Deleted Entries from Cache
    int deleted = 0;
    QMutableListIterator<CommonAPIHeaderV1> it(*objectHeader);
    while (it.hasNext()) {
        CommonAPIHeaderV1 header = it.next();
        if (header.Deleted) {
            deleteUserMetricCache(header.Id);
            it.remove();
            deleted ++;
        }
    }
    if (deleted > 0) {
        // store cache for next time
        CloudDBHeader::writeHeaderCache(objectHeader, CloudDBHeader::CloudDB_UserMetric, g_cacheDir);
    }

}

bool
CloudDBUserMetricClient::unmarshallAPIv1(QByteArray json, QList<UserMetricAPIv1> *usermetrics) {

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(json, &parseError);

    // all these things should not happen and we have not valid object to return
    if (parseError.error != QJsonParseError::NoError || document.isEmpty() || document.isNull()) {
        return false;
    }

    // do we have a single object or an array ?
    if (document.isObject()) {
        UserMetricAPIv1 usermetric;
        QJsonObject object = document.object();
        unmarshallAPIv1Object(&object, &usermetric);
        QJsonObject header = object["header"].toObject();
        CloudDBCommon::unmarshallAPIHeaderV1Object(&header, &usermetric.Header);
        usermetrics->append(usermetric);

    } else if (document.isArray()) {
        QJsonArray array(document.array());
        for (int i = 0; i< array.size(); i++) {
            QJsonValue value = array.at(i);
            if (value.isObject()) {
                UserMetricAPIv1 usermetric;
                QJsonObject object = value.toObject();
                unmarshallAPIv1Object(&object, &usermetric);
                QJsonObject header = object["header"].toObject();
                CloudDBCommon::unmarshallAPIHeaderV1Object(&header, &usermetric.Header);
                usermetrics->append(usermetric);
            }
        }
    }

    return true;
}

void
CloudDBUserMetricClient::unmarshallAPIv1Object(QJsonObject* object, UserMetricAPIv1* usermetric) {

    usermetric->UserMetricXML = object->value("metrictxml").toString();
    usermetric->CreatorNick = object->value("creatorNick").toString();
    usermetric->CreatorEmail = object->value("creatorEmail").toString();

}




//------------------------------------------------------------------------------------------------------------
//  Dialog to show and retrieve Shared UserMetrics
//------------------------------------------------------------------------------------------------------------

CloudDBUserMetricListDialog::CloudDBUserMetricListDialog() : const_stepSize(5)
{
   g_client = new CloudDBUserMetricClient();
   g_currentHeaderList = new QList<CommonAPIHeaderV1>;
   g_fullHeaderList = new QList<CommonAPIHeaderV1>;
   CloudDBHeader::setUserMetricHeaderStale(true);  //Force Headers to be loaded
   g_currentPresets = new QList<UserMetricWorkingStructure>;
   g_textFilterActive = false;
   g_networkrequestactive = false; // don't allow Dialog to close while we are retrieving data

   showing = new QLabel;
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

   ownUserMetricsOnly = new QCheckBox(tr("My UserMetrics"));
   ownUserMetricsOnly->setChecked(false);

   connect(ownUserMetricsOnly, SIGNAL(toggled(bool)), this, SLOT(ownUserMetricsToggled(bool)));

   curationStateCombo = new QComboBox();
   curationStateCombo->addItem(tr("All"));
   curationStateCombo->addItem(tr("Curated Only"));
   curationStateCombo->addItem(tr("Uncurated Only"));

   // default to curated usermetrics only
   curationStateCombo->setCurrentIndex(1);

   connect(curationStateCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(curationStateFilterChanged(int)));

   langCombo = new QComboBox();
   langCombo->addItem(tr("Any Language"));
   foreach (QString lang, CloudDBCommon::cloudDBLangs) {
       langCombo->addItem(lang);
   }

   textFilterApply = new QPushButton(tr("Search Keyword"));
   textFilterApply->setFixedWidth(180 *dpiXFactor); // To allow proper translation
   textFilter = new QLineEdit;
   g_textFilterActive = false;

   connect(textFilter, SIGNAL(editingFinished()), this, SLOT(textFilterEditingFinished()));
   connect(langCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(languageFilterChanged(int)));
   connect(textFilterApply, SIGNAL(clicked()), this, SLOT(toggleTextFilterApply()));

   filterLayout = new QHBoxLayout;
   filterLayout->addWidget(ownUserMetricsOnly);
   filterLayout->addStretch();
   filterLayout->addWidget(curationStateCombo);
   filterLayout->addStretch();
   filterLayout->addWidget(langCombo);
   filterLayout->addStretch();
   filterLayout->addWidget(textFilterApply);
   filterLayout->addWidget(textFilter);

   textFilter->setVisible(true);

   tableWidget = new QTableWidget(0, 2);
   tableWidget->setContentsMargins(0,0,0,0);
   tableWidget->horizontalHeader()->setStretchLastSection(true);
   tableWidget->horizontalHeader()->setVisible(false);
   tableWidget->verticalHeader()->setVisible(false);
   tableWidget->setShowGrid(true);

   // the name shall have a dedicated size
   tableWidget->setColumnWidth(0, 200*dpiXFactor);

   connect(tableWidget, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(cellDoubleClicked(int,int)));

   // UserGet Role
   addAndCloseUserGetButton = new QPushButton(tr("Download selected usermetric(s)"));
   closeUserGetButton = new QPushButton(tr("Close"));

   addAndCloseUserGetButton->setEnabled(true);
   closeUserGetButton->setEnabled(true);
   closeUserGetButton->setDefault(true);

   connect(addAndCloseUserGetButton, SIGNAL(clicked()), this, SLOT(addAndCloseClicked()));
   connect(closeUserGetButton, SIGNAL(clicked()), this, SLOT(closeClicked()));

   buttonUserGetLayout = new QHBoxLayout;
   buttonUserGetLayout->addStretch();
   buttonUserGetLayout->addWidget(addAndCloseUserGetButton);
   buttonUserGetLayout->addWidget(closeUserGetButton);

   // UserEdit Role
   deleteUserEditButton = new QPushButton(tr("Delete selected usermetric"));
   editUserEditButton = new QPushButton(tr("Edit selected usermetric"));
   closeUserEditButton = new QPushButton(tr("Close"));

   connect(deleteUserEditButton, SIGNAL(clicked()), this, SLOT(deleteUserEdit()));
   connect(editUserEditButton, SIGNAL(clicked()), this, SLOT(editUserEdit()));
   connect(closeUserEditButton, SIGNAL(clicked()), this, SLOT(closeClicked()));

   buttonUserEditLayout = new QHBoxLayout;
   buttonUserEditLayout->addStretch();
   buttonUserEditLayout->addWidget(deleteUserEditButton);
   buttonUserEditLayout->addWidget(editUserEditButton);
   buttonUserEditLayout->addWidget(closeUserEditButton);

   // CuratorEdit Role
   curateCuratorEditButton = new QPushButton(tr("Set selected usermetric 'Curated'"));
   editCuratorEditButton = new QPushButton(tr("Edit selected usermetric"));
   deleteCuratorEditButton = new QPushButton(tr("Delete selected usermetric"));
   closeCuratorButton = new QPushButton(tr("Close"));

   connect(curateCuratorEditButton, SIGNAL(clicked()), this, SLOT(curateCuratorEdit()));
   connect(editCuratorEditButton, SIGNAL(clicked()), this, SLOT(editCuratorEdit()));
   connect(deleteCuratorEditButton, SIGNAL(clicked()), this, SLOT(deleteCuratorEdit()));
   connect(closeCuratorButton, SIGNAL(clicked()), this, SLOT(closeClicked()));

   buttonCuratorEditLayout = new QHBoxLayout;
   buttonCuratorEditLayout->addStretch();
   buttonCuratorEditLayout->addWidget(curateCuratorEditButton);
   buttonCuratorEditLayout->addWidget(editCuratorEditButton);
   buttonCuratorEditLayout->addWidget(deleteCuratorEditButton);
   buttonCuratorEditLayout->addWidget(closeCuratorButton);

   // prepare the main layouts - with different buttons layouts
   mainLayout = new QVBoxLayout;
   mainLayout->addLayout(showingLayout);
   mainLayout->addLayout(filterLayout);
   mainLayout->addWidget(tableWidget);

   mainLayout->addLayout(buttonUserGetLayout);
   mainLayout->addLayout(buttonUserEditLayout);
   mainLayout->addLayout(buttonCuratorEditLayout);

   setMinimumHeight(500 *dpiYFactor);
   setMinimumWidth(700 *dpiXFactor);
   setLayout(mainLayout);

}

CloudDBUserMetricListDialog::~CloudDBUserMetricListDialog() {
    delete g_client;
    delete g_currentHeaderList;
    delete g_fullHeaderList;
    delete g_currentPresets;
}

// block DialogWindow close while networkrequest is processed
void
CloudDBUserMetricListDialog::closeEvent(QCloseEvent* event) {

     if (g_networkrequestactive) {
         event->ignore();
         return;
     }
     QDialog::closeEvent(event);
}

bool
CloudDBUserMetricListDialog::prepareData(QString athlete, CloudDBCommon::UserRole role) {

    g_role = role;
    // and now initialize the dialog
    setVisibleButtonsForRole();

    if (g_role == CloudDBCommon::UserEdit) {
        ownUserMetricsOnly->setChecked(true);
        ownUserMetricsOnly->setEnabled(false);
        setWindowTitle(tr("UserMetric maintenance - Edit or Delete your UserMetrics"));
        tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    } else if (role == CloudDBCommon::CuratorEdit) {
        ownUserMetricsOnly->setChecked(false);
        curationStateCombo->setCurrentIndex(2); // start with uncurated
        setWindowTitle(tr("Curator usermetric maintenance - Curate, Edit or Delete UserMetrics"));
        tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    } else {
        setWindowTitle(tr("Select usermetrics to download"));
        tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    }

    if (CloudDBHeader::isStaleUserMetricHeader()) {
        if (!refreshStaleUserMetricHeader()) return false;
        CloudDBHeader::setUserMetricHeaderStale(false);
    }
    g_currentAthleteId = appsettings->cvalue(athlete, GC_ATHLETE_ID, "").toString();
    applyAllFilters();
    return true;
}


void
CloudDBUserMetricListDialog::updateCurrentPresets(int index, int count) {

    // while getting the data (which may take of few seconds), disable the UI
    resetToStart->setEnabled(false);
    nextSet->setEnabled(false);
    prevSet->setEnabled(false);
    closeUserGetButton->setEnabled(false);
    addAndCloseUserGetButton->setEnabled(false);
    curationStateCombo->setEnabled(false);
    ownUserMetricsOnly->setEnabled(false);
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
    UserMetricAPIv1* usermetric = new UserMetricAPIv1;
    g_networkrequestactive = true;
    bool noError = true;
    for (int i = index; i< index+count && i<g_currentHeaderList->count() && noError; i++) {
        if (g_client->getUserMetricByID(g_currentHeaderList->at(i).Id, usermetric)) {
            UserMetricWorkingStructure preset;
            preset.id = usermetric->Header.Id;
            preset.name = usermetric->Header.Name;
            preset.description = usermetric->Header.Description;
            preset.language = usermetric->Header.Language;
            preset.createdAt = usermetric->Header.LastChanged;
            preset.usermetricXML = usermetric->UserMetricXML;
            preset.creatorNick = usermetric->CreatorNick;
            g_currentPresets->append(preset);
        } else {
            noError = false;
        }
    }
    g_networkrequestactive = false;
    delete usermetric;

    // update table with current list
    tableWidget->clearContents();
    tableWidget->setRowCount(0);

    int usermetricCount = (g_currentHeaderList->size() == 0) ? 0 : g_currentIndex+1;
    int lastIndex = (g_currentIndex+const_stepSize > g_currentHeaderList->size()) ? g_currentHeaderList->size() : g_currentIndex+const_stepSize;

    showing->setText(tr("Showing %1 to %2 of %3 usermetrics / Total uploaded %4")
                     .arg(QString::number(usermetricCount))
                     .arg(QString::number(lastIndex))
                     .arg(QString::number(g_currentHeaderList->size()))
                     .arg(QString::number(g_fullHeaderList->size())));

    for (int i = 0; i< g_currentPresets->size(); i++ ) {
        tableWidget->insertRow(i);
        UserMetricWorkingStructure preset =  g_currentPresets->at(i);

        QTableWidgetItem *newItem = new QTableWidgetItem(preset.name);
        newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
        tableWidget->setItem(i, 0, newItem);
        tableWidget->item(i,0)->setBackground(Qt::darkGray);
        tableWidget->setRowHeight(i, 80*dpiYFactor);

        QString cellText = QString(tr("%1<h4>Last Edited At: %2 - Creator: %3</h4>"))
                .arg(CloudDBCommon::encodeHTML(preset.description))
                .arg(preset.createdAt.date().toString(Qt::ISODate))
                .arg(CloudDBCommon::encodeHTML(preset.creatorNick));

        QTextEdit *formattedText = new QTextEdit;
        formattedText->setHtml(cellText);
        formattedText->setReadOnly(true);
        tableWidget->setCellWidget(i, 1, formattedText);
    }


    // enable UI again
    resetToStart->setEnabled(true);
    nextSet->setEnabled(true);
    prevSet->setEnabled(true);
    closeUserGetButton->setEnabled(true);
    addAndCloseUserGetButton->setEnabled(true);
    curationStateCombo->setEnabled(true);
    ownUserMetricsOnly->setEnabled(true);
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
        ownUserMetricsOnly->setEnabled(false);
    }

}

void
CloudDBUserMetricListDialog::setVisibleButtonsForRole() {
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
CloudDBUserMetricListDialog::refreshStaleUserMetricHeader() {

    g_fullHeaderList->clear();
    g_currentHeaderList->clear();
    g_currentIndex = 0;
    g_networkrequestactive = true;
    if (!g_client->getAllUserMetricHeader(g_fullHeaderList)) {
        g_networkrequestactive = false;
        return false;
    }

    g_networkrequestactive = false;
    return true;
}



void
CloudDBUserMetricListDialog::addAndCloseClicked() {

    // check if an item of the table is selected


    QList<QTableWidgetItem*> selected = tableWidget->selectedItems();
    if (selected.count()>0)
    {
        g_selected.clear();
        // the selectionMode allows multiple selection so get them all
        for (int i = 0; i< selected.count(); i++) {
            QTableWidgetItem* s = selected.at(i);
            if (s->row() >= 0 && s->row() <= g_currentPresets->count()) {
                g_selected << g_currentPresets->at(s->row()).usermetricXML;
            }
        }
        accept();
    }
}


void
CloudDBUserMetricListDialog::closeClicked()  {
    reject();
}


void
CloudDBUserMetricListDialog::resetToStartClicked()  {

    if (g_currentIndex == 0) return;

    g_currentIndex = 0;
    updateCurrentPresets(g_currentIndex, const_stepSize);

}

void
CloudDBUserMetricListDialog::nextSetClicked()  {

    g_currentIndex += const_stepSize;
    if (g_currentIndex >= g_currentHeaderList->size()) {
        g_currentIndex = g_currentHeaderList->size() - const_stepSize;
    }
    if (g_currentIndex < 0) g_currentIndex = 0;
    updateCurrentPresets(g_currentIndex, const_stepSize);

}

void
CloudDBUserMetricListDialog::prevSetClicked()  {

    g_currentIndex -= const_stepSize;
    if (g_currentIndex < 0) g_currentIndex = 0;
    updateCurrentPresets(g_currentIndex, const_stepSize);

}

void
CloudDBUserMetricListDialog::curateCuratorEdit() {

    if (tableWidget->selectedItems().size()>0)
    {
        // the selectionMode allows only 1 item to be selected at a time
        QTableWidgetItem* s = tableWidget->selectedItems().at(0);
        if (s->row() >= 0 && s->row() <= g_currentPresets->count()) {
            qint64 id = g_currentPresets->at(s->row()).id;
            if (!g_client->curateUserMetricByID(id, true)) {
                return;
            }
            // refresh header buffer
            refreshStaleUserMetricHeader();

            // curated usermetric appears on top of the list / and needs to be filtered
            applyAllFilters();
        }
    }

}
void
CloudDBUserMetricListDialog::deleteCuratorEdit(){
   // currently same like User
   deleteUserEdit();
}
void
CloudDBUserMetricListDialog::editCuratorEdit(){
    // currently same like User
    editUserEdit();
}

void
CloudDBUserMetricListDialog::deleteUserEdit(){

    if (tableWidget->selectedItems().size()>0)
    {
        // usermetric selected for deletion - but ask again to be sure
        if (QMessageBox::question(0, tr("UserMetric Maintenance"), QString(tr("Do you really want to delete this usermetric definition ?"))) != QMessageBox::Yes) return;

        // the selectionMode allows only 1 item to be selected at a time
        QTableWidgetItem* s = tableWidget->selectedItems().at(0);
        if (s->row() >= 0 && s->row() <= g_currentPresets->count()) {
            qint64 id = g_currentPresets->at(s->row()).id;
            if (!g_client->deleteUserMetricByID(id)) {
                return;
            }
            // set stale for subsequent list dialog calls
            CloudDBHeader::setUserMetricHeaderStale(true);

            // remove deleted usermetric from both lists
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
CloudDBUserMetricListDialog::editUserEdit(){

    if (tableWidget->selectedItems().size()>0)
    {
        // the selectionMode allows only 1 item to be selected at a time
        QTableWidgetItem* s = tableWidget->selectedItems().at(0);
        if (s->row() >= 0 && s->row() <= g_currentPresets->count()) {
            qint64 id = g_currentPresets->at(s->row()).id;
            UserMetricAPIv1 usermetric;
            if (!g_client->getUserMetricByID(id, &usermetric)) {
                return;
            }

            // now complete the usermetric with for the user manually added fields
            CloudDBUserMetricObjectDialog dialog(usermetric, "", true);
            if (dialog.exec() == QDialog::Accepted) {
                if (g_client->putUserMetric(dialog.getUserMetric())) {
                    refreshStaleUserMetricHeader();
                } else {
                    return;
                }
            } else {
                return;
            }

        }

        // updated usermetric appears on top of the list / and needs to be filtered
        applyAllFilters();

    }
}

void
CloudDBUserMetricListDialog::curationStateFilterChanged(int)  {
   applyAllFilters();
}

void
CloudDBUserMetricListDialog::ownUserMetricsToggled(bool)  {
   applyAllFilters();
}

void
CloudDBUserMetricListDialog::toggleTextFilterApply()  {
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
CloudDBUserMetricListDialog::languageFilterChanged(int) {
    applyAllFilters();
}


void
CloudDBUserMetricListDialog::applyAllFilters() {

    QStringList searchList;
    g_currentHeaderList->clear();
    if (!textFilter->text().isEmpty()) {
        // split by any whitespace
        searchList = textFilter->text().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    }
    foreach (CommonAPIHeaderV1 usermetric, *g_fullHeaderList) {

        // list does not contain any deleted usermetric id's
        int curationState = curationStateCombo->currentIndex();
        // check curated first
        if (curationState == 0 ||
                (curationState == 1 && usermetric.Curated) ||
                (curationState == 2 && !usermetric.Curated ) ) {

            //check own usermetric only
            if (!ownUserMetricsOnly->isChecked() ||
                    (ownUserMetricsOnly->isChecked() && usermetric.CreatorId == g_currentAthleteId)) {

                // then check language (observe that we have an additional entry "all" in the combo !
                if (langCombo->currentIndex() == 0 ||
                        (langCombo->currentIndex() > 0 && CloudDBCommon::cloudDBLangsIds[langCombo->currentIndex()-1] == usermetric.Language )) {

                    // at last check text filter
                    if (g_textFilterActive) {
                        // search with filter Keywords
                        QString usermetricInfo = usermetric.Name + " " + usermetric.Description;
                        foreach (QString search, searchList) {
                            if (usermetricInfo.contains(search, Qt::CaseInsensitive)) {
                                g_currentHeaderList->append(usermetric);
                                break;
                            }
                        }
                    } else {
                        g_currentHeaderList->append(usermetric);
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
CloudDBUserMetricListDialog::textFilterEditingFinished() {
    if (g_textFilterActive) {
        applyAllFilters();
    }
}

void
CloudDBUserMetricListDialog::cellDoubleClicked(int row, int /*column */) {
    UserMetricAPIv1* usermetric = new UserMetricAPIv1;
    if (row >= 0 && row < g_currentHeaderList->size() ) {
        g_networkrequestactive = true;
        if (g_client->getUserMetricByID(g_currentHeaderList->at(g_currentIndex+row).Id, usermetric)) {
            //TODO CloudDBUserMetricShowPictureDialog showImage(usermetric->Image);
            //showImage.exec();

        }
    }
    g_networkrequestactive = false;
    delete usermetric;
}



//------------------------------------------------------------------------------------------------------------
//  Dialog for publishing UserMetric Details
//------------------------------------------------------------------------------------------------------------


CloudDBUserMetricObjectDialog::CloudDBUserMetricObjectDialog(UserMetricAPIv1 data, QString athlete, bool update) : data(data), athlete(athlete), update(update) {

   QLabel *usermetricName = new QLabel(tr("UserMetric Name"));
   name = new QLineEdit();
   nameDefault = tr("<UserMetric Name>");
   name->setMaxLength(50);
   if (data.Header.Name.isEmpty()) {
       name->setText(nameDefault);
       nameOk = false;
   } else {
       name->setText(data.Header.Name);
       nameOk = true; // no need to re-check
   }
   QRegularExpression name_rx("^.{5,50}$");
   QValidator *name_validator = new QRegularExpressionValidator(name_rx, this);
   name->setValidator(name_validator);

   QLabel* langLabel = new QLabel(tr("Language"));
   langCombo = new QComboBox();
   foreach (QString lang, CloudDBCommon::cloudDBLangs) {
       langCombo->addItem(lang);
   }
   if (update) {
       int index = CloudDBCommon::cloudDBLangsIds.indexOf(data.Header.Language);
       langCombo->setCurrentIndex( index<0 ? 0 : index);
   }

   connect(name, SIGNAL(textChanged(QString)), this, SLOT(nameTextChanged(QString)));
   connect(name, SIGNAL(editingFinished()), this, SLOT(nameEditingFinished()));

   QLabel *nickLabel = new QLabel(tr("Nickname"));
   nickName = new QLineEdit();
   if (update) {
       nickName->setText(data.CreatorNick);
   } else {
       nickName->setText(appsettings->cvalue(athlete, GC_NICKNAME, "").toString());
   }

   QLabel *emailLabel = new QLabel(tr("E-Mail"));
   email = new QLineEdit();
   email->setMaxLength(100);
   if (update) {
       email->setText(data.CreatorEmail);
   } else {

       email->setText(appsettings->cvalue(athlete, GC_CLOUDDB_EMAIL, "").toString());
   }
   // regexp: simple e-mail validation / also allow long domain types & subdomains
   QRegularExpression email_rx("^.+@([a-zA-Z0-9-]+\\.)+[a-zA-Z]{2,10}$");
   QValidator *email_validator = new QRegularExpressionValidator(email_rx, this);
   email->setValidator(email_validator);
   emailOk = !email->text().isEmpty(); // email from properties is ok when loaded

   connect(email, SIGNAL(textChanged(QString)), this, SLOT(emailTextChanged(QString)));
   connect(email, SIGNAL(editingFinished()), this, SLOT(emailEditingFinished()));

   QLabel* gcVersionLabel = new QLabel(tr("Version Details"));
   QString versionString = VERSION_STRING;
   versionString.append(" : " + data.Header.GcVersion);
   gcVersionString = new QLabel(versionString);
   QLabel* creatorIdLabel = new QLabel(tr("Creator UUid"));
   creatorId = new QLabel(data.Header.CreatorId);

   QGridLayout *detailsLayout = new QGridLayout;
   detailsLayout->addWidget(usermetricName, 0, 0, Qt::AlignLeft);
   detailsLayout->addWidget(name, 0, 1);

   detailsLayout->addWidget(langLabel, 1, 0, Qt::AlignLeft);
   detailsLayout->addWidget(langCombo, 1, 1, Qt::AlignLeft);

   detailsLayout->addWidget(nickLabel, 2, 0, Qt::AlignLeft);
   detailsLayout->addWidget(nickName, 2, 1);
   detailsLayout->addWidget(emailLabel, 2, 2, Qt::AlignLeft);
   detailsLayout->addWidget(email, 2, 3);

   detailsLayout->addWidget(gcVersionLabel, 3, 0, Qt::AlignLeft);
   detailsLayout->addWidget(gcVersionString, 3, 1);
   detailsLayout->addWidget(creatorIdLabel, 3, 2, Qt::AlignLeft);
   detailsLayout->addWidget(creatorId, 3, 3);

   description = new QTextEdit();
   description->setAcceptRichText(false);
   descriptionDefault = tr("<Enter the description of the usermetric here>");
   description->setText(descriptionDefault);
   if (data.Header.Description.isEmpty()) {
       description->setText(descriptionDefault);
   } else {
       description->setText(data.Header.Description);
   }

   publishButton = new QPushButton(tr("Upload"), this);
   cancelButton = new QPushButton(tr("Cancel"), this);

   publishButton->setEnabled(true);

   connect(publishButton, SIGNAL(clicked()), this, SLOT(publishClicked()));
   connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));

   QHBoxLayout *buttonLayout = new QHBoxLayout;
   buttonLayout->addStretch();
   buttonLayout->addWidget(publishButton);
   buttonLayout->addWidget(cancelButton);

   QVBoxLayout *mainLayout = new QVBoxLayout(this);
   mainLayout->addLayout(detailsLayout);
   mainLayout->addWidget(description);
   mainLayout->addLayout(buttonLayout);

   setLayout(mainLayout);

}

CloudDBUserMetricObjectDialog::~CloudDBUserMetricObjectDialog() {

}

void
CloudDBUserMetricObjectDialog::publishClicked() {

    // check data consistency

    if (name->text().isEmpty() || name->text() == nameDefault || !nameOk) {
        QMessageBox::warning(0, tr("Upload UserMetric"), QString(tr("Please enter a valid usermetric name with min. 5 characters length.")));
        return;
    }

    if (nickName->text().isEmpty()) {
        QMessageBox::warning(0, tr("Upload UserMetric"), QString(tr("Please enter a nickname for this athlete.")));
        return;
    }
    if (email->text().isEmpty() || !emailOk) {
        QMessageBox::warning(0, tr("Upload UserMetric"), QString(tr("Please enter a valid e-mail address.")));
        return;
    }

    if (description->toPlainText().isEmpty() || description->toPlainText() == descriptionDefault) {
        QMessageBox::warning(0, tr("Upload UserMetric"), QString(tr("Please enter a sensible usermetric description.")));
        return;
    }

    if (QMessageBox::question(0, tr("Cloud Upload"), QString(tr("Do you want to upload this usermetric definition ?"))) != QMessageBox::Yes) return;

    data.Header.Name = name->text();
    data.Header.Description = description->toPlainText();
    data.CreatorEmail = email->text();
    data.CreatorNick = nickName->text();
    data.Header.Language = CloudDBCommon::cloudDBLangsIds.at(langCombo->currentIndex());

    if (!update) {
        appsettings->setCValue(athlete, GC_NICKNAME, data.CreatorNick);
        appsettings->setCValue(athlete, GC_CLOUDDB_EMAIL, data.CreatorEmail);
    }
    accept();
}


void
CloudDBUserMetricObjectDialog::cancelClicked()  {
    reject();
}


void
CloudDBUserMetricObjectDialog::emailTextChanged(QString text)  {

    if (text.isEmpty()) {
        QMessageBox::warning(0, tr("Upload UserMetric"), QString(tr("Please enter a valid e-mail address.")));
    } else {
        emailOk = false;
    }
}

void
CloudDBUserMetricObjectDialog::emailEditingFinished()  {
    // validator check passed
    emailOk = true;
}

void
CloudDBUserMetricObjectDialog::nameTextChanged(QString text)  {

    if (text.isEmpty()) {
        QMessageBox::warning(0, tr("Upload UserMetric"), QString(tr("Please enter a valid usermetric name with min. 5 characters length.")));
    } else {
        nameOk = false;
    }
}

void
CloudDBUserMetricObjectDialog::nameEditingFinished()  {
    // validator check passed
    nameOk = true;
}
