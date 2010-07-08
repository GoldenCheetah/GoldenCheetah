#ifndef TWITTERDIALOG_H
#define TWITTERDIALOG_H

#include <QObject>
#include <QtGui>
#include "MainWindow.h"
#include "RideItem.h"

class TwitterDialog : public QDialog
{
    Q_OBJECT
public:
     TwitterDialog(MainWindow *mainWindow, RideItem *item);

signals:

private slots:
     void updateTwitterStatusFinish(bool error);
     void onCheck(int state);
     void tweetMsgChange(QString);
     void tweetCurrentRide();
private:

     QPushButton *tweetButton;
     QPushButton *cancelButton;
     MainWindow *mainWindow;
     QCheckBox *workoutTimeChk;
     QCheckBox *timeRidingChk;
     QCheckBox *totalDistanceChk;
     QCheckBox *elevationGainChk;
     QCheckBox *totalWorkChk;
     QCheckBox *averageSpeedChk;
     QCheckBox *averagePowerChk;
     QCheckBox *averageHRMChk;
     QCheckBox *averageCadenceChk;
     QCheckBox *maxPowerChk;
     QCheckBox *maxHRMChk;
     QLineEdit *twitterMessageEdit;
     QLineEdit *twitterLengthLabel;

     RideItem *ride;
     QString getTwitterMessage();
     boost::shared_ptr<QSettings> settings;

};

#endif // TWITTERDIALOG_H
