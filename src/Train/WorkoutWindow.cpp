/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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


#include "WorkoutWindow.h"
#include "WorkoutWidget.h"
#include "WorkoutWidgetItems.h"

static int MINTOOLHEIGHT = 350; // smaller than this, lose the toolbar

WorkoutWindow::WorkoutWindow(Context *context) :
    GcChartWindow(context), draw(true), context(context), active(false), recording(false)
{
    setContentsMargins(0,0,0,0);
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));

    setControls(NULL);
    ergFile = NULL;
    format = 0;

    QVBoxLayout *main = new QVBoxLayout;
    QHBoxLayout *layout = new QHBoxLayout;
    QVBoxLayout *codeLayout = new QVBoxLayout;
    QVBoxLayout *editor = new QVBoxLayout;
    setChartLayout(main);

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // the workout scene
    workout = new WorkoutWidget(this, context);

    // paint the TTE curve
    mmp = new WWMMPCurve(workout);

    // add a line between the dots
    line = new WWLine(workout);

    // block cursos
    bcursor = new WWBlockCursor(workout);

    // block selection
    brect = new WWBlockSelection(workout);

    // paint the W'bal curve
    wbline = new WWWBLine(workout, context);

    // telemetry
    telemetry = new WWTelemetry(workout, context);

    // add the power, W'bal scale
    powerscale = new WWPowerScale(workout, context);
    wbalscale = new WWWBalScale(workout, context);

    // lap markers
    lap = new WWLap(workout);

    // tte warning bar at bottom
    tte = new WWTTE(workout);

    // selection tool
    rect = new WWRect(workout);

    // guides always on top!
    guide = new WWSmartGuide(workout);

    // recording ...
    now = new WWNow(workout, context);

    // scroller, hidden until needed
    scroll = new QScrollBar(Qt::Horizontal, this);
    scroll->hide();

    // setup the toolbar
    toolbar = new QToolBar(this);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolbar->setFloatable(true);
    toolbar->setIconSize(QSize(18 *dpiXFactor,18 *dpiYFactor));

    newAct = new QAction("ERG", this);
    QAction *newMrcAct = new QAction("MRC", this);
    connect(newAct, SIGNAL(triggered()), this, SLOT(newErgFile()));
    connect(newMrcAct, SIGNAL(triggered()), this, SLOT(newMrcFile()));

    QMenu *menu = new QMenu();
    menu->addAction(newAct);
    menu->addAction(newMrcAct);

    QIcon newIcon(":images/toolbar/new doc.png");
    QToolButton *toolButton = new QToolButton();
    toolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolButton->setText(tr("New"));
    toolButton->setIcon(newIcon);
    toolButton->setMenu(menu);
    toolButton->setPopupMode(QToolButton::InstantPopup);
    toolbar->addWidget(toolButton);

    QIcon saveIcon(":images/toolbar/save.png");
    saveAct = new QAction(saveIcon, tr("Save"), this);
    connect(saveAct, SIGNAL(triggered()), this, SLOT(saveFile()));
    toolbar->addAction(saveAct);

    QIcon saveAsIcon(":images/toolbar/saveas.png");
    saveAsAct = new QAction(saveAsIcon, tr("Save As"), this);
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));
    toolbar->addAction(saveAsAct);

    toolbar->addSeparator();

    //XXX TODO
    //XXXHelpWhatsThis *helpToolbar = new HelpWhatsThis(toolbar);
    //XXXtoolbar->setWhatsThis(helpToolbar->getWhatsThisText(HelpWhatsThis::ChartRides_Editor));

    // undo and redo deliberately at a distance from the
    // save icon, since accidentally hitting the wrong
    // icon in that instance would be horrible
    QIcon undoIcon(":images/toolbar/undo.png");
    undoAct = new QAction(undoIcon, tr("Undo"), this);
    connect(undoAct, SIGNAL(triggered()), workout, SLOT(undo()));
    toolbar->addAction(undoAct);

    QIcon redoIcon(":images/toolbar/redo.png");
    redoAct = new QAction(redoIcon, tr("Redo"), this);
    connect(redoAct, SIGNAL(triggered()), workout, SLOT(redo()));
    toolbar->addAction(redoAct);
    
    toolbar->addSeparator();

    QIcon drawIcon(":images/toolbar/edit.png");
    drawAct = new QAction(drawIcon, tr("Draw"), this);
    connect(drawAct, SIGNAL(triggered()), this, SLOT(drawMode()));
    toolbar->addAction(drawAct);

    QIcon selectIcon(":images/toolbar/select.png");
    selectAct = new QAction(selectIcon, tr("Select"), this);
    connect(selectAct, SIGNAL(triggered()), this, SLOT(selectMode()));
    toolbar->addAction(selectAct);

    selectAct->setEnabled(true);
    drawAct->setEnabled(false);

    toolbar->addSeparator();

    QIcon cutIcon(":images/toolbar/cut.png");
    cutAct = new QAction(cutIcon, tr("Cut"), this);
    cutAct->setEnabled(true);
    toolbar->addAction(cutAct);
    connect(cutAct, SIGNAL(triggered()), workout, SLOT(cut()));

    QIcon copyIcon(":images/toolbar/copy.png");
    copyAct = new QAction(copyIcon, tr("Copy"), this);
    copyAct->setEnabled(true);
    toolbar->addAction(copyAct);
    connect(copyAct, SIGNAL(triggered()), workout, SLOT(copy()));

    QIcon pasteIcon(":images/toolbar/paste.png");
    pasteAct = new QAction(pasteIcon, tr("Paste"), this);
    pasteAct->setEnabled(false);
    toolbar->addAction(pasteAct);
    connect(pasteAct, SIGNAL(triggered()), workout, SLOT(paste()));

    toolbar->addSeparator();

    QIcon propertiesIcon(":images/toolbar/properties.png");
    propertiesAct = new QAction(propertiesIcon, tr("Properties"), this);
    connect(propertiesAct, SIGNAL(triggered()), this, SLOT(properties()));
    toolbar->addAction(propertiesAct);

    QIcon zoomInIcon(":images/toolbar/zoom in.png");
    zoomInAct = new QAction(zoomInIcon, tr("Zoom In"), this);
    connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));
    toolbar->addAction(zoomInAct);

    QIcon zoomOutIcon(":images/toolbar/zoom out.png");
    zoomOutAct = new QAction(zoomOutIcon, tr("Zoom Out"), this);
    connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));
    toolbar->addAction(zoomOutAct);

    // stretch the labels to the right hand side
    QWidget *empty = new QWidget(this);
    empty->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    toolbar->addWidget(empty);


    xlabel = new QLabel("00:00");
    toolbar->addWidget(xlabel);

    ylabel = new QLabel("150w");
    toolbar->addWidget(ylabel);

    IFlabel = new QLabel("0 Intensity");
    toolbar->addWidget(IFlabel);

    TSSlabel = new QLabel("0 Stress");
    toolbar->addWidget(TSSlabel);

