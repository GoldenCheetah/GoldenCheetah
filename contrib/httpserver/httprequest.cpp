/**
  @file
  @author Stefan Frings
*/

#include "httprequest.h"
#include <QList>
#include <QDir>
#include "httpcookie.h"

HttpRequest::HttpRequest(QSettings* settings) {
    status=waitForRequest;
    currentSize=0;
    expectedBodySize=0;
    maxSize=settings->value("maxRequestSize","16000").toInt();
    maxMultiPartSize=settings->value("maxMultiPartSize","1000000").toInt();
}

void HttpRequest::readRequest(QTcpSocket* socket) {
    #ifdef SUPERVERBOSE
        wDebug("HttpRequest: read request");
    #endif
    int toRead=maxSize-currentSize+1; // allow one byte more to be able to detect overflow
    lineBuffer.append(socket->readLine(toRead));
    currentSize+=lineBuffer.size();
    if (!lineBuffer.contains('\r') && !lineBuffer.contains('\n')) {
        #ifdef SUPERVERBOSE
            wDebug("HttpRequest: collecting more parts until line break");
        #endif
        return;
    }
    QByteArray newData=lineBuffer.trimmed();
    lineBuffer.clear();
    if (!newData.isEmpty()) {
        QList<QByteArray> list=newData.split(' ');
        if (list.count()!=3 || !list.at(2).contains("HTTP")) {
            qWarning("HttpRequest: received broken HTTP request, invalid first line");
            status=abort;
        }
        else {
            method=list.at(0).trimmed();
            path=list.at(1);
            version=list.at(2);
            status=waitForHeader;
        }
    }
}

void HttpRequest::readHeader(QTcpSocket* socket) {
    #ifdef SUPERVERBOSE
        wDebug("HttpRequest: read header");
    #endif
    int toRead=maxSize-currentSize+1; // allow one byte more to be able to detect overflow
    lineBuffer.append(socket->readLine(toRead));
    currentSize+=lineBuffer.size();
    if (!lineBuffer.contains('\r') && !lineBuffer.contains('\n')) {
        #ifdef SUPERVERBOSE
            wDebug("HttpRequest: collecting more parts until line break");
        #endif
        return;
    }
    QByteArray newData=lineBuffer.trimmed();
    lineBuffer.clear();
    int colon=newData.indexOf(':');
    if (colon>0)  {
        // Received a line with a colon - a header
        currentHeader=newData.left(colon);
        QByteArray value=newData.mid(colon+1).trimmed();
        headers.insert(currentHeader,value);
        #ifdef SUPERVERBOSE
            wDebug("HttpRequest: received header %s: %s",currentHeader.data(),value.data());
        #endif
    }
    else if (!newData.isEmpty()) {
        // received another line - belongs to the previous header
        #ifdef SUPERVERBOSE
            wDebug("HttpRequest: read additional line of header");
        #endif
        // Received additional line of previous header
        if (headers.contains(currentHeader)) {
            headers.insert(currentHeader,headers.value(currentHeader)+" "+newData);
        }
    }
    else {
        // received an empty line - end of headers reached
        #ifdef SUPERVERBOSE
            wDebug("HttpRequest: headers completed");
        #endif
        // Empty line received, that means all headers have been received
        // Check for multipart/form-data
        QByteArray contentType=headers.value("Content-Type");
        if (contentType.startsWith("multipart/form-data")) {
            int posi=contentType.indexOf("boundary=");
            if (posi>=0) {
                boundary=contentType.mid(posi+9);
            }
        }
        QByteArray contentLength=getHeader("Content-Length");
        if (!contentLength.isEmpty()) {
            expectedBodySize=contentLength.toInt();
        }
        if (expectedBodySize==0) {
            #ifdef SUPERVERBOSE
                wDebug("HttpRequest: expect no body");
            #endif
            status=complete;
        }
        else if (boundary.isEmpty() && expectedBodySize+currentSize>maxSize) {
            qWarning("HttpRequest: expected body is too large");
            status=abort;
        }
        else if (!boundary.isEmpty() && expectedBodySize>maxMultiPartSize) {
            qWarning("HttpRequest: expected multipart body is too large");
            status=abort;
        }
        else {
            #ifdef SUPERVERBOSE
                wDebug("HttpRequest: expect %i bytes body",expectedBodySize);
            #endif
            status=waitForBody;
        }
    }
}

