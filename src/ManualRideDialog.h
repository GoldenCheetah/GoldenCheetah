/* 
 * Copyright (c) 2009 Eric Murray (ericm@lne.com)
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

#ifndef _GC_ManualRideDialog_h
#define _GC_ManualRideDialog_h 1

#include <QtGui>
#include <qdatetimeedit.h>
#include "PowerTap.h"

class MainWindow;

class ManualRideDialog : public QDialog 
{
    Q_OBJECT

    public:
        ManualRideDialog(MainWindow *mainWindow, const QDir &home, 
		bool useMetric);

    private slots:
        void enterClicked();
        void cancelClicked();
	void bsEstChanged();
	void setBsEst();

    private:

	void estBSFromDistance();
	void estBSFromTime();

	bool useMetricUnits;
	float timeBS, distanceBS;
        MainWindow *mainWindow;
        QDir home;
        QPushButton *enterButton, *cancelButton;
        QLabel *label;

	QLabel *hrslbl, *minslbl, *secslbl;
	QLineEdit *hrsentry, *minsentry, *secsentry; 
	QLabel * distancelbl;
	QLineEdit *distanceentry;
	QLineEdit *HRentry, *BSentry;
	QDateTimeEdit *dateTimeEdit;
	QRadioButton *estBSbyTimeButton, *estBSbyDistButton;

        QVector<unsigned char> records;
        QString filename, filepath;
};

#endif // _GC_ManualRideDialog_h

