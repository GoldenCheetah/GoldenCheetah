/*
* Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#include "LionFullScreen.h"
#include "MainWindow.h"

// which version of the SDK?
#include <Availability.h>

LionFullScreen::LionFullScreen(Context *context) : QObject(context->mainWindow), context(context)
{
#ifdef GC_HAVE_LION
    // lets enable fullscreen stuff
    NSView *nsview = (NSView *) context->mainWindow->winId(); 
    NSWindow *nswindow = [nsview window];
    [nswindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];

    // watch for ESC key being hit when in full screen
    context->mainWindow->installEventFilter(this);
#endif
}

bool
LionFullScreen::eventFilter(QObject *obj, QEvent *event)
{
#ifdef GC_HAVE_LION
    if (obj != context->mainWindow) return false;

    // Ctrl-Cmd-F toggles
    if (event->type() == QEvent::KeyPress &&
         static_cast<QKeyEvent *>(event)->key() == Qt::Key_F && 
         static_cast<QKeyEvent *>(event)->modifiers() == (Qt::MetaModifier|Qt::ControlModifier)) {
        toggle();
        return false;

    }

    // ESC cancels fullscreen
    if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {

        // if in full screen then toggle, otherwise do nothing
        NSView *nsview = (NSView *) context->mainWindow->winId();
        NSWindow *nswindow = [nsview window];
        NSUInteger masks = [nswindow styleMask];
        if (masks & NSFullScreenWindowMask) toggle();
        return false;
    }
#endif
    return false; // always pass thru, just in case
}


void
LionFullScreen::toggle()
{
#ifdef GC_HAVE_LION
    // toggle full screen back
    NSView *nsview = (NSView *) context->mainWindow->winId();
    NSWindow *nswindow = [nsview window];
    [nswindow toggleFullScreen:nil];
#endif
}
