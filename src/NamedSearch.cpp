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

#include "NamedSearch.h"

// Escape special characters (JSON compliance)
static QString protect(const QString string)
{
    QString s = string;
    s.replace("\\", "\\\\"); // backslash
    s.replace("\"", "\\\""); // quote
    s.replace("\t", "\\t");  // tab
    s.replace("\n", "\\n");  // newline
    s.replace("\r", "\\r");  // carriage-return
    s.replace("\b", "\\b");  // backspace
    s.replace("\f", "\\f");  // formfeed
    s.replace("/", "\\/");   // solidus

    return s;
}

// Un-Escape special characters (JSON compliance)
static QString unprotect(const QString string)
{
    QString string2 = QString::fromLocal8Bit(string.toAscii().data());

    // this is a quoted string
    QString s = string2.mid(1,string2.length()-2);

    // now un-escape the control characters
    s.replace("\\t", "\t");  // tab
    s.replace("\\n", "\n");  // newline
    s.replace("\\r", "\r");  // carriage-return
    s.replace("\\b", "\b");  // backspace
    s.replace("\\f", "\f");  // formfeed
    s.replace("\\/", "/");   // solidus
    s.replace("\\\"", "\""); // quote
    s.replace("\\\\", "\\"); // backslash

    return s;
}
void
NamedSearches::read()
{
    QFile namedSearchFile(home.absolutePath() + "/namedsearches.xml");
    QXmlInputSource source( &namedSearchFile );
    QXmlSimpleReader xmlReader;
    NamedSearchParser( handler );
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);
    xmlReader.parse( source );

    // go read them!
    list = handler.getResults();

    // let everyone know they have changed
    changed();
}


NamedSearch NamedSearches::get(QString name)
{
    NamedSearch returning;
    foreach (NamedSearch x, list) {
        if (x.name == name) {
            returning = x;
            break;
        }
    }
    return returning;
}

void
NamedSearches::write()
{
    // update namedSearchs.xml
    QString file = QString(home.absolutePath() + "/namedsearches.xml");
    NamedSearchParser::serialize(file, list);
}

bool NamedSearchParser::startDocument()
{
    buffer.clear();
    return TRUE;
}

bool NamedSearchParser::endElement( const QString&, const QString&, const QString &qName )
{
    if(qName == "name")
        namedSearch.name = unprotect(buffer.trimmed());
    else if (qName == "count")
        namedSearch.count = unprotect(buffer.trimmed()).toInt();
    else if (qName == "type")
        namedSearch.type = unprotect(buffer.trimmed()).toInt();
    else if (qName == "text")
        namedSearch.text = unprotect(buffer.trimmed());
    else if(qName == "NamedSearch") {

        result.append(namedSearch);
    }
    return TRUE;
}

bool NamedSearchParser::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes & )
{
    buffer.clear();
    if(name == "NamedSearch") {
        namedSearch = NamedSearch();
    }

    return TRUE;
}

bool NamedSearchParser::characters( const QString& str )
{
    buffer += str;
    return TRUE;
}

bool NamedSearchParser::endDocument()
{
    return TRUE;
}

bool
NamedSearchParser::serialize(QString filename, QList<NamedSearch>NamedSearches)
{
    // open file - truncate contents
    QFile file(filename);
    file.open(QFile::WriteOnly);
    file.resize(0);
    QTextStream out(&file);
    out.setCodec("UTF-8");

    // begin document
    out << "<NamedSearches>\n";

    // write out to file
    foreach (NamedSearch search, NamedSearches) {
        // main attributes
        out<<QString("\t<NamedSearch>\n"
              "\t\t<name>\"%1\"</name>\n"
              "\t\t<type>\"%2\"</type>\n"
              "\t\t<text>\"%3\"</text>\n") .arg(protect(search.name))
                                   .arg(search.type)
                                   .arg(protect(search.text));
        out<<"\t</NamedSearch>\n";

    }

    // end document
    out << "</NamedSearches>\n";

    // close file
    file.close();

    return true; // success
}
