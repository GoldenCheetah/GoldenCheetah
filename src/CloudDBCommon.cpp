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

#include "CloudDBCommon.h"
#include "Settings.h"

CloudDBAcceptConditionsDialog::CloudDBAcceptConditionsDialog(QString athlete) : athlete(athlete)
{

    setWindowTitle(QString(tr("GoldenCheetah CloudDB - Terms and Conditions")));
    setMinimumWidth(550);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QPushButton *important = new QPushButton(style()->standardIcon(QStyle::SP_MessageBoxInformation), "", this);
    important->setFixedSize(80,80);
    important->setFlat(true);
    important->setIconSize(QSize(80,80));
    important->setAutoFillBackground(false);
    important->setFocusPolicy(Qt::NoFocus);

    QLabel *header = new QLabel(this);
    header->setWordWrap(true);
    header->setTextFormat(Qt::RichText);
    header->setText(QString(tr("<b><big>Please read terms and conditions carefully before accepting !</big></b>")));

    QHBoxLayout *toprow = new QHBoxLayout;
    toprow->addWidget(important);
    toprow->addWidget(header);
    layout->addLayout(toprow);

    QLabel *text = new QLabel(this);
    text->setWordWrap(true);
    text->setTextFormat(Qt::RichText);
    text->setText(tr("<center><b>What is GoldenCheetah CloudDB</b><br>"
                     "You have requested a function with shares or retrieves GoldenCheetah "
                     "configuration artifacts like chart definitions, worksouts, custom metrics,... "
                     "in a cloud based data storage extension which is running on Google "
                     "App Engine in a data center of variable location as provided by Google "
                     "and/or choosen by the GoldenCheetah team.<br>"
                     "<center><b>License and Privacy Rules for Sharing Artifacts</b><br>"
                     "By sharing an artifact to CloudDB you agree to provide it under "
                     "<a href=\"http://creativecommons.org/licenses/by/4.0/\" target=\"_blank\">"
                     "Creative Common License 4.0</a>. "
                     "When sharing you are asked to enter a 'Nickname' and an 'Email-Address' "
                     "which are stored together with a unique identifier (which is automatically created for your "
                     "athlete) in the CloudDB. This data is only used to support the CloudDB service "
                     "and will not be disclosed or shared for other purposes (e.g. advertisement).<br>"
                     "<center><b>License and Rules for Importing Artifacts</b><br>"
                     "By importing an artifact from CloudDB you accept the license agreement "
                     "<a href=\"http://creativecommons.org/licenses/by/4.0/\" target=\"_blank\">"
                     "Creative Common License 4.0</a>. "
                     "Any artifcats are provided as-is without warranty of any kind, either expressed "
                     "or implied, including, but not limited to, the implied warrenty of fitness "
                     "for a particular purpose. The entire risk related to the quality of the artifact "
                     "is with you. There is no liability regarding damages to your data trough the use of any "
                     "of the artifacts and there will be no quality check/ensurance by the GoldenCheetah team "
                     "for the provided artifacts.<br>"
                     "<center><b>CloudDB Service Availability Disclaimer</b><br>"
                     "The service is provisioned free-of-charge to the users of GoldenCheetah. "
                     "There is no guarantee nor service level agreement on the 7x24 availabiliy of CloudDB services. "
                     "The GoldenCheetah team has the right to delete artifacts from the CloudDB at any time "
                     "and without prior notice to the providing user (e.g. due to quality issues, or due "
                     "to limitations in storage capacity). The GoldenCheetah team has the right to suspend "
                     "or stop the complete service or parts of the service at any time and without prior notice "
                     "to the users.<br>"
                     "<center><b>CloudDB Service Rules</b><br>"
                     "The use of the service is exclusively granted to GoldenCheetah users - and the only permitted way "
                     "to access any of the artifacts is through the GoldenCheetah application. Any attempt "
                     "to access CloudDB data directly is forbidden and will either be blocked or may even end in suspending "
                     "or stopping the service in general.<br>"
                     "<center><b>Accept or Reject</b><br>"
                     "By pressing 'Accept the rules and conditions of CloudDB' you confirm that you have read and fully "
                     "understood the above text and you agree to follow all of these. This decision is recorded so that "
                     "you will not asked again when using CloudDB related functions. <br><br>"
                     "If you are not sure you can choose 'Reject and don't use CloudDB'. In this case this agreement "
                     "request will appear again whenever you request to use a CloudDB related function in GoldenCheetah.<br>"
                     "<center><b>CloudDB Source Code</b><br>"
                     "The source code of CloudDB is published under 'GNU AFFERO GENERAL PUBLIC LICENSE Version 3' here: "
                     "<a href= \"https://github.com/GoldenCheetah/CloudDB\" target=\"_blank\">"
                     "CloudDB Repository</a><br><br>"
                     ));

    scrollText = new QScrollArea();
    scrollText->setWidget(text);
    layout->addWidget(scrollText);

    QHBoxLayout *lastRow = new QHBoxLayout;

    proceedButton = new QPushButton(tr("Accept the rules and conditions of CloudDB"), this);
    proceedButton->setEnabled(true);
    connect(proceedButton, SIGNAL(clicked()), this, SLOT(acceptConditions()));
    abortButton = new QPushButton(tr("Reject and don't use CloudDB"), this);
    abortButton->setDefault(true);
    connect(abortButton, SIGNAL(clicked()), this, SLOT(rejectConditions()));

    lastRow->addWidget(abortButton);
    lastRow->addStretch();
    lastRow->addWidget(proceedButton);
    layout->addLayout(lastRow);

}

