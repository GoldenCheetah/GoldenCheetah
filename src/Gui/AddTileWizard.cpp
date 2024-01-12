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

#include "AddTileWizard.h"
#include "Context.h"
#include "MainWindow.h"
#include "HelpWhatsThis.h"

#include <QCommandLinkButton>
#include <QMessageBox>
#include <QPixmap>
#include <QRegExp>

#include "OverviewItems.h"

// WIZARD FLOW
//
// 01. select chart type
// 02. configure
// 03. finalise
//

// Main wizard - if passed a service name we are in edit mode, not add mode.
AddTileWizard::AddTileWizard(Context *context, ChartSpace *space, int scope, ChartSpaceItem * &added)
    : QWizard(context->mainWindow), context(context), scope(scope), space(space), added(added)
{
    HelpWhatsThis *help = new HelpWhatsThis(this);
    if (scope & OverviewScope::ANALYSIS) this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Overview));
    else this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Chart_Overview));
#ifdef Q_OS_MAC
    setWizardStyle(QWizard::ModernStyle);
#endif

    // delete when done
    setWindowModality(Qt::NonModal); // avoid blocking WFAPI calls for kickr
    setAttribute(Qt::WA_DeleteOnClose);
    setMinimumWidth(600 *dpiXFactor);
    setMinimumHeight(500 *dpiYFactor);

    setWindowTitle(tr("Add Tile Wizard"));
    setPage(01, new AddTileType(this));
    setPage(02, new AddTileConfig(this));
    setPage(03, new AddTileFinal(this));

    done = false;
    config = NULL;
    item = NULL;
}

/*----------------------------------------------------------------------
 * Wizard Pages
 *--------------------------------------------------------------------*/

//Select Cloud type
AddTileType::AddTileType(AddTileWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Tile Type"));
    setSubTitle(tr("Select the type of tile"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    buttons=new QWidget(this);
    buttons->setContentsMargins(0,0,0,0);
    buttonlayout= new  QVBoxLayout(buttons);
    buttonlayout->setSpacing(0);
    scrollarea=new QScrollArea(this);
    scrollarea->setWidgetResizable(true);
    scrollarea->setWidget(buttons);

    mapper = new QSignalMapper(this);
    connect(mapper, &QSignalMapper::mappedInt, this, &AddTileType::clicked);

    layout->addWidget(scrollarea);

    setFinalPage(false);
}

void
AddTileType::initializePage()
{
    // clear whatever we have, if anything
    QLayoutItem *item = NULL;
    while((item = buttonlayout->takeAt(0)) != NULL) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    ChartSpaceItemRegistry &registry = ChartSpaceItemRegistry::instance();

    QFont font; // default font size

    // iterate over names, as they are sorted alphabetically
    foreach(ChartSpaceItemDetail detail, registry.items()) {

        // limit the type of chart to add
        if ((detail.scope&wizard->scope) == 0) continue;

        // get the service
        QCommandLinkButton *p = new QCommandLinkButton(detail.quick, detail.description, this);
        p->setStyleSheet(QString("font-size: %1px;").arg(font.pointSizeF() * dpiXFactor));
        p->setFixedHeight(50 *dpiYFactor);
        connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
        mapper->setMapping(p, detail.type);
        buttonlayout->addWidget(p);
    }
    buttonlayout->addStretch();
}

void
AddTileType::clicked(int p)
{
    wizard->type = p;
    wizard->detail = ChartSpaceItemRegistry::instance().detailForType(p);

    wizard->next();
}

// Scan for Cloud port / usb etc
AddTileConfig::AddTileConfig(AddTileWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setSubTitle(tr("Tile Settings"));

    main = new QVBoxLayout(this);
}

void
AddTileConfig::initializePage()
{
    setTitle(QString(tr("Tile Settings")));

    if (wizard->config) {
        wizard->config->hide();
        main->removeWidget(wizard->config);
        wizard->config = NULL;
        delete wizard->item;
    }

    // new item with default settings to configure and add
    wizard->item = wizard->detail.create(wizard->space);

    // add config screen for this item
    wizard->config = wizard->item->config();
    main->addWidget(wizard->config);
    main->addStretch();
    wizard->config->show();
}

AddTileConfig::~AddTileConfig()
{
    // spare the config widget being destroyed
    if (wizard->config) {
        wizard->config->hide();
        main->removeWidget(wizard->config);
        wizard->config->setParent(NULL);
        wizard->config = NULL;
        wizard->added = wizard->item;
    }
}

bool
AddTileConfig::validatePage()
{
    return true;
}

// Final confirmation
AddTileFinal::AddTileFinal(AddTileWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Done"));
    setSubTitle(tr("Add Tile"));
    QVBoxLayout *blank = new QVBoxLayout(this); // just blank .. for now
    blank->addWidget(new QWidget(this));
}

void
AddTileFinal::initializePage()
{
}

bool
AddTileFinal::validatePage()
{
    // add to the top left
    if (wizard->item) {
        if (wizard->item->type == OverviewItemType::USERCHART) {

            // we add user charts but make them a bit bigger
            wizard->item->parent->addItem(0,0,2,21, wizard->item);
        } else {

            // everthing else is a bit smaller
            wizard->item->parent->addItem(0,0,1,7, wizard->item);
        }
        wizard->item->parent->updateGeometry();
    }
    return true;
}