#if 0 // not yet!
    // get updates..
    connect(context, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));
    telemetryUpdate(RealtimeData());
#endif

    codeContainer = new QWidget;
    codeContainer->setLayout(codeLayout);
    codeContainer->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    codeContainer->hide();

    // erg/mrc format
    codeFormat = new QLabel(tr("Format: %1").arg("ERG"));
    codeFormat->setStyleSheet("QLabel { color : white; }");

    // editing the code...
    code = new CodeEditor(this);
    code->setContextMenuPolicy(Qt::NoContextMenu); // no context menu
    code->installEventFilter(this); // filter the undo/redo stuff

    // WATTS and Duration for the cursor
    main->addWidget(toolbar);
    editor->addWidget(workout);
    editor->addWidget(scroll);
    layout->addLayout(editor);
    codeLayout->addWidget(codeFormat);
    codeLayout->addWidget(code);
    layout->addWidget(codeContainer);
    main->addLayout(layout);

    // make it look right
    saveAct->setEnabled(false);
    undoAct->setEnabled(false);
    redoAct->setEnabled(false);

    // watch for erg file selection
    connect(context, SIGNAL(ergFileSelected(ErgFile*)), this, SLOT(ergFileSelected(ErgFile*)));

    // watch for erg run/stop
    connect(context, SIGNAL(start()), this, SLOT(start()));
    connect(context, SIGNAL(stop()), this, SLOT(stop()));

    // text changed
    connect(code, SIGNAL(textChanged()), this, SLOT(qwkcodeChanged()));
    connect(code, SIGNAL(cursorPositionChanged()), workout, SLOT(hoverQwkcode()));

    // scrollbar
    connect(scroll, SIGNAL(sliderMoved(int)), this, SLOT(scrollMoved()));

    // set the widgets etc
    configChanged(CONFIG_APPEARANCE);
}

