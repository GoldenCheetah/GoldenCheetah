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

// graphics device
#include "RGraphicsDevice.h"

#include "Settings.h"
#include "Colors.h"

#include <QFont>
#include <QFontMetricsF>

// get rid of #define TRUE/FALSE if QT4 has defined them
// since they clash with Rboolean enums
#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif

const char * const gcDevice = "GoldenCheetahGD";

// return a QColor from an R color spec
static inline QColor qColor(int col) { return QColor(R_RED(col),R_GREEN(col),R_BLUE(col),R_ALPHA(col)); }

// get fg, bg, pen, brush from the gc
struct par {
    QColor fg,bg;
    QPen p;
    QBrush b;
    QFont f;
};

static struct par parContext(pGEcontext c)
{
    struct par returning;

    returning.bg = qColor(c->fill);
    returning.fg = qColor(c->col);

    returning.p.setColor(returning.fg);
    returning.p.setWidthF(c->lwd);

    // line style
    switch(c->lty) {
    case LTY_BLANK: returning.p.setStyle(Qt::NoPen); break;
    default:
    case LTY_SOLID: returning.p.setStyle(Qt::SolidLine); break;
    case LTY_DASHED: returning.p.setStyle(Qt::DashLine); break;
    case LTY_DOTTED: returning.p.setStyle(Qt::DotLine); break;
    case LTY_DOTDASH: returning.p.setStyle(Qt::DashDotLine); break;
    case LTY_LONGDASH: returning.p.setStyle(Qt::DashLine); break;
    case LTY_TWODASH: returning.p.setStyle(Qt::DashLine); break;
    }

    returning.b.setColor(returning.bg);
    returning.b.setStyle(Qt::SolidPattern);

    returning.f.setPointSizeF(c->ps*c->cex);

    return returning;
}

RGraphicsDevice::RGraphicsDevice ()
{
    // first time through
    gcGEDevDesc = NULL;

    // set the inital graphics device to GC
    createGD();
}

void RGraphicsDevice::NewPage(const pGEcontext, pDevDesc pDev)
{
    // fire event (pass previousPageSnapshot)
    if (!rtool || !rtool->canvas) return;

    // canvas size?
    int w = rtool->chart ? rtool->chart->geometry().width() : 500;
    int h = rtool->chart ? rtool->chart->geometry().height() : 500;

    // set the page size to user preference
    pDev->left = 0;
    pDev->right = rtool->width ? rtool->width : w;
    pDev->bottom = 0;
    pDev->top = rtool->height ? rtool->height : h;

    // clear the scene
    rtool->canvas->newPage();

}

Rboolean RGraphicsDevice::NewFrameConfirm_(pDevDesc)
{
    // returning false causes the default implementation (printing a prompt
    // of "Hit <Return> to see next plot:" to the console) to be used. this
    // seems ideal compared to any custom UI we could produce so we leave it be
    return TRUE;
}


void RGraphicsDevice::Mode(int, pDevDesc)
{
    // 0 = stop drawing
    // 1 = start drawing
    // 2 = input active
}

void RGraphicsDevice::Size(double *left, double *right, double *bottom, double *top, pDevDesc)
{
    int w = rtool->chart ? rtool->chart->geometry().width() : 500;
    int h = rtool->chart ? rtool->chart->geometry().height() : 500;

    *left = 0.0f;
    *right = rtool->width ? rtool->width : w;
    *bottom = 0.0f; //XXXs_height;
    *top = rtool->height ? rtool->height : h;
}

void RGraphicsDevice::Clip(double , double , double , double , pDevDesc)
{
    //qDebug()<<"RGD: Clip";
}


void RGraphicsDevice::Rect(double x0, double y0, double x1, double y1, const pGEcontext gc, pDevDesc)
{
    struct par p = parContext(gc);
    if (rtool && rtool->canvas) rtool->canvas->rectangle(x0,y0,x1,y1,p.p,p.b);
}

