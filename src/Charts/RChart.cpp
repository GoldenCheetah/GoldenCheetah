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
#include "RSyntax.h"

#include "Colors.h"
#include "AbstractView.h"
#include "GenericChart.h"
#include "HelpWhatsThis.h"

// unique identifier for each chart
static int id=0;

RConsole::RConsole(Context *context, RChart *parent)
    : QTextEdit(parent)
    , context(context), localEchoEnabled(true), parent(parent)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    setFrameStyle(QFrame::NoFrame);
    setAcceptRichText(false);
    document()->setMaximumBlockCount(512); // lets not get carried away!
    putData(GColor(CPLOTMARKER), QString(tr("R Console (%1)").arg(rtool->version)));
    putData(GInvertColor(CPLOTBACKGROUND), "\n> ");

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
    p.setColor(QPalette::Text, GInvertColor(CPLOTBACKGROUND));
    setPalette(p);
    setStyleSheet(AbstractView::ourStyleSheet());

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
    // if running a script we only process ESC
    if (rtool && rtool->canvas) {
        if (e->type() != QKeyEvent::KeyPress || e->key() != Qt::Key_Escape) return;
    }

    // otherwise lets go
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

    case Qt::Key_Escape: // R typically uses ESC to cancel
    case Qt::Key_C:
        {
            Qt::KeyboardModifiers kmod = static_cast<QInputEvent*>(e)->modifiers();
            bool ctrl = (kmod & Qt::ControlModifier) != 0;

            if (e->key() == Qt::Key_Escape || ctrl) {

                // are we doing something?
                if (rtool && rtool->canvas) {
                    rtool->cancel();
                }

                // ESC or ^C needs to clear program and go to next line
                rtool->R->program.clear();

                QTextCursor move = textCursor();
                move.movePosition(QTextCursor::End);
                setTextCursor(move);

                // new prompt
                putData("\n");
                putData(GInvertColor(CPLOTBACKGROUND), "> ");

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
            rtool->chart = parent;

            try {

                // replace $$ with chart identifier (to avoid shared data)
                line = line.replace("$$", chartid);

                // we always get an array of strings
                // so only print it out if its actually been set
                SEXP ret = NULL;

                rtool->cancelled = false;
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
            rtool->chart = NULL;
        }

        // prompt ">" for new command and ">>" for a continuation line
        if (rtool->R->program.count()==0)
            putData(GInvertColor(CPLOTBACKGROUND), "> ");
        else
            putData(GInvertColor(CPLOTBACKGROUND), ">>");
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
    putData(GInvertColor(CPLOTBACKGROUND), "> ");
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
    HelpWhatsThis *helpContents = new HelpWhatsThis(this);
    this->setWhatsThis(helpContents->getWhatsThisText(HelpWhatsThis::Chart_R));

    // settings - just choosing output type
    plotOnChartSetting = new QCheckBox(tr("Use GC charts"), this);
    plotOnChartSetting->setChecked(false);// by default we use R graphics device (legacy charts)
    QWidget *c=new QWidget(this);
    HelpWhatsThis *helpConfig = new HelpWhatsThis(c);
    c->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::Chart_R));
    QVBoxLayout *cl = new QVBoxLayout(c);
    cl->addWidget(plotOnChartSetting);
    setControls(c);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(2,0,2,2);
    setChartLayout(mainLayout);

    // if we failed to startup embedded R properly
    // then disable the RConsole altogether.
    if (rtool) {

        showCon = new QCheckBox(tr("Show Console"), this);
        showCon->setChecked(true);
        cl->addWidget(showCon);
        cl->addStretch();

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
        p.setColor(QPalette::Text, GInvertColor(CPLOTBACKGROUND));
        script->setPalette(p);
        script->setStyleSheet(AbstractView::ourStyleSheet());

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
        console->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        leftsplitter->addWidget(console);

        splitter = new QSplitter(Qt::Horizontal, this);
        splitter->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        splitter->setHandleWidth(1);
        mainLayout->addWidget(splitter);

        splitter->addWidget(leftsplitter);

        canvas = new RCanvas(context, this);
        canvas->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

        chart = new GenericChart(this, context);
        chart->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

        stack = new QStackedWidget(this);
        stack->addWidget(canvas);
        stack->addWidget(chart);
        stack->setCurrentIndex(0);

        splitter->addWidget(stack);

        // make splitter reasonable
        QList<int> sizes;
        sizes << 300 << 500;
        splitter->setSizes(sizes);

        if (ridesummary) {
            connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(runScript()));

            // refresh when comparing
            connect(context, SIGNAL(compareIntervalsStateChanged(bool)), this, SLOT(runScript()));
            connect(context, SIGNAL(compareIntervalsChanged()), this, SLOT(runScript()));

            // refresh when intervals are selected
            connect(context, SIGNAL(intervalSelected()), this, SLOT(runScript()));

        } else {
            connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(runScript()));

            // refresh when comparing
            connect(context, SIGNAL(compareDateRangesStateChanged(bool)), this, SLOT(runScript()));
            connect(context, SIGNAL(compareDateRangesChanged()), this, SLOT(runScript()));

            // perspective or perspective filter changed
            connect(this, SIGNAL(perspectiveFilterChanged(QString)), this, SLOT(runScript()));
            connect(this, SIGNAL(perspectiveChanged(Perspective *)), this, SLOT(runScript()));
        }

        // we apply BOTH filters, so update when either change
        connect(context, SIGNAL(filterChanged()), this, SLOT(runScript()));
        connect(context, SIGNAL(homeFilterChanged()), this, SLOT(runScript()));

        // reveal controls
        connect(showCon, SIGNAL(stateChanged(int)), this, SLOT(showConChanged(int)));

        // output to graphics device or qt chart
        connect(plotOnChartSetting, SIGNAL(stateChanged(int)), this, SLOT(plotOnChartChanged()));

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
        noR = new QLabel(tr("Warning: R is disabled"), this);
        cl->addWidget(noR);
        cl->addStretch();

        script = NULL;
        splitter = NULL;
        console = NULL;
        canvas = NULL;
        showCon = NULL;
        leftsplitter = NULL;
    }
}

