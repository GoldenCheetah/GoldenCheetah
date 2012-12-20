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

#ifndef _LibraryParser_h
#define _LibraryParser_h
#include "GoldenCheetah.h"

#include <QXmlDefaultHandler>
#include "Library.h"

class LibraryParser : public QXmlDefaultHandler
{

public:
    // save
    static bool serialize(QDir);

    // restore
    bool startDocument();
    bool endElement(const QString&, const QString&, const QString &qName);
    bool startElement(const QString&, const QString&, const QString &name, const QXmlAttributes &attrs);
    bool characters(const QString& str);

    QList<Library*> &getLibraries() { return libraries; }

protected:

    // state whilst parsing
    QString buffer;
    Library *library; // the one being currently processed

    // all libraries read
    QList<Library *> libraries;

};
#endif //LibraryParser
