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
        NamedSearch() : type(search) {}

        // we need to check the searches are functionally the same, so strip the whitespace for the search text before comparison
        bool operator==(const NamedSearch& ns) const {
            return (name == ns.name && type == ns.type &&
                text.simplified().remove(' ') == ns.text.simplified().remove(' '));
        }

        QString name; // name, typically users name them by year e.g. "2011 Season"
        int type;
        QString text;
};

class NamedSearches : public QObject {

    Q_OBJECT;

    public:

        // Singleton pattern
        static NamedSearches& getInstance() {
            static NamedSearches instance;
            return instance;
        }
        ~NamedSearches() {}
        NamedSearches(NamedSearches const&) = delete;
        void operator=(NamedSearches const&) = delete;

        void write();

        const QList<NamedSearch> &getList() const { return list; }
        NamedSearch get(const QString& name) const;
        NamedSearch get(int index) const;
        bool deleteNamedSearch(int index);
        void appendNamedSearch(const NamedSearch& x);
        bool updateNamedSearch(int index, const NamedSearch& x);
        bool swapNamedSearch(int newIndex, int index);



    private:

        NamedSearches() { read(); }
        void read();

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
};

class EditNamedSearches : public QDialog
{
    Q_OBJECT
    G_OBJECT

    public:
        EditNamedSearches(QWidget *parent, Context *context);
        void closeEvent(QCloseEvent* event); // write away on save

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
