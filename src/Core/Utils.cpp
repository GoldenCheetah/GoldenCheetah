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

#include "Utils.h"
#include <QTextEdit>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QDir>
#include <QRegularExpression>

namespace Utils
{

// Class for performing multiple string substitutions in a single
// pass over string. This implementation is only valid when substitutions
// will not inject additional substitution opportunities.
class StringSubstitutionizer
{
    QVector<QString> v;
    QMap<QString, QString> qm;
    QRegularExpression qr;

    QString GetSubstitute(QString s) const
    {
        if (!qm.contains(s)) return QString();
        return qm.value(s);
    }

    void BuildFindAnyRegex()
    {
        QString qRegexString;

        for (int i = 0; i < v.size(); i++)
        {
            if (i > 0) qRegexString.append("|");

            qRegexString.append(v[i]);
        }

        qr = QRegularExpression(qRegexString);
    }

protected:

    void PushSubstitution(QString regexstring, QString matchstring, QString substitute)
    {
        if (!qm.contains(matchstring))
        {
            v.push_back(regexstring);
            qm[matchstring] = substitute;
        }
    }

    void FinalizeInit()
    {
        BuildFindAnyRegex();
    }

    QRegularExpression GetFindAnyRegex() const
    {
        return qr;
    }

public:

    QString BuildSubstitutedString(QStringRef s) const
    {
        QRegularExpression qr = GetFindAnyRegex();

        QRegularExpressionMatchIterator i = qr.globalMatch(s);

        if (!i.hasNext())
            return s.toString();

        QString newstring;

        unsigned iCopyIdx = 0;
        do
        {
            QRegularExpressionMatch match = i.next();
            int copysize = match.capturedStart() - iCopyIdx;

            if (copysize > 0)
                newstring.append(s.mid(iCopyIdx, copysize));

            newstring.append(GetSubstitute(match.captured()));

            iCopyIdx = (match.capturedStart() + match.captured().size());
        } while (i.hasNext());

        int copysize = s.size() - iCopyIdx;
        if (copysize > 0)
            newstring.append(s.mid(iCopyIdx, copysize));

        return newstring;
    }
};

struct RidefileUnEscaper : public StringSubstitutionizer
{
    RidefileUnEscaper()
    {
        //                regex       match   replacement
        PushSubstitution("\\\\t",    "\\t",  "\t");       // tab
        PushSubstitution("\\\\n",    "\\n",  "\n");       // newline
        PushSubstitution("\\\\r",    "\\r",  "\r");       // carriage-return
        PushSubstitution("\\\\b",    "\\b",  "\b");       // backspace
        PushSubstitution("\\\\f",    "\\f",  "\f");       // formfeed
        PushSubstitution("\\\\/",    "\\/",  "/");        // solidus
        PushSubstitution("\\\\\"",   "\\\"", "\"");       // quote
        PushSubstitution("\\\\\\\\", "\\\\", "\\");       // backslash

        FinalizeInit();
    }
};

QString RidefileUnEscape(const QStringRef s)
{
    // Static const object constructs it's search regex at load time.
    static const RidefileUnEscaper s_RidefileUnescaper;

    return s_RidefileUnescaper.BuildSubstitutedString(s);
}

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

// BEWARE: this function is tied closely to RideFile parsing
//           DO NOT CHANGE IT UNLESS YOU KNOW WHAT YOU ARE DOING
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

// when reading/writing json lets use our own methods
// Escape special characters (JSON compliance)
QString jsonprotect(const QString &string)
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

    // add a trailing space to avoid conflicting with GC special tokens
    s += " ";

    return s;
}

QString jsonunprotect(const QString &string)
{
    QString s = string;
    s.replace("\\\"", "\""); // quote
    s.replace("\\t", "\t");  // tab
    s.replace("\\n", "\n");  // newline
    s.replace("\\r", "\r");  // carriage-return
    s.replace("\\b", "\b");  // backspace
    s.replace("\\f", "\f");  // formfeed
    s.replace("\\/", "/");   // solidus
    s.replace("\\\\", "\\"); // backslash

    // those trailing spaces.
    while (s.endsWith(" ")) s = s.mid(0,s.length()-1);
    return s;
}

QStringList
searchPath(QString path, QString binary, bool isexec)
{
    // search the path provided for a file named binary
    // if isexec is true it must be an executable file
    // which means on Windows its an exe or com
    // on Linux/Mac means it has executable bit set

    QStringList returning, extend; // extensions needed

#ifdef Q_OS_WIN
    if (isexec) {
        extend << ".exe" << ".com";
    } else {
        extend << "";
    }
#else
    extend << "";
#endif
    foreach(QString dir, path.split(PATHSEP)) {

        foreach(QString ext, extend) {
            QString filename(dir + QDir::separator() + binary + ext);

            // exists and executable (or not if we don't care) and not already found (dupe paths)
            if (QFileInfo(filename).exists() && (!isexec || QFileInfo(filename).isExecutable()) && !returning.contains(filename)) {
                returning << filename;
            }
        }
    }
    return returning;
}

QString
removeDP(QString in)
{
    QString out;
    if (in.contains('.')) {

        int n=in.indexOf('.');
        out += in.mid(0,n);
        int i=in.length()-1;
        for(; in[i] != '.'; i--)
            if (in[i] != '0')
                break;
        if (in[i]=='.') return out;
        else out += in.mid(n, i-n+1);
        return out;
    } else {
        return in;
    }
}

};

