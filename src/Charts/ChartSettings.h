/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmal.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#include "GoldenCheetah.h"
#include <QDialog>
#include <QPushButton>
#include <QCloseEvent>

#ifndef GC_ChartSettings_h
#define GC_ChartSettings_h

class ChartSettings : public QDialog 
{
        Q_OBJECT
        G_OBJECT

    public:
        ChartSettings(QWidget *parent, QWidget *contents);
        void closeEvent(QCloseEvent* event);

    public slots:
        void reject();

    private:
        QPushButton *btnOK;

    private slots:
        void on_btnOK_clicked();
};
#endif
