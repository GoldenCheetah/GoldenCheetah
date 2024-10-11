
/*
 * Metadata Dialog Copyright (c) 2024 Paul Johnson (paulj49457@gmail.com)
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

#ifndef _MetadataDialog_h
#define _MetadataDialog_h

#include "GoldenCheetah.h"
#include "Context.h"
#include "Settings.h"
#include "Units.h"

#include "RideItem.h"
#include "RideFile.h"

#include <QtGui>
#include <QList>
#include <QFileDialog>
#include <QCheckBox>
#include <QLabel>
#include <QListIterator>
#include <QDebug>

#include "RideMetadata.h"

// Dialog class to allow editing of metadata items
class MetadataDialog : public QDialog
{
    Q_OBJECT
        G_OBJECT

    public:
        MetadataDialog(Context *context, const QString& fieldName, const QString& value);
        virtual ~MetadataDialog();

    private slots:

        void cancelClicked();
        void okClicked();

    private:

        bool isTime = false; // when we edit metrics but they are really times

        Context *context_ = nullptr;
        FieldDefinition field_;
        QCompleter* completer_ = nullptr;

        QLabel* metaLabel_ = nullptr;
        QWidget* metaEdit_ = nullptr;
};

#endif // _MetadataDialog_h
