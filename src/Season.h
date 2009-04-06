/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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

#ifndef SEASON_H_
#define SEASON_H_

#include <QString>
#include <QDateTime>

class Season 
    {
	public:
		Season();
        QDateTime getStart();
        QDateTime getEnd();
        QString getName();
        
        void setStart(QDateTime _start);
        void setEnd(QDateTime _end);
        void setName(QString _name);
        
	private:
        QDateTime start;
        QDateTime end;
        QString name;
        
    };


#endif /* SEASON_H_ */
