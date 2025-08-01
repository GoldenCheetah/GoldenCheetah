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
#include "HelpWhatsThis.h"

static int MINTOOLHEIGHT = 350; // smaller than this, lose the toolbar

WorkoutWindow::WorkoutWindow(Context *context) :
    GcChartWindow(context), draw(true), context(context), active(false), recording(false)
{
    HelpWhatsThis *helpContents = new HelpWhatsThis(this);
    this->setWhatsThis(helpContents->getWhatsThisText(HelpWhatsThis::ChartTrain_WorkoutEditor));

    setContentsMargins(0,0,0,0);
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));

    //
    // Chart settings
    //

    QWidget *settingsWidget = new QWidget(this);
    HelpWhatsThis *helpConfig = new HelpWhatsThis(settingsWidget);
    settingsWidget->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::ChartTrain_WorkoutEditor));
    settingsWidget->setContentsMargins(0,0,0,0);

    QGridLayout *gridLayout = new QGridLayout(settingsWidget);


    plotHrCB = new QCheckBox();
    plotPwrCB = new QCheckBox();
    plotCadenceCB = new QCheckBox();
    plotWbalCB = new QCheckBox();
    plotVo2CB = new QCheckBox();
    plotVentilationCB = new QCheckBox();
    plotSpeedCB = new QCheckBox();
    plotCoreTempCB = new QCheckBox();
    plotSkinTempCB = new QCheckBox();
    plotHSICB = new QCheckBox();

    plotHrSB = new QSpinBox(); plotHrSB->setMinimum(1);
    plotPwrSB = new QSpinBox(); plotPwrSB->setMinimum(1);
    plotCadenceSB = new QSpinBox(); plotCadenceSB->setMinimum(1);
    plotVo2SB = new QSpinBox(); plotVo2SB->setMinimum(1);
    plotVentilationSB = new QSpinBox(); plotVentilationSB->setMinimum(1);
    plotSpeedSB = new QSpinBox(); plotSpeedSB->setMinimum(1);

    // enable all plots by default. gets overriden by the corresponding property
    // stored in the layout xml.
    plotHrCB->setChecked(true);
    plotPwrCB->setChecked(true);
    plotCadenceCB->setChecked(true);
    plotWbalCB->setChecked(true);
    plotVo2CB->setChecked(true);
    plotVentilationCB->setChecked(true);
    plotSpeedCB->setChecked(true);

    connect(plotHrSB, SIGNAL(valueChanged(int)), this, SLOT(plotHrAvgChanged(int)));
    connect(plotPwrSB, SIGNAL(valueChanged(int)), this, SLOT(plotPwrAvgChanged(int)));
    connect(plotCadenceSB, SIGNAL(valueChanged(int)), this, SLOT(plotCadenceAvgChanged(int)));
    connect(plotVo2SB, SIGNAL(valueChanged(int)), this, SLOT(plotVo2AvgChanged(int)));
    connect(plotVentilationSB, SIGNAL(valueChanged(int)), this, SLOT(plotVentilationAvgChanged(int)));
    connect(plotSpeedSB, SIGNAL(valueChanged(int)), this, SLOT(plotSpeedAvgChanged(int)));


    int row = 0;
    gridLayout->addWidget(new QLabel(tr("Show Heartrate")),row,0);
    gridLayout->addWidget(plotHrCB,row,1);
    gridLayout->addWidget(new QLabel(tr("Seconds to average")),row,2);
    gridLayout->addWidget(plotHrSB,row++,3);

    gridLayout->addWidget(new QLabel(tr("Show Power")),row,0);
    gridLayout->addWidget(plotPwrCB,row,1);
    gridLayout->addWidget(new QLabel(tr("Seconds to average")),row,2);
    gridLayout->addWidget(plotPwrSB,row++,3);

    gridLayout->addWidget(new QLabel(tr("Show Cadence")),row,0);
    gridLayout->addWidget(plotCadenceCB,row,1);
    gridLayout->addWidget(new QLabel(tr("Seconds to average")),row,2);
    gridLayout->addWidget(plotCadenceSB,row++,3);

    gridLayout->addWidget(new QLabel(tr("Show VO2")),row,0);
    gridLayout->addWidget(plotVo2CB,row,1);
    gridLayout->addWidget(new QLabel(tr("Seconds to average")),row,2);
    gridLayout->addWidget(plotVo2SB,row++,3);

    gridLayout->addWidget(new QLabel(tr("Show Ventilation")),row,0);
    gridLayout->addWidget(plotVentilationCB,row,1);
    gridLayout->addWidget(new QLabel(tr("Seconds to average")),row,2);
    gridLayout->addWidget(plotVentilationSB,row++,3);

    gridLayout->addWidget(new QLabel(tr("Show Speed")),row,0);
    gridLayout->addWidget(plotSpeedCB,row,1);
    gridLayout->addWidget(new QLabel(tr("Seconds to average")),row,2);
    gridLayout->addWidget(plotSpeedSB,row++,3);

    gridLayout->addWidget(new QLabel(tr("Show WBal")),row,0);
    gridLayout->addWidget(plotWbalCB,row++,1);

    gridLayout->addWidget(new QLabel(tr("Show Core Temp")),row,0);
    gridLayout->addWidget(plotCoreTempCB,row++,1);
    gridLayout->addWidget(new QLabel(tr("Show Skin Temp")),row,0);
    gridLayout->addWidget(plotSkinTempCB,row++,1);
    gridLayout->addWidget(new QLabel(tr("Show Heat Strain Index")),row,0);
    gridLayout->addWidget(plotHSICB,row++,1);

    gridLayout->addItem(new QSpacerItem(1,1,QSizePolicy::Preferred,QSizePolicy::Expanding),row,0);

    setControls(settingsWidget);
    ergFile = NULL;
    format = ErgFileFormat::unknown;

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

    newAct = new QAction(tr("ERG - Absolute Watts"), this);
    QAction *newMrcAct = new QAction(tr("MRC - Relative Watts"), this);
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
    codeFormat = new QLabel(tr("ERG - Absolute Watts"));
    codeFormat->setStyleSheet("QLabel { color : white; }");

    coalescedSections = new QLabel(tr("Coalesced sections of same wattage"));
    coalescedSections->setStyleSheet("QLabel { color : white; }");
    coalescedSections->setVisible(ergFile != nullptr && ergFile->hasCoalescedSections());

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
    codeLayout->addWidget(coalescedSections);
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
    if (!recording && height() > (MINTOOLHEIGHT*dpiYFactor)) toolbar->show();
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

    scroll->setStyleSheet(AbstractView::ourStyleSheet());
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
    code->setStyleSheet(AbstractView::ourStyleSheet());
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
WorkoutWindow::ergFileSelected(ErgFile*f, ErgFileFormat format)
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
    format = f ? f->format() : format;
    if (format == ErgFileFormat::mrc) codeFormat->setText(tr("MRC - Relative Watts"));
    else codeFormat->setText(tr("ERG - Absolute Watts"));

    ergFile = f;
    this->format = format;
    workout->ergFileSelected(f, format);
    coalescedSections->setVisible(f != nullptr && ergFile->hasCoalescedSections());

    // almost certainly hides it on load
    setScroller(QPointF(workout->minVX(), workout->maxVX()));
}

