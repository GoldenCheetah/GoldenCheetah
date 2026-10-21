#ifndef _GC_VideoHtmlWindow_h
#define _GC_VideoHtmlWindow_h

#include "GoldenCheetah.h"
#include "Context.h"
#include "VideoSyncFile.h"
#include "ErgFile.h"
#include "RealtimeData.h"

#include <QPlainTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWebChannel>

#if defined(GC_VIDEO_QT6)
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>
#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QPainter>
#endif

class VideoHtmlWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    // Persisted in the perspective
    Q_PROPERTY(QString html READ getHtml WRITE setHtml USER true)
    Q_PROPERTY(bool showEditor READ isEditorVisible WRITE setEditorVisible USER true)
    Q_PROPERTY(bool showTitleBar READ isTitleVisible WRITE setTitleVisible USER true)

public:
    VideoHtmlWindow(Context *context);
    ~VideoHtmlWindow();

    QString getHtml() const;
    void setHtml(const QString &text);

    bool isEditorVisible() const;
    void setEditorVisible(bool visible);

    bool isTitleVisible() const;
    void setTitleVisible(bool visible);

public slots:
    void startPlayback();
    void stopPlayback();
    void pausePlayback();
    void resumePlayback();
    void mediaSelected(QString filename);
    void telemetryUpdate(const RealtimeData &rtd);
    void seekPlayback(long ms);

    void applyHtml();
    void showEditorChanged(int state);
    void showTitleChanged(int state);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void showEvent(QShowEvent *e) override;

private:
#if defined(GC_VIDEO_QT6)
    class VideoSurfaceWidget : public QWidget {
    public:
        explicit VideoSurfaceWidget(QWidget *parent = nullptr);
        void setFrame(const QImage &img);
    protected:
        void paintEvent(QPaintEvent *) override;
    private:
        QImage frame;
    };

    QWidget            *container     = nullptr;
    VideoSurfaceWidget *videoSurface  = nullptr;
    QWebEngineView     *htmlOverlay   = nullptr;
    QMediaPlayer       *mp            = nullptr;
    QVideoSink         *videoSink     = nullptr;

    QSplitter          *splitter      = nullptr;
    QPlainTextEdit     *sourceEditor  = nullptr;
    QWebChannel        *m_webChannel  = nullptr;

    QCheckBox          *showEditorBtn = nullptr;
    QCheckBox          *showTitleBtn  = nullptr;

    QString             htmlSource;
#endif

    // Video sync state (same idea as VideoWindow)
    int    curPosition = 1;
    float  currentVideoRate = 1.0f;
    double videoSyncTimeAdjustFactor = 1.0;
    double videoSyncDistanceAdjustFactor = 1.0;

    enum PlaybackState { None, Playing, Paused };
    PlaybackState state = None;

    VideoSyncFilePoint VideoSyncPointAdjust(const VideoSyncFilePoint &vsfp) const;
    bool hasActiveVideo() const;

    Context *context;
    bool init = false;
};

#endif