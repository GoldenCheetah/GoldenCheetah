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

#include "Library.h"
#include "LibraryParser.h"
#include <QDebug>
#include <QMessageBox>

bool LibraryParser::startDocument()
{
    buffer.clear();
    return true;
}

bool LibraryParser::endElement( const QString&, const QString&, const QString &qName )
{
    if(qName == "library") {
        libraries.append(library);
    }
    // another search path for this library
    if (qName == "path") {
        library->paths.append(buffer.trimmed());
    }

    // a reference
    if (qName == "ref") {
        library->refs.append(buffer.trimmed());
    }
    return true;
}

bool LibraryParser::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs)
{
    // start of a new library definition
    buffer.clear();
    if(name == "library") {
        library = new Library();
        for(int i=0; i<attrs.count(); i++) {
            if (attrs.qName(i) == "name") library->name = attrs.value(i);
        }
    }

    return true;
}

bool LibraryParser::characters(const QString& str)
{
    buffer += str;
    return true;
}

bool
LibraryParser::serialize(QDir home)
{
    // we write to root of all cyclists
    home.cdUp();

    // open file - truncate contents
    QString filename = home.canonicalPath() + "/library.xml";
    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(QObject::tr("Problem Saving Workout Library"));
        msgBox.setInformativeText(QObject::tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(filename));
        msgBox.exec();
        return false;
    };
    file.resize(0);
    QTextStream out(&file);


    // write out to file
    foreach (Library *l, ::libraries) {
        // begin document
        out << QString("<library name=\"%1\">\n").arg(l->name);

        // paths...
        foreach(QString p, l->paths)
            out << QString("\t<path>%1</path>\n").arg(p);

        // paths...
        foreach(QString r, l->refs)
            out << QString("\t<ref>%1</ref>\n").arg(r);

        // end document
        out << "</library>\n";
    }


    // close file
    file.close();

    return true; // success
}
