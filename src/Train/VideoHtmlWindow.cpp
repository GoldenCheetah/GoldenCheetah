/*
 * Copyright (c) 2026 GoldenCheetah
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

#include "VideoHtmlWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "HelpWhatsThis.h"
#include "HtmlTrainingBridge.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QUrl>
#include <QPainter>
#include <QLabel>
#include <QResizeEvent>
#include <QShowEvent>
#include <QVideoFrame>
#include <QWebEngineSettings>

#include <QMediaMetaData>
#include <QtMath>


// ---------------------------------------------------------------
// Simple widget that paints the latest video frame
// ---------------------------------------------------------------
VideoHtmlWindow::VideoSurfaceWidget::VideoSurfaceWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
}

void VideoHtmlWindow::VideoSurfaceWidget::setFrame(const QImage &img)
{
    frame = img;
    update();
}

void VideoHtmlWindow::VideoSurfaceWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Qt::black);

    if (!frame.isNull()) {
        Qt::AspectRatioMode mode = m_keepAspectRatio
            ? Qt::KeepAspectRatio
            : Qt::KeepAspectRatioByExpanding;

        QImage scaled = frame.scaled(size(), mode, Qt::SmoothTransformation);
        QPoint topLeft((width()  - scaled.width())  / 2,
                       (height() - scaled.height()) / 2);
        p.drawImage(topLeft, scaled);
    }
}



// ---------------------------------------------------------------
// Helper: VideoSync
// ---------------------------------------------------------------

VideoSyncFilePoint VideoHtmlWindow::VideoSyncPointAdjust(const VideoSyncFilePoint &vsfp) const
{
    VideoSyncFilePoint r(vsfp);
    r.secs *= videoSyncTimeAdjustFactor;
    r.km   *= videoSyncDistanceAdjustFactor;
    return r;
}

bool VideoHtmlWindow::hasActiveVideo() const
{
    return mp && mp->playbackState() != QMediaPlayer::StoppedState;
}



// ---------------------------------------------------------------
// The Core Sync: TelemetryUpdate
// ---------------------------------------------------------------
void VideoHtmlWindow::telemetryUpdate(const RealtimeData &rtd)
{
    if (!isVisible() || !hasActiveVideo())
        return;
    if (state != Playing)
        return;

    auto pushRouteKmToHtml = [this](double km) {
        if (!htmlOverlay || !htmlOverlay->page())
            return;
        QString js = QString(
            "if (typeof currentDistance !== 'undefined') {"
            "  currentDistance = %1;"
            "  if (typeof updateMapRider === 'function') updateMapRider();"
            "  if (typeof updateRiderMarker === 'function') updateRiderMarker();"
            "}")
            .arg(km, 0, 'f', 6);
        htmlOverlay->page()->runJavaScript(js);
    };

    if (context->currentVideoSyncFile()) {
        QVector<VideoSyncFilePoint> points = context->currentVideoSyncFile()->Points;
        if (points.count() < 2) {
            pushRouteKmToHtml(rtd.getRouteDistance());
            return;
        }

        if (curPosition > points.count() - 1 || curPosition < 1)
            curPosition = 1;

        double currentDistance = qBound(0.0,
                                        rtd.getRouteDistance(),
                                        context->currentVideoSyncFile()->distance());

        while ((VideoSyncPointAdjust(points[curPosition]).km > currentDistance) && (curPosition > 1))
            curPosition--;
        while ((VideoSyncPointAdjust(points[curPosition]).km <= currentDistance) &&
               (curPosition < points.count() - 1))
            curPosition++;

        pushRouteKmToHtml(currentDistance);

        VideoSyncFilePoint syncPrev = VideoSyncPointAdjust(points[curPosition - 1]);
        VideoSyncFilePoint syncNext = VideoSyncPointAdjust(points[curPosition]);

        double syncKmDelta   = syncNext.km  - syncPrev.km;
        double syncKphDelta  = syncNext.kph - syncPrev.kph;
        double syncTimeDelta = syncNext.secs - syncPrev.secs;

        double distanceFactor = (syncKmDelta == 0.0)
                                    ? 0.0
                                    : (currentDistance - syncPrev.km) / syncKmDelta;

        double ghostSecs, ghostKph;
        if (syncKphDelta == 0.0) {
            ghostKph  = syncPrev.kph;
            ghostSecs = syncPrev.secs + syncTimeDelta * distanceFactor;
        } else {
            double timeExtra = syncTimeDelta - ((syncKmDelta / syncPrev.kph) * 3600.0);
            double speedFactor, timeFactor;
            if (syncKphDelta > 0) {
                speedFactor = qPow(distanceFactor, 0.66667);
                timeFactor  = qPow(distanceFactor, 0.33333);
                ghostKph = syncPrev.kph + speedFactor * syncKphDelta;
            } else {
                speedFactor = 1.0 - qPow(distanceFactor, 1.5);
                timeFactor  = qPow(distanceFactor, 3.0);
                ghostKph = syncNext.kph - speedFactor * syncKphDelta;
            }
            ghostSecs = syncPrev.secs
                        + (distanceFactor * (syncTimeDelta - timeExtra))
                        + (timeFactor * timeExtra);
        }

        float video_time_shift_ms = (float)(ghostSecs * 1000.0 - mp->position());

        float rate;
        if (ghostKph == 0.0)
            rate = 1.0f;
        else
            rate = (float)(rtd.getSpeed() / ghostKph);

        if (fabs(video_time_shift_ms) > 5000.0f) {
            mp->setPosition((qint64)(ghostSecs * 1000.0));
        } else {
            rate *= 1.0f + (video_time_shift_ms / 10000.0f);
        }

        if (rate < 0.01f)
            mp->pause();
        else
            mp->play();

        if (rate != 0.0f && fabs((rate - currentVideoRate) / rate) > 0.05f) {
            mp->setPlaybackRate(rate);
            currentVideoRate = rate;
        }
    } else {
        pushRouteKmToHtml(rtd.getRouteDistance());
    }
}


// ---------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------
VideoHtmlWindow::VideoHtmlWindow(Context *context)
    : GcChartWindow(context), context(context)
{
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartTrain_VideoPlayer));

    setProperty("color", QColor(Qt::black));
    setWidthFactor(1.0);
    setHeightFactor(1.0);
    setResizable(true);


    // ------------------------------------------------------------------
    // Chart Settings (controls panel)
    // ------------------------------------------------------------------
    QWidget *controls = new QWidget(this);
    QVBoxLayout *controlsLayout = new QVBoxLayout(controls);

    showEditorBtn = new QCheckBox(tr("Show Editor"), controls);
    showEditorBtn->setChecked(false);
    controlsLayout->addWidget(showEditorBtn);

    showTitleBtn = new QCheckBox(tr("Show Title"), controls);
    showTitleBtn->setChecked(true);
    controlsLayout->addWidget(showTitleBtn);

    keepAspectRatioBtn = new QCheckBox(tr("Keep aspect ratio (no crop)"), controls);
    keepAspectRatioBtn->setChecked(true);
    controlsLayout->addWidget(keepAspectRatioBtn);

    QPushButton *applyBtn = new QPushButton(tr("Apply HTML"), controls);
    controlsLayout->addWidget(applyBtn);
    connect(applyBtn, &QPushButton::clicked, this, &VideoHtmlWindow::applyHtml);

    controlsLayout->addStretch();
    setControls(controls);

    connect(showEditorBtn, &QCheckBox::checkStateChanged, this, &VideoHtmlWindow::showEditorChanged);
    connect(showTitleBtn,  &QCheckBox::checkStateChanged, this, &VideoHtmlWindow::showTitleChanged);
    connect(keepAspectRatioBtn, &QCheckBox::checkStateChanged, this, &VideoHtmlWindow::keepAspectRatioChanged);

    // ------------------------------------------------------------------
    // Main layout – splitter (video area + optional editor)
    // ------------------------------------------------------------------
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    setChartLayout(mainLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(400, 300);

    splitter = new QSplitter(Qt::Vertical, this);
    mainLayout->addWidget(splitter);
    splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    splitter->setMinimumSize(320, 240);

    container = new QWidget(this);
    splitter->addWidget(container);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    container->setMinimumSize(320, 240);

    videoSurface = new VideoSurfaceWidget(container);
    videoSurface->show();

    mp = new QMediaPlayer(this);
    videoSink = new QVideoSink(this);
    mp->setVideoSink(videoSink);
    mp->setAudioOutput(new QAudioOutput(this));

    connect(videoSink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame) {
        if (!frame.isValid() || !videoSurface) return;
        QVideoFrame f = frame;
        if (!f.map(QVideoFrame::ReadOnly)) return;
        QImage img = f.toImage();
        f.unmap();
        if (!img.isNull()) {
            if (img.format() != QImage::Format_RGB32 && img.format() != QImage::Format_ARGB32)
                img = img.convertToFormat(QImage::Format_RGB32);
            videoSurface->setFrame(img);
        }
    });

    htmlOverlay = new QWebEngineView(container);
    htmlOverlay->setAttribute(Qt::WA_TranslucentBackground, true);
    htmlOverlay->setStyleSheet("background: transparent;");
    htmlOverlay->page()->setBackgroundColor(Qt::transparent);
    htmlOverlay->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    htmlOverlay->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    htmlOverlay->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);

    videoSurface->setGeometry(container->rect());
    htmlOverlay->setGeometry(container->rect());
    videoSurface->lower();
    htmlOverlay->raise();
    htmlOverlay->show();

    m_webChannel = new QWebChannel(this);
    if (context->getHtmlTrainingBridge()) {
        m_webChannel->registerObject(QStringLiteral("gc"), context->getHtmlTrainingBridge());
    }
    htmlOverlay->page()->setWebChannel(m_webChannel);

    sourceEditor = new QPlainTextEdit(this);
    sourceEditor->setPlaceholderText(tr("HTML / CSS / JavaScript source…"));
    sourceEditor->setTabStopDistance(4 * sourceEditor->fontMetrics().horizontalAdvance(' '));
    sourceEditor->setVisible(false);
    splitter->addWidget(sourceEditor);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setSizes({1000, 0});

    htmlSource = R"(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<script src="qrc:///qtwebchannel/qwebchannel.js"></script>
<style>
html, body {
    margin: 0; padding: 0;
    width: 100%; height: 100%;
    background: transparent !important;
    overflow: hidden;
    font-family: sans-serif;
    color: white;
}
#info {
    position: absolute;
    top: 40px; left: 40px;
    background: rgba(0,0,0,0.55);
    color: #00ff88;
    font-size: 28px;
    padding: 12px 20px;
    border-radius: 10px;
    text-shadow: 0 0 6px black;
}
</style>
</head>
<body>
<div id="info">VideoHtmlWindow – WebChannel ready</div>
<script>
new QWebChannel(qt.webChannelTransport, function(channel) {
    window.gc = channel.objects.gc;
    console.log("gc bridge connected:", !!window.gc);
});
</script>
</body>
</html>
)";

    sourceEditor->setPlainText(htmlSource);
    htmlOverlay->page()->setHtml(htmlSource, QUrl("qrc:/"));
    htmlOverlay->raise();
    htmlOverlay->show();

    init = true;

    connect(context, &Context::telemetryUpdate, this, &VideoHtmlWindow::telemetryUpdate);
    connect(context, &Context::stop,           this, &VideoHtmlWindow::stopPlayback);
    connect(context, &Context::start,          this, &VideoHtmlWindow::startPlayback);
    connect(context, &Context::pause,          this, &VideoHtmlWindow::pausePlayback);
    connect(context, &Context::unpause,        this, &VideoHtmlWindow::resumePlayback);
    connect(context, &Context::seek,           this, &VideoHtmlWindow::seekPlayback);
    connect(context, &Context::mediaSelected,  this, &VideoHtmlWindow::mediaSelected);

    if (!context->videoFilename.isEmpty())
        mediaSelected(context->videoFilename);
    if (context->isRunning)
        startPlayback();

}


QSize VideoHtmlWindow::sizeHint() const
{
    return QSize(1200, 800);
}

QSize VideoHtmlWindow::minimumSizeHint() const
{
    return QSize(400, 300);
}


// ---------------------------------------------------------------
// Property / editor helpers
// ---------------------------------------------------------------
QString VideoHtmlWindow::getHtml() const
{
    if (sourceEditor) return sourceEditor->toPlainText();
    return htmlSource;
}

void VideoHtmlWindow::setHtml(const QString &text)
{
    htmlSource = text;
    if (sourceEditor) sourceEditor->setPlainText(text);
    applyHtml();
}

void VideoHtmlWindow::applyHtml()
{
    if (!htmlOverlay) return;
    htmlSource = sourceEditor ? sourceEditor->toPlainText() : htmlSource;
    htmlOverlay->page()->setHtml(htmlSource, QUrl("qrc:/"));
}

bool VideoHtmlWindow::isEditorVisible() const
{
    return showEditorBtn && showEditorBtn->isChecked();
}

void VideoHtmlWindow::setEditorVisible(bool visible)
{
    if (showEditorBtn) {
        showEditorBtn->blockSignals(true);
        showEditorBtn->setChecked(visible);
        showEditorBtn->blockSignals(false);
    }
    showEditorChanged(visible ? Qt::Checked : Qt::Unchecked);
}

void VideoHtmlWindow::showEditorChanged(Qt::CheckState state)
{
    if (!sourceEditor || !splitter) return;
    bool show = (state == Qt::Checked);
    sourceEditor->setVisible(show);
    if (show)
        splitter->setSizes({700, 300});
    else
        splitter->setSizes({1000, 0});
}

void VideoHtmlWindow::showTitleChanged(Qt::CheckState state)
{
    setTitleVisible(state == Qt::Checked);
}

void VideoHtmlWindow::keepAspectRatioChanged(Qt::CheckState state)
{
    setKeepAspectRatio(state == Qt::Checked);
}

bool VideoHtmlWindow::isTitleVisible() const
{
    return showTitleBtn && showTitleBtn->isChecked();
}

void VideoHtmlWindow::setTitleVisible(bool visible)
{
    if (showTitleBtn) {
        showTitleBtn->blockSignals(true);
        showTitleBtn->setChecked(visible);
        showTitleBtn->blockSignals(false);
    }
    GcChartWindow::setShowTitle(visible);
}

bool VideoHtmlWindow::isKeepAspectRatio() const
{
    return m_keepAspectRatio;
}

void VideoHtmlWindow::setKeepAspectRatio(bool keep)
{
    m_keepAspectRatio = keep;
    if (keepAspectRatioBtn) {
        keepAspectRatioBtn->blockSignals(true);
        keepAspectRatioBtn->setChecked(keep);
        keepAspectRatioBtn->blockSignals(false);
    }
    if (videoSurface)
        videoSurface->setKeepAspectRatio(keep);
}

// ---------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------
VideoHtmlWindow::~VideoHtmlWindow()
{
    if (mp)
        mp->stop();
}


// ---------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------
void VideoHtmlWindow::resizeEvent(QResizeEvent *e)
{
    GcChartWindow::resizeEvent(e);

    if (container) {
        QRect r = container->rect();
        if (videoSurface) videoSurface->setGeometry(r);
        if (htmlOverlay) {
            htmlOverlay->setGeometry(r);
            videoSurface->lower();
            htmlOverlay->raise();
        }
    }
}

void VideoHtmlWindow::showEvent(QShowEvent *e)
{
    GcChartWindow::showEvent(e);

    if (container) {
        QRect r = container->rect();
        if (videoSurface) videoSurface->setGeometry(r);
        if (htmlOverlay) {
            htmlOverlay->setGeometry(r);
            videoSurface->lower();
            htmlOverlay->raise();
        }
    }
}


// ---------------------------------------------------------------
// Playback control
// ---------------------------------------------------------------
void VideoHtmlWindow::startPlayback()
{
    if (!mp) return;

    float rate = 1.0f;
    if (context->currentVideoSyncFile() &&
        context->currentVideoSyncFile()->Points.count() > 1)
        rate = 0.1f;

    mp->setPlaybackRate(rate);
    currentVideoRate = rate;
    mp->play();

    videoSyncTimeAdjustFactor     = 1.0;
    videoSyncDistanceAdjustFactor = 1.0;

    VideoSyncFile *vsf = context->currentVideoSyncFile();
    if (vsf) {
        double videoSyncDistanceMeters = vsf->distance() * 1000.0;
        if (videoSyncDistanceMeters > 0.0) {
            ErgFile *erg = context->currentErgFile();
            if (erg && erg->mode() == ErgFileFormat::crs) {
                double ergMeters = erg->duration();
                if (ergMeters > 0.0)
                    videoSyncDistanceAdjustFactor = ergMeters / videoSyncDistanceMeters;
            }
        }

        double videoSyncFrameRate = vsf->videoFrameRate();
        if (videoSyncFrameRate > 0.0) {
            bool ok = false;
            auto md = mp->metaData();
            double mediaFrameRate = md.value(QMediaMetaData::VideoFrameRate).toReal(&ok);
            if (ok && mediaFrameRate > 0.0) {
                videoSyncTimeAdjustFactor = videoSyncFrameRate / mediaFrameRate;
            } else {
                double videoSyncDuration = vsf->duration();
                if (videoSyncDuration > 0.0) {
                    double mediaDuration = (double)mp->duration();
                    if (mediaDuration > 0.0)
                        videoSyncTimeAdjustFactor = mediaDuration / videoSyncDuration;
                }
            }
        }
    }

    curPosition = 1;
    state = Playing;
}

void VideoHtmlWindow::stopPlayback()
{
    if (!mp) return;
    mp->stop();
    state = None;
    currentVideoRate = 1.0f;
}

void VideoHtmlWindow::pausePlayback()
{
    if (!mp) return;
    mp->pause();
    state = Paused;
}

void VideoHtmlWindow::resumePlayback()
{
    if (!mp) return;
    mp->play();
    state = Playing;
}

void VideoHtmlWindow::seekPlayback(long ms)
{
    if (!hasActiveVideo())
        return;

    if (context->currentVideoSyncFile())
        return;

    mp->setPosition(ms);
}

void VideoHtmlWindow::mediaSelected(QString filename)
{
    if (!mp) return;
    if (filename.isEmpty()) return;

    mp->setSource(QUrl::fromLocalFile(filename));

    if (context && context->isRunning)
        startPlayback();
}
