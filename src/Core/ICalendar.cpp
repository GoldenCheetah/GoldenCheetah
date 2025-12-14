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

#include "Athlete.h"
#include "ICalendar.h"
#include "GcCalendarModel.h"
#include "CalendarDownload.h"
#include <libical/ical.h>

void ICalendar::setICalendarProperties()
{
#if 0 // keeping it for later, we will eventually use this
      // commented out to reduce compiler warnings
  static struct {
    icalproperty_kind type;
    QString friendly;
  } ICalendarProperties[] = {
    { ICAL_ACTION_PROPERTY, tr("Action") },
    { ICAL_ALLOWCONFLICT_PROPERTY, tr("Allow Conflict") },
    { ICAL_ATTACH_PROPERTY, tr("Attachment") },
    { ICAL_ATTENDEE_PROPERTY, tr("Attendee") },
    { ICAL_CALID_PROPERTY, tr("Calendar Identifier") },
    { ICAL_CALMASTER_PROPERTY, tr("Master") },
    { ICAL_CALSCALE_PROPERTY, tr("Scale") },
    { ICAL_CAPVERSION_PROPERTY, tr("Version") },
    { ICAL_CARLEVEL_PROPERTY, tr("Level") },
    { ICAL_CARID_PROPERTY, tr("Event Identifier") },
    { ICAL_CATEGORIES_PROPERTY, tr("Category") },
    { ICAL_CLASS_PROPERTY, tr("Class") },
    { ICAL_CMD_PROPERTY, tr("Command") },
    { ICAL_COMMENT_PROPERTY, tr("Comment") },
    { ICAL_COMPLETED_PROPERTY, tr("Completed") },
    { ICAL_COMPONENTS_PROPERTY, tr("") },
    { ICAL_CONTACT_PROPERTY, tr("Contact") },
    { ICAL_CREATED_PROPERTY, tr("Date Created") },
    { ICAL_CSID_PROPERTY, tr("CSID?") },
    { ICAL_DATEMAX_PROPERTY, tr("No later than") },
    { ICAL_DATEMIN_PROPERTY, tr("No earlier than") },
    { ICAL_DECREED_PROPERTY, tr("Decreed") },
    { ICAL_DEFAULTCHARSET_PROPERTY, tr("Default character set") },
    { ICAL_DEFAULTLOCALE_PROPERTY, tr("Default locale") },
    { ICAL_DEFAULTTZID_PROPERTY, tr("Default timezone") },
    { ICAL_DEFAULTVCARS_PROPERTY, tr("Default VCar") },
    { ICAL_DENY_PROPERTY, tr("Deny") },
    { ICAL_DESCRIPTION_PROPERTY, tr("Description") },
    { ICAL_DTEND_PROPERTY, tr("End Date & Time") },
    { ICAL_DTSTAMP_PROPERTY, tr("Timestamp") },
    { ICAL_DTSTART_PROPERTY, tr("Start Date & Time") },
    { ICAL_DUE_PROPERTY, tr("Due Date") },
    { ICAL_DURATION_PROPERTY, tr("Duration") },
    { ICAL_EXDATE_PROPERTY, tr("Expiry Date") },
    { ICAL_EXPAND_PROPERTY, tr("Expand") },
    { ICAL_EXRULE_PROPERTY, tr("Exclusive rule") },
    { ICAL_FREEBUSY_PROPERTY, tr("Freebusy") },
    { ICAL_GEO_PROPERTY, tr("Geo") },
    { ICAL_GRANT_PROPERTY, tr("Grant") },
    { ICAL_ITIPVERSION_PROPERTY, tr("ITIP Version") },
    { ICAL_LASTMODIFIED_PROPERTY, tr("Modified Date") },
    { ICAL_LOCATION_PROPERTY, tr("Location") },
    { ICAL_MAXCOMPONENTSIZE_PROPERTY, tr("Max component size") },
    { ICAL_MAXDATE_PROPERTY, tr("No later than") },
    { ICAL_MAXRESULTS_PROPERTY, tr("Maximum results") },
    { ICAL_MAXRESULTSSIZE_PROPERTY, tr("Maximum results size") },
    { ICAL_METHOD_PROPERTY, tr("Method") },
    { ICAL_MINDATE_PROPERTY, tr("Np earlier than") },
    { ICAL_MULTIPART_PROPERTY, tr("Is Multipart") },
    { ICAL_NAME_PROPERTY, tr("Name") },
    { ICAL_ORGANIZER_PROPERTY, tr("Organised by") },
    { ICAL_OWNER_PROPERTY, tr("Owner") },
    { ICAL_PERCENTCOMPLETE_PROPERTY, tr("Percent Complete") },
    { ICAL_PERMISSION_PROPERTY, tr("Permissions") },
    { ICAL_PRIORITY_PROPERTY, tr("Priority") },
    { ICAL_PRODID_PROPERTY, tr("Prod Identifier") },
    { ICAL_QUERY_PROPERTY, tr("Query") },
    { ICAL_QUERYLEVEL_PROPERTY, tr("Query Level") },
    { ICAL_QUERYID_PROPERTY, tr("Query Identifier") },
    { ICAL_QUERYNAME_PROPERTY, tr("Query Name") },
    { ICAL_RDATE_PROPERTY, tr("Recurring Date") },
    { ICAL_RECURACCEPTED_PROPERTY, tr("Recurring Accepted") },
    { ICAL_RECUREXPAND_PROPERTY, tr("Recurring Expanded") },
    { ICAL_RECURLIMIT_PROPERTY, tr("Recur no later than") },
    { ICAL_RECURRENCEID_PROPERTY, tr("Reccurrence Identifier") },
    { ICAL_RELATEDTO_PROPERTY, tr("Related to") },
    { ICAL_RELCALID_PROPERTY, tr("Related to Calendar Identifier") },
    { ICAL_REPEAT_PROPERTY, tr("Repeat") },
    { ICAL_REQUESTSTATUS_PROPERTY, tr("Request Status") },
    { ICAL_RESOURCES_PROPERTY, tr("Resources") },
    { ICAL_RESTRICTION_PROPERTY, tr("Restriction") },
    { ICAL_RRULE_PROPERTY, tr("Rule") },
    { ICAL_SCOPE_PROPERTY, tr("Scope") },
    { ICAL_SEQUENCE_PROPERTY, tr("Sequence Number") },
    { ICAL_STATUS_PROPERTY, tr("Status") },
    { ICAL_STORESEXPANDED_PROPERTY, tr("Stores Expanded") },
    { ICAL_SUMMARY_PROPERTY, tr("Summary") },
    { ICAL_TARGET_PROPERTY, tr("Target") },
    { ICAL_TRANSP_PROPERTY, tr("Transport") },
    { ICAL_TRIGGER_PROPERTY, tr("Trigger") },
    { ICAL_TZID_PROPERTY, tr("Timezone Identifier") },
    { ICAL_TZNAME_PROPERTY, tr("Timezone Name") },
    { ICAL_TZOFFSETFROM_PROPERTY, tr("Timezone Offset from") },
    { ICAL_TZOFFSETTO_PROPERTY, tr("Timezone Offset to") },
    { ICAL_TZURL_PROPERTY, tr("Timezone URL") },
    { ICAL_UID_PROPERTY, tr("Unique Identifier") },
    { ICAL_URL_PROPERTY, tr("URL") },
    { ICAL_VERSION_PROPERTY, tr("Version") },
    { ICAL_X_PROPERTY, tr("X-Property") },
    { ICAL_XLICCLASS_PROPERTY, tr("XLI Class") },
    { ICAL_XLICCLUSTERCOUNT_PROPERTY, tr("XLI Cluster count") },
    { ICAL_XLICERROR_PROPERTY, tr("XLI error") },
    { ICAL_XLICMIMECHARSET_PROPERTY, tr("XLI mime character set") },
    { ICAL_XLICMIMECID_PROPERTY, tr("XLI mime class Identifier") },
    { ICAL_XLICMIMECONTENTTYPE_PROPERTY, tr("XLI mime content type") },
    { ICAL_XLICMIMEENCODING_PROPERTY, tr("XLI mime encoding") },
    { ICAL_XLICMIMEFILENAME_PROPERTY, tr("XLI mime filename") },
    { ICAL_XLICMIMEOPTINFO_PROPERTY, tr("XLI mime optional information") },
    { ICAL_NO_PROPERTY, tr("") } // ICAL_NO_PROPERTY must always be last!!
  };
#endif
}

