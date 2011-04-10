/*
* Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "VideoWindow.h"

VideoWindow::VideoWindow(MainWindow *parent, const QDir &home)  :
GcWindow(parent), home(home), main(parent)
{
    setControls(NULL);
    setInstanceName("Video Window");

    QHBoxLayout *layout = new QHBoxLayout();
    setLayout(layout);

    // config paramaters to libvlc
    const char * const vlc_args[] = {
                    "-I", "dummy", /* Don't use any interface */
                    "--ignore-config", /* Don't use VLC's config */
                    "--extraintf=logger", //log anything
                    "--verbose=-1" // -1 = no output at all
                };

    /* create an exception handler */
    //libvlc_exception_init(&exceptions);
    //vlc_exceptions(&exceptions);

    /* Load the VLC engine */
    inst = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
    //vlc_exceptions(&exceptions);

    /* Create a new item */
    // XXX need to add controls - not everyone is going to want to play a video from
    //                            my desktop!!!
    m = libvlc_media_new_path(inst, "/home/markl/Desktop/My Documents/fightclub.mp4");
    //vlc_exceptions(&exceptions);
        
    /* Create a media player playing environement */
    mp = libvlc_media_player_new (inst);
    //vlc_exceptions(&exceptions);

    /* set the media to playback */
    libvlc_media_player_set_media (mp, m);
    //vlc_exceptions(&exceptions);

    /* No need to keep the media now */
    libvlc_media_release (m);
 
    /* This is a non working code that show how to hooks into a window,
     * if we have a window around */
#ifdef Q_OS_LINUX
     x11Container = new QX11EmbedContainer(this);
     layout->addWidget(x11Container);
     libvlc_media_player_set_xwindow (mp, x11Container->winId());
#endif
#ifdef WIN32
     container = new QWidget(this);
     layout->addWidget(container);
     libvlc_media_player_set_hwnd (mp, container->winId());
#endif

#if 0 // XXX what abut mac!!!
     libvlc_media_player_set_nsobject (mp, view);
#endif

    /* play the media_player */
    libvlc_media_player_play (mp);
}

VideoWindow::~VideoWindow()
{
#ifdef Q_OS_LINUX
    // unembed vlc backend first
    x11Container->discardClient();
#endif

    // stop playback & wipe player
    libvlc_media_player_stop (mp);
    libvlc_media_player_release (mp);

    // unload vlc 
    libvlc_release (inst);
}

void VideoWindow::resizeEvent(QResizeEvent * )
{
    // do nothing .. for now
}
