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

#include "TPDownload.h"
#include <QString>
#include "Settings.h"
#include "RideFile.h"
#include "PwxRideFile.h"
#include <QtXml>

#include <QtCore/QCoreApplication>
#include <QtCore/QLocale>

static QString tptypestrings[]= { "SharedFree", "CoachedFree", "SelfCoachedPremium",
                                  "SharedSelfCoachedPremium", "CoachedPremium",
                                  "SharedCoachedPremium"
                                };
//
// Get athletes available for download
//
TPAthlete::TPAthlete(QObject *parent) : QObject(parent), http(this), waiting(false)
{
    connect(&http, SIGNAL(responseReady(const QtSoapMessage &)),
            this, SLOT(getResponse(const QtSoapMessage &)));
}


void
TPAthlete::list(int type, QString user, QString pass)
{
    // if currently downloading fail!
    if (waiting == true) return;

    // setup the soap message
    QString url = appsettings->value(this, GC_TPURL, "www.trainingpeaks.com").toString();
    http.setHost("www.trainingpeaks.com");
    http.setAction("http://www.trainingpeaks.com/TPWebServices/GetAccessibleAthletes");
    current.setMethod("GetAccessibleAthletes", "http://www.trainingpeaks.com/TPWebServices/");
    current.addMethodArgument("username", "", user);
    current.addMethodArgument("password", "", pass);
    current.addMethodArgument("types", "", tptypestrings[type].toLatin1().constData());

    // do it!
    waiting = true;
    http.submitRequest(current, "/tpwebservices/service.asmx");
}

void TPAthlete::getResponse(const QtSoapMessage &message)
{
    waiting = false;
    QString resultStr;
    QList< QMap<QString, QString> > athletelist;

    if (message.isFault()) {
        resultStr = tr("Error:") + qPrintable(message.faultString().toString());
    }
    else {
        const QtSoapType &response = message.returnValue();

        if (response.isValid()) {

            // lets take look at the payload as a DomDocument
            // I tried to use the QtSoapType routines to walk
            // through the response tree but couldn't get them
            // to work. This code could be simplified if we
            // use the QtSoap routines instead.
            QDomDocument doc;
            QDomElement results = response.toDomElement(doc);
            QMap<QString, QString> athlete;

            for (QDomElement personbase = results.firstChildElement("PersonBase");
                 !personbase.isNull();
                 personbase = personbase.nextSiblingElement()) {

                athlete.clear();

                // shove all the personbase attribs into a QMap
                for (QDomNode child = personbase.firstChild();
                     !child.isNull();
                    child = child.nextSibling()) {

                    athlete.insert(child.nodeName(), child.toElement().text());
                }
                athletelist << athlete;
            }
        }
    }

    // return what we got (empty if non-valid response)
    completed(resultStr, athletelist);
}

//
// List workouts
//
TPWorkout::TPWorkout(QObject *parent) : QObject(parent), http(this), waiting(false)
{
    connect(&http, SIGNAL(responseReady(const QtSoapMessage &)),
            this, SLOT(getResponse(const QtSoapMessage &)));
}


void
TPWorkout::list(int id, QDate from, QDate to, QString user, QString pass)
{
    // if currently downloading fail!
    if (waiting == true) return;

    current = QtSoapMessage(); // reset

    // setup the soap message
    QString url = appsettings->value(this, GC_TPURL, "www.trainingpeaks.com").toString();
    http.setHost("www.trainingpeaks.com");
    http.setAction("http://www.trainingpeaks.com/TPWebServices/GetWorkoutsForAccessibleAthlete");
    current.setMethod("GetWorkoutsForAccessibleAthlete", "http://www.trainingpeaks.com/TPWebServices/");
    current.addMethodArgument("username", "", user);
    current.addMethodArgument("password", "", pass);
    current.addMethodArgument("personId", "", QString("%1").arg(id).toLatin1().constData());
    current.addMethodArgument("startDate", "", from.toString("yyyy-MM-dd").toLatin1().constData());
    current.addMethodArgument("endDate", "", to.toString("yyyy-MM-dd").toLatin1().constData());

    // do it!
    waiting = true;
    http.submitRequest(current, "/tpwebservices/service.asmx");
}

void TPWorkout::getResponse(const QtSoapMessage &message)
{
    waiting = false;
    QList< QMap<QString, QString> > workoutlist;

    if (!message.isFault()) {
        const QtSoapType &response = message.returnValue();

        if (response.isValid()) {

            // lets take look at the payload as a DomDocument
            // I tried to use the QtSoapType routines to walk
            // through the response tree but couldn't get them
            // to work. This code could be simplified if we
            // use the QtSoap routines instead.
            QDomDocument doc;
            QDomElement results = response.toDomElement(doc);
            QMap<QString, QString> athlete;

            for (QDomElement personbase = results.firstChildElement("Workout");
                 !personbase.isNull();
                 personbase = personbase.nextSiblingElement()) {

                athlete.clear();

                // shove all the personbase attribs into a QMap
                for (QDomNode child = personbase.firstChild();
                     !child.isNull();
                    child = child.nextSibling()) {

                    athlete.insert(child.nodeName(), child.toElement().text());
                }
                workoutlist << athlete;
            }
        }
    }

    // return what we got (empty if failed)
    completed(workoutlist);
}


//
// Workout download
//
TPDownload::TPDownload(QObject *parent) : QObject(parent), http(this), downloading(false)
{
    connect(&http, SIGNAL(responseReady(const QtSoapMessage &)),
            this, SLOT(getResponse(const QtSoapMessage &)));
}

void
TPDownload::download(QString cyclist, int personId, int workoutId)
{
    // if currently downloading fail!
    if (downloading == true) return;

    current = QtSoapMessage();

    // setup the soap message
    QString url = appsettings->value(this, GC_TPURL, "www.trainingpeaks.com").toString();
    http.setHost("www.trainingpeaks.com");
    http.setAction("http://www.trainingpeaks.com/TPWebServices/GetExtendedWorkoutsForAccessibleAthlete");
    current.setMethod("GetExtendedWorkoutsForAccessibleAthlete", "http://www.trainingpeaks.com/TPWebServices/");
    current.addMethodArgument("username", "", appsettings->cvalue(cyclist, GC_TPUSER).toString());
    current.addMethodArgument("password", "", appsettings->cvalue(cyclist, GC_TPPASS).toString());
    current.addMethodArgument("personId", "", personId);

    QtSoapStruct *ints = new QtSoapStruct; // gets taken over by message and free'd there
                                           // if you pass a pointer it all goes boom!

    // create a little snippit: <workoutIds><int>xx</int></workoutIds>
    QDomDocument meh;
    QDomNode workoutIds = meh.createElement("workoutIds");
    QDomElement elint = meh.createElement("int");
    QDomText text = meh.createTextNode(QString("%1").arg(workoutId));
    elint.appendChild(text);
    workoutIds.appendChild(elint);

    // turn it into the QtSoapStruct needed by the QtSoapMessage
    ints->parse(workoutIds);

    // add as method arguments
    current.addMethodArgument(ints);

    // do it!
    downloading = true;

    http.submitRequest(current, "/tpwebservices/service.asmx");
    return;
}

void TPDownload::getResponse(const QtSoapMessage &message)
{
    downloading = false;
    QString result;
    QDomDocument pwx("PWX");

    if (!message.isFault()) {
        const QtSoapType &response = message.returnValue();
        QDomNode workout = response.toDomElement(pwx).firstChild();
        pwx.appendChild(workout);
    }
    completed(pwx);
}