void
WorkoutWindow::resizeEvent(QResizeEvent *)
{
    // show or hide toolbar if big enough
    if (!recording && height() > MINTOOLHEIGHT) toolbar->show();
    else toolbar->hide();
}

void
WorkoutWindow::configChanged(qint32)
{
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));
    QFontMetrics fm(workout->bigFont);
    xlabel->setFont(workout->bigFont);
    ylabel->setFont(workout->bigFont);
    IFlabel->setFont(workout->bigFont);
    TSSlabel->setFont(workout->bigFont);
    IFlabel->setFixedWidth(fm.boundingRect(" 0.85 Intensity ").width());
    TSSlabel->setFixedWidth(fm.boundingRect(" 100 Stress ").width());
    xlabel->setFixedWidth(fm.boundingRect(" 00:00:00 ").width());
    ylabel->setFixedWidth(fm.boundingRect(" 1000w ").width());

    scroll->setStyleSheet(TabView::ourStyleSheet());
    toolbar->setStyleSheet(QString("::enabled { background: %1; color: %2; border: 0px; } ")
                           .arg(GColor(CTRAINPLOTBACKGROUND).name())
                           .arg(GCColor::invertColor(GColor(CTRAINPLOTBACKGROUND)).name()));

    xlabel->setStyleSheet("color: darkGray;");
    ylabel->setStyleSheet("color: darkGray;");
    TSSlabel->setStyleSheet("color: darkGray;");
    IFlabel->setStyleSheet("color: darkGray;");

    // maximum of 20 characters per line ?
    QFont f;
    QFontMetrics ff(f);
    code->setFixedWidth(ff.boundingRect("99x999s@999-999r999s@999-999").width()+(20* dpiXFactor));

    // text edit colors
    QPalette palette;
    palette.setColor(QPalette::Window, GColor(CTRAINPLOTBACKGROUND));
    palette.setColor(QPalette::Background, GColor(CTRAINPLOTBACKGROUND));

    // only change base if moved away from white plots
    // which is a Mac thing
#ifndef Q_OS_MAC
    if (GColor(CTRAINPLOTBACKGROUND) != Qt::white)
#endif
    {
        //palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CTRAINPLOTBACKGROUND)));
        palette.setColor(QPalette::Base, GColor(CTRAINPLOTBACKGROUND));
        palette.setColor(QPalette::Window, GColor(CTRAINPLOTBACKGROUND));
    }

#ifndef Q_OS_MAC // the scrollers appear when needed on Mac, we'll keep that
    code->setStyleSheet(TabView::ourStyleSheet());
#endif

    palette.setColor(QPalette::WindowText, GCColor::invertColor(GColor(CTRAINPLOTBACKGROUND)));
    palette.setColor(QPalette::Text, GCColor::invertColor(GColor(CTRAINPLOTBACKGROUND)));
    code->setPalette(palette);
    repaint();
}

bool
WorkoutWindow::eventFilter(QObject *obj, QEvent *event)
{
    bool returning=false;

    // we only filter out keyboard shortcuts for undo redo etc
    // in the qwkcode editor, anything else is of no interest.
    if (obj != code) return returning;

    if (event->type() == QEvent::KeyPress) {

        // we care about cmd / ctrl
        Qt::KeyboardModifiers kmod = static_cast<QInputEvent*>(event)->modifiers();
        bool ctrl = (kmod & Qt::ControlModifier) != 0;

        switch(static_cast<QKeyEvent*>(event)->key()) {

        case Qt::Key_Y:
            if (ctrl) {
                workout->redo();
                returning = true; // we grab all key events
            }
            break;

        case Qt::Key_Z:
            if (ctrl) {
                workout->undo();
                returning=true;
            }
            break;

        }

    }
    return returning;
}

void
WorkoutWindow::zoomIn()
{
    setScroller(workout->zoomIn());
}

void
WorkoutWindow::zoomOut()
{
    setScroller(workout->zoomOut());
}

void
WorkoutWindow::setScroller(QPointF v)
{
    double minVX=v.x();
    double maxVX=v.y();

    // do we even need it ?
    double vwidth = maxVX - minVX;
    if (vwidth >= workout->maxWX()) {
        // it needs to be hidden, the view fits
        scroll->hide();
    } else {
        // we need it, so set to right place
        scroll->setMinimum(0);
        scroll->setMaximum(workout->maxWX() - vwidth);
        scroll->setPageStep(vwidth);
        scroll->setValue(minVX);

        // and show
        scroll->show();
    }
}

