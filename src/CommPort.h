/* 
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_CommPort_h
#define _GC_CommPort_h 1

#include <QtCore>
#include <boost/shared_ptr.hpp>

class CommPort;
typedef boost::shared_ptr<CommPort> CommPortPtr;

class CommPort
{
    public:

    typedef QVector<CommPortPtr> (*ListFunction)(QString &err);
    static bool addListFunction(ListFunction f);
    static QVector<CommPortPtr> listCommPorts(QString &err);

    virtual ~CommPort() {}
    virtual bool open(QString &err) = 0;
    virtual void close() = 0;
    virtual int read(void *buf, size_t nbyte, QString &err) = 0;
    virtual int write(void *buf, size_t nbyte, QString &err) = 0;
    virtual QString name() const = 0;

};

#endif // _GC_CommPort_h

