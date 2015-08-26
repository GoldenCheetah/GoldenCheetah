/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#include "UserData.h"

#include <QTextEdit> // for parsing trademark symbols (!)

UserData::UserData() 
    : name(""), units(""), formula(""), color(QColor(0,0,0)), rideItem(NULL)
{
}

UserData::UserData(QString settings) 
    : name(""), units(""), formula(""), color(QColor(0,0,0)), rideItem(NULL)
{
    // and apply settings
    setSettings(settings);
}

UserData::UserData(QString name, QString units, QString formula, QColor color)
    : name(name), units(units), formula(formula), color(color), rideItem(NULL)
{
}

UserData::~UserData()
{
}

//
// User dialogs to maintain settings
//
bool
UserData::edit()
{
    // settings...
    return false; //XXX
}

//
// XML settings; read, write, parse, protect
//
static QString xmlprotect(QString string)
{
    QTextEdit trademark("&#8482;"); // process html encoding of(TM)
    QString tm = trademark.toPlainText();

    QString s = string;
    s.replace( tm, "&#8482;" );
    s.replace( "&", "&amp;" );
    s.replace( ">", "&gt;" );
    s.replace( "<", "&lt;" );
    s.replace( "\"", "&quot;" );
    s.replace( "\'", "&apos;" );
    return s;
}

static QString unprotect(QString buffer)
{
    // get local TM character code
    QTextEdit trademark("&#8482;"); // process html encoding of(TM)
    QString tm = trademark.toPlainText();

    // remove quotes
    QString s = buffer.trimmed();

    // replace html (TM) with local TM character
    s.replace( "&#8482;", tm );

    // html special chars are automatically handled
    // NOTE: other special characters will not work
    // cross-platform but will work locally, so not a biggie
    // i.e. if the default charts.xml has a special character
    // in it it should be added here
    return s;
}

// output a snippet
QString 
UserData::settings() const
{
    QString returning;
    returning = "<userdata name=\"" + xmlprotect(name) + "\" units=\"" +  xmlprotect(units)+ "\"";
    returning += " color=\""+ color.name() + "\">";
    returning += xmlprotect(formula);
    returning += "</userdata>";

    // xml snippet 
    return returning;

}

// read in a snippet
void 
UserData::setSettings(QString settings)
{
    // need to parse the user data xml snipper
    // via xml parser, which is a little over the
    // top but better to keep this stuff as text
    // to avoid the nasty place we ended up in
    // with the LTMSettings structure (yuck!)

    // setup the handler
    QXmlInputSource source;
    source.setData(settings);
    QXmlSimpleReader xmlReader;
    UserDataParser handler(this);
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);

    // parse and instantiate the charts
    xmlReader.parse(source);
}

//
// view layout parser - reads in athletehome/xxx-layout.xml
//

bool UserDataParser::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs )
{
    // get the attributes; color, name, units
    for(int i=0; i<attrs.count(); i++) {

        // only 3 attributes for now
        if (attrs.qName(i) == "color") here->color = QColor(attrs.value(i));
        if (attrs.qName(i) == "name")  here->name  = unprotect(attrs.value(i));
        if (attrs.qName(i) == "units") here->units = unprotect(attrs.value(i));
    }
    return true;
}

bool UserDataParser::endElement( const QString&, const QString&, const QString &) { return true; }
bool UserDataParser::characters( const QString&chrs) { here->formula += chrs; return true; }
bool UserDataParser::startDocument() { here->formula.clear(); return true; }
bool UserDataParser::endDocument() { here->formula = unprotect(here->formula); return true; }

// set / get the current ride item
RideItem* 
UserData::getRideItem() const
{
    return rideItem;
}

void 
UserData::setRideItem(RideItem*m)
{
    rideItem = m;
}