void RGraphicsDevice::Path(double *, double *, int , int *, Rboolean, const pGEcontext, pDevDesc)
{
    qDebug()<<"RGD: Path";
}

void RGraphicsDevice::Raster(unsigned int *, int , int , double , double , double ,
                             double , double , Rboolean , const pGEcontext , pDevDesc )
{
    qDebug()<<"RGD: Raster";
}

void RGraphicsDevice::Circle(double x, double y, double r, const pGEcontext gc, pDevDesc)
{
    struct par p = parContext(gc);
    if (rtool && rtool->canvas) rtool->canvas->circle(x,y,r,p.p,p.b);
}

void RGraphicsDevice::Line(double x1, double y1, double x2, double y2, const pGEcontext gc, pDevDesc)
{
    struct par p = parContext(gc);
    if (rtool && rtool->canvas) rtool->canvas->line(x1,y1,x2,y2,p.p);
}

void RGraphicsDevice::Polyline(int n, double *x, double *y, const pGEcontext gc, pDevDesc)
{
    struct par p = parContext(gc);
    if (rtool && rtool->canvas && n > 1) rtool->canvas->polyline(n,x,y,p.p);
}

void RGraphicsDevice::Polygon(int n, double *x, double *y, const pGEcontext gc, pDevDesc)
{
    struct par p = parContext(gc);
    if (rtool && rtool->canvas) rtool->canvas->polygon(n,x,y,p.p,p.b);
}

void RGraphicsDevice::MetricInfo(int, const pGEcontext gc, double* ascent, double* descent, double* width, pDevDesc)
{
    struct par p = parContext(gc);
    QFontMetricsF fm(p.f);
    *ascent = fm.ascent();
    *descent = fm.descent();
    *width = fm.averageCharWidth();

    return;
}

double RGraphicsDevice::StrWidth(const char *str, const pGEcontext gc, pDevDesc)
{
    struct par p = parContext(gc);
    QFontMetricsF fm(p.f);
    return fm.boundingRect(QString(str)).width();
}

double RGraphicsDevice::StrWidthUTF8(const char *str, const pGEcontext gc, pDevDesc)
{
    struct par p = parContext(gc);
    QFontMetricsF fm(p.f);
    return fm.boundingRect(QString(str)).width();
}

void RGraphicsDevice::Text(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc)
{
    // fonts too
    struct par p = parContext(gc);
    if (rtool && rtool->canvas) rtool->canvas->text(x,y,QString(str), rot, hadj, p.p, p.f);
}

void RGraphicsDevice::TextUTF8(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc)
{
    // fonts too
    struct par p = parContext(gc);
    if (rtool && rtool->canvas) rtool->canvas->text(x,y,QString(str), rot, hadj, p.p, p.f);
}

void RGraphicsDevice::Activate(pDevDesc)
{
}

void RGraphicsDevice::Deactivate(pDevDesc)
{
}

void RGraphicsDevice::Close(pDevDesc)
{
    if (rtool->dev->gcGEDevDesc != NULL) {

        // explicitly free and then null out the dev pointer of the GEDevDesc
        // This is to avoid incompatabilities between the heap we are compiled with
        // and the heap R is compiled with (we observed this to a problem with
        // 64-bit R)
        free(rtool->dev->gcGEDevDesc->dev);
        rtool->dev->gcGEDevDesc->dev = NULL;

        // set GDDevDesc to NULL so we don't reference it again
        rtool->dev->gcGEDevDesc = NULL;
    }
}

void RGraphicsDevice::OnExit(pDevDesc)
{
    // NOTE: this may be called at various times including during error
    // handling (jump_to_top_ex). therefore, do not place any process or device
    // final termination code here (even though the name of the function
    // suggests you might want to do this!)
}

