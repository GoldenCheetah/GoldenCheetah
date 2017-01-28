/*
 * Copyright (c) 2017 Joern Rischmueller (joern.rm@gmail.com)
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

#ifndef CLOUDDBTELEMETRY_H
#define CLOUDDBTELEMETRY_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QScrollArea>
#include <QDialog>


class CloudDBTelemetryClient : public QObject
{
    Q_OBJECT

public:

    CloudDBTelemetryClient();
    ~CloudDBTelemetryClient();

    void upsertTelemetry();

    static int CloudDBTelemetry_UpsertFrequence;




};


class CloudDBAcceptTelemetryDialog : public QDialog
{
    Q_OBJECT

public:
    CloudDBAcceptTelemetryDialog();

private slots:
    void acceptConditions();
    void rejectConditions();

private:

    QPushButton *yesButton;
    QPushButton *noButton;

};


#endif // CLOUDDBTELEMETRY_H
