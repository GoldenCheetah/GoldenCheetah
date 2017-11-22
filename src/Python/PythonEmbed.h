/*
 * Copyright (c) 2016 Mark Liversedge (liversedge@gmail.com)
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

#ifndef GC_PYTHONEMBED_H
#define GC_PYTHONEMBED_H

#include <QString>
#include <QStringList>

class PythonEmbed;
extern PythonEmbed *python;

// a plain C++ class, no QObject stuff
class PythonEmbed {

    public:
    
    PythonEmbed(const bool verbose=false, const bool interactive=false);
    ~PythonEmbed();

    // the program being constructed/parsed
    QStringList program;

    QString name;
    QString version;

    bool verbose;
    bool interactive;

    bool loaded;
};

#endif
