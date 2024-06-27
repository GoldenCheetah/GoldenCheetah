/*
 * Copyright (c) 2024 Paul Johnson (paulj49457@gmail.com)
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

#include "EquipmentTab.h"
#include "Context.h"
#include "MainWindow.h"

#include <QPaintEvent>

EquipmentTab::EquipmentTab(Context *context) : QWidget(context->mainWindow), context(context)
{
    init = false;

    setContentsMargins(0,0,0,0);
    QVBoxLayout *main = new QVBoxLayout(this);
    main->setSpacing(0);
    main->setContentsMargins(0,0,0,0);

    view_ = new QStackedWidget(this);
    view_->setContentsMargins(0,0,0,0);
    main->addWidget(view_);

     // Equipment
    eqControls_ = new QStackedWidget(this);
    eqControls_->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    eqControls_->setCurrentIndex(0);
    eqControls_->setContentsMargins(0,0,0,0);
    equipView_ = new EquipView(context, eqControls_);

    view_->addWidget(equipView_);

    init = true;

    setSidebarEnabled(true);
}

EquipmentTab::~EquipmentTab()
{
    delete equipView_;
    delete view_;
}

void
EquipmentTab::close()
{
    equipView_->saveState();
    equipView_->close();
}


