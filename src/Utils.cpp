/*
 * Copyright (c) Mark Liversedge (liversedge@gmail.com)
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

#include <QTextEdit>

namespace Utils
{
// when writing xml...
QString xmlprotect(const QString &string)
{
    static QString tm = QTextEdit("&#8482;").toPlainText();
    std::string stm(tm.toStdString());

    QString s = string;
    s.replace( tm, "&#8482;" );
    s.replace( "&", "&amp;" );
    s.replace( ">", "&gt;" );
    s.replace( "<", "&lt;" );
    s.replace( "\"", "&quot;" );
    s.replace( "\'", "&apos;" );
    s.replace( "\n", "\\n" );
    s.replace( "\r", "\\r" );
    return s;
}

QString unprotect(const QString &buffer)
{
    // get local TM character code
    // process html encoding of(TM)
    static QString tm = QTextEdit("&#8482;").toPlainText();

    // remove leading/trailing whitespace
    QString s = buffer.trimmed();
    // remove enclosing quotes
    if (s.startsWith(QChar('\"')) && s.endsWith(QChar('\"')))
    {
        s.remove(0,1);
        s.chop(1);
    }

    // replace html (TM) with local TM character
    s.replace( "&#8482;", tm );

    s.replace( "\\n", "\n" );
    s.replace( "\\r", "\r" );
    // html special chars are automatically handled
    // NOTE: other special characters will not work
    // cross-platform but will work locally, so not a biggie
    // i.e. if the default charts.xml has a special character
    // in it it should be added here
    return s;
}

};