void CloudDBAcceptConditionsDialog::acceptConditions() {

    // document the decision
    appsettings->setCValue(athlete, GC_CLOUDDB_TC_ACCEPTANCE, true);
    appsettings->setCValue(athlete, GC_CLOUDDB_TC_ACCEPTANCE_DATE,
                           QDateTime::currentDateTime().toString(Qt::ISODate));

    accept();
}

void CloudDBAcceptConditionsDialog::rejectConditions() {

    // document the decision
    appsettings->setCValue(athlete, GC_CLOUDDB_TC_ACCEPTANCE, false);
    appsettings->setCValue(athlete, GC_CLOUDDB_TC_ACCEPTANCE_DATE,
                           QDateTime::currentDateTime().toString(Qt::ISODate));

    reject();
}

// Initialize static members of CloudDBCommon

QString CloudDBCommon::cloudDBBaseURL = QString("https://%1.appspot.com/v1/").arg(GC_CLOUD_DB_APP_NAME);
QVariant  CloudDBCommon::cloudDBContentType = QVariant("application/json");
QByteArray CloudDBCommon::cloudDBBasicAuth = "Basic " + QByteArray(GC_CLOUD_DB_BASIC_AUTH) ;

QList<QString> CloudDBCommon::cloudDBLangsIds = QList<QString>() << "en" << "fr" << "ja" << "pt-br" << "it" << "de" << "ru" << "cs" << "es" << "pt" << "zh-tw" << "xx";

QList<QString> CloudDBCommon::cloudDBLangs = QList<QString>() << QObject::tr("English") << QObject::tr("French") << QObject::tr("Japanese") << QObject::tr("Portugese (Brazil)") <<
                                             QObject::tr("Italian") << QObject::tr("German") << QObject::tr("Russian") << QObject::tr("Czech") <<
                                             QObject::tr("Spanish") << QObject::tr("Portugese") << QObject::tr("Chinese (Traditional)") << QObject::tr("Other");

QString CloudDBCommon::cloudDBTimeFormat = "yyyy-MM-ddTHH:mm:ssZ";

bool CloudDBCommon::addCuratorFeatures = false;

// Initialize static member for CloudDBDataStatus
bool CloudDBDataStatus::chartHeaderStatusStale = true;