void
WorkoutWindow::scrollMoved()
{
    // is it visible!?
    if (!scroll->isVisible()) return;

    // just move the view as needed
    double minVX = scroll->value();

    // now set the view
    workout->setMinVX(minVX);
    workout->setMaxVX(minVX + scroll->pageStep());

    // bleck
    workout->setBlockCursor();

    // repaint
    workout->update();
}

void
WorkoutWindow::ergFileSelected(ErgFile*f, int format)
{
    if (active) return;

    if (workout->isDirty()) {
        QMessageBox msgBox;
        msgBox.setText(tr("You have unsaved changes to a workout."));
        msgBox.setInformativeText(tr("Do you want to save them?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();

        // save first, otherwise changes lost
        if(msgBox.clickedButton() == msgBox.button(QMessageBox::Yes)) {
            active = true;
            saveFile();
            active = false;
        }
    }

    // just get on with it.
    format = f ? f->format : format;
    QString formatText = tr("Format: %1");
    if (format == MRC) codeFormat->setText(formatText.arg("MRC"));
    else codeFormat->setText(formatText.arg("ERG"));

    ergFile = f;
    this->format = format;
    workout->ergFileSelected(f, format);

    // almost certainly hides it on load
    setScroller(QPointF(workout->minVX(), workout->maxVX()));
}

void
WorkoutWindow::newErgFile()
{
    // new blank file clear points .. texts .. metadata etc
    ergFileSelected(NULL, ERG);
}


void
WorkoutWindow::newMrcFile()
{
    // new blank file clear points .. texts .. metadata etc
    ergFileSelected(NULL, MRC);
}

void
WorkoutWindow::saveAs()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Workout File"),
                                                    appsettings->value(this, GC_WORKOUTDIR, "").toString(),
                                                    "ERG workout (*.erg);;MRC workout (*.mrc);;Zwift workout (*.zwo)");

    // if they didn't select, give up.
    if (filename.isEmpty()) {
        return;
    }

    // New ergfile will be created almost empty
    ErgFile *newergFile = new ErgFile(context);

    // we need to set sensible defaults for
    // all the metadata in the file.
    newergFile->Version = "2.0";
    newergFile->Units = "";
    newergFile->Filename = QFileInfo(filename).fileName();
    newergFile->filename = filename;
    newergFile->Name = "New Workout";
    newergFile->Ftp = newergFile->CP;
    newergFile->valid = true;
    newergFile->format = format;

    // if we're save as from an existing keep all the data
    // EXCEPT filename, which has just been changed!
    if (ergFile) newergFile->setFrom(ergFile);

    // Update with whatever is currently in editor
    workout->updateErgFile(newergFile);

    // select new workout
    workout->ergFileSelected(newergFile);

    // write file
    workout->save();

    // add to collection with new name, a single new file
    Library::importFiles(context, QStringList(filename));
}

void
WorkoutWindow::saveFile()
{
    // if ergFile is null we need to save as, with the current
    // edit state being held by the workout editor
    // otherwise just write it to disk straight away as its
    // an existing ergFile we just need to write to
    if (ergFile) workout->save();
    else saveAs();

    // force any other plots to take the changes
    context->notifyErgFileSelected(ergFile);
}

void
WorkoutWindow::properties()
{
    // metadata etc -- needs a dialog
    codeContainer->setHidden(!codeContainer->isHidden());
}

void
WorkoutWindow::qwkcodeChanged()
{
    workout->fromQwkcode(code->document()->toPlainText());
}

void
WorkoutWindow::drawMode()
{
    draw = true;
    drawAct->setEnabled(false);
    selectAct->setEnabled(true);
}

void
WorkoutWindow::selectMode()
{
    draw = false;
    drawAct->setEnabled(true);
    selectAct->setEnabled(false);
}


void
WorkoutWindow::start()
{
    recording = true;
    scroll->hide();
    toolbar->hide();
    codeContainer->hide();
    workout->start();
}

void
WorkoutWindow::stop()
{
    recording = false;
    if (height() > MINTOOLHEIGHT) toolbar->show();
    workout->stop();
}
