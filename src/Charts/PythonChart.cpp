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
#include "RideFileCommand.h"

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

PythonConsole::PythonConsole(Context *context, PythonHost *pythonHost, QWidget *parent)
    : QTextEdit(parent)
    , context(context), localEchoEnabled(true), pythonHost(pythonHost)
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
            if (pythonHost->chart()) {
                python->canvas = pythonHost->chart()->canvas;
                python->chart = pythonHost->chart();
            }

            try {

                // replace $$ with chart identifier (to avoid shared data)
                line = line.replace("$$", chartid);

                bool readOnly = pythonHost->readOnly();
                QList<RideFile *> editedRideFiles;
                python->cancelled = false;
                python->runline(ScriptContext(context, nullptr, true, readOnly, &editedRideFiles), line);

                // finish up commands on edited rides
                foreach (RideFile *f, editedRideFiles) {
                    f->command->endLUW();
                }

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
    // controls widget
    QWidget *c = new QWidget;
    setControls(c);
    //HelpWhatsThis *helpConfig = new HelpWhatsThis(c);
    //c->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::ChartRides_Performance));

    // settings
    QVBoxLayout *clv = new QVBoxLayout(c);
    web = new QCheckBox(tr("Web charting"), this);
    web->setChecked(true);
    clv->addWidget(web);
    clv->addStretch();

    // sert no render widget
    charttype=0; // not set yet
    chartview=NULL;
    canvas=NULL;
    barseries=NULL;
    bottom=left=true;

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
        console = new PythonConsole(context, this, this);
        console->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        leftsplitter->addWidget(console);

        splitter = new QSplitter(Qt::Horizontal, this);
        splitter->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        splitter->setHandleWidth(1);
        mainLayout->addWidget(splitter);

        splitter->addWidget(leftsplitter);

        // for Chart or webpage
        render = new QWidget(this);
        renderlayout = new QVBoxLayout(render);
        splitter->addWidget(render);

        // make splitter reasonable
        QList<int> sizes;
        sizes << 300 << 500;
        splitter->setSizes(sizes);

        // passing data across python and gui threads
        connect(this, SIGNAL(setUrl(QUrl)), this, SLOT(webpage(QUrl)));
        connect(this, SIGNAL(emitChart(QString,int,bool)), this, SLOT(configChart(QString,int,bool)));
        connect(this, SIGNAL(emitCurve(QString,QVector<double>,QVector<double>,QString,QString,QStringList,QStringList,int,int,int,QString,int,bool)),
                this,   SLOT( setCurve(QString,QVector<double>,QVector<double>,QString,QString,QStringList,QStringList,int,int,int,QString,int,bool)));
        connect(this, SIGNAL(emitAxis(QString,bool,int,double,double,int,QString,QString,bool,QStringList)),
                this,   SLOT(configAxis(QString,bool,int,double,double,int,QString,QString,bool,QStringList)));

        if (ridesummary) {

            // refresh when comparing
            connect(context, SIGNAL(compareIntervalsStateChanged(bool)), this, SLOT(runScript()));
            connect(context, SIGNAL(compareIntervalsChanged()), this, SLOT(runScript()));

            // refresh when intervals changed / selected
            connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(runScript())); // not needed since get signal below
            //connect(context, SIGNAL(intervalsChanged()), this, SLOT(runScript()));
            //connect(context, SIGNAL(intervalSelected()), this, SLOT(runScript()));

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
        connect(web, SIGNAL(stateChanged(int)), this, SLOT(showWebChanged(int)));

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


// switch between rendering to a web page and rendering to a chart page
void
PythonChart::setWeb(bool x)
{
    // toggle the use of a web chart or a qt chart for rendering the data
    if (x && canvas==NULL) {

        // delete the chart view if exists
        if (chartview) {
            renderlayout->removeWidget(chartview);
            delete chartview; // deletes associated chart too
            chartview=NULL;
            qchart=NULL;
        }

        // setup the canvas
        canvas = new QWebEngineView(this);
        canvas->setContentsMargins(0,0,0,0);
        canvas->page()->view()->setContentsMargins(0,0,0,0);
        canvas->setZoomFactor(dpiXFactor);
        canvas->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
#if QT_VERSION >= 0x050800
        // stop stealing focus!
        canvas->settings()->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, false);
#endif
        renderlayout->insertWidget(0, canvas);
    }

    if (!x && chartview==NULL) {

        // delete the canvas if exists
        if (canvas) {
            renderlayout->removeWidget(canvas);
            delete canvas;
            canvas = NULL;
        }

        // setup the chart
        qchart = new QChart();
        qchart->setBackgroundVisible(false); // draw on canvas
        qchart->legend()->setVisible(true); // no legends
        qchart->setTitle("No title set"); // none wanted
        qchart->setAnimationOptions(QChart::NoAnimation);
        qchart->setFont(QFont());

        // set theme, but for now use a std one TODO: map color scheme to chart theme
        qchart->setTheme(QChart::ChartThemeDark);

        chartview = new QChartView(qchart, this);
        chartview->setRenderHint(QPainter::Antialiasing);
        renderlayout->insertWidget(0, chartview);
    }

    // set the check state!
    web->setChecked(x);

    // config changed...
    configChanged(0);
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

    // chart colors
    if (chartview) {
        chartview->setBackgroundBrush(QBrush(GColor(CPLOTBACKGROUND)));
        qchart->setBackgroundBrush(QBrush(GColor(CPLOTBACKGROUND)));
        qchart->setBackgroundPen(QPen(GColor(CPLOTMARKER)));
    }
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

void
PythonChart::showWebChanged(int state)
{
    setWeb(state);
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

        // polish  the chart if needed
        if (qchart) polishChart();

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

// rendering to a web page
void
PythonChart::webpage(QUrl url)
{
    if (canvas) canvas->setUrl(url);
}

// rendering to qt chart
bool
PythonChart::setCurve(QString name, QVector<double> xseries, QVector<double> yseries, QString xname, QString yname,
                      QStringList labels, QStringList colors,
                      int line, int symbol, int size, QString color, int opacity, bool opengl)
{
    // if curve already exists, remove it
    if (charttype==GC_CHART_LINE || charttype==GC_CHART_SCATTER || charttype==GC_CHART_PIE) {
        QAbstractSeries *existing = curves.value(name);
        if (existing) {
            qchart->removeSeries(existing);
            delete existing;
        }
    }

    // lets find that axis - even blank ones
    AxisInfo *xaxis, *yaxis;
    xaxis=axisinfos.value(xname);
    yaxis=axisinfos.value(yname);
    if (xaxis==NULL) {
        xaxis=new AxisInfo(Qt::Horizontal, xname);

        // default alignment toggles
        xaxis->align = bottom ? Qt::AlignBottom : Qt::AlignTop;
        bottom = !bottom;

        // use marker color for x axes
        xaxis->labelcolor = xaxis->axiscolor = GColor(CPLOTMARKER);

        // add to list
        axisinfos.insert(xname, xaxis);
    }
    if (yaxis==NULL) {
        yaxis=new AxisInfo(Qt::Vertical, yname);

        // default alignment toggles
        yaxis->align = left ? Qt::AlignLeft : Qt::AlignRight;
        left = !left;

        // yaxis color matches, but not done for xaxis above
        yaxis->labelcolor = yaxis->axiscolor = QColor(color);

        // add to list
        axisinfos.insert(yname, yaxis);
    }

    switch (charttype) {
    default:

    case GC_CHART_LINE:
        {
            // set up the curves
            QLineSeries *add = new QLineSeries();
            add->setName(name);

            // aesthetics
            add->setBrush(Qt::NoBrush);
            QPen pen(color);
            pen.setStyle(Qt::SolidLine);
            pen.setWidth(size);
            add->setPen(pen);
            add->setOpacity(double(opacity) / 100.0); // 0-100% to 0.0-1.0 values

            // data
            for (int i=0; i<xseries.size() && i<yseries.size(); i++) {
                add->append(xseries.at(i), yseries.at(i));

                // tell axis about the data
                xaxis->point(xseries.at(i), yseries.at(i));
                yaxis->point(xseries.at(i), yseries.at(i));
            }

            // hardware support?
            chartview->setRenderHint(QPainter::Antialiasing);
            add->setUseOpenGL(opengl); // for scatter or line only apparently

            // chart
            qchart->addSeries(add);

            // add to list of curves
            curves.insert(name,add);
            xaxis->series.append(add);
            yaxis->series.append(add);
        }
        break;

    case GC_CHART_SCATTER:
        {
            // set up the curves
            QScatterSeries *add = new QScatterSeries();
            add->setName(name);

            // aesthetics
            add->setMarkerShape(QScatterSeries::MarkerShapeCircle); //TODO: use 'symbol'
            add->setMarkerSize(size);
            QColor col=QColor(color);
            add->setBrush(QBrush(col));
            add->setPen(Qt::NoPen);
            add->setOpacity(double(opacity) / 100.0); // 0-100% to 0.0-1.0 values

            // data
            for (int i=0; i<xseries.size() && i<yseries.size(); i++) {
                add->append(xseries.at(i), yseries.at(i));

                // tell axis about the data
                xaxis->point(xseries.at(i), yseries.at(i));
                yaxis->point(xseries.at(i), yseries.at(i));

            }

            // hardware support?
            chartview->setRenderHint(QPainter::Antialiasing);
            add->setUseOpenGL(opengl); // for scatter or line only apparently

            // chart
            qchart->addSeries(add);
            qchart->legend()->setMarkerShape(QLegend::MarkerShapeRectangle);
            qchart->setDropShadowEnabled(false);

            // add to list of curves
            curves.insert(name,add);
            xaxis->series.append(add);
            yaxis->series.append(add);

        }
        break;

    case GC_CHART_BAR:
        {
            // set up the barsets
            QBarSet *add= new QBarSet(name);

            // aesthetics
            add->setBrush(QBrush(QColor(color)));
            add->setPen(Qt::NoPen);

            // data and min/max values
            for (int i=0; i<yseries.size(); i++) {
                double value = yseries.at(i);
                *add << value;
                yaxis->point(i,value);
                xaxis->point(i,value);
            }

            // we are very particular regarding axis
            yaxis->type = AxisInfo::CONTINUOUS;
            xaxis->type = AxisInfo::CATEGORY;

            // add to list of barsets
            barsets << add;
        }
        break;

    case GC_CHART_PIE:
        {
            // set up the curves
            QPieSeries *add = new QPieSeries();

            // setup the slices
            for(int i=0; i<yseries.size(); i++) {
                // get label?
                if (i>=labels.size())
                    add->append(QString("%1").arg(i), yseries.at(i));
                else
                    add->append(labels.at(i), yseries.at(i));
            }

            // now do the colors
            int i=0;
            foreach(QPieSlice *slice, add->slices()) {

                slice->setExploded();
                slice->setLabelVisible();
                slice->setPen(Qt::NoPen);
                if (i <colors.size()) slice->setBrush(QColor(colors.at(i)));
                else slice->setBrush(Qt::red);
                i++;
            }

            // set the pie chart
            qchart->addSeries(add);

            // add to list of curves
            curves.insert(name,add);
        }
        break;

    }
    return true;
}

// once python script has run polish the chart, fixup axes/ranges and so on.
void
PythonChart::polishChart()
{
    if (!qchart) return;

    // basic aesthetics
    qchart->legend()->setMarkerShape(QLegend::MarkerShapeRectangle);
    qchart->setDropShadowEnabled(false);

    // no more than 1 category axis since barsets are all assigned.
    bool donecategory=false;

    // Create axes - for everyone except pie charts that don't have any
    if (charttype != GC_CHART_PIE) {
        // create desired axis
        foreach(AxisInfo *axisinfo, axisinfos) {
//fprintf(stderr, "Axis: %s, orient:%s, type:%d\n",axisinfo->name.toStdString().c_str(),axisinfo->orientation==Qt::Vertical?"vertical":"horizontal",(int)axisinfo->type);
//fflush(stderr);
            QAbstractAxis *add=NULL;
            switch (axisinfo->type) {
            case AxisInfo::DATERANGE: // TODO
            case AxisInfo::TIME:      // TODO
            case AxisInfo::CONTINUOUS:
                {
                    QValueAxis *vaxis= new QValueAxis(qchart);
                    add=vaxis; // gets added later

                    vaxis->setMin(axisinfo->min());
                    vaxis->setMax(axisinfo->max());

                    // attach to the chart
                    qchart->addAxis(add, axisinfo->locate());
                }
                break;
            case AxisInfo::CATEGORY:
                {
                    if (!donecategory) {

                        donecategory=true;

                        QBarCategoryAxis *caxis = new QBarCategoryAxis(qchart);
                        add=caxis;

                        // add the bar series
                        if (!barseries) { barseries = new QBarSeries(); qchart->addSeries(barseries); }
                        else barseries->clear();

                        // add the new barsets
                        foreach (QBarSet *bs, barsets)
                            barseries->append(bs);

                        // attach before addig barseries
                        qchart->addAxis(add, axisinfo->locate());

                        // attach to category axis
                        barseries->attachAxis(caxis);

                        // category labels
                        for(int i=axisinfo->categories.count(); i<=axisinfo->maxx; i++)
                            axisinfo->categories << QString("%1").arg(i+1);
                        caxis->setCategories(axisinfo->categories);
                    }
                }
                break;
            }

            // at this point the basic settngs have been done and the axis
            // is attached to the chart, so we can go ahead and apply common settings
            if (add) {

                // once we've done the basics, lets do the aesthetics
                if (axisinfo->name != "x" && axisinfo->name != "y")  // equivalent to being blank
                    add->setTitleText(axisinfo->name);
                add->setLinePenColor(axisinfo->axiscolor);
                add->setLabelsColor(axisinfo->labelcolor);
                add->setTitleBrush(QBrush(axisinfo->labelcolor));

                // add the series that are associated with this
                foreach(QAbstractSeries *series, axisinfo->series)
                    series->attachAxis(add);
            }
        }
    }

    if (charttype ==GC_CHART_PIE) {
        // pie, never want a legend
        qchart->legend()->setVisible(false);
    }

    // barseries special case
    if (charttype==GC_CHART_BAR && barseries) {

        // need to attach barseries to the value axes
        foreach(QAbstractAxis *axis, qchart->axes(Qt::Vertical))
            barseries->attachAxis(axis);
    }
}

bool
PythonChart::configAxis(QString name, bool visible, int align, double min, double max,
                      int type, QString labelcolor, QString color, bool log, QStringList categories)
{
    AxisInfo *axis = axisinfos.value(name);
    if (axis == NULL) return false;

    // lets update the settings then
    axis->visible = visible;

    // -1 if not passed
    if (align == 0) axis->align = Qt::AlignBottom;
    if (align == 1) axis->align = Qt::AlignLeft;
    if (align == 2) axis->align = Qt::AlignTop;
    if (align == 3) axis->align = Qt::AlignRight;

    // -1 if not passed
    if (min != -1)  axis->minx = axis->miny = min;
    if (max != -1)  axis->maxx = axis->maxy = max;

    // type
    if (type != -1) axis->type = static_cast<AxisInfo::AxisInfoType>(type);

    // color
    if (labelcolor != "") axis->labelcolor=QColor(labelcolor);
    if (color != "") axis->axiscolor=QColor(color);

    // log .. hmmm
    axis->log = log;

    // categories
    if (categories.count()) axis->categories = categories;
    return true;
}
