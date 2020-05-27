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

#ifndef _GC_Utils_h
#define _GC_Utils_h

// Common shared utility functions
#include <QVector>

#ifdef WIN32
#define PATHSEP ";"
#else
#define PATHSEP ":"
#endif
class QString;
class QStringList;

#define GC_SMOOTH_FORWARD 0
#define GC_SMOOTH_BACKWARD 1
#define GC_SMOOTH_CENTERED 2

namespace Utils
{
    QString xmlprotect(const QString &string);
    QString unprotect(const QString &buffer);
    QString jsonprotect(const QString &buffer);
    QString jsonunprotect(const QString &buffer);
    QStringList searchPath(QString path, QString binary, bool isexec=true);
    QString removeDP(QString);
    QVector<int> rank(QVector<double>&, bool ascending=false);
    QVector<int> argsort(QVector<double>&, bool ascending=false);
    QVector<int> arguniq(QVector<double> &v);
    QVector<double> smooth_sma(QVector<double>&, int pos, int window);
    QVector<double> smooth_ewma(QVector<double>&, double alpha);
};


#endif
