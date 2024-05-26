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

class FreeSearch;
class DataFilter;

class SearchFilterBox : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int xwidth READ xwidth WRITE setXWidth USER true)

public:
    SearchFilterBox(QWidget *parent, Context *context, bool nochooser = true, bool useToolbarBkgd = false);
    void setMode(SearchBox::SearchBoxMode x) { searchbox->setMode(x); }

    QString filter();
    void setFilter(QString); // filter:text or search:text
    bool isFiltered() const { return searchbox->isFiltered(); }

    QString text() const { return searchbox->text(); }
    void setText(QString t) { searchbox->setText(t); }

    void setContext(Context *c) { context = c; searchbox->setContext(c); }

    SearchBox *searchbox;

    void setXWidth(int x) { setFixedWidth(x); }
    int xwidth() const { return width(); }

    static QStringList matches(Context *context, QString filter); // get matches
    static bool isNull(QString filter); // is the filter null ?

private slots:

signals:
    void searchResults(QStringList);    // what was search/filtered
    void searchClear();                 // we stopped search/filtering looking

private:
    Context *context;
    FreeSearch *freeSearch;
    DataFilter *datafilter;
};

#endif
