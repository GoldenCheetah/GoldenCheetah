#include <qapplication.h>
#include <qframe.h>
#include <qwt_scale_map.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qcolor.h>
#include <qpainter.h>
#include <math.h>

//------------------------------------------------------------
//      curvdemo1
//
//  This example program features some of the different
//  display styles of the QwtPlotCurve class
//------------------------------------------------------------


//
//   Array Sizes
//
const int Size = 27;
#if QT_VERSION >= 0x040000
const int CurvCnt = 6;
#else
const int CurvCnt = 5;
#endif

//
//   Arrays holding the values
//
double xval[Size];
double yval[Size];
QwtScaleMap xMap; 
QwtScaleMap yMap;

class MainWin : public QFrame 
{
public:
    MainWin();
    
protected:
#if QT_VERSION >= 0x040000
    virtual void paintEvent(QPaintEvent *);
#endif
    void drawContents(QPainter *p);

private:
    void shiftDown(QRect &rect, int offset) const;

    QwtPlotCurve crv[CurvCnt];
};

MainWin::MainWin() 
{
    int i;

    xMap.setScaleInterval(-0.5, 10.5);
    yMap.setScaleInterval(-1.1, 1.1);

    //
    //  Frame style
    //  
    setFrameStyle(QFrame::Box|QFrame::Raised);
    setLineWidth(2);
    setMidLineWidth(3);

    //
    // Calculate values
    //
    for(i=0; i<Size;i++)
    {   xval[i] = double(i) * 10.0 / double(Size - 1);
        yval[i] = sin(xval[i]) * cos(2.0 * xval[i]);
    }
    
    //
    //  define curve styles
    // 
    QwtSymbol sym;
    i = 0;

    sym.setStyle(QwtSymbol::Cross);
    sym.setPen(QColor(Qt::black));
    sym.setSize(5);
    crv[i].setSymbol(sym);
    crv[i].setPen(QColor(Qt::darkGreen));
    crv[i].setStyle(QwtPlotCurve::Lines);
    crv[i].setCurveAttribute(QwtPlotCurve::Fitted);
    i++;

    sym.setStyle(QwtSymbol::Ellipse);
    sym.setPen(QColor(Qt::blue));
    sym.setBrush(QColor(Qt::yellow));
    sym.setSize(5);
    crv[i].setSymbol(sym);
    crv[i].setPen(QColor(Qt::red));
    crv[i].setStyle(QwtPlotCurve::Sticks);
    i++;

    crv[i].setPen(QColor(Qt::darkBlue));
    crv[i].setStyle(QwtPlotCurve::Lines);
    i++;

#if QT_VERSION >= 0x040000
    crv[i].setPen(QColor(Qt::darkBlue));
    crv[i].setStyle(QwtPlotCurve::Lines);
    crv[i].setRenderHint(QwtPlotItem::RenderAntialiased);
    i++;
#endif


    crv[i].setPen(QColor(Qt::darkCyan));
    crv[i].setStyle(QwtPlotCurve::Steps);
    i++;

    sym.setStyle(QwtSymbol::XCross);
    sym.setPen(QColor(Qt::darkMagenta));
    crv[i].setSymbol(sym);
    crv[i].setStyle(QwtPlotCurve::NoCurve);
    i++;


    //
    // attach data
    //
    for(i=0;i<CurvCnt;i++)
        crv[i].setRawData(xval,yval,Size);
}

void MainWin::shiftDown(QRect &rect, int offset) const
{
#if QT_VERSION < 0x040000
        rect.moveBy(0, offset);     
#else
        rect.translate(0, offset);     
#endif
}

#if QT_VERSION >= 0x040000
void MainWin::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);

    QPainter painter(this);
    painter.setClipRect(contentsRect());
    drawContents(&painter);
}
#endif


//
//  REDRAW CONTENTS
//
void MainWin::drawContents(QPainter *painter)
{
    int deltay,i;

    QRect r = contentsRect();

    deltay = r.height() / CurvCnt - 1;

    r.setHeight(deltay);

    //
    //  draw curves
    //
    for (i=0;i<CurvCnt;i++)
    {
        xMap.setPaintInterval(r.left(), r.right());
        yMap.setPaintInterval(r.top(), r.bottom());

#if QT_VERSION >= 0x040000
        painter->setRenderHint(QPainter::Antialiasing,
            crv[i].testRenderHint(QwtPlotItem::RenderAntialiased) );
#endif
        crv[i].draw(painter, xMap, yMap, r);

        shiftDown(r, deltay);
    }

    //
    // draw titles
    //
    r = contentsRect();     // reset r
    painter->setFont(QFont("Helvetica", 8));
    
    const int alignment = Qt::AlignTop|Qt::AlignHCenter;

    painter->setPen(Qt::black);

    painter->drawText(0,r.top(),r.width(), painter->fontMetrics().height(),
        alignment, "Style: Line/Fitted, Symbol: Cross");
    shiftDown(r, deltay);

    painter->drawText(0,r.top(),r.width(), painter->fontMetrics().height(),
        alignment, "Style: Sticks, Symbol: Ellipse");
    shiftDown(r, deltay);
    
    painter->drawText(0 ,r.top(),r.width(), painter->fontMetrics().height(),
        alignment, "Style: Lines, Symbol: None");
    shiftDown(r, deltay);

#if QT_VERSION >= 0x040000
    painter->drawText(0 ,r.top(),r.width(), painter->fontMetrics().height(),
        alignment, "Style: Lines, Symbol: None, Antialiased");
    shiftDown(r, deltay);
#endif
    
    
    painter->drawText(0, r.top(),r.width(), painter->fontMetrics().height(),
        alignment, "Style: Steps, Symbol: None");
    shiftDown(r, deltay);
    
    painter->drawText(0,r.top(),r.width(), painter->fontMetrics().height(),
        alignment, "Style: NoCurve, Symbol: XCross");
}

int main (int argc, char **argv)
{
    QApplication a(argc, argv);

    MainWin w;

#if QT_VERSION < 0x040000
    a.setMainWidget(&w);
#endif
    w.resize(300,600);
    w.show();

    return a.exec();
}
