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

#ifndef _AddTileWizard_h
#define _AddTileWizard_h

#include "GoldenCheetah.h"
#include "Context.h"
#include "ChartSpace.h"

#include <QWizard>

class AddTileWizard : public QWizard
{
    Q_OBJECT

public:

    AddTileWizard(Context *context, ChartSpace *space, int scope, ChartSpaceItem * &added);
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

class AddTileType : public QWizardPage
{
    Q_OBJECT

    public:
        AddTileType(AddTileWizard *);
        void initializePage();
        bool validate() const { return false; }
        bool isComplete() const { return false; }
        int nextId() const { return 2; };

    public slots:
        void clicked(int);

    private:
        AddTileWizard *wizard;
        QSignalMapper *mapper;
        QWidget *buttons;
        QVBoxLayout *buttonlayout;
        QScrollArea *scrollarea;
};


class AddTileConfig : public QWizardPage
{
    Q_OBJECT

    public:
        AddTileConfig(AddTileWizard *);
        ~AddTileConfig(); // spare the item's config widget when we die
        void initializePage();
        bool validate() const { return true; }
        bool isComplete() const { return true; }
        bool validatePage();
        int nextId() const { return 03; };

    public slots:

    private:
        AddTileWizard *wizard;
        QVBoxLayout *main;
};


class AddTileFinal : public QWizardPage
{
    Q_OBJECT

    public:
        AddTileFinal(AddTileWizard *);
        void initializePage();
        bool validatePage();
        bool isCommitPage() { return true; }

    private:
        AddTileWizard *wizard;

};

#endif // _AddTileWizard_h
