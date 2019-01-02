/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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

#include "PythonChart.h"
#include "PythonEmbed.h"
#include "PythonSyntax.h"

#include "Colors.h"
#include "TabView.h"

#include <QtConcurrent>

#if QT_VERSION >= 0x050800
#include <QWebEngineSettings>
#endif

// always pull in after all QT headers
#ifdef slots
#undef slots
#endif
#include <Python.h>

// unique identifier for each chart
static int id=0;

PythonConsole::PythonConsole(Context *context, PythonChart *parent)
    : QTextEdit(parent)
    , context(context), localEchoEnabled(true), parent(parent)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    setFrameStyle(QFrame::NoFrame);
    setAcceptRichText(false);
    document()->setMaximumBlockCount(512); // lets not get carried away!
    putData(GColor(CPLOTMARKER), QString(tr("Python Console (%1)").arg(python->version)));
    putData(GCColor::invertColor(GColor(CPLOTBACKGROUND)), "\n>>> ");

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(rMessage(QString)), this, SLOT(rMessage(QString)));

    // history position
    hpos=0;

    // set unique runtimeid name
    chartid = QString("gc%1").arg(id++);

    configChanged(0);
}

void
PythonConsole::configChanged(qint32)
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
PythonConsole::rMessage(QString x)
{
    putData(GColor(CPLOTMARKER), x);
}

void PythonConsole::putData(QColor color, QString string)
{
    // change color...
    setTextColor(color);
    putData(string);
}


void PythonConsole::putData(QString data)
{
    insertPlainText(QString(data));

    QScrollBar *bar = verticalScrollBar();
    bar->setValue(bar->maximum());
}

void PythonConsole::setLocalEchoEnabled(bool set)
{
    localEchoEnabled = set;
}

void PythonConsole::keyPressEvent(QKeyEvent *e)
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
        if (textCursor().positionInBlock() > promptStartIndex) QTextEdit::keyPressEvent(e);
        break;

    case Qt::Key_Home:
        {
            QTextCursor moveHome = textCursor();
            moveHome.setPosition(textCursor().block().position() + promptStartIndex, QTextCursor::MoveAnchor);
            setTextCursor(moveHome);
        }
        break;

    case Qt::Key_Escape: // R typically uses ESC to cancel
    case Qt::Key_C:
        {

            Qt::KeyboardModifiers kmod = static_cast<QInputEvent*>(e)->modifiers();
            bool ctrl = (kmod & Qt::ControlModifier) != 0;

            if (e->key() == Qt::Key_Escape || ctrl) {

                // are we doing something?
                python->cancel();

                // ESC or ^C needs to clear program and go to next line
                python->program.clear();

                QTextCursor move = textCursor();
                move.movePosition(QTextCursor::End);
                setTextCursor(move);

                // new prompt
                putData("\n");
                putData(GCColor::invertColor(GColor(CPLOTBACKGROUND)), ">>> ");

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
        if (line.length() > promptStartIndex) line = line.mid(promptStartIndex, line.length() - promptStartIndex);
        else line = "";

        putData("\n");

        if (line != "") {

            history << line;
            hpos = history.count();

            // lets run it
            //qDebug()<<"RUN:" << line;

            // set the context for the call
            python->canvas = parent->canvas;
            python->chart = parent;

            try {

                // replace $$ with chart identifier (to avoid shared data)
                line = line.replace("$$", chartid);

                python->cancelled = false;
                python->runline(ScriptContext(context, nullptr, nullptr, Specification(), true), line);

                // the run command should result in some messages being generated
                putData(GColor(CPLOTMARKER), python->messages.join(""));
                python->messages.clear();

            } catch(std::exception& ex) {

                putData(QColor(Qt::red), QString("%1\n").arg(QString(ex.what())));
                putData(QColor(Qt::red), python->messages.join("\n"));
                python->messages.clear();

            } catch(...) {

                putData(QColor(Qt::red), "error: general exception.\n");
                putData(QColor(Qt::red), python->messages.join("\n"));
                python->messages.clear();

            }

            // clear context
            python->canvas = NULL;
            python->chart = NULL;
        }

        // prompt ">"
        putData(GCColor::invertColor(GColor(CPLOTBACKGROUND)), ">>> ");
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
PythonConsole::setCurrentLine(QString p)
{
    QTextCursor select = textCursor();

    select.select(QTextCursor::LineUnderCursor);
    select.removeSelectedText();
    putData(GCColor::invertColor(GColor(CPLOTBACKGROUND)), ">>> ");
    putData(p);
}

QString
PythonConsole::currentLine()
{
    return textCursor().block().text().trimmed();
}

void PythonConsole::mousePressEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    setFocus();
}

void PythonConsole::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
}

