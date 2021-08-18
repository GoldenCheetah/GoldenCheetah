/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_GenericLegend_h
#define _GC_GenericLegend_h 1

#include <QWidget>
#include <QPointF>
#include <QString>
#include <QDebug>
#include <QColor>
#include <QTextEdit>
#include <QScrollBar>
#include <QCheckBox>
#include <QSplitter>
#include <QByteArray>
#include <string.h>
#include <QtCharts>
#include <QGraphicsItem>
#include <QFontMetrics>
#include "Quadtree.h"

#include "GoldenCheetah.h"
#include "Settings.h"
#include "Context.h"
#include "Athlete.h"
#include "Colors.h"
#include "RCanvas.h"

class GenericPlot;
class GenericLegend;
class GenericLegendItem : public QWidget {

    Q_OBJECT

    public:
        GenericLegendItem(Context *context, GenericLegend *parent, QString name, QColor color);

    Q_SIGNALS:
        void clicked(QString name, bool enabled); // someone clicked on a legend and enabled/disabled it

    protected:
        bool eventFilter(QObject *, QEvent *e);

    public slots:

        void paintEvent(QPaintEvent *event);
        void setValue(double p) { if (enabled) { hasvalue=true;hasstring=false; value=p; update(); } } // set value to display
        void setValue(QString s) { if (enabled) { hasvalue=false;hasstring=true; string=s; update(); } } // set value to display
        void noValue() { if (enabled) { hasstring=false;hasvalue=false; update(); } } // no value to display
        void setClickable(bool x) { clickable=x; }
        void setDateTime(bool x, QString xf) { datetime=x; datetimeformat=xf; }
        void configChanged(qint32); // context changed

    private:
        Context *context;
        QString name;
        QColor color;
        GenericLegend *legend;

        bool hasvalue;
        bool hasstring;
        bool enabled;
        bool clickable;
        bool datetime;
        QString datetimeformat;
        double value;
        QString string;

        // geometry for painting fast / updated on config changes
        QRectF blockrect, namerect, valuerect, linerect, hoverrect;

};

class GenericLegend : public QWidget {

    Q_OBJECT

    public:
        GenericLegend(Context *context, GenericPlot *parent);

        void addSeries(QString name, QColor color);
        void addX(QString name, bool datetime, QString datetimeformat);
        void addLabel(QLabel *label);
        void removeSeries(QString name);
        void removeAllSeries();
        void setScale(double);

        GenericPlot *plot() { return plot_; }

    Q_SIGNALS:
        void clicked(QString name, bool enabled); // someone clicked on a legend and enabled/disabled it

    public slots:

        void setValue(GPointF value, QString name, QString xcategory="(null)");
        void unhover(QString name);
        void unhoverx();
        void setClickable(bool x);
        void setOrientation(Qt::Orientation);
        void configChanged(qint32);

    private:
        // a label has a unique name, not directly tide to
        // a series or axis value, it depends...
        Context *context;
        GenericPlot *plot_;
        QBoxLayout *layout;
        QMap<QString,GenericLegendItem*> items;
        QString xname;
        bool clickable;
        Qt::Orientation orientation;
};

#endif
