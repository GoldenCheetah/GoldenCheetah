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

#ifndef _GC_Quadtree_h
#define _GC_Quadtree_h 1

#include <QPointF>
#include <QRectF>
#include <QList>

class Quadtree;
class QuadtreeNode
{
    static const int maxdepth=12;
    static const int maxentries=25;

    public:

        // constructor makes an empty leaf
        QuadtreeNode(QPointF topleft, QPointF bottomright) :
              topleft(topleft), bottomright(bottomright), mid((topleft+bottomright)/2.0), leaf(true) {}

        // is the point in our space - when inserting
        bool contains(QPointF p) { return ((p.x() > topleft.x() && p.x() < bottomright.x()) &&
                                           p.y() > topleft.y() && p.y() < bottomright.y()); }

        // do we overlap with the search space - when looking
        bool intersect(QRectF r) { return r.intersects(QRectF(topleft,bottomright)); }

        // add a point
        void insert(Quadtree *root, QPointF value);

        // get candidates in same quadrant (might be miles away for big quadrant).
        int candidates(QRectF,QList<QPointF>&tohere);

    protected:

        // split leaf into nodes (when to many entries)
        void split(Quadtree *root);

    private:
        // AABB children (also in a freelist in Quadtree)
        QuadtreeNode *aabb[4];

        // the points in this quadrant
        QList<QPointF> contents;

        // geom of quadrant
        QPointF topleft, mid, bottomright;

        // if no children in aabb leaf==true
        bool leaf;
};

class Quadtree
{
    public:
        Quadtree (QPointF topleft, QPointF bottomright);
        ~Quadtree();

        // add a point
        void insert(QPointF x);

        // find points in boundg rect, of course might be long way away...
        int candidates(QRectF rect, QList<QPointF>&tohere) { return root->candidates(rect, tohere); }

        // manage the entire child tree on a single qvector to delete quickly
        QuadtreeNode *newnode(QPointF topleft, QPointF bottomright);
        void reset (QPointF topleft, QPointF bottomright);
        QVector<QuadtreeNode *> nodes;

    protected:
        QuadtreeNode *root;
};


#endif
