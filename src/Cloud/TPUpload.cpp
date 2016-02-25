/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "TPUpload.h"
#include "Context.h"
#include "Athlete.h"
#include <QString>
#include "Settings.h"
#include "RideFile.h"
#include "PwxRideFile.h"
#include <QtXml>

#include <QtCore/QCoreApplication>
#include <QtCore/QLocale>

#ifdef Q_CC_MSVC
#include <QtZlib\zlib.h>
#else
#include <zlib.h>
#endif

//
// Utility function to create a QByteArray of data in GZIP format
// This is essentially the same as qCompress but creates it in
// GZIP format (with recquisite headers) instead of ZLIB's format
// which has less filename info in the header
//
static QByteArray zCompress(const QByteArray &source)
{
    // int size is source.size()
    // const char *data is source.data()
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    // note that (15+16) below means windowbits+_16_ adds the gzip header/footer
    deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, (15+16), 8, Z_DEFAULT_STRATEGY);

    // input data
    strm.avail_in = source.size();
    strm.next_in = (Bytef *)source.data();

    // output data - on stack not heap, will be released
    QByteArray dest(source.size()/2, '\0'); // should compress by 50%, if not don't bother

    strm.avail_out = source.size()/2;
    strm.next_out = (Bytef *)dest.data();

    // now compress!
    deflate(&strm, Z_FINISH);

    // return byte array on the stack
    return QByteArray(dest.data(), (source.size()/2) - strm.avail_out);
}


TPUpload::TPUpload(QObject *parent) : QObject(parent), http(this), uploading(false)
{
    connect(&http, SIGNAL(responseReady(const QtSoapMessage &)),
            this, SLOT(getResponse(const QtSoapMessage &)));
}

int
TPUpload::upload(Context *context, const RideFile *ride)
{
    // if currently uploading fail!
    if (uploading == true) return 0;

    // create the file in .pwx format for upload
    QString uploadfile(QDir::tempPath() + "/tpupload.pwx");
    QFile file(uploadfile);
    PwxFileReader reader;
    reader.writeRideFile(context, ride, file);

    // read the whole thing back and encode as base64binary
    file.open(QFile::ReadOnly);
    QTextStream stream(&file);
    QString thelot = stream.readAll();
    QString pwxFile = zCompress(thelot.toUtf8()).toBase64(); // bleck!
    file.close();

    // setup the soap message
    current = QtSoapMessage();
    http.setHost("www.trainingpeaks.com", true);
    http.setAction("http://www.trainingpeaks.com/TPWebServices/ImportFileForUser");
    current.setMethod("ImportFileForUser", "http://www.trainingpeaks.com/TPWebServices/");
    current.addMethodArgument("username", "", appsettings->cvalue(context->athlete->cyclist, GC_TPUSER).toString());
    current.addMethodArgument("password", "", appsettings->cvalue(context->athlete->cyclist, GC_TPPASS).toString());
    current.addMethodArgument("byteData", "", pwxFile);

    // do it!
    uploading = true;
    http.submitRequest(current, "/tpwebservices/service.asmx");

    return pwxFile.size();
}

void TPUpload::getResponse(const QtSoapMessage &message)
{
    uploading = false;
    QString result;

    if (message.isFault()) {
            result = tr("Error:") + qPrintable(message.faultString().toString());
    } else {
        // SOAP call succeeded, but was the file accepted?
        if (message.returnValue().toString() == "true") result = tr("Upload successful");
        else result = tr("Upload failed - file rejected");
    }
    completed(result);
}
