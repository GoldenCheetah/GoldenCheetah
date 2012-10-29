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

#include "SearchBox.h"
#include <QToolButton>

#include <QStyle>
#include <QDebug>

SearchBox::SearchBox(QWidget *parent)
    : QLineEdit(parent)
{

    //clear button
    clearButton = new QToolButton(this);
    QPixmap pixmap(":images/toolbar/clear.png");
    clearButton->setIcon(QIcon(pixmap));
    clearButton->setIconSize(pixmap.size());
    clearButton->setCursor(Qt::ArrowCursor);
    clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    clearButton->hide();
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clear()));
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clearClicked()));
    connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateCloseButton(const QString&)));

    // search button
    searchButton = new QToolButton(this);
    QPixmap search(":images/toolbar/search.png");
    searchButton->setIcon(QIcon(search));
    searchButton->setIconSize(search.size());
    searchButton->setCursor(Qt::ArrowCursor);
    searchButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    connect(searchButton, SIGNAL(clicked()), this, SLOT(toggleMode()));

    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    setStyleSheet(QString( //"QLineEdit { padding-right: %1px; } "
                          "QLineEdit {"
                          "    selection-color: white;   "
                          //"    border: 0px groove gray;"
                          "    border-radius: 5px;"
                          "    padding: 0px %1px;"
                          "}"
                          "QLineEdit:focus {"
                          "    selection-color: white;   "
                          //"    border: 0px groove gray;"
                          "    border-radius: 5px;"
                          "    padding: 0px %1px;"
                          "}"
                          ""
                          "QLineEdit:edit-focus {"
                          "    selection-color: white;   "
                          //"    border: 0px groove gray;"
                          "    border-radius: 5px;"
                          "    padding: 0px %1px;"
                          "}"
                 ).arg(clearButton->sizeHint().width() + frameWidth + 1));

    QSize msz = minimumSizeHint();
    setMinimumSize(qMax(msz.width(), clearButton->sizeHint().height() + frameWidth * 2 + 2),
                   qMax(msz.height(), clearButton->sizeHint().height() /* + frameWidth * 2 + -2*/));

    setPlaceholderText("Search...");
    mode = Search;
    setDragEnabled(true);
    connect(this, SIGNAL(returnPressed()), this, SLOT(searchSubmit()));
}

void SearchBox::resizeEvent(QResizeEvent *)
{
    QSize sz = clearButton->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    clearButton->move(rect().right() - frameWidth - sz.width(),
                      (rect().bottom() + 1 - sz.height())/2);
    searchButton->move(rect().left() + frameWidth,
                      (rect().bottom() + 1 - sz.height())/2);
}

void SearchBox::toggleMode()
{
    clear(); // clear whatever is there first
    if (mode == Search) setMode(Filter);
    else setMode(Search);
}

void SearchBox::setMode(SearchBoxMode mode)
{
    switch (mode) {

        case Filter:
        {
            QPixmap filter(":images/toolbar/filter.png");
            searchButton->setIcon(QIcon(filter));
            searchButton->setIconSize(filter.size());
            setPlaceholderText("Filter...");
        }
        break;

        case Search:
        default:
        {
            QPixmap search(":images/toolbar/search.png");
            searchButton->setIcon(QIcon(search));
            searchButton->setIconSize(search.size());
            setPlaceholderText("Search...");
        }
        break;
    }
    this->mode = mode;
}

void SearchBox::updateCloseButton(const QString& text)
{
    if (clearButton->isVisible() && text.isEmpty()) mode == Search ? clearQuery() : clearFilter();
    clearButton->setVisible(!text.isEmpty());

    if (mode == Search) searchSubmit(); // only do search as you type in search mode

    setGood(); // if user changing then don't stay red - wait till resubmitted
}

void SearchBox::searchSubmit()
{
    // return hit / key pressed
    if (text() != "") {
        mode == Search ? submitQuery(text()) : submitFilter(text());
    }
}

void SearchBox::clearClicked()
{
    mode == Search ? clearQuery() : clearFilter();
    setGood();
}

void SearchBox::setBad(QStringList errors)
{
    QPalette pal;
    pal.setColor(QPalette::Text, Qt::red);
    setPalette(pal);

    setToolTip(errors.join(" and "));
}

void SearchBox::setGood()
{
    QPalette pal;
    pal.setColor(QPalette::Text, Qt::black);
    setPalette(pal);

    setToolTip("");
}



// Drag and drop columns from the chooser...
void
SearchBox::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->data("application/x-columnchooser") != "")
        event->acceptProposedAction(); // whatever you wanna drop we will try and process!
    else
        event->ignore();
}

void
SearchBox::dropEvent(QDropEvent *event)
{
    QString name = event->mimeData()->data("application/x-columnchooser");
    // fugly, but it works for BikeScore with the (TM) in it...
    if (name == "BikeScore?") name = QString("BikeScore&#8482;").replace("&#8482;", QChar(0x2122));

    // we do very little to the name, just space to _ and lower case it for now...
    name.replace(' ', '_');
    insert(name + (mode == Search ? ":\"\"" : ""));
}