int RGraphicsDevice::HoldFlush(pDevDesc, int)
{
    // NOTE: holdflush does not apply to bitmap devices since they are
    // already "buffered" via the fact that they only do expensive operations
    // (write to file) on dev.off. We could in theory use dev.flush as
    // an indicator that we should detectChanges (e.g. when in a long
    // running piece of code which doesn't yield to the REPL -- in practice
    // however there are way too many flushes yielding lots of extra disk io
    // and http round trips. If anything perhaps we could introduce a
    // time-buffered variation where flush could set a flag that is checked
    // every e.g. 1-second during background processing.

   return 0;
}

void RGraphicsDevice::resyncDisplayList()
{
#if 0
   // get pointers to device desc and cairo data
   pDevDesc pDev = s_pGEDevDesc->dev;
   DeviceContext* pDC = (DeviceContext*)pDev->deviceSpecific;

   // destroy existing device context
   handler::destroy(pDC);

   // allocate a new one and set it to be the device specific ptr
   pDC = handler::allocate(pDev);
   pDev->deviceSpecific = pDC;

   // re-create with the correct size (don't set a file path)
   if (!handler::initialize(s_width, s_height, s_devicePixelRatio, pDC))
   {
      // if this fails we are dead so close the device
      close();
      return;
   }

   // now update the device structure
   handler::setSize(pDev);

   // replay the display list onto the resized surface
   {
      SuppressDeviceEventsScope scope(plotManager());
      Error error = r::exec::executeSafely(
                              boost::bind(GEplayDisplayList,s_pGEDevDesc));
      if (error)
      {
         std::string errMsg;
         if (r::isCodeExecutionError(error, &errMsg))
            Rprintf(errMsg.c_str());
         else
            LOG_ERROR(error);
      }

   }
#endif
}

void RGraphicsDevice::resizeGraphicsDevice()
{
    // resync display list
    resyncDisplayList();

    // notify listeners of resize
    //XXXs_graphicsDeviceEvents.onResized();
}

SEXP RGraphicsDevice::GCdisplay()
{
    //qDebug()<<"Graphics Device IS called!\n";
    return rtool->dev->createGD();
}

// routine which creates device
SEXP RGraphicsDevice::createGD()
{
    // error if not a version 7 graphics system
    if (::R_GE_getVersion() < 7) {

      qDebug()<<"R: only support v7 or higher graphics systems, this is"<<::R_GE_getVersion();
      return R_NilValue;
    }

    R_CheckDeviceAvailable();

    // define device
    pDevDesc pDev = (DevDesc *) calloc(1, sizeof(DevDesc));

    // device functions
    pDev->activate = RGraphicsDevice::Activate;
    pDev->deactivate = RGraphicsDevice::Deactivate;
    pDev->size = RGraphicsDevice::Size;
    pDev->clip = RGraphicsDevice::Clip;
    pDev->rect = RGraphicsDevice::Rect;
    pDev->path = RGraphicsDevice::Path;
    pDev->raster = RGraphicsDevice::Raster;
    pDev->cap = NULL;
    pDev->circle = RGraphicsDevice::Circle;
    pDev->line = RGraphicsDevice::Line;
    pDev->polyline = RGraphicsDevice::Polyline;
    pDev->polygon = RGraphicsDevice::Polygon;
    pDev->locator = NULL;
    pDev->mode = RGraphicsDevice::Mode;
    pDev->metricInfo = RGraphicsDevice::MetricInfo;
    pDev->strWidth = RGraphicsDevice::StrWidth;
    pDev->strWidthUTF8 = RGraphicsDevice::StrWidthUTF8;
    pDev->text = RGraphicsDevice::Text;
    pDev->textUTF8 = RGraphicsDevice::TextUTF8;
    pDev->hasTextUTF8 = TRUE;
    pDev->wantSymbolUTF8 = TRUE;
    pDev->useRotatedTextInContour = FALSE;
    pDev->newPage = RGraphicsDevice::NewPage;
    pDev->close = RGraphicsDevice::Close;
    pDev->newFrameConfirm = RGraphicsDevice::NewFrameConfirm_;
    pDev->onExit = RGraphicsDevice::OnExit;
    pDev->eventEnv = R_NilValue;
    pDev->eventHelper = NULL;
    pDev->holdflush = NULL;

    // capabilities flags
    pDev->haveTransparency = 2;
    pDev->haveTransparentBg = 2;
    pDev->haveRaster = 2;
    pDev->haveCapture = 1;
    pDev->haveLocator = 0;

    //XXX todo - not sure what we might need
    pDev->deviceSpecific = NULL;
    pDev->displayListOn = FALSE;

    // device attributes
    setSize(pDev);
    setDeviceAttributes(pDev);

    // notify handler we are about to add (enables shadow device
    // to close itself so it doesn't show up in the dev.list()
    // in front of us
    //XXXhandler::onBeforeAddDevice(pDC);

    // associate with device description and add it
    gcGEDevDesc = GEcreateDevDesc(pDev);
    GEaddDevice2(gcGEDevDesc, gcDevice);

    // notify handler we have added (so it can regenerate its context)
    //XXXhandler::onAfterAddDevice(pDC);

    // make us active
    Rf_selectDevice(Rf_ndevNumber(gcGEDevDesc->dev));


    // set colors
    rtool->configChanged();

    // done
    return R_NilValue;
}

