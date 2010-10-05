/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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

#include "TwitterDialog.h"
#include "Settings.h"
#include <QHttp>
#include <QUrl>
#include "TimeUtils.h"

TwitterDialog::TwitterDialog(MainWindow *mainWindow, RideItem *item) :
    mainWindow(mainWindow)
{
    ride = item;
    settings = GetApplicationSettings();
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Tweet Your Ride");
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *groupBox = new QGroupBox(tr("Choose which metrics you wish to tweet: "));

    workoutTimeChk = new QCheckBox(tr("Workout Time"));
    timeRidingChk = new QCheckBox(tr("Time Riding"));
    totalDistanceChk = new QCheckBox(tr("Total Distance"));
    elevationGainChk = new QCheckBox(tr("Elevation Gain"));
    totalWorkChk = new QCheckBox(tr("Total Work (kJ)"));
    averageSpeedChk = new QCheckBox(tr("Average Speed"));
    averagePowerChk = new QCheckBox(tr("Average Power"));
    averageHRMChk = new QCheckBox(tr("Average Heart Rate"));
    averageCadenceChk = new QCheckBox(tr("Average Cadence"));
    maxPowerChk = new QCheckBox(tr("Max Power"));
    maxHRMChk = new QCheckBox(tr("Max Heart Rate"));

    QGridLayout *vbox = new QGridLayout();
    vbox->addWidget(workoutTimeChk,0,0);
    vbox->addWidget(timeRidingChk,0,1);
    vbox->addWidget(totalWorkChk,1,0);
    vbox->addWidget(totalDistanceChk,1,1);
    vbox->addWidget(elevationGainChk,2,0);
    vbox->addWidget(averageSpeedChk,2,1);
    vbox->addWidget(averagePowerChk,3,0);
    vbox->addWidget(averageHRMChk,3,1);
    vbox->addWidget(averageCadenceChk,4,0);
    vbox->addWidget(maxPowerChk,4,1);
    vbox->addWidget(maxHRMChk,5,0);
    groupBox->setLayout(vbox);

    QHBoxLayout *twitterMetricLayout = new QHBoxLayout;
    QLabel *twitterLabel = new QLabel(tr("Twitter Message:"));
    twitterMessageEdit = new QLineEdit();
    twitterMetricLayout->addWidget(twitterLabel);
    twitterMetricLayout->addWidget(twitterMessageEdit);
    twitterLengthLabel = new QLineEdit(tr("Message Length: "));
    twitterLengthLabel->setEnabled(false);
    twitterMetricLayout->addWidget(twitterLengthLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    tweetButton = new QPushButton(tr("&Tweet Ride"), this);
    buttonLayout->addWidget(tweetButton);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addWidget(groupBox);
    mainLayout->addLayout(twitterMetricLayout);
    mainLayout->addLayout(buttonLayout);

    connect(tweetButton, SIGNAL(clicked()), this, SLOT(tweetCurrentRide()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    connect(workoutTimeChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    connect(timeRidingChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    connect(totalDistanceChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    connect(elevationGainChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    connect(totalWorkChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    connect(averageSpeedChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    connect(averagePowerChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    connect(averageHRMChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    connect(averageCadenceChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    connect(maxPowerChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    connect(maxHRMChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    connect(twitterMessageEdit, SIGNAL(textChanged(QString)),  this, SLOT(tweetMsgChange(QString)));
}
void
TwitterDialog::tweetCurrentRide()
{

    QString strToken = settings->value(GC_TWITTER_TOKEN).toString();
    QString strSecret = settings->value(GC_TWITTER_SECRET).toString();

    QString s_token = QString(strToken);
    QString s_secret = QString(strSecret);

    if(s_token.isEmpty() || s_secret.isEmpty()) {
      #ifdef Q_OS_MACX
      #define GC_PREF tr("Golden Cheetah->Preferences")
      #else
      #define GC_PREF tr("Tools->Options")
      #endif
      QString advise = QString(tr("Error fetching OAuth credentials.  Please make sure to complete the twitter authorization procedure found under %1.")).arg(GC_PREF);
      QMessageBox oautherr(QMessageBox::Critical, tr("OAuth Error"), advise);
      oautherr.exec();
      return;
    }

    char *postarg = NULL;
    QString qurl = "http://api.twitter.com/1/statuses/update.json?status=";

    QString twitterMsg = getTwitterMessage();

    if(twitterMsg.length() > 140) {
      QMessageBox tweetlengtherr(QMessageBox::Critical, tr("Tweet Length Error"), tr("Tweet must be 140 characters or fewer."));
      tweetlengtherr.exec();
      return;
    }

    const QString strUrl = QUrl::toPercentEncoding(twitterMsg);
    qurl.append(strUrl);
    const char *req_url = oauth_sign_url2(qurl.toLatin1(), &postarg, OA_HMAC, NULL, GC_TWITTER_CONSUMER_KEY, GC_TWITTER_CONSUMER_SECRET, s_token.toLatin1(), s_secret.toLatin1());
    const char *strreply = oauth_http_post(req_url,postarg);

    QString post_reply = QString(strreply);

    if(!post_reply.contains("created_at", Qt::CaseInsensitive)) {
      QMessageBox oautherr(QMessageBox::Critical, tr("Error Posting Tweet"), tr("There was an error connecting to Twitter.  Check your network connection and try again."));
      oautherr.setDetailedText(post_reply);
      oautherr.exec();
      return;
    }

    if(postarg) free(postarg);

    accept();

}

QString TwitterDialog::getTwitterMessage()
{
    RideMetricPtr m;
    double tmp;
    QString twitterMsg;
    QVariant unit = settings->value(GC_UNIT);
    bool useMetricUnits = (unit.toString() == "Metric");

    if(workoutTimeChk->isChecked())
    {
        m = ride->metrics.value("workout_time");
        tmp = round(m->value(true));
        QString msg = QString("Workout Time: %1").arg(time_to_string(tmp));
        twitterMsg.append(msg + " ");
    }

    if(timeRidingChk->isChecked())
    {
        m = ride->metrics.value("time_riding");
        tmp = round(m->value(true));
        QString msg = QString("Time Riding: %1").arg(time_to_string(tmp));
        twitterMsg.append(msg + " ");
    }

    if(totalDistanceChk->isChecked())
    {
        m = ride->metrics.value("total_distance");
        QString msg = QString("Total Distance: %1" + (useMetricUnits ? tr("km") : tr("mi"))).arg(m->value(useMetricUnits),1,'f',1);
        twitterMsg.append(msg + " ");
    }

    if(elevationGainChk->isChecked())
    {
        m = ride->metrics.value("elevation_gain");
        tmp = round(m->value(useMetricUnits));
        QString msg = QString("Elevation Gained: %1"+ (useMetricUnits ? tr("m") : tr("ft"))).arg(tmp);
        twitterMsg.append(msg + " ");
    }

    if(totalWorkChk->isChecked())
    {
        m = ride->metrics.value("total_work");
        tmp = round(m->value(true));
        QString msg = QString("Total Work: %1"+ tr("kJ")).arg(tmp);
        twitterMsg.append(msg + " ");
    }

    if(averageSpeedChk->isChecked())
    {
        m = ride->metrics.value("average_speed");
        QString msg = QString("Average Speed: %1"+ (useMetricUnits ? tr("km/h") : tr("mph"))).arg(m->value(useMetricUnits),1,'f',1);
        twitterMsg.append(msg + " ");
    }

    if(averagePowerChk->isChecked())
    {
        m = ride->metrics.value("average_power");
        tmp = round(m->value(true));
        QString msg = QString("Average Power: %1 "+ tr("watts")).arg(tmp);
        twitterMsg.append(msg + " ");
    }

    if(averageHRMChk->isChecked())
    {
        m = ride->metrics.value("average_hr");
        tmp = round(m->value(true));
        QString msg = QString("Average Heart Rate: %1"+ tr("bpm")).arg(tmp);
        twitterMsg.append(msg + " ");
    }

    if(averageCadenceChk->isChecked())
    {
        m = ride->metrics.value("average_cad");
        tmp = round(m->value(true));
        QString msg = QString("Average Cadence: %1"+ tr("rpm")).arg(tmp);
        twitterMsg.append(msg + " ");
    }

    if(maxPowerChk->isChecked())
    {
        m = ride->metrics.value("max_power");
        tmp = round(m->value(true));
        QString msg = QString("Max Power: %1 "+ tr("watts")).arg(tmp);
        twitterMsg.append(msg + " ");
    }

    if(maxHRMChk->isChecked())
    {
        m = ride->metrics.value("max_heartrate");
        tmp = round(m->value(true));
        QString msg = QString("Max Heart Rate: %1"+ tr("bpm")).arg(tmp);
        twitterMsg.append(msg + " ");
    }

    QString msg = twitterMessageEdit->text();
    if(!msg.endsWith(" "))
        msg.append(" ");

    QString entireTweet = QString(msg + twitterMsg + "#goldencheetah");

    return entireTweet;

}

void TwitterDialog::onCheck(int state)
{
    QString twitterMessage = getTwitterMessage();
    int tweetLength = twitterMessage.length();
    QString tweetMsgLength = QString(tr("Message Length: %1")).arg(tweetLength);
    twitterLengthLabel->setText(tweetMsgLength);
}

void TwitterDialog::tweetMsgChange(QString)
{
    QString twitterMessage = getTwitterMessage();
    int tweetLength = twitterMessage.length();
    QString tweetMsgLength = QString(tr("Message Length: %1")).arg(tweetLength);
    twitterLengthLabel->setText(tweetMsgLength);
}
