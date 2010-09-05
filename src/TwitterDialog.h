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

#ifndef TWITTERDIALOG_H
#define TWITTERDIALOG_H

#include <QObject>
#include <QtGui>
#include "MainWindow.h"
#include "RideItem.h"

#ifdef GC_HAVE_LIBOAUTH
extern "C" {
#include <oauth.h>
}
#endif

class TwitterDialog : public QDialog
{
    Q_OBJECT
public:
     TwitterDialog(MainWindow *mainWindow, RideItem *item);

signals:

private slots:
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
