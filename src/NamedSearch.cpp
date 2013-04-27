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

#include "NamedSearch.h"
#include "SearchBox.h"
#include "MainWindow.h"
#include "GcSideBarItem.h" // for iconFromPNG

// Escape special characters (JSON compliance)
static QString protect(const QString string)
{
    QString s = string;
    s.replace("\\", "\\\\"); // backslash
    s.replace("\"", "\\\""); // quote
    s.replace("\t", "\\t");  // tab
    s.replace("\n", "\\n");  // newline
    s.replace("\r", "\\r");  // carriage-return
    s.replace("\b", "\\b");  // backspace
    s.replace("\f", "\\f");  // formfeed
    s.replace("/", "\\/");   // solidus
    s.replace(">", "&gt;");   // angle
    s.replace("<", "&lt;");   // angle
    return s;
}

// Un-Escape special characters (JSON compliance)
static QString unprotect(const QString string)
{
    QString string2 = QString::fromLocal8Bit(string.toAscii().data());

    // this is a quoted string
    QString s = string2.mid(1,string2.length()-2);

    // now un-escape the control characters
    s.replace("\\t", "\t");  // tab
    s.replace("\\n", "\n");  // newline
    s.replace("\\r", "\r");  // carriage-return
    s.replace("\\b", "\b");  // backspace
    s.replace("\\f", "\f");  // formfeed
    s.replace("\\/", "/");   // solidus
    s.replace("\\\"", "\""); // quote
    s.replace("\\\\", "\\"); // backslash
    s.replace("&gt;", ">");   // angle
    s.replace("&lt;", "<");   // angle

    return s;
}
void
NamedSearches::read()
{
    QFile namedSearchFile(home.absolutePath() + "/namedsearches.xml");
    QXmlInputSource source( &namedSearchFile );
    QXmlSimpleReader xmlReader;
    NamedSearchParser( handler );
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);
    xmlReader.parse( source );

    // go read them!
    list = handler.getResults();

    // let everyone know they have changed
    changed();
}


NamedSearch NamedSearches::get(QString name)
{
    NamedSearch returning;
    foreach (NamedSearch x, list) {
        if (x.name == name) {
            returning = x;
            break;
        }
    }
    return returning;
}

void
NamedSearches::write()
{
    // update namedSearchs.xml
    QString file = QString(home.absolutePath() + "/namedsearches.xml");
    NamedSearchParser::serialize(file, list);
}

bool NamedSearchParser::startDocument()
{
    buffer.clear();
    return TRUE;
}

bool NamedSearchParser::endElement( const QString&, const QString&, const QString &qName )
{
    if(qName == "name")
        namedSearch.name = unprotect(buffer.trimmed());
    else if (qName == "count")
        namedSearch.count = unprotect(buffer.trimmed()).toInt();
    else if (qName == "type")
        namedSearch.type = unprotect(buffer.trimmed()).toInt();
    else if (qName == "text")
        namedSearch.text = unprotect(buffer.trimmed());
    else if(qName == "NamedSearch") {

        result.append(namedSearch);
    }
    return TRUE;
}

bool NamedSearchParser::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes & )
{
    buffer.clear();
    if(name == "NamedSearch") {
        namedSearch = NamedSearch();
    }

    return TRUE;
}

bool NamedSearchParser::characters( const QString& str )
{
    buffer += str;
    return TRUE;
}

bool NamedSearchParser::endDocument()
{
    return TRUE;
}

bool
NamedSearchParser::serialize(QString filename, QList<NamedSearch>NamedSearches)
{
    // open file - truncate contents
    QFile file(filename);
    file.open(QFile::WriteOnly);
    file.resize(0);
    QTextStream out(&file);
    out.setCodec("UTF-8");

    // begin document
    out << "<NamedSearches>\n";

    // write out to file
    foreach (NamedSearch search, NamedSearches) {
        // main attributes
        out<<QString("\t<NamedSearch>\n"
              "\t\t<name>\"%1\"</name>\n"
              "\t\t<type>\"%2\"</type>\n"
              "\t\t<text>\"%3\"</text>\n") .arg(protect(search.name))
                                   .arg(search.type)
                                   .arg(protect(search.text));
        out<<"\t</NamedSearch>\n";

    }

    // end document
    out << "</NamedSearches>\n";

    // close file
    file.close();

    return true; // success
}


