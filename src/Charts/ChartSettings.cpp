/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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

#include "ChartSettings.h"
#include "Colors.h"
#include <QVBoxLayout>

ChartSettings::ChartSettings(QWidget *parent, QWidget *contents) : QDialog(parent)
{
  // Set the main window title.
  setWindowTitle(tr("Chart Settings"));
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowFlags(windowFlags() | Qt::WindowCloseButtonHint);
  setModal(true);

  // Create the main layout box.
  QVBoxLayout *mainVBox = new QVBoxLayout(this);

  // Set up the instructions field, limiting heigth
  // to avoid issues on lower resolution displays
  mainVBox->addWidget(contents);
  contents->setMaximumWidth(650*dpiXFactor);
  contents->setMaximumHeight(720*dpiYFactor);

  // "Done" button.
  QHBoxLayout *buttonHBox = new QHBoxLayout;
  btnOK = new QPushButton(this);
  btnOK->setFocusPolicy(Qt::NoFocus);
  btnOK->setText(tr("Done"));
  buttonHBox->addStretch();
  buttonHBox->addWidget(btnOK);
  mainVBox->addLayout(buttonHBox);
  connect(btnOK, SIGNAL(clicked()), this, SLOT(on_btnOK_clicked()));

  hide();
}

void ChartSettings::on_btnOK_clicked() {
  // all done!
  hide();
}

// close doesnt really close, just hides
void
ChartSettings::closeEvent(QCloseEvent* event)
{
    event->ignore();
    hide();
}

void
ChartSettings::reject()
{
    hide();
}
