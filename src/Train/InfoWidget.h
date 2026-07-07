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

#ifndef _GC_InfoWidget_h
#define _GC_InfoWidget_h

#include <QFrame>
#include <QLabel>
#include <QList>
#include <QString>
#include <QColor>
#include <QEvent>

#include <TagBar.h>

#include "RatingWidget.h"
#include "PowerInfoWidget.h"
#include "PowerZonesWidget.h"
#include "ErgFileBase.h"
#include "WorkoutTagWrapper.h"


class InfoWidget : public QFrame
{
    Q_OBJECT

    public:
        InfoWidget(const QList<QColor> &powerZoneColors, const QList<QString> &powerZoneShortNames, const QList<QString> &powerZoneNames, bool showRating = true, bool showTags = true, QWidget *parent = nullptr);
        ~InfoWidget();

        void setContent(const ErgFileBase &ergFileBase, int rating, qlonglong lastRun);

        static QString milliSecondsToString(long ms);

    public slots:
        void ergFileSelected(ErgFileBase *ergFileBase);
        void setPowerZoneColors(const QList<QColor> &colors);
        void setPowerZoneNames(const QList<QString> &shortNames, const QList<QString> &names);

    signals:
        void relayErgFileSelected(ErgFileBase *ergFileBase);

    protected:
        virtual void changeEvent(QEvent *event);

    private:
        RatingWidget *ratingWidget = nullptr;
        PowerInfoWidget *powerInfoWidget;
        QLabel *slpLabel;
        PowerZonesWidget *powerZonesWidget;
        QLabel *descriptionLabel;
        QLabel *lastRunLabel;
        TagBar *tagBar = nullptr;
        QString filepath;
        WorkoutTagWrapper workoutTagWrapper;

    private slots:
        void rated(int rating);
};

#endif
