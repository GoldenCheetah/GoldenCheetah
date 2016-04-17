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

#include "RGraphicsDevice.h"

const char * const gcDevice = "GoldenCheetahGD";

RGraphicsDevice::RGraphicsDevice ()
{
    // instantiate.............
    //XXX qDebug()<<"R: instantiate graphics device...";
}

void RGraphicsDevice::NewPage(const pGEcontext gc, pDevDesc dev)
{
   // delegate
   //XXXqDebughandler::newPage(gc, dev);

   // fire event (pass previousPageSnapshot)
   //XXXSEXP previousPageSnapshot = s_pGEDevDesc->savedSnapshot;
   //XXXs_graphicsDeviceEvents.onNewPage(previousPageSnapshot);
}

Rboolean RGraphicsDevice::NewFrameConfirm(pDevDesc dd)
{
   // returning false causes the default implementation (printing a prompt
   // of "Hit <Return> to see next plot:" to the console) to be used. this
   // seems ideal compared to any custom UI we could produce so we leave it be
   return FALSE;
}


void RGraphicsDevice::Mode(int mode, pDevDesc dev)
{
   // 0 = stop drawing
   // 1 = start drawing
   // 2 = input active

   //XXXqDebughandler::mode(mode, dev);

   //XXXs_graphicsDeviceEvents.onDrawing();
}

void RGraphicsDevice::Size(double *left, double *right, double *bottom, double *top, pDevDesc dev)
{
   *left = 0.0;
   *right = 500; //XXXs_width;
   *bottom = 500; //XXXs_height;
   *top = 0.0;
}

void RGraphicsDevice::Clip(double x0, double x1, double y0, double y1, pDevDesc dev)
{
   //XXXqDebughandler::clip(x0, x1, y0, y1, dev);
}


void RGraphicsDevice::Rect(double x0, double y0, double x1, double y1, const pGEcontext gc, pDevDesc dev)
{
   //XXXqDebughandler::rect(x0, y0, x1, y1, gc, dev);
}

void RGraphicsDevice::Path(double *x, double *y, int npoly, int *nper, Rboolean winding, const pGEcontext gc, pDevDesc dd)
{
   //XXXqDebughandler::path(x, y, npoly, nper, winding, gc, dd);
}

void RGraphicsDevice::Raster(unsigned int *raster, int w, int h, double x, double y, double width,
                             double height, double rot, Rboolean interpolate, const pGEcontext gc, pDevDesc dd)
{
   //XXXqDebughandler::raster(raster, w, h, x, y, width, height, rot, interpolate, gc, dd);
}

SEXP RGraphicsDevice::Cap(pDevDesc dd)
{
   return 0;//XXXqDebughandler::cap(dd);
}

void RGraphicsDevice::Circle(double x, double y, double r, const pGEcontext gc, pDevDesc dev)
{
   //XXXqDebughandler::circle(x, y, r, gc, dev);
}

void RGraphicsDevice::Line(double x1, double y1, double x2, double y2, const pGEcontext gc, pDevDesc dev)
{
   //XXXqDebughandler::line(x1, y1, x2, y2, gc, dev);
}

void RGraphicsDevice::Polyline(int n, double *x, double *y, const pGEcontext gc, pDevDesc dev)
{
   //XXXqDebughandler::polyline(n, x, y, gc, dev);
}

void RGraphicsDevice::Polygon(int n, double *x, double *y, const pGEcontext gc, pDevDesc dev)
{
   //XXXqDebughandler::polygon(n, x, y, gc, dev);
}

void RGraphicsDevice::MetricInfo(int c, const pGEcontext gc, double* ascent, double* descent, double* width, pDevDesc dev)
{
   //XXXqDebughandler::metricInfo(c, gc, ascent, descent, width, dev);
}

double RGraphicsDevice::StrWidth(const char *str, const pGEcontext gc, pDevDesc dev)
{
   return 0; //XXXqDebughandler::strWidth(str, gc, dev);
}

double RGraphicsDevice::StrWidthUTF8(const char *str, const pGEcontext gc, pDevDesc dev)
{
   return 0; //XXXqDebughandler::strWidth(str, gc, dev);
}

void RGraphicsDevice::Text(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dev)
{
   //XXXqDebughandler::text(x, y, str, rot, hadj, gc, dev);
}

void RGraphicsDevice::TextUTF8(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dev)
{
   //XXXqDebughandler::text(x, y, str, rot, hadj, gc, dev);
}

#if 0
Rboolean RGraphicsDevice::Locator(double *x, double *y, pDevDesc dev)
{
   if (s_locatorFunction)
   {
      s_graphicsDeviceEvents.onDrawing();

      if(s_locatorFunction(x,y))
      {
         // if our graphics device went away while we were waiting
         // for locator input then we need to return false

         if (s_pGEDevDesc != NULL)
            return TRUE;
         else
            return FALSE;
      }
      else
      {
         s_graphicsDeviceEvents.onDrawing();
         return FALSE;
      }
   }
   else
   {
      return FALSE;
   }
}
#endif

void RGraphicsDevice::Activate(pDevDesc dev)
{
}

