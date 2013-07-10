/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_SearchFilter_h
#define _GC_SearchFilter_h

#include "Context.h"
#include "SearchBox.h" // for searchboxmode

class Lucene;
class DataFilter;

class SearchFilterBox : public QWidget
{
    Q_OBJECT

public:
    SearchFilterBox(QWidget *parent, Context *context, bool nochooser = true);
    void setMode(SearchBox::SearchBoxMode x) { searchbox->setMode(x); }

    QString filter();
    void setFilter(QString); // filter:text or search:text
    bool isFiltered() const { return searchbox->isFiltered(); }

private slots:

signals:
    void searchResults(QStringList);    // what was search/filtered
    void searchClear();                 // we stopped search/filtering looking

private:
    Context *context;
    Lucene *lucene;
    DataFilter *datafilter;
    SearchBox *searchbox;
};

#endif