// convert property to a string
static QString propertyToString(icalproperty *p)
{
    if (p) {
        icalvalue *v = icalproperty_get_value(p);
        QString converted(icalvalue_as_ical_string(v));

        // some special characters are escaped in the text
        converted.replace("\\n", "\n");
        converted.replace("\\;", ";");

        return converted;
    } else {
        return QString("");
    }
}

// convert property to a DateTime
static QDateTime propertyToDate(icalproperty *p)
{
    if (p) {
        icalvalue *v = icalproperty_get_value(p);
        struct icaltimetype date = icalvalue_get_datetime(v);
        QDateTime when(QDate(date.year, date.month, date.day),
                       QTime(date.hour, date.minute, date.second));
        return when;
    } else {
        return QDateTime();
    }
}

ICalendar::ICalendar(Context *context) : QWidget(context->mainWidget()), context(context)
{
    // get from local and remote calendar

    // local file
    QString localFilename = context->athlete->home->calendar().absolutePath()+"/calendar.ics";
    QFile localFile(localFilename);
    if (localFile.exists() && localFile.open(QFile::ReadOnly | QFile::Text)) {

        // read in the whole thing
        QTextStream in(&localFile);
        QString fulltext = in.readAll();
        localFile.close();

        // parse
        parse(fulltext, localCalendar);
    }

    // remote file
    context->athlete->calendarDownload->download();
}

