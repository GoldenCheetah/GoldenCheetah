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

#ifndef _GC_TreeMapPlot_h
#define _GC_TreeMapPlot_h 1
#include "GoldenCheetah.h"

#include <QtGui>

#include "TreeMapWindow.h"
#include "Context.h"


// for sorting
class TreeMap;
bool TreeMapLessThan(const TreeMap *, const TreeMap *);

class TreeMap
{
    public:
        TreeMap(TreeMap *parent = NULL, QString name = "", double value = 0.0) :
            parent(parent), name(name), value(value) {}

        // insert into children, if not there then add
        TreeMap *insert(QString name, double value = 0.0) {

            // accumulate and update my parent too
            this->value += value;
            for (TreeMap *p = parent; p != NULL; p = p->parent) p->value += value;

            foreach (TreeMap *x, children) {
                if (x->name == name) {
                    x->value += value;
                    return x;
                }
            }

            TreeMap *newone = new TreeMap(this, name, value);
            children.append(newone);
            return newone;
        }

        // find the treemap that under cursor
        TreeMap *findAt(QPoint pos) {

            // find the child that is under pos!
            foreach (TreeMap *child, children)
                if (child->rect.contains(pos)) return child;
            return NULL;
        }


        // wipe out value and all children
        void clear() {
            foreach (TreeMap *x, children) {
                x->clear();
                delete x;
            }
            children.clear();
            name = "tr((unknown))";
            value = 0.00;
        }

        void sort() {
            // sort the children in descending order
            std::sort(children.begin(), children.end(), TreeMapLessThan);
            foreach (TreeMap *child, children) child->sort();
        }

        // The main user entry point - call this on the root
        // node and it will layout all the children in the
        // rectangle supplied. The children's rectangles can
        // then be passed directly to painter.drawRect etc
        void layout(QRect rect) {

            // I'll take that
            this->rect = rect;

            // need to sort in descending order
            sort();

            // Use the squarified algorithm outlined
            // by Mark Bruls, Kees Huizing, and Jarke J. van Wijk
            // in "http://citeseerx.ist.psu.edu/viewdoc/
            // download?doi=10.1.1.36.6685&rep=rep1&type=pdf"
            // ... will recurse
            squarifyLayout(children, rect);
        }

        // we use the well-known squarify layout
        // to maintain aspect ratios as near as possible
        // to a square. It in turn uses the original
        // slice layout to split rows or columns
        // we do ourselves and then our children
        // and do it recursively, despite the fact
        // that the Treemap is currently on 2 deep
        // that may change in the future
        void squarifyLayout(QList<TreeMap*> items, QRect bounds) {
            rect = bounds;
            layout(items, 0, items.count()-1, bounds);
            foreach (TreeMap *item, items)
                item->squarifyLayout(item->children, item->rect);
        }

        // this iterates over sections of the list
        // and calls itself to process mid/left sections
        void layout(QList<TreeMap*> items, int start, int end, QRect bounds) {

            if (start > end) return;

            if (end-start < 2) {
                slicelayout(items, start, end, bounds);
                return;
            }

            // setup, using smaller vars for more
            // concise / readable code (?)
            double x=bounds.x(),
                   y=bounds.y(),
                   w=bounds.width(),
                   h=bounds.height();

            double total=0;
            for(int i=start; i<=end; i++) total += items[i]->value;
            int mid=start;
            double a=items[start]->value/total;
            double b=a;

            if (w<h) {

                // height / width
                while (mid<=end) {

                    double aspect=normAspect(h,w,a,b);
                    double q=items[mid]->value/total;

                    if (normAspect(h,w,a,b+q)>aspect) break;

                    mid++;
                    b+=q;
                }

                slicelayout(items,start,mid, QRect(x,y,w,h*b));
                layout(items,mid+1,end, QRect(x,y+h*b,w,h*(1-b)));

            } else {

                // width/height
                while (mid<=end) {

                    double aspect=normAspect(w,h,a,b);
                    double q=items[mid]->value/total;

                    if (normAspect(w,h,a,b+q)>aspect) break;

                    mid++;
                    b+=q;
                }

                slicelayout(items,start,mid, QRect(x,y,w*b,h));
                layout(items,mid+1,end, QRect(x+w*b,y,w*(1-b),h));
            }
        }

        double aspect(double _big, double _small, double a, double b) {
            return (_big*b)/(_small*a/b);
        }


        double normAspect(double _big, double _small, double a, double b) {
            double x=aspect(_big,_small,a,b);
            if (x<1) return 1/x;
            return x;
        }

        // slice the items into strips either horizontally
        // or vertically along whichever has the longest side
        void slicelayout(QList<TreeMap*> items, int start, int end, QRect bounds) {

            // setup
            double total=0, accumulator=0; // total value of items and running total
            for(int i= start; i<= end && i<items.count(); i++) total += items[i]->value;
            Qt::Orientation orientation = (bounds.width() > bounds.height()) ? Qt::Horizontal : Qt::Vertical;

            // slice em up!
            for (int i=start; i<=end && i<items.count(); i++) {

                double factor=items[i]->value/total;
                if (orientation == Qt::Vertical) {
                    // slice em into a vertical stack
                    items[i]->rect.setX(bounds.x());
                    items[i]->rect.setWidth(bounds.width());
                    items[i]->rect.setY(bounds.y()+bounds.height()*(1-accumulator-factor));
                    items[i]->rect.setHeight(bounds.height()*factor);

                } else {

                    // slice em into a horizontal stack
                    items[i]->rect.setX(bounds.x()+bounds.width()*(1-accumulator-factor));
                    items[i]->rect.setWidth(bounds.width()*factor);
                    items[i]->rect.setY(bounds.y());
                    items[i]->rect.setHeight(bounds.height());
                }
                accumulator += factor;
            }
        }


        // data
        TreeMap *parent;
        QString name;
        double value;
        QList<TreeMap*> children;

        // geometry
        QRect rect;
};


class TreeMapPlot : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        TreeMapPlot(TreeMapWindow *, Context *context);
        ~TreeMapPlot();
        void setData(TMSettings *);

    public slots:
        void configChanged(qint32);
        bool eventFilter(QObject *object, QEvent *e);

    signals:
        void clicked(QString, QString);

    protected:

        TreeMapWindow *parent;
        virtual void paintEvent(QPaintEvent *);
        virtual void resizeEvent(QResizeEvent *);

    private:
        Context *context;
        TMSettings *settings;

        TreeMap *root;      // the tree map data structure
        TreeMap *highlight; // currently needs to be highlighted
};


#endif // _GC_TreeMapPlot_h

