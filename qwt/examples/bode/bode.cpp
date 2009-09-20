#include <qregexp.h>
#include <qapplication.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qstatusbar.h>
#include <qprinter.h>
#include <qpicture.h>
#include <qpainter.h>
#include <qfiledialog.h>
#if QT_VERSION >= 0x040300
#ifdef QT_SVG_LIB
#include <qsvggenerator.h>
#endif
#endif
#if QT_VERSION >= 0x040000
#include <qprintdialog.h>
#include <qfileinfo.h>
#else
#include <qwt_painter.h>
#endif
#include <qwt_counter.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_panner.h>
#include <qwt_text.h>
#include <qwt_math.h>
#include "pixmaps.h"
#include "bode_plot.h"
#include "bode.h"

class Zoomer: public QwtPlotZoomer
{
public:
    Zoomer(int xAxis, int yAxis, QwtPlotCanvas *canvas):
        QwtPlotZoomer(xAxis, yAxis, canvas)
    {
        setSelectionFlags(QwtPicker::DragSelection | QwtPicker::CornerToCorner);
        setTrackerMode(QwtPicker::AlwaysOff);
        setRubberBand(QwtPicker::NoRubberBand);

        // RightButton: zoom out by 1
        // Ctrl+RightButton: zoom out to full size

#if QT_VERSION < 0x040000
        setMousePattern(QwtEventPattern::MouseSelect2,
            Qt::RightButton, Qt::ControlButton);
#else
        setMousePattern(QwtEventPattern::MouseSelect2,
            Qt::RightButton, Qt::ControlModifier);
#endif
        setMousePattern(QwtEventPattern::MouseSelect3,
            Qt::RightButton);
    }
};

//-----------------------------------------------------------------
//
//      bode.cpp -- A demo program featuring QwtPlot and QwtCounter
//
//      This example demonstrates the mapping of different curves
//      to different axes in a QwtPlot widget. It also shows how to
//      display the cursor position and how to implement zooming.
//
//-----------------------------------------------------------------

MainWin::MainWin(QWidget *parent): 
    QMainWindow(parent)
{
    d_plot = new BodePlot(this);
    d_plot->setMargin(5);

#if QT_VERSION >= 0x040000
    setContextMenuPolicy(Qt::NoContextMenu);
#endif

    d_zoomer[0] = new Zoomer( QwtPlot::xBottom, QwtPlot::yLeft, 
        d_plot->canvas());
    d_zoomer[0]->setRubberBand(QwtPicker::RectRubberBand);
    d_zoomer[0]->setRubberBandPen(QColor(Qt::green));
    d_zoomer[0]->setTrackerMode(QwtPicker::ActiveOnly);
    d_zoomer[0]->setTrackerPen(QColor(Qt::white));

    d_zoomer[1] = new Zoomer(QwtPlot::xTop, QwtPlot::yRight,
         d_plot->canvas());
    
    d_panner = new QwtPlotPanner(d_plot->canvas());
    d_panner->setMouseButton(Qt::MidButton);

    d_picker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
        QwtPicker::PointSelection | QwtPicker::DragSelection, 
        QwtPlotPicker::CrossRubberBand, QwtPicker::AlwaysOn, 
        d_plot->canvas());
    d_picker->setRubberBandPen(QColor(Qt::green));
    d_picker->setRubberBand(QwtPicker::CrossRubberBand);
    d_picker->setTrackerPen(QColor(Qt::white));

    setCentralWidget(d_plot);

    QToolBar *toolBar = new QToolBar(this);

    QToolButton *btnZoom = new QToolButton(toolBar);
#if QT_VERSION >= 0x040000
    btnZoom->setText("Zoom");
    btnZoom->setIcon(QIcon(zoom_xpm));
    btnZoom->setCheckable(true);
    btnZoom->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
#else
    btnZoom->setTextLabel("Zoom");
    btnZoom->setPixmap(zoom_xpm);
    btnZoom->setToggleButton(true);
    btnZoom->setUsesTextLabel(true);
#endif

    QToolButton *btnPrint = new QToolButton(toolBar);
#if QT_VERSION >= 0x040000
    btnPrint->setText("Print");
    btnPrint->setIcon(QIcon(print_xpm));
    btnPrint->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
#else
    btnPrint->setTextLabel("Print");
    btnPrint->setPixmap(print_xpm);
    btnPrint->setUsesTextLabel(true);
#endif

#if QT_VERSION < 0x040000 
    QToolButton *btnSVG = new QToolButton(toolBar);
    btnSVG->setTextLabel("SVG");
    btnSVG->setPixmap(print_xpm);
    btnSVG->setUsesTextLabel(true);
#elif QT_VERSION >= 0x040300
#ifdef QT_SVG_LIB
    QToolButton *btnSVG = new QToolButton(toolBar);
    btnSVG->setText("SVG");
    btnSVG->setIcon(QIcon(print_xpm));
    btnSVG->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
#endif
#endif

#if QT_VERSION >= 0x040000
    toolBar->addWidget(btnZoom);
    toolBar->addWidget(btnPrint);
#if QT_VERSION >= 0x040300
#ifdef QT_SVG_LIB
    toolBar->addWidget(btnSVG);
