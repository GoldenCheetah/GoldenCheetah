/*
 * Copyright (c) 2009 Eric Murray (ericm@lne.com)
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

#include "ManualRideDialog.h"
#include "MainWindow.h"
#include "Settings.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <QtGui>
#include <math.h>
#include <boost/bind.hpp>
#include "Units.h"

ManualRideDialog::ManualRideDialog(MainWindow *mainWindow,
                                   const QDir &home, bool useMetric) :
    mainWindow(mainWindow), home(home)
{
    useMetricUnits = useMetric;
    int row;

    mainWindow->getBSFactors(timeBS,distanceBS,timeDP,distanceDP);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Manually Enter Ride Data"));

    // ride date
    QLabel *manualDateLabel = new QLabel(tr("Ride date: "), this);
    dateTimeEdit = new QDateTimeEdit( QDateTime::currentDateTime(), this );
    // Wed 6/24/09 6:55 AM
    dateTimeEdit->setDisplayFormat(tr("ddd MMM d, yyyy  h:mm AP"));
    dateTimeEdit->setCalendarPopup(true);

    // ride length
    QLabel *manualLengthLabel = new QLabel(tr("Ride length: "), this);
    QHBoxLayout *manualLengthLayout = new QHBoxLayout;
    hrslbl = new QLabel(tr("hours"),this);
    hrslbl->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    hrsentry = new QLineEdit(this);
    QIntValidator * hoursValidator = new QIntValidator(0,99,this);
    //hrsentry->setInputMask("09");
    hrsentry->setValidator(hoursValidator);
    manualLengthLayout->addWidget(hrslbl);
    manualLengthLayout->addWidget(hrsentry);

    minslbl = new QLabel(tr("mins"),this);
    minslbl->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    minsentry = new QLineEdit(this);
    QIntValidator * secsValidator = new QIntValidator(0,60,this);
    //minsentry->setInputMask("00");
    minsentry->setValidator(secsValidator);
    manualLengthLayout->addWidget(minslbl);
    manualLengthLayout->addWidget(minsentry);

    secslbl = new QLabel(tr("secs"),this);
    secslbl->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    secsentry = new QLineEdit(this);
    //secsentry->setInputMask("00");
    secsentry->setValidator(secsValidator);
    manualLengthLayout->addWidget(secslbl);
    manualLengthLayout->addWidget(secsentry);

    // ride distance
    QString *DistanceString = new QString(tr("Distance "));
    if (useMetricUnits)
        DistanceString->append("(" + tr("km") + "):");
    else
        DistanceString->append("(" + tr("miles") + "):");

    QLabel *DistanceLabel = new QLabel(*DistanceString, this);
    QDoubleValidator * distanceValidator = new QDoubleValidator(0,9999,2,this);
    distanceentry = new QLineEdit(this);
    //distanceentry->setInputMask("009.00");
    distanceentry->setValidator(distanceValidator);
	distanceentry->setMaxLength(6);

    // AvgHR
    QLabel *HRLabel = new QLabel(tr("Average HR: "), this);
    HRentry = new QLineEdit(this);
    QIntValidator *hrValidator =  new QIntValidator(30,200,this);
    //HRentry->setInputMask("099");
    HRentry->setValidator(hrValidator);

    // how to estimate BikeScore:
    QLabel *BSEstLabel = NULL;
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();

    QVariant BSmode = settings->value(GC_BIKESCOREMODE);

    estBSbyTimeButton = NULL;
    estBSbyDistButton = NULL;
    if (timeBS || distanceBS)  {
        BSEstLabel = new QLabel(tr("Estimate BikeScore by: "));
        if (timeBS) {
            estBSbyTimeButton = new QRadioButton(tr("Time"));
            // default to time based unless no timeBS factor
            if (BSmode.toString() != "distance")
                estBSbyTimeButton->setDown(true);
        }
        if (distanceBS) {
            estBSbyDistButton = new QRadioButton(tr("Distance"));
            if (BSmode.toString() == "distance" || ! timeBS)
                estBSbyDistButton->setDown(true);
        }
    }

    // BikeScore
    QLabel *ManualBSLabel = new QLabel(tr("BikeScore: "), this);
    QDoubleValidator * bsValidator = new QDoubleValidator(0,9999,2,this);
    BSentry = new QLineEdit(this);
    BSentry->setValidator(bsValidator);
	BSentry->setMaxLength(6);
    BSentry->clear();

    // DanielsPoints
    QLabel *ManualDPLabel = new QLabel(tr("Daniels Points: "), this);
    QDoubleValidator * dpValidator = new QDoubleValidator(0,9999,2,this);
    DPentry = new QLineEdit(this);
    DPentry->setValidator(dpValidator);
	DPentry->setMaxLength(6);
    DPentry->clear();

    // buttons
    enterButton = new QPushButton(tr("&OK"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    // don't let Enter write a new (and possibly incomplete) manual file:
    enterButton->setDefault(false);
    cancelButton->setDefault(true);

    // Set up the layout:
    QGridLayout *glayout = new QGridLayout(this);
    row = 0;
    glayout->addWidget(manualDateLabel, row, 0);
    glayout->addWidget(dateTimeEdit, row, 1, 1, -1);
    row++;

    glayout->addWidget(manualLengthLabel, row, 0);
    glayout->addLayout(manualLengthLayout,row,1,1,-1);
    row++;

    glayout->addWidget(DistanceLabel,row,0);
    glayout->addWidget(distanceentry,row,1,1,-1);
    row++;

    glayout->addWidget(HRLabel,row,0);
    glayout->addWidget(HRentry,row,1,1,-1);
    row++;

    if (timeBS || distanceBS) {
        glayout->addWidget(BSEstLabel,row,0);
        if (estBSbyTimeButton)
            glayout->addWidget(estBSbyTimeButton,row,1,1,-1);
        if (estBSbyDistButton)
            glayout->addWidget(estBSbyDistButton,row,2,1,-1);
        row++;
    }

    glayout->addWidget(ManualBSLabel,row,0);
    glayout->addWidget(BSentry,row,1,1,-1);
    row++;

    glayout->addWidget(ManualDPLabel,row,0);
    glayout->addWidget(DPentry,row,1,1,-1);
    row++;

    glayout->addWidget(enterButton,row,1);
    glayout->addWidget(cancelButton,row,2);

    this->resize(QSize(400,275));

    connect(enterButton, SIGNAL(clicked()), this, SLOT(enterClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(hrsentry, SIGNAL(editingFinished()), this, SLOT(setBsEst()));
    //connect(secsentry, SIGNAL(editingFinished()), this, SLOT(setBsEst()));
    connect(minsentry, SIGNAL(editingFinished()), this, SLOT(setBsEst()));
    connect(distanceentry, SIGNAL(editingFinished()), this, SLOT(setBsEst()));
    if (estBSbyTimeButton)
        connect(estBSbyTimeButton,SIGNAL(clicked()),this, SLOT(bsEstChanged()));
    if (estBSbyDistButton)
        connect(estBSbyDistButton,SIGNAL(clicked()),this, SLOT(bsEstChanged()));
}

void
ManualRideDialog::estBSFromDistance()
{
    // calculate distance-based BS estimate
    if (distanceBS) {
        double dist = distanceentry->text().toFloat();
        // cast to int so QLineEdit doesn't interpret "51.3" as "513"
        BSentry->clear();
        BSentry->insert(QString("%1").arg((int) (dist * distanceBS)));
        DPentry->clear();
        DPentry->insert(QString("%1").arg((int) (dist * distanceDP)));
    }
}

void
ManualRideDialog::estBSFromTime()
{
    // calculate time-based BS estimate
    if (timeBS) {
        double hrs = hrsentry->text().toInt()
            + minsentry->text().toInt() / 60.0
            + secsentry->text().toInt() / 3600.0;
        BSentry->clear();
        BSentry->insert(QString("%1").arg((int)(hrs * timeBS)));
        DPentry->clear();
        DPentry->insert(QString("%1").arg((int)(hrs * timeDP)));
    }

}

void
ManualRideDialog::bsEstChanged()
{
    if (estBSbyDistButton) {
        if (estBSbyDistButton->isChecked())
            estBSFromDistance();
        else
            estBSFromTime();
    }
}

void
ManualRideDialog::setBsEst()
{
    if (estBSbyDistButton) {
        if (estBSbyDistButton->isChecked())
            estBSFromDistance();
        else
            estBSFromTime();
    }
}


void
ManualRideDialog::cancelClicked()
{
    reject();
}

void
ManualRideDialog::enterClicked()
{

    if (!(BSentry->hasAcceptableInput() && DPentry->hasAcceptableInput() && distanceentry->hasAcceptableInput() ) ) {
        QMessageBox::warning( this,
            tr("Values out of range"),
            tr("The values you've entered in:\n ")
            +(!BSentry->hasAcceptableInput() ? "\t BikeScore\n " : "")
            +(!DPentry->hasAcceptableInput() ? "\t Daniels Points\n " : "")
            +(!distanceentry->hasAcceptableInput() ? "\t Distance\n " : "")
            + tr("are invalid, please fix.")
        );
        return;
    }

    // write data to manual entry file

    // use user's time for file:
    QDateTime lt = dateTimeEdit->dateTime().toLocalTime();

    if (filename == "") {
        char tmp[32];


        sprintf(tmp, "%04d_%02d_%02d_%02d_%02d_%02d.gc",
                lt.date().year(), lt.date().month(),
                lt.date().day(), lt.time().hour(),
                lt.time().minute(),
                lt.time().second());

        filename = tmp;
        filepath = home.absolutePath() + "/" + filename;
        FILE *out = fopen(filepath.toAscii().constData(), "r");
        if (out) {
            fclose(out);
            if (QMessageBox::warning(
                    this,
                    tr("Ride Already Downloaded"),
                    tr("This ride appears to have already ")
                    + tr("been downloaded.  Do you want to ")
                    + tr("download it again and overwrite ")
                    + tr("the previous download?"),
                    tr("&Overwrite"), tr("&Cancel"),
                    QString(), 1, 1) == 1) {
                reject();
                return ;
            }
        }
    }

    QString tmpname;
    {
        // QTemporaryFile doesn't actually close the file on .close(); it
        // closes the file when in its destructor.  On Windows, we can't
        // rename an open file.  So let tmp go out of scope before calling
        // rename.

        QString tmpl = home.absoluteFilePath(".ptdl.XXXXXX");
        QTemporaryFile tmp(tmpl);
        tmp.setAutoRemove(false);
        if (!tmp.open()) {
            QMessageBox::critical(this, tr("Error"),
                                  tr("Failed to create temporary file ")
                                  + tmpl + ": " + tmp.error());
            reject();
            return;
        }
        QTextStream out(&tmp);

        tmpname = tmp.fileName(); // after close(), tmp.fileName() is ""

        /*
         *  File format:
         * <!DOCTYPE GoldenCheetah>
         * <ride>
         *     <attributes>
         *         <attribute key="Start time" value="2010/03/18 21:29:43 UTC"/>
         *         <attribute key="Device type" value="Manual CSV"/>
         *     </attributes>
         *     <override>
         *         <metric value="14" name="daniels_points"/>
         *         <metric value="39" name="skiba_bike_score"/>
         *         <metric value="3600" name="time_riding"/>
         *         <metric value="200" name="total_distance"/>
         *         <metric value="145" name="average_heartrate"/>
         *     </override>
         * </ride>

         */

