/*
 * Copyright (c) 2016 Mark Liversedge (liversedge@gmail.com)
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


#include "PlanningWindow.h"

PlanningWindow::PlanningWindow(Context *context) :
    GcChartWindow(context), context(context)
{
    setContentsMargins(0,0,0,0);
    setProperty("color", GColor(GCol::TRENDPLOTBACKGROUND));

    setControls(NULL);

    QVBoxLayout *main = new QVBoxLayout;
    setChartLayout(main);

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // set the widgets etc
    configChanged(CONFIG_APPEARANCE);
}

void
PlanningWindow::resizeEvent(QResizeEvent *)
{
    // TBC
}

void
PlanningWindow::configChanged(qint32)
{
    setProperty("color", GColor(GCol::TRENDPLOTBACKGROUND));

    // text edit colors
    QPalette palette;
    palette.setColor(QPalette::Window, GColor(GCol::TRAINPLOTBACKGROUND));

    // only change base if moved away from white plots
    // which is a Mac thing
#ifndef Q_OS_MAC
    if (GColor(GCol::TRENDPLOTBACKGROUND) != Qt::white)
#endif
    {
        //palette.setColor(QPalette::Base, GAlternateColor(GCol::TRAINPLOTBACKGROUND));
        palette.setColor(QPalette::Base, GColor(GCol::TRAINPLOTBACKGROUND));
        palette.setColor(QPalette::Window, GColor(GCol::TRAINPLOTBACKGROUND));
    }

#ifndef Q_OS_MAC // the scrollers appear when needed on Mac, we'll keep that
    //code->setStyleSheet(TabView::ourStyleSheet());
#endif

    palette.setColor(QPalette::WindowText, GInvertColor(GCol::TRAINPLOTBACKGROUND));
    palette.setColor(QPalette::Text, GInvertColor(GCol::TRAINPLOTBACKGROUND));
    //code->setPalette(palette);
    repaint();
}

bool
PlanningWindow::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj)
    bool returning = false;

    // we only filter out keyboard shortcuts for undo redo etc
    // in the qwkcode editor, anything else is of no interest.
    if (event->type() == QEvent::KeyPress) {

        // we care about cmd / ctrl
        Qt::KeyboardModifiers kmod = static_cast<QInputEvent*>(event)->modifiers();
        bool ctrl = (kmod & Qt::ControlModifier) != 0;

        switch(static_cast<QKeyEvent*>(event)->key()) {

        case Qt::Key_Y:
            if (ctrl) {
                //workout->redo();
                returning = true; // we grab all key events
            }
            break;

        case Qt::Key_Z:
            if (ctrl) {
                //workout->undo();
                returning=true;
            }
            break;

        }

    }
    return returning;
}
