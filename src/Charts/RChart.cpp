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

#include "R.h"
#include "RTool.h"
#include "RChart.h"
#include "RSyntax.h"

#include "Colors.h"
#include "TabView.h"

// unique identifier for each chart
static int id=0;

RConsole::RConsole(Context *context, RChart *parent)
    : QTextEdit(parent)
    , context(context), localEchoEnabled(true), parent(parent)
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

    // set unique runtimeid name
    chartid = QString("gc%1").arg(id++);

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

    // set default colors
    if (rtool) rtool->configChanged();
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

    case Qt::Key_C:
        {
            Qt::KeyboardModifiers kmod = static_cast<QInputEvent*>(e)->modifiers();
            bool ctrl = (kmod & Qt::ControlModifier) != 0;

            if (ctrl) {
                // ^C needs to clear program and go to next line
                rtool->R->program.clear();

                QTextCursor move = textCursor();
                move.movePosition(QTextCursor::End);
                setTextCursor(move);

                // new prompt
                putData("\n");
                putData(GCColor::invertColor(GColor(CPLOTBACKGROUND)), "> ");

            } else {
                // normal C just do the usual
                if (localEchoEnabled) QTextEdit::keyPressEvent(e);
            }
        }
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
            rtool->canvas = parent->canvas;

            try {

                // replace $$ with chart identifier (to avoid shared data)
                line = line.replace("$$", chartid);

                // we always get an array of strings
                // so only print it out if its actually been set
                SEXP ret = NULL;

                int rc = rtool->R->parseEval(line, ret);

                // if this isn't an assignment then print the result
                // bit hacky, there must be a better way!
                if(rc == 0 && ret != NULL && !Rf_isNull(ret) && !line.contains("<-") && !line.contains("print"))
                    Rf_PrintValue(ret);

                putData(GColor(CPLOTMARKER), rtool->messages.join(""));
                rtool->messages.clear();

            } catch(std::exception& ex) {

                putData(QColor(Qt::red), QString("%1\n").arg(QString(ex.what())));
                putData(QColor(Qt::red), rtool->messages.join(""));
                rtool->messages.clear();

            } catch(...) {

                putData(QColor(Qt::red), "error: general exception.\n");
                putData(QColor(Qt::red), rtool->messages.join(""));
                rtool->messages.clear();

            }

            // clear context
            rtool->context = NULL;
            rtool->canvas = NULL;
        }

        // prompt ">" for new command and ">>" for a continuation line
        if (rtool->R->program.count()==0)
            putData(GCColor::invertColor(GColor(CPLOTBACKGROUND)), "> ");
        else
            putData(GCColor::invertColor(GColor(CPLOTBACKGROUND)), ">>");
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

RChart::RChart(Context *context, bool ridesummary) : GcChartWindow(context), context(context), ridesummary(ridesummary)
{
    setControls(NULL);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(2,0,2,2);
    setChartLayout(mainLayout);

    // if we failed to startup embedded R properly
    // then disable the RConsole altogether.
    if (rtool) {

        leftsplitter = new QSplitter(Qt::Vertical, this);
        leftsplitter->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        leftsplitter->setHandleWidth(1);

        // LHS
        script = new  QTextEdit(this);
        script->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        script->setFrameStyle(QFrame::NoFrame);
        QFont courier("Courier", QFont().pointSize());
        script->setFont(courier);
        QPalette p = palette();
        p.setColor(QPalette::Base, GColor(CPLOTBACKGROUND));
        p.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
        script->setPalette(p);
        script->setStyleSheet(TabView::ourStyleSheet());

        // syntax highlighter
        setScript("## R script will run on selection.\n"
                  "##\n"
                  "## GC.activity(compare=FALSE)\n"
                  "## GC.metrics(all=FALSE,\n"
                  "##            compare=FALSE)\n"
                  "##\n"
                  "## Get the current ride or metrics\n"
                  "##\n");

        leftsplitter->addWidget(script);
        console = new RConsole(context, this);
        leftsplitter->addWidget(console);

        splitter = new QSplitter(Qt::Horizontal, this);
        splitter->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        splitter->setHandleWidth(1);
        mainLayout->addWidget(splitter);

        splitter->addWidget(leftsplitter);

        canvas = new RCanvas(context, this);
        canvas->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        splitter->addWidget(canvas);

        if (ridesummary) {
            connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(runScript()));

            // refresh when comparing
            connect(context, SIGNAL(compareIntervalsStateChanged(bool)), this, SLOT(runScript()));
            connect(context, SIGNAL(compareIntervalsChanged()), this, SLOT(runScript()));

        } else {
            connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(runScript()));

            // refresh when comparing
            connect(context, SIGNAL(compareDateRangesStateChanged(bool)), this, SLOT(runScript()));
            connect(context, SIGNAL(compareDateRangesChanged()), this, SLOT(runScript()));
        }

    } else {

        // not starting
        script = NULL;
        console = NULL;
        canvas = NULL;
    }
}

QString
RChart::getScript() const
{
    if (rtool && script) return script->toPlainText();
    else return text;
}
void
RChart::setScript(QString string)
{
    if (rtool && script) {
        script->setText(string);
        new RSyntax(script->document());
    }
    text = string;
}


void
RChart::runScript()
{
    if (script->toPlainText() != "") {

        // run it !!
        rtool->context = context;
        rtool->canvas = canvas;

        QString line = script->toPlainText();

        try {

            // replace $$ with chart identifier (to avoid shared data)
            line = line.replace("$$", console->chartid);

            // run it
            rtool->R->parseEval(line);

            // output on console
            if (rtool->messages.count()) {
                console->putData("\n");
                console->putData(GColor(CPLOTMARKER), rtool->messages.join(""));
                rtool->messages.clear();
            }

        } catch(std::exception& ex) {

            console->putData(QColor(Qt::red), QString("\n%1\n").arg(QString(ex.what())));
            console->putData(QColor(Qt::red), rtool->messages.join(""));
            rtool->messages.clear();

            // clear
            canvas->newPage();

        } catch(...) {

            console->putData(QColor(Qt::red), "\nerror: general exception.\n");
            console->putData(QColor(Qt::red), rtool->messages.join(""));
            rtool->messages.clear();

            // clear
            canvas->newPage();
        }

        // if the program expects more we clear it, otherwise
        // weird things can happen!
        rtool->R->program.clear();

        // clear context
        rtool->context = NULL;
        rtool->canvas = NULL;
    }
}