#endif
#endif
#endif
    toolBar->addSeparator();

    QWidget *hBox = new QWidget(toolBar);

    QHBoxLayout *layout = new QHBoxLayout(hBox);
    layout->setSpacing(0);
    layout->addWidget(new QWidget(hBox), 10); // spacer
    layout->addWidget(new QLabel("Damping Factor", hBox), 0);
    layout->addSpacing(10);

    QwtCounter *cntDamp = new QwtCounter(hBox);
    cntDamp->setRange(0.0, 5.0, 0.01);
    cntDamp->setValue(0.0);
    
    layout->addWidget(cntDamp, 0);

#if QT_VERSION >= 0x040000
    (void)toolBar->addWidget(hBox);
#else
    toolBar->setStretchableWidget(hBox);
#endif

    addToolBar(toolBar);
#ifndef QT_NO_STATUSBAR
    (void)statusBar();
#endif

    enableZoomMode(false);
    showInfo();

    connect(cntDamp, SIGNAL(valueChanged(double)), 
        d_plot, SLOT(setDamp(double))); 

    connect(btnPrint, SIGNAL(clicked()), SLOT(print()));
#if QT_VERSION < 0x040000 
    connect(btnSVG, SIGNAL(clicked()), SLOT(exportSVG()));
#elif QT_VERSION >= 0x040300
#ifdef QT_SVG_LIB
    connect(btnSVG, SIGNAL(clicked()), SLOT(exportSVG()));
#endif
#endif
    connect(btnZoom, SIGNAL(toggled(bool)), SLOT(enableZoomMode(bool)));

    connect(d_picker, SIGNAL(moved(const QPoint &)),
            SLOT(moved(const QPoint &)));
    connect(d_picker, SIGNAL(selected(const QwtPolygon &)),
            SLOT(selected(const QwtPolygon &)));
}

void MainWin::print()
{
#if 1
    QPrinter printer;
#else
    QPrinter printer(QPrinter::HighResolution);
#if QT_VERSION < 0x040000
    printer.setOutputToFile(true);
    printer.setOutputFileName("/tmp/bode.ps");
    printer.setColorMode(QPrinter::Color);
#else
    printer.setOutputFileName("/tmp/bode.pdf");
#endif
#endif

    QString docName = d_plot->title().text();
    if ( !docName.isEmpty() )
    {
        docName.replace (QRegExp (QString::fromLatin1 ("\n")), tr (" -- "));
        printer.setDocName (docName);
    }

    printer.setCreator("Bode example");
    printer.setOrientation(QPrinter::Landscape);

#if QT_VERSION >= 0x040000
    QPrintDialog dialog(&printer);
    if ( dialog.exec() )
    {
#else
    if (printer.setup())
    {
#endif
        QwtPlotPrintFilter filter;
        if ( printer.colorMode() == QPrinter::GrayScale )
        {
            int options = QwtPlotPrintFilter::PrintAll;
            options &= ~QwtPlotPrintFilter::PrintBackground;
            options |= QwtPlotPrintFilter::PrintFrameWithScales;
            filter.setOptions(options);
        }
        d_plot->print(printer, filter);
    }
}

void MainWin::exportSVG()
{
    QString fileName = "bode.svg";

#if QT_VERSION < 0x040000

#ifndef QT_NO_FILEDIALOG
    fileName = QFileDialog::getSaveFileName(
        "bode.svg", "SVG Documents (*.svg)", this);
#endif
    if ( !fileName.isEmpty() )
    {
        // enable workaround for Qt3 misalignments
        QwtPainter::setSVGMode(true); 

        QPicture picture;

        QPainter p(&picture);
        d_plot->print(&p, QRect(0, 0, 800, 600));
        p.end();

        picture.save(fileName, "svg");
    }

#elif QT_VERSION >= 0x040300

#ifdef QT_SVG_LIB
#ifndef QT_NO_FILEDIALOG
    fileName = QFileDialog::getSaveFileName(
        this, "Export File Name", QString(),
        "SVG Documents (*.svg)");
#endif
    if ( !fileName.isEmpty() )
    {
        QSvgGenerator generator;
        generator.setFileName(fileName);
        generator.setSize(QSize(800, 600));

        d_plot->print(generator);
    }
#endif
#endif
}

void MainWin::enableZoomMode(bool on)
{
    d_panner->setEnabled(on);

    d_zoomer[0]->setEnabled(on);
    d_zoomer[0]->zoom(0);

    d_zoomer[1]->setEnabled(on);
    d_zoomer[1]->zoom(0);

    d_picker->setEnabled(!on);

    showInfo();
}

void MainWin::showInfo(QString text)
{
    if ( text == QString::null )
    {
        if ( d_picker->rubberBand() )
            text = "Cursor Pos: Press left mouse button in plot region";
        else
            text = "Zoom: Press mouse button and drag";
    }

#ifndef QT_NO_STATUSBAR
#if QT_VERSION >= 0x040000
    statusBar()->showMessage(text);
#else
    statusBar()->message(text);
#endif
#endif
}

void MainWin::moved(const QPoint &pos)
{
    QString info;
    info.sprintf("Freq=%g, Ampl=%g, Phase=%g",
        d_plot->invTransform(QwtPlot::xBottom, pos.x()),
        d_plot->invTransform(QwtPlot::yLeft, pos.y()),
        d_plot->invTransform(QwtPlot::yRight, pos.y())
    );
    showInfo(info);
}

void MainWin::selected(const QwtPolygon &)
{
    showInfo();
}

int main (int argc, char **argv)
{
    QApplication a(argc, argv);

    MainWin w;
#if QT_VERSION < 0x040000
    a.setMainWidget(&w);
#endif
    w.resize(540,400);
    w.show();

    int rv = a.exec();
    return rv;
}