#if 0
// ensure that our device is created and active (required for snapshot
// creation/restoration)
bool RGraphicsDevice::makeActive()
{
    // make sure we have been created
    if (gcGEDevDesc == NULL) return false;

    // select us
    Rf_selectDevice(Rf_ndevNumber(gcGEDevDesc->dev));

    return true;
}

bool RGraphicsDevice::isActive()
{
    return gcGEDevDesc != NULL && Rf_ndevNumber(gcGEDevDesc->dev) == Rf_curDevice();
}
#endif

#if 0
SEXP RGraphicsDevice::activateGD()
{
    bool success = makeActive();
    if (!success) qDebug()<<"make active failed";
    return R_NilValue;
}
#endif

double RGraphicsDevice::grconvert(double val, const std::string& , const std::string& , const std::string& )
{
    //XXXr::exec::RFunction grconvFunc("graphics:::grconvert" + type, val, from, to);
    double convertedVal = val; // default in case of error
#if 0
    Error error = grconvFunc.call(&convertedVal);
    if (error) LOG_ERROR(error);
#endif
    return convertedVal;
}

double RGraphicsDevice::grconvertX(double x, const std::string& from, const std::string& to)
{
    return grconvert(x, "X", from, to);
}

double RGraphicsDevice::grconvertY(double y, const std::string& from, const std::string& to)
{
    return grconvert(y, "Y", from, to);
}

void RGraphicsDevice::deviceToUser(double* x, double* y)
{
    *x = grconvertX(*x, "device", "user");
    *y = grconvertY(*y, "device", "user");
}

void RGraphicsDevice::deviceToNDC(double* x, double* y)
{
    *x = grconvertX(*x, "device", "ndc");
    *y = grconvertY(*y, "device", "ndc");
}

#if 0
Error RGraphicsDevice::saveSnapshot(const core::FilePath& snapshotFile, const core::FilePath& imageFile)
{
   // ensure we are active
   Error error = makeActive();
   if (error)
      return error ;

   // save snaphot file
   error = r::exec::RFunction(".rs.saveGraphics",
                              string_utils::utf8ToSystem(snapshotFile.absolutePath())).call();
   if (error)
      return error;

   // save png file
   DeviceContext* pDC = (DeviceContext*)s_pGEDevDesc->dev->deviceSpecific;
   return handler::writeToPNG(imageFile, pDC);
}

Error RGraphicsDevice::restoreSnapshot(const core::FilePath& snapshotFile)
{
   // ensure we are active
   Error error = makeActive();
   if (error)
      return error ;

   // restore
   return r::exec::RFunction(".rs.restoreGraphics",
                             string_utils::utf8ToSystem(snapshotFile.absolutePath())).call();
}
#endif

