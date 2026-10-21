/*
 * VideoHtmlWindow – Video background + transparent HTML overlay
 * (no classic MeterWidgets)
 */

#include "VideoHtmlWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "HelpWhatsThis.h"
#include "HtmlTrainingBridge.h"

#include <QHBoxLayout>
#include <QUrl>
#include <QPainter>

#include <QMediaMetaData>
#include <QtMath>   // for qPow

#if defined(GC_VIDEO_QT6)

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
        QImage scaled = frame.scaled(size(),
                                     Qt::KeepAspectRatioByExpanding,
                                     Qt::SmoothTransformation);
        QPoint topLeft((width()  - scaled.width())  / 2,
                       (height() - scaled.height()) / 2);
        p.drawImage(topLeft, scaled);
    }
}

#endif // GC_VIDEO_QT6


// ---------------------------------------------------------------
// Helper: VideoSync
// ---------------------------------------------------------------
#if defined(GC_VIDEO_QT6)

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

#endif


// ---------------------------------------------------------------
// The Core Sync: TelemetryUpdate
// ---------------------------------------------------------------
void VideoHtmlWindow::telemetryUpdate(const RealtimeData &rtd)
{
#if defined(GC_VIDEO_QT6)
    if (!isVisible() || !hasActiveVideo())
        return;
    if (state != Playing)
        return;

    // Push the same route distance the video uses into the HTML overlay
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

    // ---------- VideoSync-driven rate / seek ----------
    if (context->currentVideoSyncFile()) {
        QVector<VideoSyncFilePoint> points = context->currentVideoSyncFile()->Points;
        if (points.count() < 2) {
            // Still update HTML rider even if sync file is too short
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

        // Keep HTML map / elevation rider locked to the same distance
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

        // Hard seek if more than 5 s out of sync
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
        // No videosync: still keep HTML rider on route distance
        pushRouteKmToHtml(rtd.getRouteDistance());
    }
#else
    Q_UNUSED(rtd);
#endif
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

#if defined(GC_VIDEO_QT6)

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

    QPushButton *applyBtn = new QPushButton(tr("Apply HTML"), controls);
    controlsLayout->addWidget(applyBtn);
    connect(applyBtn, &QPushButton::clicked, this, &VideoHtmlWindow::applyHtml);

    controlsLayout->addStretch();
    setControls(controls);

    connect(showEditorBtn, &QCheckBox::stateChanged, this, &VideoHtmlWindow::showEditorChanged);
    connect(showTitleBtn,  &QCheckBox::stateChanged, this, &VideoHtmlWindow::showTitleChanged);

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

    // ----- Video + HTML overlay -----
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

    // ----- WebChannel -----
    m_webChannel = new QWebChannel(this);
    if (context->getHtmlTrainingBridge()) {
        m_webChannel->registerObject(QStringLiteral("gc"), context->getHtmlTrainingBridge());
    }
    htmlOverlay->page()->setWebChannel(m_webChannel);

    // ----- Source editor (hidden by default) -----
    sourceEditor = new QPlainTextEdit(this);
    sourceEditor->setPlaceholderText(tr("HTML / CSS / JavaScript source…"));
    sourceEditor->setTabStopDistance(4 * sourceEditor->fontMetrics().horizontalAdvance(' '));
    sourceEditor->setVisible(false);
    splitter->addWidget(sourceEditor);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setSizes({1000, 0});

    // Default HTML
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

#else
    QVBoxLayout *fallback = new QVBoxLayout();
    setChartLayout(fallback);
    fallback->addWidget(new QLabel(tr("VideoHtmlWindow requires Qt6 video support (GC_VIDEO_QT6)")));
    init = false;
#endif
}


// ---------------------------------------------------------------
// Property / editor helpers
// ---------------------------------------------------------------
QString VideoHtmlWindow::getHtml() const
{
#if defined(GC_VIDEO_QT6)
    if (sourceEditor) return sourceEditor->toPlainText();
    return htmlSource;
#else
    return QString();
#endif
}

void VideoHtmlWindow::setHtml(const QString &text)
{
#if defined(GC_VIDEO_QT6)
    htmlSource = text;
    if (sourceEditor) sourceEditor->setPlainText(text);
    applyHtml();
#endif
}

void VideoHtmlWindow::applyHtml()
{
#if defined(GC_VIDEO_QT6)
    if (!htmlOverlay) return;
    htmlSource = sourceEditor ? sourceEditor->toPlainText() : htmlSource;
    htmlOverlay->page()->setHtml(htmlSource, QUrl("qrc:/"));
#endif
}

bool VideoHtmlWindow::isEditorVisible() const
{
#if defined(GC_VIDEO_QT6)
    return showEditorBtn && showEditorBtn->isChecked();
#else
    return false;
#endif
}

void VideoHtmlWindow::setEditorVisible(bool visible)
{
#if defined(GC_VIDEO_QT6)
    if (showEditorBtn) {
        showEditorBtn->blockSignals(true);
        showEditorBtn->setChecked(visible);
        showEditorBtn->blockSignals(false);
    }
    showEditorChanged(visible ? Qt::Checked : Qt::Unchecked);
#endif
}

void VideoHtmlWindow::showEditorChanged(int state)
{
#if defined(GC_VIDEO_QT6)
    if (!sourceEditor || !splitter) return;
    bool show = (state == Qt::Checked);
    sourceEditor->setVisible(show);
    if (show)
        splitter->setSizes({700, 300});
    else
        splitter->setSizes({1000, 0});
#endif
}

bool VideoHtmlWindow::isTitleVisible() const
{
#if defined(GC_VIDEO_QT6)
    return showTitleBtn && showTitleBtn->isChecked();
#else
    return true;
#endif
}

void VideoHtmlWindow::setTitleVisible(bool visible)
{
#if defined(GC_VIDEO_QT6)
    if (showTitleBtn) {
        showTitleBtn->blockSignals(true);
        showTitleBtn->setChecked(visible);
        showTitleBtn->blockSignals(false);
    }
    GcChartWindow::setShowTitle(visible);
#endif
}

void VideoHtmlWindow::showTitleChanged(int state)
{
    setTitleVisible(state == Qt::Checked);
}


// ---------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------
VideoHtmlWindow::~VideoHtmlWindow()
{
#if defined(GC_VIDEO_QT6)
    if (mp)
        mp->stop();
#endif
}


// ---------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------
void VideoHtmlWindow::resizeEvent(QResizeEvent *e)
{
    GcChartWindow::resizeEvent(e);

#if defined(GC_VIDEO_QT6)
    if (container) {
        QRect r = container->rect();
        if (videoSurface) videoSurface->setGeometry(r);
        if (htmlOverlay) {
            htmlOverlay->setGeometry(r);
            videoSurface->lower();
            htmlOverlay->raise();
        }
    }
#endif
}

void VideoHtmlWindow::showEvent(QShowEvent *e)
{
    GcChartWindow::showEvent(e);

#if defined(GC_VIDEO_QT6)
    if (container) {
        QRect r = container->rect();
        if (videoSurface) videoSurface->setGeometry(r);
        if (htmlOverlay) {
            htmlOverlay->setGeometry(r);
            videoSurface->lower();
            htmlOverlay->raise();
        }
    }
#endif
}


// ---------------------------------------------------------------
// Playback control
// ---------------------------------------------------------------
void VideoHtmlWindow::startPlayback()
{
#if defined(GC_VIDEO_QT6)
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
#endif
}

void VideoHtmlWindow::stopPlayback()
{
#if defined(GC_VIDEO_QT6)
    if (!mp) return;
    mp->stop();
    state = None;
    currentVideoRate = 1.0f;
#endif
}

void VideoHtmlWindow::pausePlayback()
{
#if defined(GC_VIDEO_QT6)
    if (!mp) return;
    mp->pause();
    state = Paused;
#endif
}

void VideoHtmlWindow::resumePlayback()
{
#if defined(GC_VIDEO_QT6)
    if (!mp) return;
    mp->play();
    state = Playing;
#endif
}

void VideoHtmlWindow::seekPlayback(long ms)
{
#if defined(GC_VIDEO_QT6)
    if (!hasActiveVideo())
        return;

    // With a videosync file, distance is driven by telemetryUpdate
    if (context->currentVideoSyncFile())
        return;

    mp->setPosition(ms);
#else
    Q_UNUSED(ms);
#endif
}

void VideoHtmlWindow::mediaSelected(QString filename)
{
#if defined(GC_VIDEO_QT6)
    if (!mp) return;
    if (filename.isEmpty()) return;

    mp->setSource(QUrl::fromLocalFile(filename));

    if (context && context->isRunning)
        startPlayback();
#else
    Q_UNUSED(filename);
#endif
}