void RGraphicsDevice::Deactivate(pDevDesc dev)
{
}

void RGraphicsDevice::Close(pDevDesc dev)
{
#if 0
   if (s_pGEDevDesc != NULL)
   {
      // destroy device specific struct
      DeviceContext* pDC = (DeviceContext*)s_pGEDevDesc->dev->deviceSpecific;
      //XXXqDebughandler::destroy(pDC);

      // explicitly free and then null out the dev pointer of the GEDevDesc
      // This is to avoid incompatabilities between the heap we are compiled with
      // and the heap R is compiled with (we observed this to a problem with
      // 64-bit R)
      std::free(s_pGEDevDesc->dev);
      s_pGEDevDesc->dev = NULL;

      // set GDDevDesc to NULL so we don't reference it again
      s_pGEDevDesc = NULL;
   }

   s_graphicsDeviceEvents.onClosed();
#endif
}

void RGraphicsDevice::OnExit(pDevDesc dd)
{
   // NOTE: this may be called at various times including during error
   // handling (jump_to_top_ex). therefore, do not place any process or device
   // final termination code here (even though the name of the function
   // suggests you might want to do this!)
}

int RGraphicsDevice::HoldFlush(pDevDesc dd, int level)
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

// routine which creates device
SEXP RGraphicsDevice::createGD()
{
#if 0
   // error if there is already an RStudio device
   if (s_pGEDevDesc != NULL)
   {
      if (!r::session::utils::isServerMode())
      {
         Error error = r::exec::executeString(".rs.newDesktopGraphicsDevice()");
         if (error)
         {
            std::string msg = error.summary();
            r::isCodeExecutionError(error, &msg);
            Rf_warning(("Error creating graphics device: " + msg).c_str());
         }
      }
      else
      {
         Rf_warning("Only one RStudio graphics device is permitted");
      }
      return R_NilValue;
   }


   R_CheckDeviceAvailable();

   BEGIN_SUSPEND_INTERRUPTS
   {
      // initialize v9 structure
      DevDescVersion9 devDesc;

      // device functions
      devDesc.activate = GD_Activate;
      devDesc.deactivate = GD_Deactivate;
      devDesc.size = GD_Size;
      devDesc.clip = GD_Clip;
      devDesc.rect = GD_Rect;
      devDesc.path = GD_Path;
      devDesc.raster = GD_Raster;
      devDesc.cap = GD_Cap;
      devDesc.circle = GD_Circle;
      devDesc.line = GD_Line;
      devDesc.polyline = GD_Polyline;
      devDesc.polygon = GD_Polygon;
      devDesc.locator = GD_Locator;
      devDesc.mode = GD_Mode;
      devDesc.metricInfo = GD_MetricInfo;
      devDesc.strWidth = GD_StrWidth;
      devDesc.strWidthUTF8 = GD_StrWidthUTF8;
      devDesc.text = GD_Text;
      devDesc.textUTF8 = GD_TextUTF8;
      devDesc.hasTextUTF8 = TRUE;
      devDesc.wantSymbolUTF8 = TRUE;
      devDesc.useRotatedTextInContour = FALSE;
      devDesc.newPage = GD_NewPage;
      devDesc.close = GD_Close;
      devDesc.newFrameConfirm = GD_NewFrameConfirm;
      devDesc.onExit = GD_OnExit;
      devDesc.eventEnv = R_NilValue;
      devDesc.eventHelper = NULL;
      devDesc.holdflush = GD_HoldFlush;

      // capabilities flags
      devDesc.haveTransparency = 2;
      devDesc.haveTransparentBg = 2;
      devDesc.haveRaster = 2;
      devDesc.haveCapture = 1;
      devDesc.haveLocator = 2;

      // allocate device
      pDevDesc pDev = handler::dev_desc::allocate(devDesc);

      // allocate and initialize context
      DeviceContext* pDC = handler::allocate(pDev);
      if (!handler::initialize(s_width, s_height, s_devicePixelRatio, pDC))
      {
         handler::destroy(pDC);

         // leak the pDev on purpose because we don't have
         // access to R's heap/free function

         Rf_error("Unable to start RStudio device");
      }

      // set device specific context
      pDev->deviceSpecific = pDC;

      // device attributes
      handler::setSize(pDev);
      handler::setDeviceAttributes(pDev);

      // notify handler we are about to add (enables shadow device
      // to close itself so it doesn't show up in the dev.list()
      // in front of us
      handler::onBeforeAddDevice(pDC);

      // associate with device description and add it
      s_pGEDevDesc = GEcreateDevDesc(pDev);
      GEaddDevice2(s_pGEDevDesc, kRStudioDevice);

      // notify handler we have added (so it can regenerate its context)
      handler::onAfterAddDevice(pDC);

      // make us active
      Rf_selectDevice(Rf_ndevNumber(s_pGEDevDesc->dev));
   }
   END_SUSPEND_INTERRUPTS;
#endif
   return R_NilValue;
}

