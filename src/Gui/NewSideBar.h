/*
 * Copyright (c) 2020 Mark Liversedge <liversedge@gmail.com>
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

#ifndef _GC_NewSideBar_h
#define _GC_NewSideBar_h 1

#include <QWidget>
#include <QMap>
#include <QVBoxLayout>

enum class GcSideBarBtnId : int {
    NO_BUTTON_SET = -1,
    SELECT_ATHLETE_BTN = 0,
    PLAN_BTN = 1,
    TRENDS_BTN = 2,
    ACTIVITIES_BTN = 3,
    REFLECT_BTN = 4,
    TRAIN_BTN = 5,
    APPS_BTN = 6,
    SYNC_BTN = 7,
    OPTIONS_BTN = 8
};

class NewSideBarItem;
class NewSideBar : public QWidget
{

    Q_OBJECT
    Q_ENUM(GcSideBarBtnId)

    friend class ::NewSideBarItem;

    public:
        NewSideBar(QWidget *parent);

    public slots:

        // managing items - addItem returns the id of the button added, or NO_BUTTON_SET.
        GcSideBarBtnId addItem(QImage icon, const QString& name, GcSideBarBtnId id, const QString& whatsThisText);

        // leave a gap- we have main icons, gap, apps, gap, sync, prefs
        void addStretch();

        // is it shown, is it usable?
        void setItemVisible(GcSideBarBtnId id, bool);
        void setItemEnabled(GcSideBarBtnId id, bool);

        // can we select it, select it by program?
        void setItemSelectable(GcSideBarBtnId id, bool);
        void setItemSelected(GcSideBarBtnId id, bool);

        // config changed
        void configChanged(qint32);

        // called by children
        void clicked(GcSideBarBtnId id);
        void selected(GcSideBarBtnId id);

    signals:

        void itemSelected(GcSideBarBtnId id);
        void itemClicked(GcSideBarBtnId id);

    private:

        QWidget *top, *middle, *bottom; // bars at top and the bottom
        QMap<GcSideBarBtnId,NewSideBarItem*> items;

        GcSideBarBtnId lastid; // when autoallocating
        QVBoxLayout *layout;
};

// tightly bound with parent
class NewSideBarItem : public QWidget
{
    Q_OBJECT

    public:

        NewSideBarItem(NewSideBar *sidebar, GcSideBarBtnId id, QImage icon, QString name);

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
        GcSideBarBtnId id;

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

#endif // _GC_NewSideBar_h
