/*
 * Copyright 2015 (c) Mark Liversedge (liversedge@gmail.com)
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

#include "APIWebService.h"
#include "GcUpgrade.h"

void
APIWebService::service(HttpRequest &request, HttpResponse &response)
{
    // request needs a response, just provide the home page
    // response to everything for now
    // Set a response header
    response.setHeader("Content-Type", "text/html; charset=ISO-8859-1");
    response.write("<html><body>");
    response.write("GoldenCheetah ");
    response.write(QString("%1").arg(VERSION_STRING).toLocal8Bit());
    response.write("</body></html>");
}
