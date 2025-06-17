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
#include "Context.h"
#include "AthleteTab.h"
#include "Athlete.h"
#include "NamedSearch.h"
#include "RideNavigator.h"
#include "GcSideBarItem.h"
#include "AnalysisSidebar.h"
#include "DataFilter.h"
#include "SpecialFields.h"
#include <QToolButton>
#include <QInputDialog>

#include <QStyle>
#include <QMenu>
#include <QDebug>

const int BUTTON_SIZE = 12;

SearchBox::SearchBox(Context *context, QWidget *parent, bool nochooser)
    : QLineEdit(parent), context(context), parent(parent), filtered(false), nochooser(nochooser), active(false), fixed(false)
{
    setFixedHeight(28*dpiYFactor);
    //clear button
    clearButton = new QToolButton(this);
    clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    QIcon pixmap = QPixmap::fromImage(QImage(":images/toolbar/popbutton.png").scaled(BUTTON_SIZE*dpiXFactor, BUTTON_SIZE*dpiXFactor));
    clearButton->setIcon(QIcon(pixmap));
    clearButton->setIconSize(QSize(BUTTON_SIZE*dpiXFactor, BUTTON_SIZE*dpiYFactor));
    clearButton->setCursor(Qt::ArrowCursor);
    clearButton->hide();
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clearClicked()));
    connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateCloseButton(const QString&)));

    // tool button
    toolButton = new QToolButton(this);
    QIcon toolB = iconFromPNG(":images/sidebar/extra.png", QSize(BUTTON_SIZE*dpiXFactor, BUTTON_SIZE*dpiYFactor));
#ifdef Q_OS_MAC
    toolButton->setStyleSheet("QToolButton { background: transparent; }");
#else
    toolButton->setStyleSheet("QToolButton { border: none; padding: 0px; background-color: none; }");
#endif
    toolButton->setIconSize(QSize(BUTTON_SIZE*dpiXFactor, BUTTON_SIZE*dpiYFactor));
    toolButton->setIcon(toolB);
    toolButton->setCursor(Qt::ArrowCursor);

    // drop menu
    dropMenu = new QMenu(this);
    connect(toolButton, SIGNAL(clicked()), this, SLOT(setMenu()));
    connect(dropMenu, SIGNAL(triggered(QAction*)), this, SLOT(runMenu(QAction*)));

    // search button
    searchButton = new QToolButton(this);
    QIcon search = iconFromPNG(":images/toolbar/search3.png", QSize(BUTTON_SIZE*dpiXFactor, BUTTON_SIZE*dpiYFactor));
    searchButton->setStyleSheet("QToolButton { border: none; padding: 1px; }");
    searchButton->setIconSize(QSize(BUTTON_SIZE*dpiXFactor, BUTTON_SIZE *dpiYFactor));
    searchButton->setIcon(search);
    searchButton->setCursor(Qt::ArrowCursor);
    connect(searchButton, SIGNAL(clicked()), this, SLOT(toggleMode()));

    // create an empty completer, configchanged will fix it
    completer = new DataFilterCompleter(QStringList(), parent);
    setCompleter(completer);

