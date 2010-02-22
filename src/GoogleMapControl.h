/* 
 * Copyright (c) 2009 Greg Lonnon (greg.lonnon@gmail.com)
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

#ifndef _GC_GoogleMapControl_h
#define _GC_GoogleMapControl_h

#include <QWidget>
#include <QtWebKit>
#include <string>
#include <iostream>
#include <sstream>
#include <string>

class QMouseEvent;
class RideItem;
class MainWindow;
class RideFilePoint;
class QColor;
class QVBoxLayout;
class QTabWidget;

class GoogleMapControl : public QWidget
{
Q_OBJECT

 private:
	QVBoxLayout *layout;
	QWebView *view;
	RideItem *ride;
	MainWindow *parent;
	std::string CreatePolyLine(RideItem *);
	void CreateSubPolyLine(const std::vector<RideFilePoint> &points,
						   std::ostringstream &oss,
						   QColor color);
	std::string CreateIntervalMarkers(RideItem *);
	GoogleMapControl();
	QColor GetColor(int cp, int watts);
	// tabIndex tracks the index of the Maps tab, -1 means it's not showing
	int tabIndex;
	QTabWidget *tabWidget;

 public slots:
	void rideSelected();

 protected:
	void createHtml();

 public:
	GoogleMapControl(MainWindow *,QTabWidget *);
	virtual ~GoogleMapControl() { }
	void setData(RideItem *file);
};

#endif
