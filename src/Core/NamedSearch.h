/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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

#ifndef Gc_NamedSearch_h
#define Gc_NamedSearch_h
#include "GoldenCheetah.h"
#include "Athlete.h"

#include <QXmlDefaultHandler>
#include <QString>
#include <QHeaderView>
#include <QDir>
#include <QDialog>
#include <QLabel>


class SearchBox;
class NamedSearch
{
	public:
        enum Type { search=0, filter=1 };
        NamedSearch() : type(search), count(0) {}

        QString name; // name, typically users name them by year e.g. "2011 Season"
        int type;
        int count;   // how many times has it been used (not counting in charts) ?
        QString text;
};

class NamedSearches : public QObject {

    Q_OBJECT;

    public:
        NamedSearches(Athlete *athlete) : athlete(athlete) { 
            home = athlete->home->config();
            read();
        }
        void read();
        void write();

        QList<NamedSearch> &getList() { return list; }
        NamedSearch get(QString name);
        NamedSearch get(int index);
        void deleteNamedSearch(int index);

    signals:
        void changed();


    private:
        Athlete *athlete;
        QDir home;
        QList<NamedSearch> list;
};

class NamedSearchParser : public QXmlDefaultHandler
{

public:
    // marshall
    static bool serialize(QString, QList<NamedSearch>);

    // unmarshall
    bool startDocument();
    bool endDocument();
    bool endElement( const QString&, const QString&, const QString &qName );
    bool startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs );
    bool characters( const QString& str );
    QList<NamedSearch> &getResults() { return result; };

protected:
    QString buffer;
    NamedSearch namedSearch;
    QList<NamedSearch> result;
    int loadcount;

};

class EditNamedSearches : public QDialog
{
    Q_OBJECT
    G_OBJECT

    public:
        EditNamedSearches(QWidget *parent, Context *context);
        void closeEvent(QCloseEvent* event); // write away on save
        void writeSearches();

    public slots:
        void reject(); // write away on close

    private:
        Context *context;
        bool active;
        QLineEdit *editName;
        SearchBox *editSearch;
        QTreeWidget *searchList;
        QPushButton *addButton,
                    *updateButton,
                    *upButton,
                    *downButton,
                    *deleteButton,
                    *closeButton;
        QIcon searchIcon, filterIcon;

    private slots:
        void addClicked();
        void updateClicked();
        void upClicked();
        void downClicked();
        void deleteClicked();
        void selectionChanged();
};


#endif
