/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_TabView_h
#define _GC_TabView_h 1

#include <QWidget>
#include <QSplitter>
#include <QFont>
#include <QFontMetrics>
#include <QMetaObject>
#include <QStackedWidget>

#include "Perspective.h"
#include "Colors.h"
#include "GcSideBarItem.h"
#include "GcWindowRegistry.h"

class AthleteTab;
class ViewSplitter;
class Context;
class BlankStatePage;
class TrainSidebar;
class PerspectiveDialog;

class AbstractView : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool sidebar READ sidebarEnabled WRITE setSidebarEnabled USER true)
    Q_PROPERTY(bool tiled READ isTiled WRITE setTiled USER true)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected USER true) // make this last always

    friend class ::MainWindow;
    friend class ::TrainSidebar;
    friend class ::PerspectiveDialog;

    public:

        AbstractView(Context *context, int type);
        virtual ~AbstractView();
        virtual void close() {};

        // add the widgets to the view
        void setSidebar(QWidget *sidebar);
        QWidget *sidebar() { return sidebar_; }
        void setPages(QStackedWidget *pages);
        Perspective *page() { return perspective_;}
        void setBlank(BlankStatePage *blank);
        BlankStatePage *blank() { return blank_; }
        void setBottom(QWidget *bottom);
        QWidget *bottom() { return bottom_; }

        // sidebar
        static QString ourStyleSheet();
        void setSidebarEnabled(bool x) { _sidebar=x; sidebarChanged(); }
        bool sidebarEnabled() const { return _sidebar; }

        // tiled
        void setTiled(bool x) { _tiled=x; tileModeChanged(); }
        bool isTiled() const { return _tiled; }

        // load/save perspectives
        void restoreState(bool useDefault = false);
        void saveState();
        void appendPerspective(Perspective *page);

        void setPerspectives(QComboBox *perspectiveSelector, bool selectChart=false); // set the combobox when view selected
        void perspectiveSelected(int index); // combobox selections changed because the user selected a perspective

        // add a new perspective
        Perspective *addPerspective(QString);
        void removePerspective(Perspective *);
        void swapPerspective(int from, int to); // reorder by moving 1 pos at a time

        // bottom
        void dragEvent(bool); // showbottom on drag event
        void setShowBottom(bool x);
        void setBottomRequested(bool x);
        bool isShowBottom() { if (bottom_) return bottom_->isVisible(); return false; }
        bool isBottomRequested() { return bottomRequested; }
        bool hasBottom() { return (bottom_!=NULL); }
        void setHideBottomOnIdle(bool x);

        ViewSplitter *bottomSplitter() { return mainSplitter; }

        // select / deselect view
        void setSelected(bool x) { _selected=x; selectionChanged(); }
        bool isSelected() const { return _selected; }

        int viewType() { return type; }

        void importChart(QMap<QString,QString>properties, bool select) { perspective_->importChart(properties, select); }

        bool importPerspective(QString filename);
        void exportPerspective(Perspective *, QString filename);

    signals:

        void sidebarClosed(); // the user dragged the sidebar closed.
        void onSelectionChanged();

    public slots:

        // interface used by the Tab class - must be implemented by a TabView
        virtual bool isBlank() = 0;
        virtual void setRide(RideItem*);
        virtual void checkBlank();

        // can be overriden by the derived class but we provide a working version
        virtual void sidebarChanged();
        virtual void tileModeChanged();
        virtual void selectionChanged();
        virtual void resetLayout(QComboBox *perspectiveSelector);

        virtual void addChart(GcWinID id);

        // Let the base class handle the splitter movement and
        // hiding the sidebar by dragging it closed.
        void splitterMoved(int, int);

        //void mediaSelected(QString filename);
        //void ergSelected(ErgFile *erg);
        //void videosyncSelected(VideosyncFile *videosync);
        void configChanged(qint32);

        void resizeEvent(QResizeEvent*);

    protected:

        Context *context;
        int type; // used by windowregistry; e.g VIEW_TRAIN VIEW_ANALYSIS VIEW_DIARY VIEW_TRENDS
                  // we don't care what values are pass through to the GcWindowRegistry to decide
                  // what charts are relevant for this view.

        // properties
        bool _filtered;
        QStringList _filter;
        bool _sidebar;
        bool _tiled;
        bool _selected;
        int lastHeight; // last height of splitter, default to 100...
        int sidewidth; // width of sidebar
        bool active;
        bool bottomRequested;
        bool bottomHideOnIdle;
        bool perspectiveactive;

        QStackedWidget *stack;
        QSplitter *splitter;
        ViewSplitter *mainSplitter;
        QPropertyAnimation *anim;
        QWidget *sidebar_;
        QWidget *bottom_;

        Perspective *perspective_; // currently selected page
        QList<Perspective *> perspectives_;

        // the perspectives are stacked- charts and their associatated controls
        QStackedWidget *pstack, *cstack;
        BlankStatePage *blank_;

        bool loaded;

    private slots:
        void onIdle();
        void onActive();
};

// reads in perspectives
class ViewParser : public QXmlDefaultHandler
{

public:
    ViewParser(Context *context, int type, bool useDefault) : style(2), context(context), type(type), useDefault(useDefault) {}

