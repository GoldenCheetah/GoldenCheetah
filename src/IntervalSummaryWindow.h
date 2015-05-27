/*
 * Copyright (c) 2011 Eric Brandt (eric.l.brandt@gmail.com)
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

#ifndef INTERVALSUMMARYWINDOW_H_
#define INTERVALSUMMARYWINDOW_H_

#include <QtGui>
#include <QTextEdit>

#include "RideFile.h"

class Context;
class IntervalItem;

class IntervalSummaryWindow : public QTextEdit {
	Q_OBJECT;

public:
	IntervalSummaryWindow(Context *context);
	virtual ~IntervalSummaryWindow();

public slots:

    void intervalSelected();
    void intervalHover(IntervalItem*);

protected:
    QString summary(QList<IntervalItem*>, QString&);
    QString summary(IntervalItem *);

    Context *context;
};

#endif /* INTERVALSUMMARYWINDOW_H_ */
