/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _AddChartWizard_h
#define _AddChartWizard_h

#include "GoldenCheetah.h"
#include "Context.h"
#include "ChartSpace.h"

#include <QWizard>

class AddChartWizard : public QWizard
{
    Q_OBJECT

public:

    AddChartWizard(Context *context, ChartSpace *space, int scope, ChartSpaceItem * &added);
    QSize sizeHint() const { return QSize(600,650); }

    Context *context;
    bool done; // have we finished?

    // what type of chart
    int type;
    int scope;

    // add here
    ChartSpace *space;
    struct ChartSpaceItemDetail detail;

    // creating
    ChartSpaceItem *item;
    QWidget *config;

    // tell overview what we did
    ChartSpaceItem * &added;

};

class AddChartType : public QWizardPage
{
    Q_OBJECT

    public:
        AddChartType(AddChartWizard *);
        void initializePage();
        bool validate() const { return false; }
        bool isComplete() const { return false; }
        int nextId() const { return 2; };

    public slots:
        void clicked(int);

    private:
        AddChartWizard *wizard;
        QSignalMapper *mapper;
        QWidget *buttons;
        QVBoxLayout *buttonlayout;
        QScrollArea *scrollarea;
};


class AddChartConfig : public QWizardPage
{
    Q_OBJECT

    public:
        AddChartConfig(AddChartWizard *);
        ~AddChartConfig(); // spare the item's config widget when we die
        void initializePage();
        bool validate() const { return true; }
        bool isComplete() const { return true; }
        bool validatePage();
        int nextId() const { return 03; };

    public slots:

    private:
        AddChartWizard *wizard;
        QVBoxLayout *main;
};


class AddChartFinal : public QWizardPage
{
    Q_OBJECT

    public:
        AddChartFinal(AddChartWizard *);
        void initializePage();
        bool validatePage();
        bool isCommitPage() { return true; }

    private:
        AddChartWizard *wizard;

};

#endif // _AddChartWizard_h
