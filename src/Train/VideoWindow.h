/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
 *               2015 Vianney Boyer   (vlcvboyer@gmail.com)
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
#include "MeterWidget.h"

// QT6 Video Stuff
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QAudioOutput>

// QT stuff etc
#include <QtGui>
#include <QTimer>
#include <QMutex>
#include "Context.h"
#include "DeviceConfiguration.h"
#include "DeviceTypes.h"
#include "RealtimeData.h"
#include "TrainSidebar.h"

#include <mutex>
#include <thread>

// We always have a media helper
class MediaHelper
{
    public:

        MediaHelper();
        ~MediaHelper();

        // get a list of supported media
        // found in the supplied directory
        QStringList listMedia(QDir directory);
        bool isMedia(QString filename);

    private:
        QStringList supported;
};

class VideoWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    // which layout to use
    Q_PROPERTY(int videoLayout READ videoLayout WRITE setVideoLayout USER true)

    public:

        VideoWindow(Context *);
        ~VideoWindow();
        int videoLayout() const { return layoutSelector ? layoutSelector->currentIndex() : 0; }
        void setVideoLayout(int x) { if (layoutSelector) layoutSelector->setCurrentIndex(x); }


    public slots:

        void layoutChanged();
        void resetLayout();
        void startPlayback();
        void stopPlayback();
        void pausePlayback();
        void resumePlayback();
        void telemetryUpdate(const RealtimeData &rtd);
        void seekPlayback(long ms);
        void mediaSelected(QString filename);
        bool hasActiveVideo() const;

    protected:

        void showEvent(QShowEvent *event) override;
        void hideEvent(QHideEvent *event) override;

        void resizeEvent(QResizeEvent *) override;

        // media data

         // Support case where media fps and videosync fps disagree.
        double videoSyncTimeAdjustFactor;

        // Support case where workout distance and videosync distance disagree.
        double videoSyncDistanceAdjustFactor;

        // Adjust time in videosync file point on load.
        VideoSyncFilePoint VideoSyncPointAdjust(const VideoSyncFilePoint& vsfp) const;

        // current data
        int curPosition;
        RideFilePoint rfp;
        float currentVideoRate;

        // passed from Context *
        Context *context;

        bool m_MediaChanged;

        QList<MeterWidget*> m_metersWidget;
        QPoint prevPosition;

    private:
        QList<QString> layoutNames;
        void readVideoLayout(int x, bool useDefault=false);
        void showMeters();

        // QT native
        QVideoWidget *wd;
        QMediaPlayer *mp;

        QWidget *container;
        QComboBox *layoutSelector;
        QPushButton *resetLayoutBtn;

        enum PlaybackState { None, Playing, Paused };

        PlaybackState state;
        mutable QRecursiveMutex stateLock;

        bool init; // we initialised ok ?
};

#endif // _GC_VideoWindow_h