    // the results!
    QList<Perspective*> perspectives;
    int style;

    // unmarshall
    bool startDocument();
    bool endDocument();
    bool endElement( const QString&, const QString&, const QString &qName );
    bool startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs );
    bool characters( const QString& str );

protected:
    Context *context;
    GcChartWindow *chart;
    Perspective *page; // current
    int type; // what type of view is this VIEW_{HOME,ANALYSIS,DIARY,TRAIN}
    bool useDefault; // force a reset by using the default layouts

};
// we make our own view splitter for the bespoke handle
class ViewSplitter : public QSplitter
{
    Q_OBJECT

public:
    ViewSplitter(Qt::Orientation orientation, QString name, AbstractView *parent=0) :
        QSplitter(orientation, parent), orientation(orientation), name(name), tabView(parent), showForDrag(false) {
        setAcceptDrops(true);
        qRegisterMetaType<ViewSplitter*>("hpos");
        clearbutton=toggle=NULL;
    }

protected:
    double fh() { QFontMetrics fm(baseFont); return fm.height(); }
    double spacer() { return (2 * dpiYFactor); }
    QSplitterHandle *createHandle() {
        if (this->tabView)
        {
            if (this->tabView->viewType() == VIEW_TRAIN)
            {
                return new GcSplitterHandle(name, orientation, NULL, NULL, NULL, this);
            }
        }
        return new GcSplitterHandle(name, orientation, NULL, newclear(), newtoggle(), this);
    }
    int handleWidth() { return fh() + (spacer()*4); };

    QPushButton *newclear() {
        if (clearbutton) delete clearbutton; // we only need one!
        clearbutton = new QPushButton(tr("Clear"), this);
//      clearbutton->setFixedWidth(60); // no fixed length to allow translation
        clearbutton->setFixedHeight(fh() + (spacer() * 2));
        clearbutton->setFocusPolicy(Qt::NoFocus);
        connect(clearbutton, SIGNAL(clicked()), this, SLOT(clearClicked()));

        return clearbutton;
    }

    QPushButton *newtoggle() {
        if (toggle) delete toggle; // we only need one!
        toggle = new QPushButton(tr("OFF"), this);
        toggle->setCheckable(true);
        toggle->setChecked(false);
//      toggle->setFixedWidth(40);   // no fixed length to allow translation
        toggle->setFixedHeight(fh() + (spacer()*2));
        toggle->setFocusPolicy(Qt::NoFocus);
        connect(toggle, SIGNAL(clicked()), this, SLOT(toggled()));

        return toggle;
    }

    virtual void dragEnterEvent(QDragEnterEvent *event) {

        // we handle intervals or seasons
        if (event->mimeData()->formats().contains("application/x-gc-intervals") ||
            event->mimeData()->formats().contains("application/x-gc-seasons")) {
            if (tabView->hasBottom() && tabView->isShowBottom() == false) {
                showForDrag = true;
                tabView->dragEvent(true);
            }
            event->acceptProposedAction(); // always accept for this mimeData type
                                           // so we continue to get the leave events we ignore (!)
        }
    }

    virtual void dragLeaveEvent(QDragLeaveEvent *) {

        int X = this->mapFromGlobal(QCursor::pos()).x();
        int Y = this->mapFromGlobal(QCursor::pos()).y();

        // ignore events when the cursor doesn't actually leave the bounds
        // of our view -- its just going over multiple widgets
        if (X>0 && Y>0 && X <= geometry().width() && Y <= geometry().height()) return;

        if (showForDrag == true && !underMouse() && tabView->hasBottom() && tabView->isShowBottom() == true) {
            showForDrag = false;
            tabView->dragEvent(false);
        }
    }

public:
    Q_PROPERTY(int hpos READ hpos WRITE sethpos USER true)

    // handle position
    int hpos() const { if (sizes().count() == 2) return sizes()[0]; else return 0; }

    void sethpos(int x) {
        if (x<0) return; //r requested size too small!
        QList<int> csizes = sizes();
        if (csizes.count() != 2) return; //don't have two widgets!
        int tot = csizes[0] + csizes[1];
        if (tot < x) return; // requested size too big!
        csizes[0] = x;
        csizes[1] = tot-x;
        setSizes(csizes);
    }

    // max hpos
    int maxhpos() {
        QList<int> csizes = sizes();
        if (csizes.count() != 2) return 0; //don't have two widgets!
        int tot = csizes[0] + csizes[1];
        return tot - 1;
    }

signals:
    void compareChanged(bool);
    void compareClear();

public slots:
    void toggled() {
        QFont font;
        if (toggle->isChecked()) {
            font.setWeight(QFont::Bold);
            toggle->setFont(font);
            toggle->setStyleSheet("color: red");
            toggle->setText(tr("ON"));
        } else {
            font.setWeight(QFont::Normal);
            toggle->setFont(font);
            toggle->setStyleSheet("");
            toggle->setText(tr("OFF"));
        }
        // we started compare mode
        emit compareChanged(toggle->isChecked());
    }

    void clearClicked() {
        emit compareClear();
    }

private:
    Qt::Orientation orientation;
    QString name;
    AbstractView *tabView;
    bool showForDrag;
    QPushButton *toggle, *clearbutton;
};

#endif // _GC_TabView_h
