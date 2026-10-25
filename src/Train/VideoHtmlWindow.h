/*
 * VideoHtmlWindow – Video background + transparent HTML overlay
 * (no classic MeterWidgets)
 */

#ifndef _GC_VideoHtmlWindow_h
#define _GC_VideoHtmlWindow_h

#include "GoldenCheetah.h"
#include "Context.h"
#include "RealtimeData.h"
#include "VideoSyncFile.h"
#include "ErgFile.h"

#include <QWidget>
#include <QSplitter>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QWebEngineView>
#include <QWebChannel>
#include <QVideoSink>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QImage>
#include <QRecursiveMutex>

class VideoHtmlWindow : public GcChartWindow
{
    Q_OBJECT

    Q_PROPERTY(QString html READ getHtml WRITE setHtml USER true)
    Q_PROPERTY(bool showEditor READ isEditorVisible WRITE setEditorVisible USER true)
    Q_PROPERTY(bool showTitle READ isTitleVisible WRITE setTitleVisible USER true)
    Q_PROPERTY(bool keepAspectRatio READ isKeepAspectRatio WRITE setKeepAspectRatio USER true)

public:
    explicit VideoHtmlWindow(Context *context);
    ~VideoHtmlWindow();

    QString getHtml() const;
    void setHtml(const QString &text);

    bool isEditorVisible() const;
    void setEditorVisible(bool);

    bool isTitleVisible() const;
    void setTitleVisible(bool);

    bool isKeepAspectRatio() const;
    void setKeepAspectRatio(bool);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    void applyHtml();
    void telemetryUpdate(const RealtimeData &rtd);
    void startPlayback();
    void stopPlayback();
    void pausePlayback();
    void resumePlayback();
    void seekPlayback(long ms);
    void mediaSelected(QString filename);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void showEvent(QShowEvent *e) override;

private slots:
    void showEditorChanged(int state);
    void showTitleChanged(int state);
    void keepAspectRatioChanged(int state);

private:
#if defined(GC_VIDEO_QT6)
    class VideoSurfaceWidget : public QWidget
    {
    public:
        explicit VideoSurfaceWidget(QWidget *parent = nullptr);
        void setFrame(const QImage &img);
        void setKeepAspectRatio(bool keep) { m_keepAspectRatio = keep; update(); }

    protected:
        void paintEvent(QPaintEvent *event) override;

    private:
        QImage frame;
        bool m_keepAspectRatio = true;
    };

    VideoSyncFilePoint VideoSyncPointAdjust(const VideoSyncFilePoint &vsfp) const;
    bool hasActiveVideo() const;

    enum PlaybackState { None, Playing, Paused };

    Context *context = nullptr;
    bool init = false;

    QWidget *container = nullptr;
    QSplitter *splitter = nullptr;
    VideoSurfaceWidget *videoSurface = nullptr;
    QMediaPlayer *mp = nullptr;
    QVideoSink *videoSink = nullptr;
    QWebEngineView *htmlOverlay = nullptr;
    QWebChannel *m_webChannel = nullptr;
    QPlainTextEdit *sourceEditor = nullptr;

    QCheckBox *showEditorBtn = nullptr;
    QCheckBox *showTitleBtn = nullptr;
    QCheckBox *keepAspectRatioBtn = nullptr;

    QString htmlSource;
    bool m_keepAspectRatio = true;

    PlaybackState state = None;
    int curPosition = 1;
    float currentVideoRate = 1.0f;
    double videoSyncTimeAdjustFactor = 1.0;
    double videoSyncDistanceAdjustFactor = 1.0;
#endif
};

#endif // _GC_VideoHtmlWindow_h