void HttpRequest::readBody(QTcpSocket* socket) {
    Q_ASSERT(expectedBodySize!=0);
    if (boundary.isEmpty()) {
        // normal body, no multipart
        #ifdef SUPERVERBOSE
            wDebug("HttpRequest: receive body");
        #endif
        int toRead=expectedBodySize-bodyData.size();
        QByteArray newData=socket->read(toRead);
        currentSize+=newData.size();
        bodyData.append(newData);
        if (bodyData.size()>=expectedBodySize) {
            status=complete;
        }
    }
    else {
        // multipart body, store into temp file
        #ifdef SUPERVERBOSE
            wDebug("HttpRequest: receiving multipart body");
        #endif
        if (!tempFile.isOpen()) {
            tempFile.open();
        }
        // Transfer data in 64kb blocks
        int fileSize=tempFile.size();
        int toRead=expectedBodySize-fileSize;
        if (toRead>65536) {
            toRead=65536;
        }
        fileSize+=tempFile.write(socket->read(toRead));
        if (fileSize>=maxMultiPartSize) {
            qWarning("HttpRequest: received too many multipart bytes");
            status=abort;
        }
        else if (fileSize>=expectedBodySize) {
        #ifdef SUPERVERBOSE
            wDebug("HttpRequest: received whole multipart body");
        #endif
            tempFile.flush();
            if (tempFile.error()) {
                qCritical("HttpRequest: Error writing temp file for multipart body");
            }
            parseMultiPartFile();
            tempFile.close();
            status=complete;
        }
    }
}

void HttpRequest::decodeRequestParams() {
    #ifdef SUPERVERBOSE
        wDebug("HttpRequest: extract and decode request parameters");
    #endif
    // Get URL parameters
    QByteArray rawParameters;
    int questionMark=path.indexOf('?');
    if (questionMark>=0) {
        rawParameters=path.mid(questionMark+1);
        path=path.left(questionMark);
    }
    // Get request body parameters
    QByteArray contentType=headers.value("Content-Type");
    if (!bodyData.isEmpty() && (contentType.isEmpty() || contentType.startsWith("application/x-www-form-urlencoded"))) {
        if (!rawParameters.isEmpty()) {
            rawParameters.append('&');
            rawParameters.append(bodyData);
        }
        else {
            rawParameters=bodyData;
        }
    }
    // Split the parameters into pairs of value and name
    QList<QByteArray> list=rawParameters.split('&');
    foreach (QByteArray part, list) {
        int equalsChar=part.indexOf('=');
        if (equalsChar>=0) {
            QByteArray name=part.left(equalsChar).trimmed();
            QByteArray value=part.mid(equalsChar+1).trimmed();
            parameters.insert(urlDecode(name),urlDecode(value));
        }
        else if (!part.isEmpty()){
            // Name without value
            parameters.insert(urlDecode(part),"");
        }
    }
}

void HttpRequest::extractCookies() {
    #ifdef SUPERVERBOSE
        wDebug("HttpRequest: extract cookies");
    #endif
    foreach(QByteArray cookieStr, headers.values("Cookie")) {
        QList<QByteArray> list=HttpCookie::splitCSV(cookieStr);
        foreach(QByteArray part, list) {
            #ifdef SUPERVERBOSE
                wDebug("HttpRequest: found cookie %s",part.data());
            #endif                // Split the part into name and value
            QByteArray name;
            QByteArray value;
            int posi=part.indexOf('=');
            if (posi) {
                name=part.left(posi).trimmed();
                value=part.mid(posi+1).trimmed();
            }
            else {
                name=part.trimmed();
                value="";
            }
            cookies.insert(name,value);
        }
    }
    headers.remove("Cookie");
}

void HttpRequest::readFromSocket(QTcpSocket* socket) {
    Q_ASSERT(status!=complete);
    if (status==waitForRequest) {
        readRequest(socket);
    }
    else if (status==waitForHeader) {
        readHeader(socket);
    }
    else if (status==waitForBody) {
        readBody(socket);
    }
    if ((boundary.isEmpty() && currentSize>maxSize) || (!boundary.isEmpty() && currentSize>maxMultiPartSize)) {
        qWarning("HttpRequest: received too many bytes");
        status=abort;
    }
    if (status==complete) {
        // Extract and decode request parameters from url and body
        decodeRequestParams();
        // Extract cookies from headers
        extractCookies();
    }
}


HttpRequest::RequestStatus HttpRequest::getStatus() const {
    return status;
}


QByteArray HttpRequest::getMethod() const {
    return method;
}


QByteArray HttpRequest::getPath() const {
    return urlDecode(path);
}


QByteArray HttpRequest::getVersion() const {
    return version;
}


QByteArray HttpRequest::getHeader(const QByteArray& name) const {
    return headers.value(name);
}

QList<QByteArray> HttpRequest::getHeaders(const QByteArray& name) const {
    return headers.values(name);
}

QMultiMap<QByteArray,QByteArray> HttpRequest::getHeaderMap() const {
    return headers;
}

QByteArray HttpRequest::getParameter(const QByteArray& name) const {
    return parameters.value(name);
}

QList<QByteArray> HttpRequest::getParameters(const QByteArray& name) const {
    return parameters.values(name);
}

QMultiMap<QByteArray,QByteArray> HttpRequest::getParameterMap() const {
    return parameters;
}

QByteArray HttpRequest::getBody() const {
    return bodyData;
}

