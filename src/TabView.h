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
#include <QMetaObject>
#include <QStackedWidget>

#include "HomeWindow.h"
#include "GcSideBarItem.h"
#include "GcWindowRegistry.h"

class Tab;
class ViewSplitter;
class Context;
class BlankStatePage;

class TabView : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool sidebar READ sidebarEnabled WRITE setSidebarEnabled USER true)
    Q_PROPERTY(bool tiled READ isTiled WRITE setTiled USER true)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected USER true) // make this last always

    public:

        TabView(Context *context, int type);
        virtual ~TabView();
        virtual void close() {};

        // add the widgets to the view
        void setSidebar(QWidget *sidebar);
        QWidget *sidebar() { return sidebar_; }
        void setPage(HomeWindow *page);
        HomeWindow *page() { return page_;}
        void setBlank(BlankStatePage *blank);
        BlankStatePage *blank() { return blank_; }
        void setBottom(QWidget *bottom);
        QWidget *bottom() { return bottom_; }

        // sidebar
        void setSidebarEnabled(bool x) { _sidebar=x; sidebarChanged(); }
        bool sidebarEnabled() const { return _sidebar; }

        // tiled
        void setTiled(bool x) { _tiled=x; tileModeChanged(); }
        bool isTiled() const { return _tiled; }

        // bottom
        void setShowBottom(bool x);
        bool isShowBottom() { if (bottom_) return bottom_->isVisible(); return false; }
        bool hasBottom() { return (bottom_!=NULL); }

        // select / deselect view
        void setSelected(bool x) { _selected=x; selectionChanged(); }
        bool isSelected() const { return _selected; }

        void saveState() { if (page_) page_->saveState(); }

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
        virtual void resetLayout();
        
        virtual void addChart(GcWinID id);

        // Let the base class handle the splitter movement and
        // hiding the sidebar by dragging it closed.
        void splitterMoved(int, int);

        //void mediaSelected(QString filename);
        //void ergSelected(ErgFile *erg);

    protected:

        Context *context;
        int type; // used by windowregistry; e.g VIEW_TRAIN VIEW_ANALYSIS VIEW_DIARY VIEW_HOME
                  // we don't care what values are pass through to the GcWindowRegistry to decide
                  // what charts are relevant for this view.

        // properties
        bool _filtered;
        QStringList _filter;
        bool _sidebar;
        bool _tiled;
        bool _selected;
        int lastHeight; // last height of splitter, default to 100...

        QStackedWidget *stack;
        QSplitter *splitter;
        ViewSplitter *mainSplitter;
        QPropertyAnimation *anim;
        QWidget *sidebar_;
        QWidget *bottom_;
        HomeWindow *page_;
        BlankStatePage *blank_;

};

// we make our own view splitter for the bespoke handle
class ViewSplitter : public QSplitter
{
    Q_OBJECT

public:
    ViewSplitter(Qt::Orientation orientation, QString name, TabView *parent=0) :
        orientation(orientation), name(name), tabView(parent), showForDrag(false),
        QSplitter(orientation, parent) {
        setAcceptDrops(true);
        qRegisterMetaType<ViewSplitter*>("hpos");
    }

protected:
    QSplitterHandle *createHandle() {
        return new GcSplitterHandle(name, orientation, this);
    }
    int handleWidth() { return 23; };

    virtual void dragEnterEvent(QDragEnterEvent *event) {
        if (event->mimeData()->formats().contains("application/x-qabstractitemmodeldatalist")) {
            if (tabView->hasBottom() && tabView->isShowBottom() == false) {
                showForDrag = true;
                tabView->setShowBottom(true);
            }
            event->acceptProposedAction(); // always accept for this mimeData type
                                           // so we continue to get the leave events we ignore (!)
        }
    }

    virtual void dragLeaveEvent(QDragLeaveEvent *event) {

        int X = this->mapFromGlobal(QCursor::pos()).x();
        int Y = this->mapFromGlobal(QCursor::pos()).y();

        // ignore events when the cursor doesn't actually leave the bounds
        // of our view -- its just going over multiple widgets
        if (X>0 && Y>0 && X <= geometry().width() && Y <= geometry().height()) return;

        if (showForDrag == true && !underMouse() && tabView->hasBottom() && tabView->isShowBottom() == true) {
            showForDrag = false;
            tabView->setShowBottom(false);
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

private:
    Qt::Orientation orientation;
    QString name;
    TabView *tabView;
    bool showForDrag;
};

#endif // _GC_TabView_h
