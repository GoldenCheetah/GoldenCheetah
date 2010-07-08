#include "TwitterDialog.h"
#include "Settings.h"
#include <Qhttp>
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
    totalWorkChk = new QCheckBox(tr("Total Work (kj)"));
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
    setEnabled(false);
    QString twitterMsg = getTwitterMessage();
    QString post = "/statuses/update.xml?status=";
    QString strUrl = QUrl::toPercentEncoding(twitterMsg + " #goldencheetah");
    qDebug() << post << strUrl;
    QHttp *twitter_http_updater = new QHttp();
    twitter_http_updater->setHost("www.twitter.com");
    twitter_http_updater->setUser(settings->value(GC_TWITTER_USERNAME).toString(), settings->value(GC_TWITTER_PASSWORD).toString());
    twitter_http_updater->post(post+strUrl, QByteArray());
    connect(twitter_http_updater, SIGNAL(done(bool)), this, SLOT(updateTwitterStatusFinish(bool)));
}

void TwitterDialog::updateTwitterStatusFinish(bool error)
{
    if(error == true)
        QMessageBox::warning(this, tr("Tweeted Ride"), tr("Tweet Not Sent"));
    else
        QMessageBox::information(this, tr("Tweeted Ride"), tr("Tweet Sent"));

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
        tmp = round(m->value(useMetricUnits));
        QString msg = QString("Total Distance: %1 " + (useMetricUnits ? tr("KM") : tr("Miles"))).arg(tmp);
        qDebug() << msg;
        twitterMsg.append(msg + " ");
    }

    if(elevationGainChk->isChecked())
    {
        m = ride->metrics.value("elevation_gain");
        tmp = round(m->value(useMetricUnits));
        QString msg = QString("Elevation Gained: %1"+ (useMetricUnits ? tr("Meters") : tr("Feet"))).arg(tmp);
        twitterMsg.append(msg + " ");
    }

    if(totalWorkChk->isChecked())
    {
        m = ride->metrics.value("total_work");
        tmp = round(m->value(true));
        QString msg = QString("Total Work (kj): %1").arg(tmp);
        twitterMsg.append(msg + " ");
    }

    if(averageSpeedChk->isChecked())
    {
        m = ride->metrics.value("average_speed");
        tmp = round(m->value(useMetricUnits));
        QString msg = QString("Average Speed: %1"+ (useMetricUnits ? tr("KM/H") : tr("MPH"))).arg(tmp);
        twitterMsg.append(msg + " ");
    }

    if(averagePowerChk->isChecked())
    {
        m = ride->metrics.value("average_power");
        tmp = round(m->value(true));
        QString msg = QString("Average Power: %1 watts").arg(tmp);
        twitterMsg.append(msg + " ");
    }

    if(averageHRMChk->isChecked())
    {
        m = ride->metrics.value("average_hr");
        tmp = round(m->value(true));
        QString msg = QString("Average Heart Rate: %1").arg(tmp);
        twitterMsg.append(msg + " ");
    }

    if(averageCadenceChk->isChecked())
    {
        m = ride->metrics.value("average_cad");
        tmp = round(m->value(true));
        QString msg = QString("Average Cadence: %1").arg(tmp);
        twitterMsg.append(msg + " ");
    }

    if(maxPowerChk->isChecked())
    {
        m = ride->metrics.value("max_power");
        tmp = round(m->value(true));
        QString msg = QString("Max Power: %1 watts").arg(tmp);
        twitterMsg.append(msg + " ");
    }

    if(maxHRMChk->isChecked())
    {
        m = ride->metrics.value("max_heartrate");
        tmp = round(m->value(true));
        QString msg = QString("Max Heart Rate: %1").arg(tmp);
        twitterMsg.append(msg + " ");
    }

    QString msg = twitterMessageEdit->text();
    if(!msg.endsWith(" "))
        msg.append(" ");

    QString entireTweet = QString(msg + twitterMsg);

    return entireTweet;

}

void TwitterDialog::onCheck(int state)
{
    QString twitterMessage = getTwitterMessage();
    int tweetLength = twitterMessage.length();
    tweetLength += 15; //For the hashtag
    QString tweetMsgLength = QString(tr("Message Length: %1")).arg(tweetLength);
    twitterLengthLabel->setText(tweetMsgLength);
}

void TwitterDialog::tweetMsgChange(QString)
{
    QString twitterMessage = getTwitterMessage();
    int tweetLength = twitterMessage.length();
    tweetLength += 15; //For the hashtag
    QString tweetMsgLength = QString(tr("Message Length: %1")).arg(tweetLength);
    twitterLengthLabel->setText(tweetMsgLength);
}
