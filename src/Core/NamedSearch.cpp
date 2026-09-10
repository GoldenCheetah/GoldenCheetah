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
#include "Context.h"
#include "Athlete.h"
#include "GcSideBarItem.h" // for iconFromPNG
#include "Colors.h" // for iconFromPNG

#include <QMessageBox>

// Escape special characters (JSON compliance & XML)
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
    s.replace("&", "&amp;");   // ampersand
    s.replace("'", "&apos;");   // apostrophe
    s.replace('"', "&quot;");   // quote


    return s;
}


// Un-Escape special characters (JSON compliance)
static QString unprotect(const QString string)
{
    // this is a quoted string
    QString s = string.mid(1,string.length()-2);

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
    s.replace("&amp;", "&");  // ampersand
    s.replace("&apos;", "'"); // apostrophe
    s.replace("&quot;", "\""); // quote


    return s;
}

void
NamedSearches::read()
{
    QFile globalNamedSearchFile(QDir(gcroot).canonicalPath()+"/namedsearches.xml");

    // Read the global namedsearches.xml file if it exists
    if (globalNamedSearchFile.exists()) {

        QXmlInputSource source( &globalNamedSearchFile );
        QXmlSimpleReader xmlReader;
        NamedSearchParser handler;
        xmlReader.setContentHandler(&handler);
        xmlReader.setErrorHandler(&handler);
        xmlReader.parse(source);

        // go read them!
        list = handler.getResults();

    } else {

        // Read the individual athlete namedsearches.xml files, this should only ever
        // occur once to create the combined single namedsearches.xml file.
        QStringListIterator i(QDir(gcroot).entryList(QDir::Dirs | QDir::NoDotAndDotDot));
        while (i.hasNext()) {
            QString name = i.next();
            SKIP_QTWE_CACHE  // skip Folder Names created by QTWebEngine on Windows

            // ignore dot folders
            if (name.startsWith(".")) continue;

            QFile namedSearchFile(QDir(gcroot).canonicalPath()+"/" + name + "/config/namedsearches.xml");
            if (namedSearchFile.exists()) {

                QXmlInputSource source( &namedSearchFile );
                QXmlSimpleReader xmlReader;
                NamedSearchParser handler;
                xmlReader.setContentHandler(&handler);
                xmlReader.setErrorHandler(&handler);
                xmlReader.parse(source);

                // skip any duplicates
                for (const NamedSearch& ns : handler.getResults()) {
                    if (list.indexOf(ns) == -1) list.append(ns);
                }
            }
        }
    }

    // If there are no filters yet, add some for multisport use.
    if (list.isEmpty()) {
        NamedSearch namedSearch;
        namedSearch.type = NamedSearch::filter;
        namedSearch.name = tr("Planned");
        namedSearch.text = "isPlanned";
        list.append(namedSearch);
        namedSearch.name = tr("Actual");
        namedSearch.text = "!isPlanned";
        list.append(namedSearch);
        namedSearch.name = tr("Swim");
        namedSearch.text = "isSwim";
        list.append(namedSearch);
        namedSearch.name = tr("Bike");
        namedSearch.text = "isRide";
        list.append(namedSearch);
        namedSearch.name = tr("Run");
        namedSearch.text = "isRun";
        list.append(namedSearch);
    }

    // write the filters to the global namedsearches.xml file
    write();
}

NamedSearch NamedSearches::get(const QString& name) const
{
    foreach (NamedSearch x, list) {
        if (x.name == name) {
            return x;
        }
    }
    return NamedSearch();
}

NamedSearch NamedSearches::get(int index) const
{
    if ((index >= 0) && (index < list.size())) {
        return list[index];
    }
    return NamedSearch();
}

void
NamedSearches::write()
{
    // update namedsearches.xml
    QString file = QString(QDir(gcroot).canonicalPath()+"/namedsearches.xml");
    NamedSearchParser::serialize(file, list);

    // let everyone know the searches have changed
    GlobalContext::context()->notifyNamedSearchesChanged();
}

