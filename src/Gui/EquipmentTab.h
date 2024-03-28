/*
 * Copyright (c) 2024 Paul Johnson (paulj49457@gmail.com)
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

#ifndef _GC_Eq_Tab_h
#define _GC_Eq_Tab_h 1

#include "AbstractView.h"
#include "Views.h"

class EquipmentNavigator;
class MainWindow;

class EquipmentTab: public QWidget
{
    Q_OBJECT

    public:

        EquipmentTab(Context *context);
        ~EquipmentTab();

        void close();
        
        bool init; // am I ready?
        EquipView* equipView_;

    protected:

        friend class ::MainWindow;
        Context *context;

    signals:

    public slots:

        // MainWindow integration with Tab / TabView (mostly pass through)

        // sidebar
        void setSidebarEnabled(bool x) { equipView_->setSidebarEnabled(x); }
        bool isSidebarEnabled() { return equipView_->sidebarEnabled(); }
        void toggleSidebar() { equipView_->setSidebarEnabled(!equipView_->sidebarEnabled()); }

        // layout
        void resetLayout(QComboBox* perspectiveSelector) { equipView_->resetLayout(perspectiveSelector); }
        void addChart(GcWinID i) { equipView_->addChart(i); }

    private:

        // Each of the views
        QStackedWidget *view_;
        QStackedWidget *eqControls_;

};

#endif // _GC_TabView_h
