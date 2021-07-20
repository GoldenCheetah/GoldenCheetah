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

#include "AddChartWizard.h"
#include "Context.h"
#include "MainWindow.h"
#include "HelpWhatsThis.h"

#include <QCommandLinkButton>
#include <QMessageBox>
#include <QPixmap>
#include <QRegExp>

// WIZARD FLOW
//
// 01. select chart type
// 02. configure
// 03. finalise
//

// Main wizard - if passed a service name we are in edit mode, not add mode.
AddChartWizard::AddChartWizard(Context *context, ChartSpace *space, int scope) : QWizard(context->mainWindow), context(context), scope(scope), space(space)
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

    setWindowTitle(tr("Add Chart Wizard"));
    setPage(01, new AddChartType(this));
    setPage(02, new AddChartConfig(this));
    setPage(03, new AddChartFinal(this));

    done = false;
    config = NULL;
    item = NULL;
}

/*----------------------------------------------------------------------
 * Wizard Pages
 *--------------------------------------------------------------------*/

//Select Cloud type
AddChartType::AddChartType(AddChartWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Chart Type"));
    setSubTitle(tr("Select the chart type"));

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
    connect(mapper, SIGNAL(mapped(int)), this, SLOT(clicked(int)));

    layout->addWidget(scrollarea);

    setFinalPage(false);
}

void
AddChartType::initializePage()
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
AddChartType::clicked(int p)
{
    wizard->type = p;
    wizard->detail = ChartSpaceItemRegistry::instance().detailForType(p);

    wizard->next();
}

// Scan for Cloud port / usb etc
AddChartConfig::AddChartConfig(AddChartWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setSubTitle(tr("Chart Settings"));

    main = new QVBoxLayout(this);
}

void
AddChartConfig::initializePage()
{
    setTitle(QString(tr("Chart Settings")));

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

bool
AddChartConfig::validatePage()
{
    return true;
}

// Final confirmation
AddChartFinal::AddChartFinal(AddChartWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Done"));
    setSubTitle(tr("Add Chart"));
    QVBoxLayout *blank = new QVBoxLayout(this); // just blank .. for now
    blank->addWidget(new QWidget(this));
}

void
AddChartFinal::initializePage()
{
}

bool
AddChartFinal::validatePage()
{
    // add to the top left
    if (wizard->item) {
        wizard->item->parent->addItem(0,0,1,7, wizard->item);
        wizard->item->parent->updateGeometry();
    }
    return true;
}