bool
NamedSearches::deleteNamedSearch(int index)
{
    if ((index >= 0) && (index < list.size())) {
        list.removeAt(index);
        write();
        return true;
    }
    return false;
}

void
NamedSearches::appendNamedSearch(const NamedSearch& x)
{
    list.append(x);
    write();
}

bool
NamedSearches::updateNamedSearch(int index, const NamedSearch& x)
{
    if ((index >= 0) && (index < list.size())) {
        list[index] = x;
        write();
        return true;
    }
    return false;
}

bool
NamedSearches::swapNamedSearch(int newIndex, int index)
{
    if ((newIndex != index) && (newIndex >= 0) && (newIndex < list.size()) && (index >= 0) && (index < list.size())) {
        list.swapItemsAt(newIndex, index);

        // defer writing to the file, to prevent numerous writes as a search is moved, as
        // closing the edit dialog, or appending, deleting or updating a search will cause a write.

        // let everyone know the searches have changed
        GlobalContext::context()->notifyNamedSearchesChanged();

        return true;
    }
    return false;
}

bool NamedSearchParser::startDocument()
{
    buffer.clear();
    return true;
}

bool NamedSearchParser::endElement( const QString&, const QString&, const QString &qName )
{
    if(qName == "name")
        namedSearch.name = unprotect(buffer.trimmed());
    else if (qName == "type")
        namedSearch.type = unprotect(buffer.trimmed()).toInt();
    else if (qName == "text")
        namedSearch.text = unprotect(buffer.trimmed());
    else if(qName == "NamedSearch") {

        result.append(namedSearch);
    }
    return true;
}

bool NamedSearchParser::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes & )
{
    buffer.clear();
    if(name == "NamedSearch") {
        namedSearch = NamedSearch();
    }

    return true;
}

bool NamedSearchParser::characters( const QString& str )
{
    buffer += str;
    return true;
}

bool NamedSearchParser::endDocument()
{
    return true;
}

bool
NamedSearchParser::serialize(QString filename, QList<NamedSearch>NamedSearches)
{
    // open file - truncate contents
    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(QObject::tr("Problem Saving Named Search Configuration"));
        msgBox.setInformativeText(QObject::tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(filename));
        msgBox.exec();
        return false;
    };
    file.resize(0);
    QTextStream out(&file);

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


EditNamedSearches::EditNamedSearches(QWidget *parent, Context *context) : QDialog(parent), context(context), active(false)
{
    filterIcon = iconFromPNG(":images/toolbar/filter3.png", false);
    searchIcon = iconFromPNG(":images/toolbar/search3.png", false);

    setWindowTitle(tr("Manage Filters"));
    setWindowFlags(windowFlags() | Qt::Tool);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowModality(Qt::NonModal);
#ifdef Q_OS_MAC
    setMinimumSize(350*dpiXFactor,400*dpiYFactor);
#else
    setMinimumSize(450*dpiXFactor,400*dpiYFactor);
#endif

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
    editSearch = new SearchBox(context, this, true);
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
    headings<<tr("Name");
    headings<<tr("Query");
    searchList->setHeaderLabels(headings);
    searchList->header()->setMinimumSectionSize(30*dpiXFactor);
    searchList->header()->resizeSection(0, 30*dpiXFactor);
#ifdef Q_OS_MAC
    searchList->header()->resizeSection(1, 120);
#else
    searchList->header()->resizeSection(1, 150*dpiXFactor);
#endif
    searchList->header()->setStretchLastSection(true);

    // up/down/delete button
    QHBoxLayout *row4 = new QHBoxLayout;
    layout->addLayout(row4);
    upButton = new QPushButton(tr("Up"), this);
    row4->addWidget(upButton);
    downButton = new QPushButton(tr("Down"), this);
    row4->addWidget(downButton);
    row4->addStretch();
    deleteButton = new QPushButton(tr("Delete"), this);
    row4->addWidget(deleteButton);
    row4->addStretch();
    closeButton = new QPushButton(tr("Close"), this);
    row4->addWidget(closeButton);

    // Populate the list of named searches
    foreach(NamedSearch x, NamedSearches::getInstance().getList()) {
        QTreeWidgetItem *add = new QTreeWidgetItem(searchList->invisibleRootItem(), 0);
        add->setIcon(0, x.type == NamedSearch::search ? searchIcon : filterIcon);
        add->setText(1, x.name);
        add->setText(2, x.text);
    }
    connect(searchList, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChanged()));

    // and select the first one
    if (NamedSearches::getInstance().getList().count()) {
        searchList->setCurrentItem(searchList->invisibleRootItem()->child(0));
    }

    // connect the buttons
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(updateButton, SIGNAL(clicked()), this, SLOT(updateClicked()));
    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
}

