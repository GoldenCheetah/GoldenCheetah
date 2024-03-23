/*
 * Copyright (c) 2012 Damien Grauser (Damien.Grauser@pev-geneve.ch)  *
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

#include "BlankState.h"
#include <QtGui>
#include "MainWindow.h"
#include "Context.h"
#include "Colors.h"
#include "Athlete.h"

//
// Replace home window when no ride
//
BlankStatePage::BlankStatePage(Context *context) : GcWindow(context), context(context), canShow_(true)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addStretch();
    QHBoxLayout *homeLayout = new QHBoxLayout;
    mainLayout->addLayout(homeLayout);
    homeLayout->setAlignment(Qt::AlignCenter);
    homeLayout->addSpacing(20); // left margin
    setAutoFillBackground(true);
    setProperty("color", QColor(Qt::white));
    showMore(false);

    // left part
    QWidget *left = new QWidget(this);
    leftLayout = new QVBoxLayout(left);
    leftLayout->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    left->setLayout(leftLayout);

    welcomeTitle = new QLabel(left);
    welcomeTitle->setFont(QFont("Helvetica", 30, QFont::Bold, false));
    leftLayout->addWidget(welcomeTitle);

    welcomeText = new QLabel(left);
    welcomeText->setFont(QFont("Helvetica", 16, QFont::Light, false));
    leftLayout->addWidget(welcomeText);

    leftLayout->addSpacing(10);

    homeLayout->addWidget(left);
    homeLayout->addSpacing(50);

    QWidget *right = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(right);
    rightLayout->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    right->setLayout(rightLayout);

    img = new QToolButton(this);
    img->setFocusPolicy(Qt::NoFocus);
    img->setToolButtonStyle(Qt::ToolButtonIconOnly);
    img->setStyleSheet("QToolButton {text-align: left;color : blue;background: transparent}");
    rightLayout->addWidget(img);

    homeLayout->addWidget(right);
    // right margin
    homeLayout->addSpacing(20);

    // control if shown or not in future
    QHBoxLayout *bottomRow = new QHBoxLayout;
    mainLayout->addSpacing(20);
    mainLayout->addLayout(bottomRow);

    dontShow = new QCheckBox(tr("Don't show this next time."), this);
    dontShow->setStyleSheet("QCheckBox {color: black}");
    dontShow->setFocusPolicy(Qt::NoFocus);
    closeButton = new QPushButton(tr("Close"), this);
    closeButton->setStyleSheet("QPushButton {color: black }");
    closeButton->setFocusPolicy(Qt::NoFocus);
    bottomRow->addWidget(dontShow);
    bottomRow->addStretch();
    bottomRow->addWidget(closeButton);

    connect(closeButton, SIGNAL(clicked()), this, SLOT(setCanShow()));
    connect(closeButton, SIGNAL(clicked()), this, SIGNAL(closeClicked()));
}

void
BlankStatePage::setCanShow()
{
    // the view was closed, so set canShow_ off
    canShow_ = false;
    saveState();
}

QPushButton*
BlankStatePage::addToShortCuts(ShortCut shortCut)
{
    //
    // Separator
    //
    if (shortCuts.count()>0) {
        leftLayout->addSpacing(20);
        QFrame* line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        leftLayout->addWidget(line);
    }

    // append to the list of shortcuts
    shortCuts.append(shortCut);

    //
    // Create text and button
    //
    QLabel *shortCutLabel = new QLabel(this);
    shortCutLabel->setWordWrap(true);
    shortCutLabel->setText(shortCut.label);
    shortCutLabel->setFont(QFont("Helvetica", 14, QFont::Light, false));
    leftLayout->addWidget(shortCutLabel);

    QPushButton *shortCutButton = new QPushButton(this);
    shortCutButton->setFocusPolicy(Qt::NoFocus);
    shortCutButton->setText(shortCut.buttonLabel);
    shortCutButton->setIcon(QPixmap(shortCut.buttonIconPath));
    shortCutButton->setIconSize(QSize(40,40));
    //importButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    //importButton->setStyleSheet("QToolButton {text-align: left;color : blue;background: transparent}");
    shortCutButton->setStyleSheet("QPushButton {color: black; border-radius: 10px;border-style: outset; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #DDDDDD, stop: 1 #BBBBBB); border-width: 1px; border-color: #555555;} QPushButton:pressed {background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #BBBBBB, stop: 1 #999999);}");
    shortCutButton->setFixedSize(200*dpiXFactor, 60*dpiYFactor);
    leftLayout->addWidget(shortCutButton);

    return shortCutButton;
}

//
// Replace analysis window when no ride
//
BlankStateAnalysisPage::BlankStateAnalysisPage(Context *context) : BlankStatePage(context)
{  
    dontShow->setChecked(appsettings->cvalue(context->athlete->cyclist, GC_BLANK_ANALYSIS, false).toBool());
    welcomeTitle->setText(tr("Activities"));
    welcomeText->setText(tr("No files ?\nLet's start with some data."));

    img->setIcon(QPixmap(":images/analysis.png"));
    img->setIconSize(QSize(800,330));

    ShortCut scCloud;
    scCloud.label = tr("Connect to cloud service and download");
    scCloud.buttonLabel = tr("Cloud Download");
    scCloud.buttonIconPath = ":images/mac/download.png";
    QPushButton *cloudButton = addToShortCuts(scCloud);
    connect(cloudButton, SIGNAL(clicked()), context->mainWindow, SLOT(importCloud()));


    ShortCut scImport;
    scImport.label = tr("Import files from your disk or usb device");
    scImport.buttonLabel = tr("Import data");
    scImport.buttonIconPath = ":images/mac/download.png";
    QPushButton *importButton = addToShortCuts(scImport);
    connect(importButton, SIGNAL(clicked()), context->mainWindow, SLOT(importFile()));

    ShortCut scDownload;
    scDownload.label = tr("Download from serial device.");
    scDownload.buttonLabel = tr("Download from device");
    scDownload.buttonIconPath = ":images/mac/download.png";
    QPushButton *downloadButton = addToShortCuts(scDownload);
    connect(downloadButton, SIGNAL(clicked()), context->mainWindow, SLOT(downloadRide()));

    canShow_ = !appsettings->cvalue(context->athlete->cyclist, GC_BLANK_ANALYSIS).toBool();
}

//
// Replace home window when no ride
//
BlankStateHomePage::BlankStateHomePage(Context *context) : BlankStatePage(context)
{
    dontShow->setChecked(appsettings->cvalue(context->athlete->cyclist, GC_BLANK_HOME, false).toBool());
    welcomeTitle->setText(tr("Trends"));
    welcomeText->setText(tr("No ride ?\nLet's start with some data."));

    img->setIcon(QPixmap(":images/home.png"));
    img->setIconSize(QSize(800,330));

    ShortCut scImport;
    scImport.label = tr("Import files from your disk or usb device");
    scImport.buttonLabel = tr("Import data");
    scImport.buttonIconPath = ":images/mac/download.png";
    QPushButton *importButton = addToShortCuts(scImport);
    connect(importButton, SIGNAL(clicked()), context->mainWindow, SLOT(importFile()));

    ShortCut scDownload;
    scDownload.label = tr("Download from serial device.");
    scDownload.buttonLabel = tr("Download from device");
    scDownload.buttonIconPath = ":images/mac/download.png";
    QPushButton *downloadButton = addToShortCuts(scDownload);
    connect(downloadButton, SIGNAL(clicked()), context->mainWindow, SLOT(downloadRide()));

    canShow_ = !appsettings->cvalue(context->athlete->cyclist, GC_BLANK_HOME).toBool();
}

//
// Replace diary window when no ride
//
BlankStateDiaryPage::BlankStateDiaryPage(Context *context) : BlankStatePage(context)
{
    dontShow->setChecked(appsettings->cvalue(context->athlete->cyclist, GC_BLANK_DIARY, false).toBool());
    welcomeTitle->setText(tr("Diary"));
    welcomeText->setText(tr("No ride ?\nLet's start with some data."));

    img->setIcon(QPixmap(":images/diary.png"));
    img->setIconSize(QSize(800,330));

    ShortCut scImport;
    scImport.label = tr("Import files from your disk or usb device");
    scImport.buttonLabel = tr("Import data");
    scImport.buttonIconPath = ":images/mac/download.png";
    QPushButton *importButton = addToShortCuts(scImport);
    connect(importButton, SIGNAL(clicked()), context->mainWindow, SLOT(importFile()));

    ShortCut scDownload;
    scDownload.label = tr("Download from serial device.");
    scDownload.buttonLabel = tr("Download from device");
    scDownload.buttonIconPath = ":images/mac/download.png";
    QPushButton *downloadButton = addToShortCuts(scDownload);
    connect(downloadButton, SIGNAL(clicked()), context->mainWindow, SLOT(downloadRide()));

    canShow_ = !appsettings->cvalue(context->athlete->cyclist, GC_BLANK_DIARY).toBool();
}

//
// Replace train window when no ride
//
BlankStateTrainPage::BlankStateTrainPage(Context *context) : BlankStatePage(context)
{
    dontShow->setChecked(appsettings->cvalue(context->athlete->cyclist, GC_BLANK_TRAIN, false).toBool());
    welcomeTitle->setText(tr("Train"));
    welcomeText->setText(tr("No devices or workouts ?\nLet's get you setup."));

    img->setIcon(QPixmap(":images/train.png"));
    img->setIconSize(QSize(800,330));

    ShortCut scAddDevice;
    // - add a realtime device
    // - find video and workouts
    scAddDevice.label = tr("Find and add training devices.");
    scAddDevice.buttonLabel = tr("Add device");
    scAddDevice.buttonIconPath = ":images/devices/kickr.png";
    QPushButton *addDeviceButton = addToShortCuts(scAddDevice);
    connect(addDeviceButton, SIGNAL(clicked()), context->mainWindow, SLOT(addDevice()));


    ShortCut scImportWorkout;
    scImportWorkout.label = tr("Find and Import your videos and workouts.");
    scImportWorkout.buttonLabel = tr("Scan hard drives");
    scImportWorkout.buttonIconPath = ":images/toolbar/Disk.png";
    QPushButton *importWorkoutButton = addToShortCuts(scImportWorkout);
    connect(importWorkoutButton, SIGNAL(clicked()), context->mainWindow, SLOT(manageLibrary()));

    ShortCut scDownloadWorkout;
    scDownloadWorkout.label = tr("Download workout files from the Erg DB.");
    scDownloadWorkout.buttonLabel = tr("Download workouts");
    scDownloadWorkout.buttonIconPath = ":images/mac/download.png";
    QPushButton *downloadWorkoutButton = addToShortCuts(scDownloadWorkout);
    connect(downloadWorkoutButton, SIGNAL(clicked()), context->mainWindow, SLOT(downloadErgDB()));

    canShow_ = !appsettings->cvalue(context->athlete->cyclist, GC_BLANK_TRAIN).toBool();
}

// save away the don't show stuff
void
BlankStateAnalysisPage::saveState()
{
    appsettings->setCValue(context->athlete->cyclist, GC_BLANK_ANALYSIS, dontShow->isChecked());
}
void
BlankStateDiaryPage::saveState()
{
    appsettings->setCValue(context->athlete->cyclist, GC_BLANK_DIARY, dontShow->isChecked());
}
void
BlankStateHomePage::saveState()
{
    appsettings->setCValue(context->athlete->cyclist, GC_BLANK_HOME, dontShow->isChecked());
}
void
BlankStateTrainPage::saveState()
{
    appsettings->setCValue(context->athlete->cyclist, GC_BLANK_TRAIN, dontShow->isChecked());
}

