/*
 * Copyright (c) 2026 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#ifndef GC_Garmin_h
#define GC_Garmin_h

#include "CloudService.h"
#include <QImage>
#include <memory>

class GarminPersonalAPI;

class Garmin : public CloudService {

    Q_OBJECT

    public:

        Garmin(Context *context);
        CloudService *clone(Context *context) override { return new Garmin(context); }
        ~Garmin() override;

        QString id() const override { return "Garmin"; }
        QString uiName() const override { return tr("Garmin"); }
        QString description() const override { return(tr("Sync with the Garmin Personal API.")); }
        QImage logo() const override { return QImage(":images/services/garmin.png"); }

        // now upload only and authenticates with a user and password
        int capabilities() const override { return OAuth | Download | Query; }
        int type() const override { return Activities | Measures; }

        // open/connect and close/disconnect
        bool open(QStringList &errors) override;
        bool close() override;

        // read a file
        bool readFile(QByteArray *data, QString remotename, QString remoteid) override;

        // dirent style api
        CloudServiceEntry *root() override { return root_; }
        QList<CloudServiceEntry*> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to) override;

    private:
        Context *context = nullptr;
        CloudServiceEntry *root_ = nullptr;

        std::unique_ptr<GarminPersonalAPI> garminPersonalAPI;

};
#endif
