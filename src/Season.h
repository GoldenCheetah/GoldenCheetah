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
#include "GoldenCheetah.h"

#include <QString>
#include <QDate>
#include <QFile>

#include "MainWindow.h"

class Season
{
	public:
        enum SeasonType { season=0, cycle=1, adhoc=2, temporary=3 };
        //typedef enum seasontype SeasonType;

		Season();
        QDate getStart();
        QDate getEnd();
        QString getName();
        int getType();


        void setStart(QDate _start);
        void setEnd(QDate _end);
        void setName(QString _name);
        void setType(int _type);

	private:
        QDate start;
        QDate end;
        QString name;
        int type;
};

class EditSeasonDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        EditSeasonDialog(MainWindow *, Season *);

    public slots:
        void applyClicked();
        void cancelClicked();

    private:
        MainWindow *mainWindow;
        Season *season;

        QPushButton *applyButton, *cancelButton;
        QLineEdit *nameEdit;
        QComboBox *typeEdit;
        QDateEdit *fromEdit, *toEdit;
};
#endif /* SEASON_H_ */
