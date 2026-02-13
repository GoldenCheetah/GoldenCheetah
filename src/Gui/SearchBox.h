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
#include <QCompleter>
#include <QDebug>
#include <QStringListModel>

class QToolButton;
class QMenu;
class Context;
class QLabel;
class QFocusEvent;
class DataFilterCompleter;

class SearchBox : public QLineEdit
{
    Q_OBJECT

public:
    enum searchboxmode { Search, Filter };
    typedef enum searchboxmode SearchBoxMode;

    SearchBox(QWidget *parent, Context *context, bool nochooser=true);

    // either search box or filter box
    void setMode(SearchBoxMode mode);
    void setFixedMode(bool);
    void setText(QString);
    SearchBoxMode getMode() { return mode; }
    bool isFiltered() const { return filtered; }
    void setContext(Context *con);

protected:
    void resizeEvent(QResizeEvent *);
    void checkMenu(); // only show menu drop down when there is something to show

    void focusInEvent(QFocusEvent *e) { 
        emit haveFocus(); 
        QLineEdit::focusInEvent(e); 
    }

    void focusOutEvent(QFocusEvent *e) { 
        if (!active && e->reason() != Qt::PopupFocusReason) { 
            emit lostFocus(); 
            QLineEdit::focusOutEvent(e); 
        }
    }

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

    // completer
    void updateCompleter(const QString &);
    void setCompleter(DataFilterCompleter *completer);
    void insertCompletion(const QString& completion);
    void keyPressEvent(QKeyEvent *e);

    void configChanged(qint32);

signals:
    // text search mode
    void submitQuery(Context*,QString);
    void clearQuery();

    // db filter mode
    void submitFilter(Context*,QString);
    void clearFilter();

    // focus in/out
    void haveFocus();
    void lostFocus();

private:
    QWidget *parent;
    Context *context;
    bool nochooser;
    bool filtered;
    QToolButton *clearButton, *searchButton, *toolButton;
    QMenu *dropMenu;
    SearchBoxMode mode;
    DataFilterCompleter *completer;
    bool active;
    bool fixed;
};

class DataFilterCompleter : public QCompleter
{
    Q_OBJECT
 
public:
    inline DataFilterCompleter(const QStringList& words, QObject * parent) :
            QCompleter(parent), m_list(words), m_model()
    {
        m_model.setStringList(words);
        setModel(&m_model);
    }

    inline void setList(QStringList list)
    {
        m_list = list;
        m_model.setStringList(list);
    }
 
    inline void update(QString word)
    {
        // Do any filtering you like.
        // Here we just include all items that contain word.
        //QStringList filtered = m_list.filter(word, caseSensitivity());
        //m_model.setStringList(filtered);
        //m_word = word;
        setCompletionPrefix(word);
        complete();
    }
 
    inline QString word()
    {
        return m_word;
    }
 
private:
    QStringList m_list;
    QStringListModel m_model;
    QString m_word;
};
#endif
