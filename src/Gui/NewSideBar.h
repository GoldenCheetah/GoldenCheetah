#include <QWidget>
#include <QMap>
#include <QVBoxLayout>

class Context;
class NewSideBarItem;
class NewSideBar : public QWidget
{

    Q_OBJECT

    friend class ::NewSideBarItem;

    public:
        NewSideBar(Context *context, QWidget *parent);

    public slots:

        // managing items - the id can be assigned or will get a default
        //                  it returns the id used
        int addItem(QImage icon, QString name, int id=-1, QString whatsThisText="");

        // leave a gap- we have main icons, gap, apps, gap, sync, prefs
        void addStretch();

        // is it shown, is it usable?
        void setItemVisible(int id, bool);
        void setItemEnabled(int id, bool);

        // can we select it, select it by program?
        void setItemSelectable(int id, bool);
        void setItemSelected(int id, bool);

        // config changed
        void configChanged(qint32);

        // called by children
        void clicked(int id);
        void selected(int id);

    signals:

        void itemSelected(int id);
        void itemClicked(int id);

    protected:
        Context *context;

    private:

        QWidget *top, *middle, *bottom; // bars at top and the bottom
        QMap<int,NewSideBarItem*> items;

        int lastid; // when autoallocating
        QVBoxLayout *layout;
};

// tightly bound with parent
class NewSideBarItem : public QWidget
{
    Q_OBJECT

    public:

        NewSideBarItem(NewSideBar *sidebar, int id, QImage icon, QString name);

        void setSelectable(bool);
        void setEnabled(bool);
        void setSelected(bool);
        //void setVisible(bool); - standard qwidget method, we do not need to implement

        bool isSelected() const { return selected; }
        bool isEnabled() const { return enabled; }
        bool isSelectable() const { return selectable; }
        //bool isVisible() const { return QWidget::isVisible(); } - standard qwidget method

        // trap all events for me
        bool eventFilter(QObject *oj, QEvent *e);

    public slots:

        // config changed
        void configChanged(qint32);

        void paintEvent(QPaintEvent *event);

    private:

        NewSideBar *sidebar; // for emitting signals
        int id;

        // pre-rendered/calculated icons and colors
        QImage icon;
        QPixmap iconNormal, iconDisabled, iconSelect;
        QColor fg_normal, fg_disabled, fg_select;
        QColor bg_normal, bg_hover, bg_select, bg_disable;
        QFont textfont;

        QString name;

        bool selected;
        bool selectable;
        bool enabled;
        bool clicked;
};
