/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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

#ifndef GC_Withings_h
#define GC_Withings_h

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QImage>

class Withings : public CloudService {

    Q_OBJECT

    public:

        Withings(Context *context);
        CloudService *clone(Context *context) { return new Withings(context); }

        QString id() const { return "Withings"; }
        QString uiName() const { return tr("Withings"); }
        QString description() const { return (tr("Download weight, body fat etc from the connected health specialists.")); }
        QImage logo() const { return QImage(":images/services/withings.png"); }

        // this is a stub only used to setup configuration !
        int type() const { return Measures; }
        int capabilities() const { return OAuth | Download; }

    public slots:

    private:
        Context *context;

    private slots:
};
#endif
