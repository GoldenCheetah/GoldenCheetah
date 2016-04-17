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

//
// An R graphics device to capture all graphics requests and pass on
// to a QWidget based canvas to plot within GoldenCheetah
//
// We try to do as little as possible; literally passing primitives
// across as a proxy and not offering any advanced capabilities
//

extern "C" {
#include <Rinternals.h>

#include <R_ext/Boolean.h>
#include "R_ext/GraphicsEngine.h"
#include "R_ext/GraphicsDevice.h"

}

#include <stdlib.h>
#include <QDebug>

// we only have one instance, and it is used throughout
// when it is instantiated it registers the device with
// R so it can be used for all output.
//
// The global rtool pointer tells us which context to
// plot for and which display widget should receive the
// primitives
class RGraphicsDevice {

    public:

        RGraphicsDevice();
        ~RGraphicsDevice();

        // R Graphic Device API methods
        void NewPage(const pGEcontext gc, pDevDesc dev);
        Rboolean NewFrameConfirm(pDevDesc dd);
        void Mode(int mode, pDevDesc dev);
        void Size(double *left, double *right, double *bottom, double *top, pDevDesc dev);
        void Clip(double x0, double x1, double y0, double y1, pDevDesc dev);
        void Rect(double x0, double y0, double x1, double y1, const pGEcontext gc, pDevDesc dev);
        void Path(double *x, double *y, int npoly, int *nper, Rboolean winding, const pGEcontext gc, pDevDesc dd);
        void Raster(unsigned int *raster,int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc, pDevDesc dd);
        SEXP Cap(pDevDesc dd);
        void Circle(double x, double y, double r, const pGEcontext gc, pDevDesc dev);
        void Line(double x1, double y1, double x2, double y2, const pGEcontext gc, pDevDesc dev);
        void Polyline(int n, double *x, double *y, const pGEcontext gc, pDevDesc dev);
        void Polygon(int n, double *x, double *y, const pGEcontext gc, pDevDesc dev);
        void MetricInfo(int c, const pGEcontext gc, double* ascent, double* descent, double* width, pDevDesc dev);
        double StrWidth(const char *str, const pGEcontext gc, pDevDesc dev);
        double StrWidthUTF8(const char *str, const pGEcontext gc, pDevDesc dev);
        void Text(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dev);
        void TextUTF8(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dev);
        void Activate(pDevDesc dev);
        void Deactivate(pDevDesc dev);
        void Close(pDevDesc dev);
        void OnExit(pDevDesc dd);
        int HoldFlush(pDevDesc dd, int level);

        // event handling (?)
        void onBeforeExecute();
        void resyncDisplayList();
        void resizeGraphicsDevice();
        SEXP rs_activateGD();
        void copyToActiveDevice();
        std::string imageFileExtension();
        void close();
        //Error makeActive();
        //Rboolean Locator(double *x, double *y, pDevDesc dev);
        //DisplaySize displaySize();
        //Error saveSnapshot(const core::FilePath& snapshotFile,const core::FilePath& imageFile);
        //Error restoreSnapshot(const core::FilePath& snapshotFile);

        // utility functions (?)
        bool initialize();
        SEXP createGD();
        bool isActive();
        double grconvert(double val, const std::string& type, const std::string& from, const std::string& to);
        double grconvertX(double x, const std::string& from, const std::string& to);
        double grconvertY(double y, const std::string& from, const std::string& to);
        void deviceToUser(double* x, double* y);
        void deviceToNDC(double* x, double* y);
        void setSize(int width, int height, double devicePixelRatio);
        int getWidth();
        int getHeight();
        double devicePixelRatio();
};