void ICalendar::refreshRemote(QString fulltext)
{
    parse(fulltext, remoteCalendar);
}

void ICalendar::parse(QString fulltext, QMap<QDate, QList<icalcomponent*>*>&calendar)
{
    // parse the contents using libical
    icalcomponent *root = icalparser_parse_string(fulltext.toLatin1().constData());

    clearCalendar(calendar,false);

    if (root) {

        // iterate over events (not interested in the rest for now)
        //for (icalcomponent *c = icalcomponent_get_first_component(root, ICAL_VEVENT_COMPONENT);
        for (icalcomponent *event = icalcomponent_get_first_component(root, ICAL_VEVENT_COMPONENT);
            event != NULL;
            event = icalcomponent_get_next_component(root, ICAL_VEVENT_COMPONENT)) {

            // Get start date...
            icalproperty *date = icalcomponent_get_first_property(event, ICAL_DTSTART_PROPERTY);
            if (date) { // ignore events with no date!

                QDate startDate = propertyToDate(date).date();
                QList<icalcomponent*>* events = calendar.value(startDate, NULL);
                if (!events) {
                    events = new QList<icalcomponent*>;
                    calendar.insert(startDate, events);
                }
                events->append(event);
            }
        }
    }
    emit dataChanged();
}

void ICalendar::clearCalendar(QMap<QDate, QList<icalcomponent*>*>&calendar, bool signal)
{
    QMapIterator<QDate, QList<icalcomponent*>* >i(calendar);
    while (i.hasNext()) {
        i.next();
        delete i.value();
    }
    calendar.clear();

    if (signal) emit dataChanged();
}


// for models to pass straight through to access and
// set the calendar data
QVariant ICalendar::data(QDate date, int role)
{
    switch (role) {
        case GcCalendarModel::EventCountRole:
            {
                int count = 0;
                QList<icalcomponent*>*p = localCalendar.value(date, NULL);
                if (p) count += p->count();
                p = remoteCalendar.value(date, NULL);
                if (p) count += p->count();
                return count;
            }

        case Qt::DisplayRole: // returns a list for the given date
        case Qt::EditRole: // returns a list for the given date
            {
                QStringList strings;

                // local
                QList<icalcomponent*>*p = localCalendar.value(date, NULL);
                if (p) {
                    foreach(icalcomponent*event, *p) {
                        QString desc;
                        icalproperty *summary = icalcomponent_get_first_property(event, ICAL_SUMMARY_PROPERTY);
                        desc = propertyToString(summary);
                        icalproperty *property = icalcomponent_get_first_property(event, ICAL_DESCRIPTION_PROPERTY);
                        desc += " ";
                        desc += propertyToString(property);
                        strings << desc;
                    }
                }
                // remote
                p = remoteCalendar.value(date, NULL);
                if (p) {
                    foreach(icalcomponent*event, *p) {
                        QString desc;
                        icalproperty *summary = icalcomponent_get_first_property(event, ICAL_SUMMARY_PROPERTY);
                        desc = propertyToString(summary);
                        icalproperty *property = icalcomponent_get_first_property(event, ICAL_DESCRIPTION_PROPERTY);
                        desc += " ";
                        desc += propertyToString(property);
                        strings << desc;
                    }
                }
                return strings;
            }
            break;
        default:
            break;
    }
    return QVariant();
}