#define DATETIME_FORMAT "yyyy/MM/dd hh:mm:ss' UTC'"
        double secs = (hrsentry->text().toInt() * 3600) +
            (minsentry->text().toInt() * 60) +
            (secsentry->text().toInt());

        out << "<!DOCTYPE GoldenCheetah>\n"
            << "<ride>\n"
            << "\t<attributes>\n"
            << "\t\t<attribute key=\"Start time\" value=\""
            << lt.toUTC().toString(DATETIME_FORMAT) <<"\"/>\n"
            << "\t\t<attribute key=\"Device type\" value=\"Manual CSV\"/>\n"
            << "\t</attributes>\n"
            << "\t<override>\n"

            << "\t\t<metric value=\"" << QString("%1").arg(DPentry->text().toInt())
            << "\" name=\"daniels_points\"/>\n"

            << "\t\t<metric value=\"" << QString("%1").arg(BSentry->text().toInt())
            << "\" name=\"skiba_bike_score\"/>\n"

            << "\t\t<metric value=\"" << QString("%1").arg(secs)
            << "\" name=\"workout_time\"/>\n"

            << "\t\t<metric value=\"" << QString("%1").arg(secs)
            << "\" name=\"time_riding\"/>\n"

            << "\t\t<metric value=\"" << QString("%1").arg(useMetricUnits ?
                                               distanceentry->text().toFloat() :
                                               (1.00/MILES_PER_KM) * distanceentry->text().toFloat())
            << "\" name=\"total_distance\"/>\n"

            << "\t\t<metric value=\"" << QString("%1").arg(HRentry->text().toInt())
            << "\" name=\"average_hr\"/>\n"

            << "\t</override>\n"
            << "</ride>\n"
            ;

        tmp.close();

        // QTemporaryFile initially has permissions set to 0600.
        // Make it readable by everyone.
        tmp.setPermissions(tmp.permissions()
                           | QFile::ReadOwner | QFile::ReadUser
                           | QFile::ReadGroup | QFile::ReadOther);
    }

#ifdef __WIN32__
    // Windows ::rename won't overwrite an existing file.
    if (QFile::exists(filepath)) {
        QFile old(filepath);
        if (!old.remove()) {
            QMessageBox::critical(this, tr("Error"),
                                  tr("Failed to remove existing file ")
                                  + filepath + ": " + old.error());
            QFile::remove(tmpname);
            reject();
        }
    }
#endif

    // Use ::rename() instead of QFile::rename() to get forced overwrite.
    if (rename(QFile::encodeName(tmpname), QFile::encodeName(filepath)) < 0) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to rename ") + tmpname + tr(" to ")
                              + filepath + ": " + strerror(errno));
        QFile::remove(tmpname);
        reject();
        return;
    }

    mainWindow->addRide(filename);
    accept();
}

