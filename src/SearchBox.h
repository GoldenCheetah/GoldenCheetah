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

#ifndef _GC_SearchBox_h
#define _GC_SearchBox_h

#include <QLineEdit>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDialog>

class QToolButton;
class QMenu;
class MainWindow;
class QLabel;

class SearchBox : public QLineEdit
{
    Q_OBJECT

public:
    enum searchboxmode { Search, Filter };
    typedef enum searchboxmode SearchBoxMode;

    SearchBox(MainWindow *main, QWidget *parent = 0);

    // either search box or filter box
    void setMode(SearchBoxMode mode);
    void setText(QString);
    SearchBoxMode getMode() { return mode; }
    bool isFiltered() const { return filtered; }

protected:
    void resizeEvent(QResizeEvent *);
    void checkMenu(); // only show menu drop down when there is something to show

private slots:
    void updateCloseButton(const QString &text);
    void searchSubmit();
    void toggleMode();
    void clearClicked();

     // drop column headings from column chooser
     void dragEnterEvent(QDragEnterEvent *event);
     void dropEvent(QDropEvent *event);

     // highlight errors etc
     void setBad(QStringList errors);
     void setGood();

     // run etc
     void runMenu(QAction*);
     void setMenu();
     void addNamed();

signals:
    // text search mode
    void submitQuery(QString);
    void clearQuery();

    // db filter mode
    void submitFilter(QString);
    void clearFilter();

private:
    MainWindow *main;
    bool filtered;
    QToolButton *clearButton, *searchButton, *toolButton;
    QMenu *dropMenu;
    SearchBoxMode mode;
};
#endif
