/*
 * Copyright (c) 2015 Joern Rischmueller (joern.rm@gmail.com)
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

#ifndef CLOUDDBCOMMON_H
#define CLOUDDBCOMMON_H

#include "LTMSettings.h"
#include "Settings.h"

#include <QObject>
#include <QScrollArea>


// Central Classes related to CloudDB

// -------------------------------------------------------
// Dialog to show T&C for the use of CloudDB offering,
// and accepting those before executing any CloudDB
// functions.
// -------------------------------------------------------

class CloudDBAcceptConditionsDialog : public QDialog
{
    Q_OBJECT

    public:
        CloudDBAcceptConditionsDialog(QString athlete);

    private slots:
        void acceptConditions();
        void rejectConditions();

    private:

        QString athlete;

        QScrollArea *scrollText;
        QPushButton *proceedButton;
        QPushButton *abortButton;

};


#endif // CLOUDDBCORE_H