void PythonConsole::contextMenuEvent(QContextMenuEvent *e)
{
    Q_UNUSED(e)
}

PythonChart::PythonChart(Context *context, bool ridesummary) : GcChartWindow(context), context(context), ridesummary(ridesummary)
{
    setControls(NULL);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
    setChartLayout(mainLayout);

    // if we failed to startup embedded R properly
    // then disable the PythonConsole altogether.
    if (python) {

        // reveal controls
        QHBoxLayout *rev = new QHBoxLayout();
        showCon = new QCheckBox(tr("Show Console"), this);
        showCon->setChecked(true);

        rev->addStretch();
        rev->addWidget(showCon);
        rev->addStretch();

        setRevealLayout(rev);

        leftsplitter = new QSplitter(Qt::Vertical, this);
        leftsplitter->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        leftsplitter->setHandleWidth(1);

        // LHS
        script = new  QTextEdit(this);
        script->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        script->setFrameStyle(QFrame::NoFrame);
        script->setAcceptRichText(false);
        QFont courier("Courier", QFont().pointSize());
        script->setFont(courier);
        QPalette p = palette();
        p.setColor(QPalette::Base, GColor(CPLOTBACKGROUND));
        p.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
        script->setPalette(p);
        script->setStyleSheet(TabView::ourStyleSheet());

        // syntax highlighter
        setScript("##\n## Python program will run on selection.\n##\n");

        leftsplitter->addWidget(script);
        console = new PythonConsole(context, this);
        console->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        leftsplitter->addWidget(console);

        splitter = new QSplitter(Qt::Horizontal, this);
        splitter->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        splitter->setHandleWidth(1);
        mainLayout->addWidget(splitter);

        splitter->addWidget(leftsplitter);

        canvas = new QWebEngineView(this);
        canvas->setContentsMargins(0,0,0,0);
        canvas->page()->view()->setContentsMargins(0,0,0,0);
        canvas->setZoomFactor(dpiXFactor);
        canvas->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
#if QT_VERSION >= 0x050800
        // stop stealing focus!
        canvas->settings()->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, false);
#endif
        splitter->addWidget(canvas);

        // make splitter reasonable
        QList<int> sizes;
        sizes << 300 << 500;
        splitter->setSizes(sizes);

        connect(this, SIGNAL(setUrl(QUrl)), this, SLOT(webpage(QUrl)));

        if (ridesummary) {
            connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(runScript()));

            // refresh when comparing
            connect(context, SIGNAL(compareIntervalsStateChanged(bool)), this, SLOT(runScript()));
            connect(context, SIGNAL(compareIntervalsChanged()), this, SLOT(runScript()));

            // refresh when intervals changed / selected
            connect(context, SIGNAL(intervalsChanged()), this, SLOT(runScript()));
            connect(context, SIGNAL(intervalSelected()), this, SLOT(runScript()));

        } else {
            connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(runScript()));

            // refresh when comparing
            connect(context, SIGNAL(compareDateRangesStateChanged(bool)), this, SLOT(runScript()));
            connect(context, SIGNAL(compareDateRangesChanged()), this, SLOT(runScript()));
        }

        // we apply BOTH filters, so update when either change
        connect(context, SIGNAL(filterChanged()), this, SLOT(runScript()));
        connect(context, SIGNAL(homeFilterChanged()), this, SLOT(runScript()));

        // reveal controls
        connect(showCon, SIGNAL(stateChanged(int)), this, SLOT(showConChanged(int)));

        // config changes
        connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
        configChanged(CONFIG_APPEARANCE);

        // filter ESC so we can stop scripts
        installEventFilter(this);
        installEventFilter(console);
        installEventFilter(splitter);
        installEventFilter(canvas);

    } else {

        // not starting
        script = NULL;
        splitter = NULL;
        console = NULL;
        canvas = NULL;
        showCon = NULL;
        leftsplitter = NULL;
    }
}

