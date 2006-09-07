/* 
 * $Id: ChooseCyclistDialog.h,v 1.3 2006/07/04 12:55:40 srhea Exp $
 *
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_ChooseCyclistDialog_h
#define _GC_ChooseCyclistDialog_h 1

#include <QDialog>
#include <QDir>

class QListWidget;
class QListWidgetItem;
class QPushButton;

class ChooseCyclistDialog : public QDialog 
{
    Q_OBJECT

    public:
        ChooseCyclistDialog(const QDir &home, bool allowNew);
        QString choice();
        static QString newCyclistDialog(QDir &homeDir, QWidget *parent);

    private slots:
        void newClicked();
        void cancelClicked();
        void enableOk(QListWidgetItem *item);

    private:

        QDir home;
        QListWidget *listWidget;
        QPushButton *okButton, *newButton, *cancelButton;
};

#endif // _GC_ChooseCyclistDialog_h

