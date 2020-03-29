#include <QMainWindow>
#include <QToolBar>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>

class NewMainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        NewMainWindow(QApplication *app);

        // initially lets just do this, the code will
        // get more complex over time, but lets start
        // with the simplest methods for dragging a
        // frameless window
        void mousePressEvent(QMouseEvent *event);
        void mouseReleaseEvent(QMouseEvent *event);
        void mouseDoubleClickEvent(QMouseEvent *event);
        void mouseMoveEvent(QMouseEvent *event);
        bool isHotSpot(QPoint);

    protected:

        // setup the toolbar, needs to support
        // windows, mac and linux in a local manner
        void setupToolbar();

        // set initial geometry, screen etc
        void initialPosition();

    public slots:
        void minimizeWindow();

    private:
        QApplication *app;

        QWidget *main;
        QVBoxLayout *layout;
        QToolBar *toolbar;
        QLineEdit *searchbox;
        QPushButton *quitbutton, *minimisebutton;

        QPoint clickPos;
        enum { None, Drag, Resize } state;
};
