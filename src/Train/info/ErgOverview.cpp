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

#include "ErgOverview.h"

#include <QGridLayout>
#include <QFont>
#include <QLabel>
#include <QString>


ErgOverview::ErgOverview
(QWidget *parent)
: QWidget(parent)
{
    QFont bf = font();
    bf.setBold(true);

    QGridLayout *layout = new QGridLayout();
    int row = 0;

    layout->addWidget(new QLabel("🕑"), row, 0, 1, 2, Qt::AlignCenter);
    layout->addWidget(new QLabel(tr("Intensity")), row, 2, 1, 2, Qt::AlignCenter);
    layout->addWidget(new QLabel(tr("Stress")), row, 4, 1, 2, Qt::AlignCenter);
    ++row;

    duration = new QLabel();
    duration->setFont(bf);
    layout->addWidget(duration, row, 0, 1, 2, Qt::AlignCenter);

    intensity = new QLabel();
    intensity->setFont(bf);
    layout->addWidget(intensity, row, 2, 1, 2, Qt::AlignCenter);

    stress = new QLabel();
    stress->setFont(bf);
    layout->addWidget(stress, row, 4, 1, 2, Qt::AlignCenter);

    ++row;

    layout->addWidget(new QLabel(tr("Relative Intensity")), row, 0, 1, 3, Qt::AlignCenter);
    layout->addWidget(new QLabel(tr("BikeScore")), row, 3, 1, 3, Qt::AlignCenter);
    ++row;

    ri = new QLabel();
    ri->setFont(bf);
    layout->addWidget(ri, row, 0, 1, 3, Qt::AlignCenter);

    bs = new QLabel();
    bs->setFont(bf);
    layout->addWidget(bs, row, 3, 1, 3, Qt::AlignCenter);
    ++row;

    layout->addWidget(new QLabel(tr("Variability Index")), row, 0, 1, 3, Qt::AlignCenter);
    layout->addWidget(new QLabel(tr("Skiba Variability Index")), row, 3, 1, 3, Qt::AlignCenter);
    ++row;

    vi = new QLabel();
    vi->setFont(bf);
    layout->addWidget(vi, row, 0, 1, 3, Qt::AlignCenter);

    svi = new QLabel();
    svi->setFont(bf);
    layout->addWidget(svi, row, 3, 1, 3, Qt::AlignCenter);
    ++row;

    setLayout(layout);
    hide();
}


ErgOverview::~ErgOverview
()
{
}


void
ErgOverview::setContent
(ErgFileBase *ergFileBase)
{
    if (ergFileBase->type() != ErgFileType::erg) {
        hide();
        return;
    }
    duration->setText(milliSecondsToString(ergFileBase->duration()));
    stress->setText(QString("%1").arg(ergFileBase->bikeStress(), 0, 'f', 0));
    intensity->setText(QString("%1").arg(ergFileBase->IF(), 0, 'f', 2));
    ri->setText(QString("%1").arg(ergFileBase->RI(), 0, 'f', 2));
    bs->setText(QString("%1").arg(ergFileBase->BS(), 0, 'f', 0));
    vi->setText(QString("%1").arg(ergFileBase->VI(), 0, 'f', 2));
    svi->setText(QString("%1").arg(ergFileBase->SVI(), 0, 'f', 2));
    show();
}


QString
ErgOverview::milliSecondsToString
(long ms)
{
    long secs = ms / 1000;
    int durationHours = secs / 3600;
    int durationMins = secs % 3600 / 60;
    int durationSecs = secs % 60;
    return QString("%1:%2:%3")
                  .arg(durationHours, 1, 10, QChar('0'))
                  .arg(durationMins, 2, 10, QChar('0'))
                  .arg(durationSecs, 2, 10, QChar('0'));
}
