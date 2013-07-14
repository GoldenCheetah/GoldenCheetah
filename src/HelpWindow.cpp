/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "HelpWindow.h"
#include "MainWindow.h"

HelpWindow::HelpWindow(Context *context) : QDialog(context->mainWindow)
{
    setWindowTitle("Help");
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::Tool);
    setInstanceName("Help Window");

    view = new QWebView();
    layout = new QVBoxLayout();
    layout->addWidget(view);
    setLayout(layout);
    view->load(QUrl("http://bugs.goldencheetah.org/projects/goldencheetah/wiki"));

}
