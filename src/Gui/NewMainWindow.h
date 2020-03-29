#include <QMainWindow>
#include <QToolBar>
#include <QVBoxLayout>
#include <QLineEdit>

class NewMainWindow : public QMainWindow
{
    public:
        NewMainWindow(QApplication *app);

        // initially lets just do this, the code will
        // get more complex over time, but lets start
        // with the simplest methods for dragging a
        // frameless window
        void mousePressEvent(QMouseEvent *event);
        void mouseMoveEvent(QMouseEvent *event);

    protected:

        // setup the toolbar, needs to support
        // windows, mac and linux in a local manner
        void setupToolbar();

        // set initial geometry, screen etc
        void initialPosition();

    private:
        QApplication *app;

        QWidget *main;
        QVBoxLayout *layout;
        QToolBar *toolbar;
        QLineEdit *searchbox;

        QPoint clickPos;
};