// ensure that our device is created and active (required for snapshot
// creation/restoration)
#if 0
Error RGraphicsDevice::makeActive()
{
   // don't make active if our graphics version is incompatible
   if (!graphics::validateRequirements())
      return Error(graphics::errc::IncompatibleGraphicsEngine, ERROR_LOCATION);

   // make sure we have been created
   if (s_pGEDevDesc == NULL)
   {
      SEXP ignoredSEXP;
      Error error = r::exec::executeSafely<SEXP>(boost::bind(createGD),
                                                 &ignoredSEXP);
      if (error)
         return error;
   }

   // select us
   Rf_selectDevice(Rf_ndevNumber(s_pGEDevDesc->dev));

   return Success();
}
#endif

bool RGraphicsDevice::isActive()
{
   return 0; //XXXs_pGEDevDesc != NULL && Rf_ndevNumber(s_pGEDevDesc->dev) == Rf_curDevice();
}

SEXP RGraphicsDevice::rs_activateGD()
{
   //XXXError error = makeActive();
   //XXXif (error) LOG_ERROR(error);
   return R_NilValue;
}

#if 0
DisplaySize RGraphicsDevice::displaySize()
{
   return DisplaySize(s_width, s_height);
}
#endif

double RGraphicsDevice::grconvert(double val, const std::string& type, const std::string& from, const std::string& to)
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

const int kDefaultWidth = 500;
const int kDefaultHeight = 500;
const double kDefaultDevicePixelRatio = 1.0;

bool RGraphicsDevice::initialize()
{
#if 0
   // initialize shadow handler
   //XXXX ???? r::session::graphics::handler::installShadowHandler();

   // save reference to locator function
   //XXX don't want this ???? s_locatorFunction = locatorFunction;

   // device conversion functions
   UnitConversionFunctions convert;
   convert.deviceToUser = deviceToUser;
   convert.deviceToNDC = deviceToNDC;

   // create plot manager (provide functions & events)
   GraphicsDeviceFunctions graphicsDevice;
   graphicsDevice.isActive = isActive;
   graphicsDevice.displaySize = displaySize;
   graphicsDevice.convert = convert;
   graphicsDevice.saveSnapshot = saveSnapshot;
   graphicsDevice.restoreSnapshot = restoreSnapshot;
   graphicsDevice.copyToActiveDevice = copyToActiveDevice;
   graphicsDevice.imageFileExtension = imageFileExtension;
   graphicsDevice.close = close;
   graphicsDevice.onBeforeExecute = onBeforeExecute;
   Error error = plotManager().initialize(graphicsPath,
                                          graphicsDevice,
                                          &s_graphicsDeviceEvents);
   if (error)
      return error;

   // set size
   setSize(kDefaultWidth, kDefaultHeight, kDefaultDevicePixelRatio);

   // check for an incompatible graphics version before fully initializing.
   std::string message;
   if (graphics::validateRequirements(&message))
   {
      // register device creation routine
      R_CallMethodDef createGDMethodDef ;
      createGDMethodDef.name = "rs_createGD" ;
      createGDMethodDef.fun = (DL_FUNC) createGD ;
      createGDMethodDef.numArgs = 0;
      r::routines::addCallMethod(createGDMethodDef);

      // regsiter device activiation routine
      R_CallMethodDef activateGDMethodDef ;
      activateGDMethodDef.name = "rs_activateGD" ;
      activateGDMethodDef.fun = (DL_FUNC) rs_activateGD ;
      activateGDMethodDef.numArgs = 0;
      r::routines::addCallMethod(activateGDMethodDef);

      // initialize
      return r::exec::RFunction(".rs.initGraphicsDevice").call();
   }
   else
   {
      // if there is one then print a warning and return Success. This allows
      // users to continue using the product while still being made aware of the
      // fact that their graphics engine is incompatible
      r::exec::warning(message);

      // success with warning
      return Success();
   }
#endif
//added
    return false;
}


void RGraphicsDevice::setSize(int width, int height, double devicePixelRatio)
{
#if 0
   // only set if the values have changed (prevents unnecessary plot
   // invalidations from occuring)
   if ( width != s_width || height != s_height || devicePixelRatio != s_devicePixelRatio)
   {
      s_width = width;
      s_height = height;
      s_devicePixelRatio = devicePixelRatio;

      // if there is a device active sync its size
      if (s_pGEDevDesc != NULL)
         resizeGraphicsDevice();
   }
#endif
}

int RGraphicsDevice::getWidth()
{
   return 500; //XXXs_width;
}

int RGraphicsDevice::getHeight()
{
   return 500; //XXXs_height;
}

double RGraphicsDevice::devicePixelRatio()
{
   return 1; //XXXs_devicePixelRatio;
}

void RGraphicsDevice::close()
{
   //XXX if (s_pGEDevDesc != NULL) Rf_killDevice(Rf_ndevNumber(s_pGEDevDesc->dev));
}


#if 0 //XXX not sure
// if we don't have pango cairo then provide a null definition
// for the pango cairo init routine (for linking on other platforms)
#ifndef PANGO_CAIRO_FOUND
namespace handler {
void installCairoHandler() {}
}
#endif
#endif
