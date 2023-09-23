/*
 * Copyright (c) 2023, Joachim Kohlhammer <joachim.kohlhammer@gmx.de>
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

#ifndef TAGGABLE_H
#define TAGGABLE_H

#include <QList>


class Taggable
{
public:
    virtual ~Taggable() {}
    virtual bool hasTag(int id) const = 0;
    virtual void addTag(int id) = 0;
    virtual void removeTag(int id) = 0;
    virtual void clearTags() = 0;
    virtual QList<int> getTagIds() const = 0;
};

#endif
