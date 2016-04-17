/*
 * Copyright (c) 2016 Mark Liversedge (liversedge@gmail.com)
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

#include "RTool.h"
#include "RChart.h"

#include "Colors.h"
#include "TabView.h"

RConsole::RConsole(Context *context, QWidget *parent)
    : QTextEdit(parent)
    , context(context), localEchoEnabled(true)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    setFrameStyle(QFrame::NoFrame);
    document()->setMaximumBlockCount(10240);
    putData(GColor(CPLOTMARKER), QString(tr("R Console (%1)").arg(rtool->version)));
    putData(GCColor::invertColor(GColor(CPLOTBACKGROUND)), "\n> ");

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(rMessage(QString)), this, SLOT(rMessage(QString)));

    // history position
    hpos=0;

    configChanged(0);
}

void
RConsole::configChanged(qint32)
{
    QFont courier("Courier", QFont().pointSize());
    setFont(courier);
    QPalette p = palette();
    p.setColor(QPalette::Base, GColor(CPLOTBACKGROUND));
    p.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    setPalette(p);
    setStyleSheet(TabView::ourStyleSheet());
}

void
RConsole::rMessage(QString x)
{
    putData(GColor(CPLOTMARKER), x);
}

void RConsole::putData(QColor color, QString string)
{
    // change color...
    setTextColor(color);
    putData(string);
}


void RConsole::putData(QString data)
{
    insertPlainText(QString(data));

    QScrollBar *bar = verticalScrollBar();
    bar->setValue(bar->maximum());
}

void RConsole::setLocalEchoEnabled(bool set)
{
    localEchoEnabled = set;
}

void RConsole::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Up:
        if (hpos) {
            hpos--;
            setCurrentLine(history[hpos]);
        }
        break;

    case Qt::Key_Down:
        if (hpos < history.count()-1) {
            hpos++;
            setCurrentLine(history[hpos]);
        }
        break;

    // you can always go to the right
    case Qt::Key_Right:
        QTextEdit::keyPressEvent(e);
        break;

    // you can only delete or move left from past first character
    case Qt::Key_Left:
    case Qt::Key_Backspace:
        if (textCursor().position() - textCursor().block().position() > 2) QTextEdit::keyPressEvent(e);
        break;

    case Qt::Key_Enter:
    case Qt::Key_Return:
    {
        QTextCursor move = textCursor();
        move.movePosition(QTextCursor::End);
        setTextCursor(move);

        QString line = currentLine();
        if (line.length() > 1) line = line.mid(2, line.length()-2);
        else line = "";

        putData("\n");

        if (line != "") {

            history << line;
            hpos = history.count();

            // lets run it
            //qDebug()<<"RUN:" << line;

            // set the context for the call - used by all the
            // R functions to access the athlete data/model etc
            rtool->context = context;
            (*rtool->R)["GC.athlete"] = context->athlete->cyclist.toStdString();
            (*rtool->R)["GC.athlete.home"] = context->athlete->home->root().absolutePath().toStdString();

            try {
                SEXP ret = rtool->R->parseEval(line.toStdString());

                // if this isn't an assignment then print the result
                // bit hacky, there must be a better way!
                if(!Rf_isNull(ret) && !line.contains("<-") && !line.contains("print")) Rcpp::print(ret);

                QStringList &response = rtool->callbacks->getConsoleOutput();
                putData(GColor(CPLOTMARKER), response.join(""));
                response.clear();

            } catch(std::exception& ex) {

                putData(QColor(Qt::red), QString("%1\n").arg(QString(ex.what())));
                QStringList &response = rtool->callbacks->getConsoleOutput();
                putData(QColor(Qt::red), response.join(""));
                response.clear();

            } catch(...) {

                putData(QColor(Qt::red), "error: general exception.\n");
                QStringList &response = rtool->callbacks->getConsoleOutput();
                putData(QColor(Qt::red), response.join(""));
                response.clear();

            }

            // clear context
            rtool->context = NULL;

        }

        // next prompt
        putData(GCColor::invertColor(GColor(CPLOTBACKGROUND)), "> ");
    }
    break;

    default:
        if (localEchoEnabled) QTextEdit::keyPressEvent(e);
        emit getData(e->text().toLocal8Bit());
    }

    // if we edit or anything reset the history position
    if (e->key()!= Qt::Key_Up && e->key()!=Qt::Key_Down) hpos = history.count();
}

void
RConsole::setCurrentLine(QString p)
{
    QTextCursor select = textCursor();

    select.select(QTextCursor::LineUnderCursor);
    select.removeSelectedText();
    putData(GCColor::invertColor(GColor(CPLOTBACKGROUND)), "> ");
    putData(p);
}

QString
RConsole::currentLine()
{
    return textCursor().block().text().trimmed();
}

void RConsole::mousePressEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    setFocus();
}

void RConsole::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
}

void RConsole::contextMenuEvent(QContextMenuEvent *e)
{
    Q_UNUSED(e)
}

RChart::RChart(Context *context) : GcChartWindow(context)
{
    setControls(NULL);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(2,0,2,2);
    setChartLayout(mainLayout);

    // if we failed to startup the RInside properly
    // then disable the RConsole altogether.
    if (rtool->R) {
        splitter = new QSplitter(Qt::Horizontal, this);
        splitter->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        splitter->setHandleWidth(1);
        mainLayout->addWidget(splitter);

        console = new RConsole(context, this);
        splitter->addWidget(console);
        QWidget *surface = new QSvgWidget(this);
        surface->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        splitter->addWidget(surface);

        QPalette p = palette();
        p.setColor(QPalette::Base, GColor(CPLOTBACKGROUND));
        p.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
        surface->setPalette(p);
    }
}

