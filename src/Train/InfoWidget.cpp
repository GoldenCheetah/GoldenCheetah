/*
 * Copyright (c) 2023 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "InfoWidget.h"

#include <QDebug>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QFont>
#include <QLocale>

#include "Shy.h"
#include "ErgOverview.h"
#include "TrainDB.h"
#include "Colors.h"


InfoWidget::InfoWidget
(QList<QColor> powerZoneColors, QList<QString> powerZoneNames, QWidget *parent)
: QFrame(parent)
{
    QGridLayout *l = new QGridLayout(this);
    int row = 0;

    ratingWidget = new RatingWidget();
    ratingWidget->setAlignment(Qt::AlignHCenter);
    connect(ratingWidget, SIGNAL(rated(int)), this, SLOT(rated(int)));
    QFont ratingFont = ratingWidget->font();
    ratingFont.setPointSizeF(ratingFont.pointSizeF() * 1.5);
    ratingWidget->setFont(ratingFont);
    l->addWidget(ratingWidget, row, 0, 1, -1);
    ++row;

    ErgOverview *eo = new ErgOverview();
    connect(this, SIGNAL(relayErgFileSelected(ErgFileBase*)), eo, SLOT(setContent(ErgFileBase*)));
    QBoxLayout *eoLayout = new QHBoxLayout();
    eoLayout->setSpacing(0);
    eoLayout->setContentsMargins(0, 0, 0, 0);
    eoLayout->addWidget(eo);
    Shy *eoShy = new Shy(34, ShyType::percent);
    eoShy->setLayout(eoLayout);
    l->addWidget(eoShy, row, 0, 1, -1);
    ++row;

    slpLabel = new QLabel();
    l->addWidget(slpLabel, row, 0, 1, -1, Qt::AlignCenter);
    slpLabel->hide();
    ++row;

    powerInfoWidget = new PowerInfoWidget();
    powerInfoWidget->setContentsMargins(0, 10, 0, 5);
    l->addWidget(powerInfoWidget, row, 0, 1, -1);
    ++row;

    powerZonesWidget = new PowerZonesWidget(powerZoneColors, powerZoneNames);
    powerZonesWidget->setContentsMargins(0, 5, 0, 10);
    l->addWidget(powerZonesWidget, row, 0, 1, -1);
    ++row;

    tagBar = new TagBar(trainDB, GCColor::invertColor(GColor(CTRAINPLOTBACKGROUND)), this);
    connect(trainDB, SIGNAL(tagsChanged(int, int, int)), tagBar, SLOT(tagStoreChanged(int, int, int)));
    l->addWidget(tagBar, row, 0, 1, -1);
    ++row;

    descriptionLabel = new QLabel();
    descriptionLabel->setWordWrap(true);
    QBoxLayout *descriptionLayout = new QHBoxLayout();
    descriptionLayout->setSpacing(0);
    descriptionLayout->setContentsMargins(0, 0, 0, 0);
    descriptionLayout->addWidget(descriptionLabel);
    Shy *descriptionShy = new Shy(2.5, ShyType::lines);
    descriptionShy->setLayout(descriptionLayout);
    l->addWidget(descriptionShy, row, 0, 1, -1);
    ++row;

    lastRunLabel = new QLabel();
    l->addWidget(lastRunLabel, row, 0, 1, -1);
    ++row;

    l->setRowStretch(l->rowCount(), 1);

    setLayout(l);
}


InfoWidget::~InfoWidget()
{
}


void
InfoWidget::changeEvent
(QEvent *event)
{
    if (event->type() == QEvent::FontChange) {
        QFont ratingFont = font();
        ratingFont.setPointSizeF(ratingFont.pointSizeF() * 1.5);
        ratingWidget->setFont(ratingFont);
    }
    tagBar->setColor(GCColor::invertColor(GColor(CTRAINPLOTBACKGROUND)));
}


void
InfoWidget::setContent
(const ErgFileBase &ergFileBase, int rating, qlonglong lastRun)
{
    if (lastRun > 0) {
        QDateTime timestamp;
        timestamp.setSecsSinceEpoch(lastRun);
        int daysAgo = timestamp.daysTo(QDateTime::currentDateTime());
        QString ago;
        if (daysAgo == 0) {
            ago = "today";
        } else if (daysAgo == 1) {
            ago = "yesterday";
        } else if (daysAgo < 21) {
            ago = QString("%1 days ago").arg(daysAgo);
        } else if (daysAgo < 56) {
            ago = QString("%1 weeks ago").arg(daysAgo / 7);
        } else if (daysAgo < 365) {
            ago = QString("%1 months ago").arg(daysAgo / 30);
        } else if (daysAgo < 730) {
            ago = QString("1 year ago");
        } else {
            ago = QString("%1 years ago").arg(daysAgo / 365);
        }
        lastRunLabel->setText(QString("Last Run: <b>%1 (%2)</b>")
                                     .arg(ago, timestamp.toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat))));
    } else {
        lastRunLabel->setText(QString("Last Run: <b>Never</b>"));
    }
    ratingWidget->setRating(rating);

    if (filepath == ergFileBase.filename()) {
        // Stop if the workout is only reselected
        return;
    }

    filepath = ergFileBase.filename();

    // Common fields
    workoutTagWrapper.setFilepath(ergFileBase.filename());
    if (! ergFileBase.filename().isEmpty()) {
        tagBar->setTaggable(&workoutTagWrapper);
    } else {
        tagBar->setTaggable(nullptr);
    }

    if (! ergFileBase.description().isEmpty()) {
        descriptionLabel->setText(ergFileBase.description());
        descriptionLabel->show();
    } else {
        descriptionLabel->hide();
    }

    // Hide all individual fields
    slpLabel->hide();
    powerZonesWidget->hide();
    powerInfoWidget->hide();

    // Individual fields
    if (ergFileBase.type() == ErgFileType::erg) {
        if (ergFileBase.dominantZone() != ErgFilePowerZone::unknown) {
            powerZonesWidget->setPowerZones(ergFileBase.powerZonesPC(), ergFileBase.dominantZoneInt(), ergFileBase.duration());
            powerZonesWidget->show();
        }
        if (ergFileBase.maxWatts() > 0) {
            powerInfoWidget->setPower(ergFileBase.minWatts(), ergFileBase.maxWatts(), ergFileBase.AP(), ergFileBase.IsoPower(), ergFileBase.XP());
            powerInfoWidget->show();
        }
    } else if (ergFileBase.type() == ErgFileType::slp) {
        slpLabel->show();
        QString slpInfo = "<table>";
        slpInfo.append(QString("<tr><td>%1</td><td>&nbsp;</td><td><b>%2 km</b></td></tr>")
                              .arg(tr("Distance"))
                              .arg(ergFileBase.distance() / 1000.0, 0, 'f', 1));
        slpInfo.append(QString("<tr><td>%1</td><td></td><td><b>%2 m<br/>%3 km @ %4 %</b></b></td></tr>")
                              .arg(tr("Elevation Gain"))
                              .arg(ergFileBase.ele(), 0, 'f', 1)
                              .arg(ergFileBase.eleDist() / 1000.0, 0, 'f', 1)
                              .arg(ergFileBase.grade(), 0, 'f', 1));
        slpInfo.append(QString("<tr><td>%1</td><td></td><td><b>%2 m</b></td></tr>")
                              .arg(tr("Min Elevation"))
                              .arg(ergFileBase.minElevation(), 0, 'f', 1));
        slpInfo.append(QString("<tr><td>%1</td><td></td><td><b>%2 m</b></td></tr>")
                              .arg(tr("Max Elevation"))
                              .arg(ergFileBase.maxElevation(), 0, 'f', 1));
        slpInfo.append("</table>");
        slpLabel->setText(slpInfo);
    }
}


void
InfoWidget::ergFileSelected
(ErgFileBase *ergFileBase)
{
    if (ergFileBase != nullptr) {
        WorkoutUserInfo wui = trainDB->getWorkoutUserInfo(ergFileBase->filename());
        setContent(*ergFileBase, wui.rating, wui.lastRun);
        emit relayErgFileSelected(ergFileBase);
    }
}


void
InfoWidget::setPowerZoneColors
(QList<QColor> colors)
{
    powerZonesWidget->setColors(colors);
}


void
InfoWidget::setPowerZoneNames
(QList<QString> names)
{
    powerZonesWidget->setNames(names);
}


void
InfoWidget::rated
(int rating)
{
    trainDB->rateWorkout(filepath, rating);
}