void
WorkoutWindow::newErgFile()
{
    // new blank file clear points .. texts .. metadata etc
    ergFileSelected(NULL, ErgFileFormat::erg);
}


void
WorkoutWindow::newMrcFile()
{
    // new blank file clear points .. texts .. metadata etc
    ergFileSelected(NULL, ErgFileFormat::mrc);
}

void
WorkoutWindow::saveAs()
{
    QString selected = format == ErgFileFormat::mrc ? "MRC workout (*.mrc)" : "ERG workout (*.erg)";
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Workout File"),
                                                    appsettings->value(this, GC_WORKOUTDIR, "").toString(),
                                                    "ERG workout (*.erg);;MRC workout (*.mrc);;Zwift workout (*.zwo)",
                                                    &selected);

    // if they didn't select, give up.
    if (filename.isEmpty()) {
        return;
    }

    // filetype defaults to .erg
    if(!filename.endsWith(".erg") && !filename.endsWith(".mrc") && !filename.endsWith(".zwo")) {
        if (format == ErgFileFormat::mrc) filename.append(".mrc");
        else filename.append(".erg");
    }

    // New ergfile will be created almost empty
    ErgFile *newergFile = new ErgFile(context);

    // we need to set sensible defaults for
    // all the metadata in the file.
    newergFile->version("2.0");
    newergFile->units("");
    newergFile->originalFilename(QFileInfo(filename).fileName());
    newergFile->filename(filename);
    newergFile->name("New Workout");
    newergFile->ftp(newergFile->CP());
    newergFile->valid = true;
    newergFile->format(format);

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