bool
RChart::plotOnChart() const
{
    return plotOnChartSetting->isChecked();
}

void
RChart::setPlotOnChart(bool x)
{
    plotOnChartSetting->setChecked(x);
}

void
RChart::plotOnChartChanged()
{

    // now deal with that by adjusting which is visible.
    if (plotOnChartSetting->isChecked()) stack->setCurrentIndex(1); // generic chart
    else stack->setCurrentIndex(0); // r graphics device
}

bool
RChart::eventFilter(QObject *, QEvent *e)
{
    // on resize event scale the display
    if (e->type() == QEvent::Resize) {
        canvas->fitInView(canvas->sceneRect(), Qt::KeepAspectRatio);
    }

    // not running a script
    if (!rtool || !rtool->canvas) return false;

    // is it an ESC key?
    if (e->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
        // stop!
        rtool->cancel();
        return true;
    }
    // otherwise do nothing
    return false;
}

void
RChart::configChanged(qint32)
{
    QColor bgcolor = !ridesummary ? GColor(CTRENDPLOTBACKGROUND) : GColor(CPLOTBACKGROUND);
    setProperty("color", bgcolor);
    chart->setBackgroundColor(bgcolor);


    // tinted palette for headings etc
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Base, GCColor::instance()->alternateColor(CPLOTBACKGROUND));
    setPalette(palette);

    runScript(); // to update
}

void
RChart::setConsole(bool show)
{
    if (showCon) showCon->setChecked(show);
}

void
RChart::showConChanged(int state)
{
    if (leftsplitter) leftsplitter->setVisible(state);
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

QString
RChart::getState() const
{
    //XXX FIXME
    //if (rtool && splitter)  return QString(splitter->saveState());
    //else return "";
    return "";
}

void
RChart::setState(QString)
{
    //XXX FIXME
    //if (rtool && splitter && b != "") splitter->restoreState(QByteArray(b.toLatin1()));
}


void
RChart::runScript()
{
    // don't run until we can be seen!
    if (!isVisible()) return;

    if (script->toPlainText() != "") {

        // hourglass .. for long running ones this helps user know its busy
        QApplication::setOverrideCursor(Qt::WaitCursor);

        // turn off updates for a sec
        setUpdatesEnabled(false);

        // run it !!
        rtool->context = context;
        rtool->canvas = canvas;
        rtool->perspective = myPerspective;
        rtool->chart = this;

        // set default page size
        rtool->width = rtool->height = 0; // sets the canvas to the window size

        // set to defaults with gc applied
        rtool->cancelled = false;
        rtool->R->parseEvalQNT("par(par.gc)\n");

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

        // finalise the chart (even if not on show)
        chart->finaliseChart();

        // turn off updates for a sec
        setUpdatesEnabled(true);

        // reset cursor
        QApplication::restoreOverrideCursor();

        // if the program expects more we clear it, otherwise
        // weird things can happen!
        rtool->R->program.clear();

        // scale to fit and center on output
        canvas->fitInView(canvas->sceneRect(), Qt::KeepAspectRatio);

        // clear context
        rtool->context = NULL;
        rtool->canvas = NULL;
        rtool->perspective = NULL;
        rtool->chart = NULL;
    }
}