void
EditNamedSearches::selectionChanged()
{
    if (active || searchList->currentItem() == NULL) return;

    int index = searchList->invisibleRootItem()->indexOfChild(searchList->currentItem());
    NamedSearch x = NamedSearches::getInstance().get(index);

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
    NamedSearches::getInstance().appendNamedSearch(x);

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

    NamedSearch x;
    x.text = editSearch->text();
    x.name = editName->text();
    x.type = editSearch->getMode();
    NamedSearches::getInstance().updateNamedSearch(index, x);

    QTreeWidgetItem *here = searchList->invisibleRootItem()->child(index);
    here->setIcon(0, editSearch->getMode() == 0 ? searchIcon : filterIcon);
    here->setText(1, editName->text());
    here->setText(2, editSearch->text());

    active = false;
    selectionChanged(); // QT signals whilst rows are being removed, this is very confusing
}

void
EditNamedSearches::upClicked()
{
    if (active || searchList->currentItem() == NULL) return;
    active = true;

    int index = searchList->invisibleRootItem()->indexOfChild(searchList->currentItem());
    int newIndex = index - 1;

    if (index > 0) {
        NamedSearches::getInstance().swapNamedSearch(newIndex, index);
        QTreeWidgetItem* child = searchList->invisibleRootItem()->takeChild(index);
        searchList->invisibleRootItem()->insertChild(newIndex, child);
        searchList->setCurrentItem(child);
    }

    active = false;
    selectionChanged(); // QT signals whilst rows are being removed, this is very confusing
}

void
EditNamedSearches::downClicked()
{
    if (active || searchList->currentItem() == NULL) return;
    active = true;

    int index = searchList->invisibleRootItem()->indexOfChild(searchList->currentItem());
    int newIndex = index + 1;

    if (index < (NamedSearches::getInstance().getList().size() - 1)) {
        NamedSearches::getInstance().swapNamedSearch(newIndex, index);
        QTreeWidgetItem* child = searchList->invisibleRootItem()->takeChild(index);
        searchList->invisibleRootItem()->insertChild(newIndex, child);
        searchList->setCurrentItem(child);
    }

    active = false;
    selectionChanged(); // QT signals whilst rows are being removed, this is very confusing
}

void
EditNamedSearches::deleteClicked()
{
    if (active || searchList->currentItem() == NULL) return;
    active = true;

    int index = searchList->invisibleRootItem()->indexOfChild(searchList->currentItem());
    NamedSearches::getInstance().deleteNamedSearch(index);
    delete searchList->invisibleRootItem()->takeChild(index);

    active = false;
    selectionChanged(); // QT signals whilst rows are being removed, this is very confusing
}

// trap close dialog and update named searches in mainwindow/on disk
void EditNamedSearches::closeEvent(QCloseEvent*) { NamedSearches::getInstance().write(); }
void EditNamedSearches::reject() { NamedSearches::getInstance().write(); }