void WorkoutWindow::setShouldPlotHr(bool value)
{
    plotHrCB->setChecked(value);
}

void WorkoutWindow::setShouldPlotPwr(bool value)
{
    plotPwrCB->setChecked(value);
}

void WorkoutWindow::setShouldPlotCadence(bool value)
{
    plotCadenceCB->setChecked(value);
}

void WorkoutWindow::setShouldPlotWbal(bool value)
{
    plotWbalCB->setChecked(value);
}

void WorkoutWindow::setShouldPlotVo2(bool value)
{
    plotVo2CB->setChecked(value);
}

void WorkoutWindow::setShouldPlotVentilation(bool value)
{
    plotVentilationCB->setChecked(value);
}

void WorkoutWindow::setShouldPlotSpeed(bool value)
{
    plotSpeedCB->setChecked(value);
}
void WorkoutWindow::setShouldPlotCoreTemp(bool value)
{
    plotCoreTempCB->setChecked(value);
}
void WorkoutWindow::setShouldPlotSkinTemp(bool value)
{
    plotSkinTempCB->setChecked(value);
}
void WorkoutWindow::setShouldPlotHSI(bool value)
{
    plotHSICB->setChecked(value);
}

int WorkoutWindow::hrPlotAvgLength()
{
    return plotHrSB->value();
}

int WorkoutWindow::pwrPlotAvgLength()
{
    return plotPwrSB->value();
}

int WorkoutWindow::cadencePlotAvgLength()
{
    return plotCadenceSB->value();
}

int WorkoutWindow::vo2PlotAvgLength()
{
  return plotVo2SB->value();
}

int WorkoutWindow::ventilationPlotAvgLength()
{
  return plotVentilationSB->value();
}

int WorkoutWindow::speedPlotAvgLength()
{
    return plotSpeedSB->value();
}

void WorkoutWindow::setPlotHrAvgLength(int value)
{
    plotHrSB->setValue(value);
}

void WorkoutWindow::setPlotPwrAvgLength(int value)
{
    plotPwrSB->setValue(value);
}

void WorkoutWindow::setPlotCadenceAvgLength(int value)
{
    plotCadenceSB->setValue(value);
}

void WorkoutWindow::setPlotVo2AvgLength(int value)
{
    plotVo2SB->setValue(value);
}

void WorkoutWindow::setPlotVentilationAvgLength(int value)
{
    plotVentilationSB->setValue(value);
}

void WorkoutWindow::setPlotSpeedAvgLength(int value)
{
    plotSpeedSB->setValue(value);
}

void
WorkoutWindow::plotHrAvgChanged(int)
{
    workout->hrAvg.clear();
}

void
WorkoutWindow::plotPwrAvgChanged(int)
{
    workout->pwrAvg.clear();
}

void
WorkoutWindow::plotCadenceAvgChanged(int)
{
    workout->cadenceAvg.clear();
}

void
WorkoutWindow::plotVo2AvgChanged(int)
{
    workout->vo2Avg.clear();
}

void
WorkoutWindow::plotVentilationAvgChanged(int)
{
    workout->ventilationAvg.clear();
}

void
WorkoutWindow::plotSpeedAvgChanged(int)
{
    workout->speedAvg.clear();
}
