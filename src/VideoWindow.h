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

#ifndef _GC_VideoWindow_h
#define _GC_VideoWindow_h 1
#include "GoldenCheetah.h"

// for vlc
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern "C" {
#include <vlc/vlc.h>
#include <vlc/libvlc_media.h>
}

// QT stuff etc
#include <QtGui>
#include <QTimer>
#include "MainWindow.h"
#include "DeviceConfiguration.h"
#include "DeviceTypes.h"
#include "RealtimeData.h"
#include "TrainTool.h"

#ifdef Q_OS_LINUX
#include <QX11EmbedContainer>
#endif

class MediaHelper
{
    public:

        MediaHelper();
        ~MediaHelper();

        // get a list of supported media
        // found in the supplied directory
        QStringList listMedia(QDir directory);

    private:

        libvlc_instance_t * inst;
};

class VideoWindow : public GcWindow
{
    Q_OBJECT
    G_OBJECT


    public:

        VideoWindow(MainWindow *, const QDir &);
        ~VideoWindow();

    public slots:

        void startPlayback();
        void stopPlayback();
        void pausePlayback();
        void resumePlayback();
        void seekPlayback(long ms);
        void mediaSelected(QString filename);

    protected:

        void resizeEvent(QResizeEvent *);

        // passed from MainWindow
        QDir home;
        MainWindow *main;

        bool m_MediaChanged;

        libvlc_instance_t * inst;
        //libvlc_exception_t exceptions;
        libvlc_media_player_t *mp;
        libvlc_media_t *m;

#ifdef Q_OS_LINUX
        QX11EmbedContainer *x11Container;
#endif
#ifdef WIN32
        QWidget *container;
#endif
};

#endif // _GC_VideoWindow_h