void RGraphicsDevice::copyToActiveDevice()
{
#if 0
   int rsDeviceNumber = GEdeviceNumber(s_pGEDevDesc);
   GEcopyDisplayList(rsDeviceNumber);
#endif
}

std::string RGraphicsDevice::imageFileExtension()
{
    return "png";
}

void RGraphicsDevice::onBeforeExecute()
{
#if 0
   if (s_pGEDevDesc != NULL)
   {
      DeviceContext* pDC = (DeviceContext*)s_pGEDevDesc->dev->deviceSpecific;
      if (pDC != NULL)
         //XXXqDebughandler::onBeforeExecute(pDC);
   }
#endif
}

void RGraphicsDevice::setDeviceAttributes(pDevDesc pDev)
{
    double pointsize = 12 / dpiXFactor;
    double xoff=0, yoff=0, width=7, height=7;

    pDev->startps = pointsize;
    pDev->startfont = 1;
    pDev->startlty = 0;

    QColor bg=GColor(CPLOTBACKGROUND);
    QColor fg=GInvertColor(CPLOTBACKGROUND);

    pDev->startfill = R_RGB(bg.red(),bg.green(),bg.blue());
    pDev->startcol = R_RGB(fg.red(),fg.green(),fg.blue());
    pDev->startgamma = 1;

    pDev->left = 72 * xoff;			/* left */
    pDev->right = 72 * (xoff + width);	/* right */
    pDev->bottom = 72 * yoff;			/* bottom */
    pDev->top = 72 * (yoff + height);	        /* top */

    pDev->clipLeft = pDev->left; pDev->clipRight = pDev->right;
    pDev->clipBottom = pDev->bottom; pDev->clipTop = pDev->top;

    pDev->cra[0] = 0.9 * pointsize;
    pDev->cra[1] = 1.2 * pointsize;

    /* Character Addressing Offsets */
    /* These offsets should center a single */
    /* plotting character over the plotting point. */
    /* Pure guesswork and eyeballing ... */

    pDev->xCharOffset =  0.4900;
    pDev->yCharOffset =  0.3333;
    pDev->yLineBias = 0.2;

    /* Inches per Raster Unit */
    /* We use points (72 dots per inch) */

    pDev->ipr[0] = 1.0/72.0;
    pDev->ipr[1] = 1.0/72.0;

    // no support for qt events yet
    pDev->canGenMouseDown = FALSE;
    pDev->canGenMouseMove = FALSE;
    pDev->canGenMouseUp = FALSE;
    pDev->canGenKeybd = FALSE;
    pDev->gettingEvent = FALSE;

}

void RGraphicsDevice::setSize(pDevDesc pDev)
{
    pDev->left = 0;
    pDev->right = rtool->width;
    pDev->top = rtool->height;
    pDev->bottom = 0;
}

// below get called when the window resizes
void RGraphicsDevice::setSize(int , int , double )
{
#if 0
   // only set if the values have changed (prevents unnecessary plot
   // invalidations from occuring)
   if ( width != s_width || height != s_height || devicePixelRatio != s_devicePixelRatio) {
      s_width = width;
      s_height = height;
      s_devicePixelRatio = devicePixelRatio;

      // if there is a device active sync its size
      if (gcGEDevDesc != NULL) resizeGraphicsDevice();
   }
#endif
}

int RGraphicsDevice::getWidth()
{
    return rtool->width;
}

int RGraphicsDevice::getHeight()
{
    return rtool->height;
}

double RGraphicsDevice::devicePixelRatio()
{
    return 1.0f; //s_devicePixelRatio;
}

void RGraphicsDevice::close()
{
    if (gcGEDevDesc != NULL) {
        Rf_killDevice(Rf_ndevNumber(gcGEDevDesc->dev));
        gcGEDevDesc=NULL;
    }
}
