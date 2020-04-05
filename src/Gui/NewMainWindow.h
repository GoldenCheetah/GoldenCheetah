#include <QMainWindow>
#ifdef Q_OS_MAC
#include <QMacToolBar>
#else
#include <QToolBar>
#endif
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>

class NewMainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        NewMainWindow(QApplication *app);

        // we are frameless, so we handle functions that
        // would normally be performed by the window manager
        // in exchange we get to handle all the aesthetics
        enum { Inactive, Drag, EdgeHover, Resize } state;
        enum ResizeType { None, Top, Left, Right, Bottom, TL, TR, BL, BR } resize;

        // initially lets just do this, the code will
        // get more complex over time, but lets start
        // with the simplest methods for dragging a
        // frameless window
        void mousePressEvent(QMouseEvent *event);
        void mouseReleaseEvent(QMouseEvent *event);
        void mouseDoubleClickEvent(QMouseEvent *event);
        void mouseMoveEvent(QMouseEvent *event);
        bool eventFilter(QObject *oj, QEvent *e);
        bool isDragHotSpot(QPoint);
        int isResizeHotSpot(QPoint);

    protected:

        // setup the toolbar, needs to support
        // windows, mac and linux in a local manner
        void setupToolbar();

#ifdef Q_OS_MAC
        void macNativeSettings();
#endif
        // set initial geometry, screen etc
        void initialPosition();

    public slots:
        void minimizeWindow();

    private:
        QApplication *app;

        QWidget *main;
        QVBoxLayout *layout;
#ifdef Q_OS_MAC
        QMacToolBar *toolbar;
#else
        QToolBar *toolbar;
#endif
        QLineEdit *searchbox;
        QPushButton *quitbutton, *minimisebutton;

        QPoint clickPos;
        QRect clickGeom;
};