EditNamedSearches::EditNamedSearches(QWidget *parent, MainWindow *main) : QDialog(parent), mainWindow(main), active(false)
{
    filterIcon = iconFromPNG(":images/toolbar/filter3.png", false);
    searchIcon = iconFromPNG(":images/toolbar/search3.png", false);

    setWindowTitle(tr("Edit Favourites"));
    setWindowFlags(windowFlags() | Qt::Tool);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowModality(Qt::NonModal);
    setFixedSize(450,400);

    QVBoxLayout *layout = new QVBoxLayout(this);

    // editing the search/filter
    QHBoxLayout *row1 = new QHBoxLayout;
    layout->addLayout(row1);
    QLabel *name = new QLabel(tr("Name"), this);
    row1->addWidget(name);
    editName = new QLineEdit(this);
    row1->addWidget(editName);
    QHBoxLayout *row2 = new QHBoxLayout;
    layout->addLayout(row2);
    QLabel *filter = new QLabel(tr("Filter"), this);
    row2->addWidget(filter);
    editSearch = new SearchBox(mainWindow, this, true);
    row2->addWidget(editSearch);

    // add/update buttons
    QHBoxLayout *row3 = new QHBoxLayout;
    layout->addLayout(row3);
    row3->addStretch();
    addButton = new QPushButton(tr("Add"), this);
    row3->addWidget(addButton);
    updateButton = new QPushButton(tr("Update"), this);
    row3->addWidget(updateButton);

    // Selection List
    searchList = new QTreeWidget(this);
    layout->addWidget(searchList);
#ifdef Q_OS_MAC
    searchList->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    searchList->setSelectionMode(QAbstractItemView::SingleSelection);
    searchList->setColumnCount(3);
    searchList->setIndentation(3);
    QStringList headings;
    headings<<"";
    headings<<"Name";
    headings<<"Query";
    searchList->setHeaderLabels(headings);
    searchList->header()->setMinimumSectionSize(30);
    searchList->header()->resizeSection(0, 30);
    searchList->header()->resizeSection(1, 150);
    searchList->header()->setStretchLastSection(true);

    // delete button
    QHBoxLayout *row4 = new QHBoxLayout;
    layout->addLayout(row4);
    row4->addStretch();
    deleteButton = new QPushButton(tr("Delete"), this);
    row4->addWidget(deleteButton);

    // Populate the list of named searches
    foreach(NamedSearch x, mainWindow->namedSearches->getList()) {
        QTreeWidgetItem *add = new QTreeWidgetItem(searchList->invisibleRootItem(), 0);
        add->setIcon(0, x.type == NamedSearch::search ? searchIcon : filterIcon);
        add->setText(1, x.name);
        add->setText(2, x.text);
    }
    connect(searchList, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChanged()));

    // and select the first one
    if (mainWindow->namedSearches->getList().count()) {
        searchList->setCurrentItem(searchList->invisibleRootItem()->child(0));
    }

    // connect the buttons
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(updateButton, SIGNAL(clicked()), this, SLOT(updateClicked()));
}

void
EditNamedSearches::selectionChanged()
{
    if (active || searchList->currentItem() == NULL) return;

    int index = searchList->invisibleRootItem()->indexOfChild(searchList->currentItem());
    NamedSearch x = mainWindow->namedSearches->getList().at(index);

    editName->setText(x.name);
    editSearch->setText(x.text);
    editSearch->setMode(static_cast<SearchBox::SearchBoxMode>(x.type));
}

void
EditNamedSearches::addClicked()
{
    if (editName->text() == "" || editSearch->text() == "" || active) return;
    active = true;

    NamedSearch x;
    x.text = editSearch->text();
    x.name = editName->text();
    x.type = editSearch->getMode();
    mainWindow->namedSearches->getList().append(x);

    QTreeWidgetItem *add = new QTreeWidgetItem(searchList->invisibleRootItem(), 0);
    add->setIcon(0, x.type == NamedSearch::search ? searchIcon : filterIcon);
    add->setText(1, x.name);
    add->setText(2, x.text);

    searchList->setCurrentItem(add);
    active=false;
    selectionChanged();
}

void
EditNamedSearches::updateClicked()
{
    if (active || searchList->currentItem() == NULL) return;
    active = true;

    int index = searchList->invisibleRootItem()->indexOfChild(searchList->currentItem());

    // update the text
    mainWindow->namedSearches->getList()[index].name = editName->text();
    mainWindow->namedSearches->getList()[index].type = editSearch->getMode();
    mainWindow->namedSearches->getList()[index].text = editSearch->text();

    QTreeWidgetItem *here = searchList->invisibleRootItem()->child(index);
    here->setIcon(0, editSearch->getMode() == 0 ? searchIcon : filterIcon);
    here->setText(1, editName->text());
    here->setText(2, editSearch->text());

    active = false;
    selectionChanged(); // QT signals whilst rows are being removed, this is very confusing
}

void
EditNamedSearches::deleteClicked()
{
    if (active || searchList->currentItem() == NULL) return;
    active = true;

    int index = searchList->invisibleRootItem()->indexOfChild(searchList->currentItem());
    mainWindow->namedSearches->getList().removeAt(index);
    delete searchList->invisibleRootItem()->takeChild(index);

    active = false;
    selectionChanged(); // QT signals whilst rows are being removed, this is very confusing
}

// trap close dialog and update named searches in mainwindow/on disk
void EditNamedSearches::closeEvent(QCloseEvent*) { writeSearches(); }
void EditNamedSearches::reject() { writeSearches(); }

void 
EditNamedSearches::writeSearches()
{
    mainWindow->namedSearches->write();
}

