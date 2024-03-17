/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_TreeMapWindow_h
#define _GC_TreeMapWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QTimer>
#include "Context.h"
#include "RideMetadata.h"
#include "Season.h"
#include "LTMPopup.h"
#include "GcPane.h"
#include "SpecialFields.h"
#include "Specification.h"

#include <cmath>

#include <qwt_plot_picker.h>
#include <qwt_text_engine.h>

class TMSettings
{
    public:
        QString symbol;
        QString field1, field2;
        Specification specification;
};

class TreeMapPlot;
class TreeMapWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(QString f1 READ f1 WRITE setf1 USER true)
    Q_PROPERTY(QString f2 READ f2 WRITE setf2 USER true)
    Q_PROPERTY(QString metric READ symbol WRITE setsymbol USER true)
    Q_PROPERTY(QDate fromDate READ fromDate WRITE setFromDate USER true)
    Q_PROPERTY(QDate toDate READ toDate WRITE setToDate USER true)
    Q_PROPERTY(QDate startDate READ startDate WRITE setStartDate USER true)
    Q_PROPERTY(int lastN READ lastN WRITE setLastN USER true)
    Q_PROPERTY(int lastNX READ lastNX WRITE setLastNX USER true)
    Q_PROPERTY(int prevN READ prevN WRITE setPrevN USER true)
    Q_PROPERTY(int useSelected READ useSelected WRITE setUseSelected USER true) // !! must be last property !!

    public:

        Context *context; // used by zones shader
        TreeMapWindow(Context *); 
        ~TreeMapWindow();

        bool isFiltered() const { return context->ishomefiltered || context->isfiltered; }

        QString f1 ()
        {   // consider translation on Screen, but Store only in EN
            if (field1->currentIndex() == 0) {
               return "None"; // dont' translate
            } else {
               SpecialFields sp;
               return ( sp.internalName(field1->currentText()));
            }
        }

        QString f2()
        {   // consider translation on Screen, but Store only in EN
            if (field2->currentIndex() == 0) {
               return "None"; // dont' translate
            } else {
               SpecialFields sp;
               return ( sp.internalName(field2->currentText()));
            }
        }

        void setf1(QString x)
        {   // consider translation on Screen, but Store only in EN
            SpecialFields sp;
            if (x == "None") field1->setCurrentIndex(0); // "None" is the first item in Combo Box
            else field1->setCurrentIndex(field1->findText(sp.displayName(x)));
        }

        void setf2(QString x)
        {   // consider translation on Screen, but Store only in EN
            SpecialFields sp;
            if (x == "None") field2->setCurrentIndex(0); // // "None" is the first item in Combo Box
            else field2->setCurrentIndex(field2->findText(sp.displayName(x)));
        }

        int useSelected() { return dateSetting->mode(); }
        void setUseSelected(int x) { dateSetting->setMode(x); }

        QDate fromDate() { return dateSetting->fromDate(); }
        void setFromDate(QDate date)  { return dateSetting->setFromDate(date); }
        QDate toDate() { return dateSetting->toDate(); }
        void setToDate(QDate date)  { return dateSetting->setToDate(date); }
        QDate startDate() { return dateSetting->startDate(); }
        void setStartDate(QDate date)  { return dateSetting->setStartDate(date); }

        int lastN() { return dateSetting->lastN(); }
        void setLastN(int x) { dateSetting->setLastN(x); }
        int lastNX() { return dateSetting->lastNX(); }
        void setLastNX(int x) { dateSetting->setLastNX(x); }

        int prevN() { return dateSetting->prevN(); }
        void setPrevN(int x) { dateSetting->setPrevN(x); }

        QString symbol() const {
            // we got a selection?
            if (metricTree->selectedItems().count()) {
                return metricTree->selectedItems().first()->text(1);
            } else {
                return "";
            }
        }
        void setsymbol(QString name) {
            if (name == "") return;

            // Go find it...
            active = true;
            for (int i=0; i<allMetrics->childCount(); i++) {
                if (allMetrics->child(i)->text(1) == name) {
                    allMetrics->child(i)->setSelected(true);
                } else {
                    allMetrics->child(i)->setSelected(false);
                }
            }
            active = false;
        }

    public slots:
        void rideSelected();
        void refreshPlot();
        void dateRangeChanged(DateRange);
        void metricTreeWidgetSelectionChanged();
        void refresh();
        void fieldSelected(int);
        void cellClicked(QString, QString); // cell clicked

        void useCustomRange(DateRange);
        void useStandardRange();
        void useThruToday();

    private:
        // passed from Context *
        TMSettings settings;
        DateSettingsEdit *dateSetting;

        // popup - the GcPane to display within
        //         and the LTMPopup contents widdget
        GcPane *popup;
        LTMPopup *ltmPopup;

        // local state
        bool active;
        bool dirty;
        bool useCustom;
        bool useToToday;
        DateRange custom; // custom date range supplied
        QList<KeywordDefinition> keywordDefinitions;
        QList<FieldDefinition>   fieldDefinitions;
        QList<DefaultDefinition>   defaultDefinitions;

        // Widgets
        QVBoxLayout *mainLayout;
        TreeMapPlot *treemapPlot;
        QLabel *title;

        QComboBox *field1,
                  *field2;
        QTreeWidget *metricTree;
        QTreeWidgetItem *allMetrics;
        void addTextFields(QComboBox *);
};

#endif // _GC_TreeMapWindow_h
