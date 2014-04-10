/*
 * Copyright (c) 2010 Justin Knotzke (jknotzke@shampoo.ca)
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
#include "Athlete.h"
#include "Context.h"
#include "Settings.h"
#include <QUrl>
#include "TimeUtils.h"

TwitterDialog::TwitterDialog(Context *context, RideItem *item) :
    context(context)
{
    ride = item;
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Tweet Your Ride"));
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

    QString strToken = appsettings->value(this, GC_TWITTER_TOKEN).toString();
    QString strSecret = appsettings->value(this, GC_TWITTER_SECRET).toString();

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

    // This is for API 1.0
    // QString qurl = "http://api.twitter.com/1/statuses/update.json?status=";
    // This is for API 1.1
    QString qurl = "https://api.twitter.com/1.1/statuses/update.json?status=";

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

QString TwitterDialog::metricToString(const RideMetric *m, SummaryMetrics &metrics, bool metricUnits)
{
    QString s = "%1%2";
    if (m->units(metricUnits) == "seconds" || m->units(metricUnits) == tr("seconds")) {
        s = s.arg(time_to_string(metrics.getForSymbol(m->symbol())));
        s = s.arg(""); // no units
    } else {
        s = s.arg(metrics.getForSymbol(m->symbol()) * (metricUnits ? 1 : m->conversion()), 0, 'f', m->precision() + (metricUnits ? 0 : m->conversionSum()));
        s = s.arg(m->units(metricUnits));
    }
    return s;
}

QString TwitterDialog::getTwitterMessage()
{
    RideMetricFactory &factory = RideMetricFactory::instance();
    QString twitterMesg;

    RideItem *ride = const_cast<RideItem*>(context->currentRideItem());

    SummaryMetrics metrics = context->athlete->metricDB->getRideMetrics(ride->fileName);
    if(workoutTimeChk->isChecked())
    {
        twitterMesg.append(QString(tr("Duration: %1 ")).arg(metricToString(factory.rideMetric("workout_time"), metrics, context->athlete->useMetricUnits)));
    }

    if(timeRidingChk->isChecked())
    {
        twitterMesg.append(QString(tr("Time Riding: %1 ")).arg(metricToString(factory.rideMetric("time_riding"), metrics, context->athlete->useMetricUnits)));
    }

    if(totalDistanceChk->isChecked())
    {
        twitterMesg.append(QString(tr("Distance: %1 ")).arg(metricToString(factory.rideMetric("total_distance"), metrics, context->athlete->useMetricUnits)));
    }

    if(elevationGainChk->isChecked())
    {
        twitterMesg.append(QString(tr("Climbing: %1 ")).arg(metricToString(factory.rideMetric("elevation_gain"), metrics, context->athlete->useMetricUnits)));
    }

    if(totalWorkChk->isChecked())
    {
        twitterMesg.append(QString(tr("Work: %1 ")).arg(metricToString(factory.rideMetric("total_work"), metrics, context->athlete->useMetricUnits)));
    }

    if(averageSpeedChk->isChecked())
    {
        twitterMesg.append(QString(tr("Avg Speed: %1 ")).arg(metricToString(factory.rideMetric("average_speed"), metrics, context->athlete->useMetricUnits)));
    }

    if(averagePowerChk->isChecked())
    {
        twitterMesg.append(QString(tr("Avg Power: %1 ")).arg(metricToString(factory.rideMetric("average_power"), metrics, context->athlete->useMetricUnits)));
    }

    if(averageHRMChk->isChecked())
    {
        twitterMesg.append(QString(tr("Avg HR: %1 ")).arg(metricToString(factory.rideMetric("average_hr"), metrics, context->athlete->useMetricUnits)));
    }

    if(averageCadenceChk->isChecked())
    {
        twitterMesg.append(QString(tr("Avg Cadence: %1 ")).arg(metricToString(factory.rideMetric("average_cad"), metrics, context->athlete->useMetricUnits)));
    }

    if(maxPowerChk->isChecked())
    {
        twitterMesg.append(QString(tr("Max Power: %1 ")).arg(metricToString(factory.rideMetric("max_power"), metrics, context->athlete->useMetricUnits)));
    }

    if(maxHRMChk->isChecked())
    {
        twitterMesg.append(QString(tr("Max HR: %1 ")).arg(metricToString(factory.rideMetric("max_heartrate"), metrics, context->athlete->useMetricUnits)));
    }

    QString msg = twitterMessageEdit->text();
    if(!msg.endsWith(" ")) msg.append(" ");

    QString entireTweet = QString(msg + twitterMesg + "#goldencheetah");

    return entireTweet;

}

void TwitterDialog::onCheck(int /*state*/)
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