QByteArray HttpRequest::urlDecode(const QByteArray source) {
    QByteArray buffer(source);
    buffer.replace('+',' ');
    int percentChar=buffer.indexOf('%');
    while (percentChar>=0) {
        bool ok;
        char byte=buffer.mid(percentChar+1,2).toInt(&ok,16);
        if (ok) {
            buffer.replace(percentChar,3,(char*)&byte,1);
        }
        percentChar=buffer.indexOf('%',percentChar+1);
    }
    return buffer;
}


void HttpRequest::parseMultiPartFile() {
    wDebug("HttpRequest: parsing multipart temp file");
    tempFile.seek(0);
    bool finished=false;
    while (!tempFile.atEnd() && !finished && !tempFile.error()) {
        #ifdef SUPERVERBOSE
            wDebug("HttpRequest: reading multpart headers");
        #endif
        QByteArray fieldName;
        QByteArray fileName;
        while (!tempFile.atEnd() && !finished && !tempFile.error()) {
            QByteArray line=tempFile.readLine(65536).trimmed();
            if (line.startsWith("Content-Disposition:")) {
                if (line.contains("form-data")) {
                    int start=line.indexOf(" name=\"");
                    int end=line.indexOf("\"",start+7);
                    if (start>=0 && end>=start) {
                        fieldName=line.mid(start+7,end-start-7);
                    }
                    start=line.indexOf(" filename=\"");
                    end=line.indexOf("\"",start+11);
                    if (start>=0 && end>=start) {
                        fileName=line.mid(start+11,end-start-11);
                    }
                    #ifdef SUPERVERBOSE
                        wDebug("HttpRequest: multipart field=%s, filename=%s",fieldName.data(),fileName.data());
                    #endif
                }
                else {
                    wDebug("HttpRequest: ignoring unsupported content part %s",line.data());
                }
            }
            else if (line.isEmpty()) {
                break;
            }
        }

        #ifdef SUPERVERBOSE
            wDebug("HttpRequest: reading multpart data");
        #endif
        QTemporaryFile* uploadedFile=0;
        QByteArray fieldValue;
        while (!tempFile.atEnd() && !finished && !tempFile.error()) {
            QByteArray line=tempFile.readLine(65536);
            if (line.startsWith("--"+boundary)) {
                // Boundary found. Until now we have collected 2 bytes too much,
                // so remove them from the last result
                if (fileName.isEmpty() && !fieldName.isEmpty()) {
                    // last field was a form field
                    fieldValue.remove(fieldValue.size()-2,2);
                    parameters.insert(fieldName,fieldValue);
                    wDebug("HttpRequest: set parameter %s=%s",fieldName.data(),fieldValue.data());
                }
                else if (!fileName.isEmpty() && !fieldName.isEmpty()) {
                    // last field was a file
                    #ifdef SUPERVERBOSE
                        wDebug("HttpRequest: finishing writing to uploaded file");
                    #endif
                    uploadedFile->resize(uploadedFile->size()-2);
                    uploadedFile->flush();
                    uploadedFile->seek(0);
                    parameters.insert(fieldName,fileName);
                    wDebug("HttpRequest: set parameter %s=%s",fieldName.data(),fileName.data());
                    uploadedFiles.insert(fieldName,uploadedFile);
                    wDebug("HttpRequest: uploaded file size is %i",(int) uploadedFile->size());
                }
                if (line.contains(boundary+"--")) {
                    finished=true;
                }
                break;
            }
            else {
                if (fileName.isEmpty() && !fieldName.isEmpty()) {
                    // this is a form field.
                    currentSize+=line.size();
                    fieldValue.append(line);
                }
                else if (!fileName.isEmpty() && !fieldName.isEmpty()) {
                    // this is a file
                    if (!uploadedFile) {
                        uploadedFile=new QTemporaryFile();
                        uploadedFile->open();
                    }
                    uploadedFile->write(line);
                    if (uploadedFile->error()) {
                        qCritical("HttpRequest: error writing temp file, %s",qPrintable(uploadedFile->errorString()));
                    }
                }
            }
        }
    }
    if (tempFile.error()) {
        qCritical("HttpRequest: cannot read temp file, %s",qPrintable(tempFile.errorString()));
    }
    #ifdef SUPERVERBOSE
        wDebug("HttpRequest: finished parsing multipart temp file");
    #endif
}

HttpRequest::~HttpRequest() {
    foreach(QByteArray key, uploadedFiles.keys()) {
        QTemporaryFile* file=uploadedFiles.value(key);
        file->close();
        delete file;
    }
}

QTemporaryFile* HttpRequest::getUploadedFile(const QByteArray fieldName) {
    return uploadedFiles.value(fieldName);
}

QByteArray HttpRequest::getCookie(const QByteArray& name) const {
    return cookies.value(name);
}

/** Get the map of cookies */
QMap<QByteArray,QByteArray>& HttpRequest::getCookieMap() {
    return cookies;
}