#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    setObjectName("SearchBox");
    resizeEvent(nullptr);
    mode = Search;
    setMode(mode);
    setDragEnabled(true);

    connect(this, SIGNAL(returnPressed()), this, SLOT(searchSubmit()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // set colors and curviness
    configChanged(CONFIG_APPEARANCE | CONFIG_FIELDS);
}

static bool insensitiveLessThan(const QString &a, const QString &b)
{
    return (QString::compare(a,b,Qt::CaseInsensitive)<0);
}

void
SearchBox::configChanged(qint32)
{
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    QColor color = QPalette().color(QPalette::Highlight);

    setStyleSheet(QString( //"QLineEdit { padding-right: %1px; } "
                      "QLineEdit#SearchBox {"
                      "    border-radius: 3px; "
                      "    border: 1px solid rgba(127,127,127,127);"
                      "    padding: 0px %1px;"
                      "}"
                      "QLineEdit#SearchBox:focus {"
                      "    border-radius: 3px; "
#ifdef WIN32
                      "    border: 1px solid rgba(%2,%3,%4,255);"
#else
                      "    border: 2px solid rgba(%2,%3,%4,255);"
#endif
                      "    padding: 0px %5px;"
                      "}"
             ).arg(clearButton->sizeHint().width() + frameWidth + 12)
              .arg(color.red()).arg(color.green()).arg(color.blue())
              .arg(clearButton->sizeHint().width() + frameWidth + 12));

    // get suitably formated list
    QList<QString> list;
    QString last;

    // start with just a list of functions
    list = DataFilter::builtins(context);

    // add special functions (older code needs fixing !)
    list << "config(cranklength)";
    list << "config(cp)";
    list << "config(aetp)";
    list << "config(ftp)";
    list << "config(w')";
    list << "config(pmax)";
    list << "config(cv)";
    list << "config(aetv)";
    list << "config(sex)";
    list << "config(dob)";
    list << "config(height)";
    list << "config(weight)";
    list << "config(lthr)";
    list << "config(aethr)";
    list << "config(maxhr)";
    list << "config(rhr)";
    list << "config(units)";
    list << "const(e)";
    list << "const(pi)";
    list << "daterange(start)";
    list << "daterange(stop)";
    list << "ctl";
    list << "tsb";
    list << "atl";
    list << "sb(BikeStress)";
    list << "lts(BikeStress)";
    list << "sts(BikeStress)";
    list << "rr(BikeStress)";
    list << "tiz(power, 1)";
    list << "tiz(hr, 1)";
    list << "best(power, 3600)";
    list << "best(hr, 3600)";
    list << "best(cadence, 3600)";
    list << "best(speed, 3600)";
    list << "best(torque, 3600)";
    list << "best(isopower, 3600)";
    list << "best(xpower, 3600)";
    list << "best(vam, 3600)";
    list << "best(wpk, 3600)";
    //list<<"RECINTSECS" is NOT added since its really only
    //valid to use it when working on ride samples
    list << "NA";

    // get sorted list
    QStringList names = context->rideNavigator->logicalHeadings;
    std::sort(names.begin(), names.end(), insensitiveLessThan);

    SpecialFields& sp = SpecialFields::getInstance();

    foreach(QString name, names) {

        // handle dups
        if (last == name) continue;
        last = name;

        // Handle bikescore tm
        if (name.startsWith("BikeScore")) name = QString("BikeScore");

        //  Always use the "internalNames" in Filter expressions
        name = sp.internalName(name);

        // we do very little to the name, just space to _ and lower case it for now...
        name.replace(' ', '_');
        list << name;
    }

    // sort the list
    std::sort(list.begin(), list.end(), insensitiveLessThan);

    // set new list
    completer->setList(list);
}

void
SearchBox::updateCompleter(const QString &text)
{
    active = true;
    if (mode == Filter && text.length()) {
        QChar last = text[text.length()-1];

        // are we tying characters that might be a symbol ?
        if (last.isLetterOrNumber() || last == '_') {

            // find out what end is
            int i=text.length();
            while (i && text[i-1].isLetterOrNumber()) i--;
            QString prefix = text.mid(i, text.length()-i);

            completer->update(prefix);

            // get that popup ?
            if (completer->completionCount()) {
                completer->popup()->setCurrentIndex(completer->completionModel()->index(0, 0));
            }
        } else {

            // hide it when we stop typing
            completer->popup()->hide();
        }

    }
    active=false;
}

void SearchBox::resizeEvent(QResizeEvent *)
{
    QSize cbz = clearButton->sizeHint();
    QSize tsz = toolButton->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

    // Create layout order left to right
    searchButton->move(3*dpiXFactor, 6*dpiYFactor);
    setTextMargins(0, 0, frameWidth + tsz.width(), 0);
    clearButton->move(rect().right() - frameWidth - tsz.width() - cbz.width() - 3*dpiXFactor, 6*dpiYFactor);
#ifdef Q_OS_MAC
    toolButton->move(rect().right() - frameWidth - tsz.width() - 3*dpiXFactor, 3*dpiYFactor);
#else
    toolButton->move(rect().right() - frameWidth - tsz.width() - 3*dpiXFactor, 6*dpiYFactor);
#endif
}

void SearchBox::setFixedMode(bool fixed)
{
    this->fixed = fixed;
}
void SearchBox::toggleMode()
{
    if (fixed) return;

    clear(); // clear whatever is there first
    if (mode == Search) setMode(Filter);
    else setMode(Search);
}

void SearchBox::setMode(SearchBoxMode mode)
{
    switch (mode) {

        case Filter:
        {
            QIcon filter = iconFromPNG(":images/toolbar/filter3.png", QSize(BUTTON_SIZE*dpiXFactor, BUTTON_SIZE*dpiYFactor));
            searchButton->setStyleSheet("QToolButton { border: none; padding: 1px; }");
            searchButton->setIconSize(QSize(BUTTON_SIZE*dpiXFactor, BUTTON_SIZE*dpiYFactor));
            searchButton->setIcon(filter);
            setPlaceholderText(tr("Filter..."));
        }
        break;

        case Search:
        default:
        {
            QIcon search = iconFromPNG(":images/toolbar/search3.png", QSize(BUTTON_SIZE*dpiXFactor, BUTTON_SIZE*dpiYFactor));
            searchButton->setStyleSheet("QToolButton { border: none; padding: 1px; }");
            searchButton->setIconSize(QSize(BUTTON_SIZE*dpiXFactor, BUTTON_SIZE*dpiYFactor));
            searchButton->setIcon(search);
            setPlaceholderText(tr("Search..."));
        }
        break;
    }
    this->mode = mode;

    checkMenu();
}

void SearchBox::updateCloseButton(const QString& text)
{
    if (clearButton->isVisible() && text.isEmpty()) mode == Search ? clearQuery() : clearFilter();
    clearButton->setVisible(!text.isEmpty());

    //REMOVED SINCE TOO HEAVY NOW AFFECTS CHARTS TOO
    //if (mode == Search) searchSubmit(); // only do search as you type in search mode

    setGood(); // if user changing then don't stay red - wait till resubmitted
    checkMenu();
}

void SearchBox::searchSubmit()
{
    // return hit / key pressed
    if (text() != "") {
        filtered = true;
        mode == Search ? submitQuery(context, text()) : submitFilter(context, text());
    }
}

void SearchBox::clearClicked()
{
    setText("");
    filtered = false;
    //mode == Search ? clearQuery() : clearFilter();
    setGood();
}

void SearchBox::checkMenu()
{
    if (context->athlete->namedSearches->getList().count() || text() != "") toolButton->show();
    else toolButton->hide();
}

void SearchBox::setMenu()
{
    dropMenu->clear();
    if (text() != "") dropMenu->addAction(tr("Add to Named Filters"));
    if (context->athlete->namedSearches->getList().count()) {
        if (text() != "") dropMenu->addSeparator();
        foreach(NamedSearch x, context->athlete->namedSearches->getList()) {
            dropMenu->addAction(x.name);
        }
        dropMenu->addSeparator();
        dropMenu->addAction(tr("Manage Filters"));
    }
    if (!nochooser) dropMenu->addAction(tr("Column Chooser"));

    dropMenu->exec(mapToGlobal((QPoint(toolButton->pos().x() + toolButton->width() - 20, toolButton->pos().y()))));
}

void SearchBox::runMenu(QAction *x)
{
    // just qdebug for now
    if (x->text() == tr("Add to Named Filters")) addNamed();
    else if (x->text() == tr("Manage Filters")) {

        EditNamedSearches *editor = new EditNamedSearches(context->mainWindow, context);
        editor->move(QCursor::pos() - QPoint(460, -5));
        editor->show();

    } else if (x->text() == tr("Column Chooser")) {

        ColumnChooser *selector = new ColumnChooser(context->rideNavigator->logicalHeadings);
        selector->show();

    } else {
        NamedSearch get = context->athlete->namedSearches->get(x->text());
        if (get.name == x->text()) {
            setMode(static_cast<SearchBox::SearchBoxMode>(get.type));
            setText(get.text);
        }
    }
}

void SearchBox::setBad(QStringList errors)
{
    QPalette pal;
    pal.setColor(QPalette::Text, Qt::red);
    setPalette(pal);

    setToolTip(errors.join(tr(" and ")));
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
    QByteArray rawData = event->mimeData()->data("application/x-columnchooser");
    QDataStream stream(&rawData, QIODevice::ReadOnly);
    stream.setVersion(QDataStream::Qt_4_6);
    QString name;
    stream >> name;

    // fugly, but it works for BikeScore with the (TM) in it... so...
    // independent of Latin1 or UTF-8 coming from "Column Chooser" the "TM" special sign is not recognized by the parser,
    // so strip the "TM" off for this case (only)
    if (name.startsWith("BikeScore")) name = QString("BikeScore");

    //  Always use the "internalNames" in Filter expressions
    name = SpecialFields::getInstance().internalName(name);

    // we do very little to the name, just space to _ and lower case it for now...
    name.replace(' ', '_');
    insert(name);
}

void
SearchBox::setText(QString s)
{
    QLineEdit::setText(s);
    if (s != "") searchSubmit();
}

void
SearchBox::addNamed()
{
     bool ok;
     QString text = QInputDialog::getText(this, tr("Add new search"),
                                          tr("Name:"), QLineEdit::Normal, QString(""), &ok);

    if (ok && !text.isEmpty()) {
        NamedSearch x;
        x.name = text;
        x.text = this->text();
        x.type = mode;
        x.count = 0;
        context->athlete->namedSearches->getList().append(x);
        context->athlete->namedSearches->write();
    }
}

//
// Working with the autocompleter
//
void SearchBox::setCompleter(DataFilterCompleter *completer)
{
    if (this->completer) QObject::disconnect(this->completer, 0, this, 0);
 
    this->completer = completer;
 
    if (!this->completer) return;
 
    this->completer->setWidget(this);
    connect(completer, SIGNAL(activated(const QString&)), this, SLOT(insertCompletion(const QString&)));
}
 
void SearchBox::insertCompletion(const QString& completion)
{
    if (mode == Search) return;

    QString t = text();

    // blank!!
    if (t.length()==0) return;

    QChar last = t[t.length()-1];

    if (last.isLetterOrNumber()) {

        // find out what end is
        int i=t.length();
        while (i && t[i-1].isLetterOrNumber()) i--;
        t.replace(i, t.length()-i, completion);

        // now replace
        setText(t);
    }
}
 
 
void SearchBox::keyPressEvent(QKeyEvent *e)
{
    // only intercept in filter mode
    if (mode == Search) return QLineEdit::keyPressEvent(e);

    if (completer && completer->popup()->isVisible())
    {
        // The following keys are forwarded by the completer to the widget
        switch (e->key())
        {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            e->ignore();
            return; // Let the completer do default behavior
        }
    }
 
    bool isShortcut = (e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_E;
    if (!isShortcut)
        QLineEdit::keyPressEvent(e); // Don't send the shortcut (CTRL-E) to the text edit.
 
    if (!this->completer) return;
 
    bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    if (!isShortcut && !ctrlOrShift && e->modifiers() != Qt::NoModifier)
    {
        completer->popup()->hide();
        return;
    }

    updateCompleter(text()); 
    //completer->update(text());
    //completer->popup()->setCurrentIndex(completer->completionModel()->index(0, 0));
}
