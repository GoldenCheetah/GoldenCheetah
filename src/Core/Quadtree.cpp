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

#include "Quadtree.h"

// add a node
bool
QuadtreeNode::insert(Quadtree *root, GPointF value)
{
    if (!contains(value)) return false;

    // here will do
    if (leaf && contents.count() < maxentries) {
        if (!contents.contains(value)) contents.append(value); // de dupe
        return true;
    }

    // full, so split out
    if (leaf)  split(root);

    // find me a home below
    for(int i=0; i<4; i++)
        if (aabb[i]->insert(root, value) == true)
            return true;

    // should not get here
    return false;
}

// get candidates
int
QuadtreeNode::candidates(QRectF rect, QList<GPointF> &here)
{
    // nope
    if (!intersect(rect)) return 0;

    if (leaf) {

        // lemme see if any of mine match
        int found=0;
        for(int i=0; i<contents.count(); i++) {
            if (rect.contains(contents.at(i))) {
                here.append(contents.at(i));
                found++;
            }
        }
        return found;

    } else {

        // recurse through my children
        int count=0;
        for (int i=0; i<4; i++)
            count += aabb[i]->candidates(rect, here);
        return count;
    }
}

// split leaf into nodes (when to many entries)
void
QuadtreeNode::split(Quadtree *root)
{
    leaf=false;
    aabb[0]=root->newnode(topleft, mid);
    aabb[1]=root->newnode(QPointF(mid.x(),topleft.y()), QPointF(bottomright.x(), mid.y()));
    aabb[2]=root->newnode(QPointF(topleft.x(),mid.y()), QPointF(mid.x(), bottomright.y()));
    aabb[3]=root->newnode(mid,bottomright);

    // find them homes and clear here
    for(int i=0; i<contents.count(); i++)
        for(int j=0; j<4; j++)
            if (aabb[j]->insert(root, contents.at(i)) == true)
                break;
    contents.clear();

}

Quadtree::Quadtree(QPointF topleft, QPointF bottomright)
{
    root = new QuadtreeNode(topleft, bottomright);
}

// manage the entire child tree on a single qvector to delete quickly
void
Quadtree::reset (QPointF topleft, QPointF bottomright)
{
    for(int i=0; i<nodes.count(); i++) delete nodes.at(i);
    nodes.clear();
    delete root;
    root = new QuadtreeNode(topleft,bottomright);
}

Quadtree::~Quadtree()
{
    // zap the nodes
    for(int i=0; i<nodes.count(); i++) delete nodes.at(i);
    nodes.clear();
    delete root;
}

QuadtreeNode *
Quadtree::newnode(QPointF topleft, QPointF bottomright)
{
    QuadtreeNode *add = new QuadtreeNode(topleft, bottomright);
    nodes.append(add);
    return add;
}

bool Quadtree::insert(GPointF point)
{
    bool result= root->insert(this, point);
    if (result == false) {

        fprintf(stderr, "quadtree: insert failed (%f,%f) in [%f,%f = %f-%f]\n", point.x(), point.y(),
                                                                               root->topleft.x(), root->topleft.y(),
                                                                               root->bottomright.x(), root->bottomright.y());
        fflush(stderr);
    }
    return result;
}