bool
PythonChart::eventFilter(QObject *, QEvent *e)
{
    // on resize event scale the display
    if (e->type() == QEvent::Resize) {
        //canvas->fitInView(canvas->sceneRect(), Qt::KeepAspectRatio);
    }

    // not running a script
    if (!python) return false;

    // is it an ESC key?
    if (e->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
        // stop!
        python->cancel();
        return true;
    }
    // otherwise do nothing
    return false;
}

void
PythonChart::configChanged(qint32)
{
    if (!ridesummary) setProperty("color", GColor(CTRENDPLOTBACKGROUND));
    else setProperty("color", GColor(CPLOTBACKGROUND));

    // tinted palette for headings etc
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
    setPalette(palette);
}

void
PythonChart::setConsole(bool show)
{
    if (showCon) showCon->setChecked(show);
}

void
PythonChart::showConChanged(int state)
{
    if (leftsplitter) leftsplitter->setVisible(state);
}

QString
PythonChart::getScript() const
{
    if (python && script) return script->toPlainText();
    else return text;
}
void
PythonChart::setScript(QString string)
{
    if (python && script) {
        script->setText(string);
        new PythonSyntax(script->document());
    }
    text = string;
}

QString
PythonChart::getState() const
{
    //XXX FIXME
    //if (python && splitter)  return QString(splitter->saveState());
    //else return "";
    return "";
}

void
PythonChart::setState(QString)
{
    //XXX FIXME
    //if (python && splitter && b != "") splitter->restoreState(QByteArray(b.toLatin1()));
}

// this is executed in its own thread.
void
PythonChart::execScript(PythonChart *chart)
{
    python->runline(ScriptContext(chart->context), chart->script->toPlainText());
}

void
PythonChart::runScript()
{
    // don't run until we can be seen!
    if (!isVisible()) return;

    if (script->toPlainText() != "") {

        // if it is running another for us?
        if (python->chart == this) python->cancel();

        // hourglass .. for long running ones this helps user know its busy
        QApplication::setOverrideCursor(Qt::WaitCursor);

        // turn off updates for a sec
        setUpdatesEnabled(false);

        // run it !!
        python->canvas = canvas;
        python->chart = this;

        // set default page size
        //python->width = python->height = 0; // sets the canvas to the window size

        // set to defaults with gc applied
        python->cancelled = false;

        QString line = script->toPlainText();

        try {

            // replace $$ with chart identifier (to avoid shared data)
            line = line.replace("$$", console->chartid);

            // run it
            QFutureWatcher<void>watcher;
            QFuture<void>f= QtConcurrent::run(execScript,this);

            // wait for it to finish -- remember ESC can be pressed to cancel
            watcher.setFuture(f);
            QEventLoop loop;
            connect(&watcher, SIGNAL(finished()), &loop, SLOT(quit()));
            loop.exec();

            // output on console
            if (python->messages.count()) {
                console->putData(GColor(CPLOTMARKER), python->messages.join("\n"));
                python->messages.clear();
            }

        } catch(std::exception& ex) {

            console->putData(QColor(Qt::red), QString("\n%1\n").arg(QString(ex.what())));
            console->putData(QColor(Qt::red), python->messages.join(""));
            python->messages.clear();

        } catch(...) {

            console->putData(QColor(Qt::red), "\nerror: general exception.\n");
            console->putData(QColor(Qt::red), python->messages.join(""));
            python->messages.clear();

        }

        // turn off updates for a sec
        setUpdatesEnabled(true);

        // reset cursor
        QApplication::restoreOverrideCursor();

        // if the program expects more we clear it, otherwise
        // weird things can happen!
        //python->R->program.clear();

        // scale to fit and center on output
        //canvas->fitInView(canvas->sceneRect(), Qt::KeepAspectRatio);

        // clear context
        python->canvas = NULL;
        python->chart = NULL;
    }
}

void
PythonChart::webpage(QUrl url)
{
    canvas->setUrl(url);
}
