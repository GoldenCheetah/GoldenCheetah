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


#include "KmlRideFile.h"
#include <QDebug>
#include <QObject>

#include <time.h>
#include <iostream>
#include <string>
#include "kml/base/date_time.h"
#include "kml/base/expat_parser.h"
#include "kml/base/file.h"
#include "kml/base/vec3.h"
#include "kml/convenience/convenience.h"
#include "kml/convenience/gpx_trk_pt_handler.h"
#include "kml/dom.h"
#include "kml/dom/kml_ptr.h"

// majority of code swiped from the libkml example gpx2kml.cc
using kmlbase::ExpatParser;
using kmlbase::DateTime;
using kmlbase::Vec3;
using kmlbase::Attributes;
using kmldom::ContainerPtr;
using kmldom::FolderPtr;
using kmldom::IconStylePtr;
using kmldom::IconStyleIconPtr;
using kmldom::KmlFactory;
using kmldom::KmlPtr;
using kmldom::LabelStylePtr;
using kmldom::ListStylePtr;
using kmldom::PairPtr;
using kmldom::PointPtr;
using kmldom::SchemaPtr;
using kmldom::ExtendedDataPtr;
using kmldom::SchemaDataPtr;
using kmldom::GxSimpleArrayFieldPtr;
using kmldom::GxSimpleArrayDataPtr;
using kmldom::GxMultiTrackPtr;
using kmldom::GxTrackPtr;
using kmldom::PlacemarkPtr;
using kmldom::TimeStampPtr;
using kmldom::StylePtr;
using kmldom::StyleMapPtr;
using kmldom::LineStylePtr;
using kmldom::CoordinatesPtr;
using kmldom::PointPtr;

static int kmlFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "kml", "Google Earth KML", new KmlFileReader());
//
// Utility functions
//
static const string kDotIcon =
    "http://maps.google.com/mapfiles/kml/shapes/shaded_dot.png";

static const string kStartIcon =
    "http://maps.google.com/mapfiles/kml/paddle/wht-circle.png";

static const string kEndIcon =
    "http://maps.google.com/mapfiles/kml/paddle/stop.png";

static const char* kStyleMapId = "style-map";
static const char* kStyleMapIdUrl = "#style-map";


static ExtendedDataPtr CreateExtendedData() {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  ExtendedDataPtr ed = kml_factory->CreateExtendedData();
  return ed;
}

static SchemaDataPtr CreateSchemaData(string name) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  SchemaDataPtr sd = kml_factory->CreateSchemaData();
  sd->set_schemaurl("#" + name);
  return sd;
}

static GxSimpleArrayDataPtr CreateGxSimpleArrayData(string name) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  GxSimpleArrayDataPtr sa = kml_factory->CreateGxSimpleArrayData();
  sa->set_name(name);
  return sa;
}

static PlacemarkPtr CreateGxTrackPlacemark(string name, string description, GxTrackPtr tracks) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();

  PlacemarkPtr placemark = kml_factory->CreatePlacemark();
  placemark->set_name(name);
  placemark->set_id(name);
  placemark->set_description(description);
  placemark->set_styleurl(kStyleMapIdUrl);

  placemark->set_geometry(tracks);
  return placemark;
}

static GxTrackPtr CreateGxTrack(string name) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  GxTrackPtr track = kml_factory->CreateGxTrack();
  track->set_id(name);
  return track;
}

static IconStylePtr CreateIconStyle(double scale, string iconUrl ) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  IconStyleIconPtr icon = kml_factory->CreateIconStyleIcon();
  icon->set_href(iconUrl);
  IconStylePtr icon_style = kml_factory->CreateIconStyle();
  icon_style->set_icon(icon);
  icon_style->set_scale(scale);
  return icon_style;
}

static PlacemarkPtr CreateStartPlacemark(double lat, double lon) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();

  // Create <coordinates>.
  CoordinatesPtr coordinates = kml_factory->CreateCoordinates();
  coordinates->add_latlng(lat, lon);
  // Create <Point> and give it <coordinates>.
  PointPtr point = kml_factory->CreatePoint();
  point->set_coordinates(coordinates);  // point takes ownership
  // Create Placemark
  PlacemarkPtr placemark = kml_factory->CreatePlacemark();
  placemark->set_name(QObject::tr("Start").toStdString());
  placemark->set_id("start");
  placemark->set_geometry(point);  // placemark takes ownership

  StylePtr style = kml_factory->CreateStyle();
  style->set_iconstyle(CreateIconStyle(1, kStartIcon));
  placemark->set_styleselector(style);
  return placemark;
}

static PlacemarkPtr CreateEndPlacemark(double lat, double lon) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();

  // Create <coordinates>.
  CoordinatesPtr coordinates = kml_factory->CreateCoordinates();
  coordinates->add_latlng(lat, lon);
  // Create <Point> and give it <coordinates>.
  PointPtr point = kml_factory->CreatePoint();
  point->set_coordinates(coordinates);  // point takes ownership
  // Create Placemark
  PlacemarkPtr placemark = kml_factory->CreatePlacemark();
  placemark->set_name(QObject::tr("End").toStdString());
  placemark->set_id("start");
  placemark->set_geometry(point);  // placemark takes ownership

  StylePtr style = kml_factory->CreateStyle();
  style->set_iconstyle(CreateIconStyle(1, kEndIcon));
  placemark->set_styleselector(style);
  return placemark;
}


static SchemaPtr CreateSchema(string name) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  SchemaPtr schema = kml_factory->CreateSchema();
  schema->set_id(name);
  schema->set_name(name);
  return schema;
}

static GxSimpleArrayFieldPtr CreateGxSimpleArrayField(string name, string displayName) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  GxSimpleArrayFieldPtr field = kml_factory->CreateGxSimpleArrayField();
  field->set_type("float");
  field->set_name(name);
  field->set_displayname(displayName);
  return field;
}

static LabelStylePtr CreateLabelStyle(double scale) {
  LabelStylePtr label_style = KmlFactory::GetFactory()->CreateLabelStyle();
  label_style->set_scale(scale);
  return label_style;
}

static PairPtr CreatePair(int style_state, double icon_scale, kmlbase::Color32 color ) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  PairPtr pair = kml_factory->CreatePair();
  pair->set_key(style_state);
  StylePtr style = kml_factory->CreateStyle();
  style->set_iconstyle(CreateIconStyle(icon_scale, kDotIcon));
  // Hide the label in normal style state, visible in highlight.
  style->set_labelstyle(CreateLabelStyle(
      style_state == kmldom::STYLESTATE_NORMAL ? 0 : 1 ));

  LineStylePtr lineStyle = kml_factory->CreateLineStyle();
  lineStyle->set_width(3);
  lineStyle->set_color(color);
  style->set_linestyle(lineStyle);
  pair->set_styleselector(style);
  return pair;
}

static StylePtr CreateRadioFolder(const char* id) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  ListStylePtr list_style = kml_factory->CreateListStyle();
  list_style->set_listitemtype(kmldom::LISTITEMTYPE_RADIOFOLDER);
  StylePtr style = kml_factory->CreateStyle();
  style->set_liststyle(list_style);
  style->set_id(id);
  return style;
}

static StyleMapPtr CreateStyleMap(const char* id) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  StyleMapPtr style_map = kml_factory->CreateStyleMap();
  style_map->set_id(id);
  style_map->add_pair(CreatePair(kmldom::STYLESTATE_NORMAL, 0.1, kmlbase::Color32(0xffffffff)));
  style_map->add_pair(CreatePair(kmldom::STYLESTATE_HIGHLIGHT, 0.3, kmlbase::Color32(0xffffffff)));
  return style_map;
}

//
// Serialise the ride
//
bool
KmlFileReader::writeRideFile(Context *, const RideFile * ride, QFile &file) const
{
    double start_lat = 0.0;
    double start_lon = 0.0;
    double end_lat = 0.0;
    double end_lon = 0.0;

    // Create a new DOM document and setup styles et al
    kmldom::KmlFactory* kml_factory = kmldom::KmlFactory::GetFactory();
    kmldom::DocumentPtr document = kml_factory->CreateDocument();
    const char* kRadioFolderId = "radio-folder-style";
    document->add_styleselector(CreateRadioFolder(kRadioFolderId));
    document->set_styleurl(std::string("#") + kRadioFolderId);
    document->add_styleselector(CreateStyleMap(kStyleMapId));
    document->set_name("Golden Cheetah");

    // add the schema elements for each data series
    SchemaPtr schemadef = CreateSchema("schema");
    // gx:SimpleArrayField ...

    // Note - no all available data series are added here for KML export - further additions on demand
    // general data series
    if (ride->areDataPresent()->hr)
        schemadef->add_gx_simplearrayfield(CreateGxSimpleArrayField("heartrate", QObject::tr("heartrate").toStdString()));
    if (ride->areDataPresent()->temp)
        schemadef->add_gx_simplearrayfield(CreateGxSimpleArrayField("temperature", QObject::tr("temperature").toStdString()));

    // run specific series
    if (ride->isRun()) {
        if (ride->areDataPresent()->rcad)
            schemadef->add_gx_simplearrayfield(CreateGxSimpleArrayField("run cadence", QObject::tr("run cadence").toStdString()));
        if (ride->areDataPresent()->rvert)
            schemadef->add_gx_simplearrayfield(CreateGxSimpleArrayField("vertical oscillation", QObject::tr("vertical oscillation").toStdString()));
        if (ride->areDataPresent()->rcontact)
            schemadef->add_gx_simplearrayfield(CreateGxSimpleArrayField("ground contact time", QObject::tr("ground contact time").toStdString()));
    } else { // Bike specific
        if (ride->areDataPresent()->cad)
            schemadef->add_gx_simplearrayfield(CreateGxSimpleArrayField("cadence", QObject::tr("cadence").toStdString()));
        if (ride->areDataPresent()->watts)
            schemadef->add_gx_simplearrayfield(CreateGxSimpleArrayField("power", QObject::tr("power").toStdString()));
        if (ride->areDataPresent()->nm)
            schemadef->add_gx_simplearrayfield(CreateGxSimpleArrayField("torque", QObject::tr("torque").toStdString()));
        if (ride->areDataPresent()->headwind)
            schemadef->add_gx_simplearrayfield(CreateGxSimpleArrayField("headwind", QObject::tr("headwind").toStdString()));
    }

    document->add_schema(schemadef);

    // setup trip folder (shown on lhs of google earth
    FolderPtr folder = kmldom::KmlFactory::GetFactory()->CreateFolder();
    folder->set_name(QObject::tr("Activities").toStdString());
    document->add_feature(folder);

    // Create a track for the entire ride
    GxTrackPtr track = CreateGxTrack(QObject::tr("Entire Activity").toStdString());
    QString activityType;
    if (ride->isRun())
        activityType = QObject::tr("Run %1").arg(ride->startTime().toString());
    else
        activityType = QObject::tr("Bike %1").arg(ride->startTime().toString());

    QString description = ride->getTag("Calendar Text", activityType);

    PlacemarkPtr placemark = CreateGxTrackPlacemark(activityType.toStdString(), description.toStdString(), track);
    folder->add_feature(placemark);

    //
    // Basic Data -- Lat/Lon/Alt and Timestamp
    //

    foreach (RideFilePoint *datapoint, ride->dataPoints()) {

        // lots of arsing around with dates - use as simple functions a possible to get the right format
        // since some of the convenience functions for Date/Time and String have problems on e.g. Windows platform
        // by using pure QT functions - the conversion to expected format works fine for <when> tag
        QDateTime timestamp(ride->startTime().addSecs(datapoint->secs));
        QString s = timestamp.toString(Qt::ISODate) + "Z";
        if (datapoint->lat && datapoint->lon) {
            track->add_when(s.toStdString());
            if (start_lat == 0.0) {
               start_lat = datapoint->lat;
               start_lon = datapoint->lon;
            };
            end_lat = datapoint->lat;
            end_lon = datapoint->lon;
        }
    }

    // <when> loop through the entire ride
    foreach (RideFilePoint *datapoint, ride->dataPoints()) {
        if (datapoint->lat && datapoint->lon) track->add_gx_coord(kmlbase::Vec3(datapoint->lon, datapoint->lat, datapoint->alt));
    }

    //
    // Extended Data -- similar series to schema definition
    //
    ExtendedDataPtr extended = CreateExtendedData();
    track->set_extendeddata(extended);
    SchemaDataPtr schema = CreateSchemaData("schema");
    extended->add_schemadata(schema);

    // heartrate
    if (ride->areDataPresent()->hr) {
        GxSimpleArrayDataPtr heartrate = CreateGxSimpleArrayData("heartrate");
        schema->add_gx_simplearraydata(heartrate);

        // now create a GxSimpleArrayData
        foreach (RideFilePoint *datapoint, ride->dataPoints()) {
            if (datapoint->lat && datapoint->lon) heartrate->add_gx_value(QString("%1").arg(datapoint->hr).toStdString());
        }
    }

    // temperature
    if (ride->areDataPresent()->temp) {
        GxSimpleArrayDataPtr temperature = CreateGxSimpleArrayData("temperature");
        schema->add_gx_simplearraydata(temperature);

        // now create a GxSimpleArrayData
        foreach (RideFilePoint *datapoint, ride->dataPoints()) {
            if (datapoint->lat && datapoint->lon) temperature->add_gx_value(QString("%1").arg(datapoint->temp).toStdString());
        }
    }

    if (ride->isRun()) {

        // running cadence
        if (ride->areDataPresent()->cad) {
            GxSimpleArrayDataPtr rcadence = CreateGxSimpleArrayData("running cadence");
            schema->add_gx_simplearraydata(rcadence);

            // now create a GxSimpleArrayData
            foreach (RideFilePoint *datapoint, ride->dataPoints()) {
                if (datapoint->lat && datapoint->lon) rcadence->add_gx_value(QString("%1").arg(datapoint->rcad).toStdString());
            }
        }
        // vertical oscillation
        if (ride->areDataPresent()->rvert) {
            GxSimpleArrayDataPtr rvert = CreateGxSimpleArrayData("vertical oscillation");
            schema->add_gx_simplearraydata(rvert);

            // now create a GxSimpleArrayData
            foreach (RideFilePoint *datapoint, ride->dataPoints()) {
                if (datapoint->lat && datapoint->lon) rvert->add_gx_value(QString("%1").arg(datapoint->rvert).toStdString());
            }
        }
        // ground contact time
        if (ride->areDataPresent()->rcontact) {
            GxSimpleArrayDataPtr rgct = CreateGxSimpleArrayData("ground contact time");
            schema->add_gx_simplearraydata(rgct);

            // now create a GxSimpleArrayData
            foreach (RideFilePoint *datapoint, ride->dataPoints()) {
                if (datapoint->lat && datapoint->lon) rgct->add_gx_value(QString("%1").arg(datapoint->rcontact).toStdString());
            }
        }


    } else {  // Biking,...

        // cadence
        if (ride->areDataPresent()->cad) {
            GxSimpleArrayDataPtr cadence = CreateGxSimpleArrayData("cadence");
            schema->add_gx_simplearraydata(cadence);

            // now create a GxSimpleArrayData
            foreach (RideFilePoint *datapoint, ride->dataPoints()) {
                if (datapoint->lat && datapoint->lon) cadence->add_gx_value(QString("%1").arg(datapoint->cad).toStdString());
            }
        }
        // power
        if (ride->areDataPresent()->watts) {
            GxSimpleArrayDataPtr power = CreateGxSimpleArrayData("power");
            schema->add_gx_simplearraydata(power);

            // now create a GxSimpleArrayData
            foreach (RideFilePoint *datapoint, ride->dataPoints()) {
                if (datapoint->lat && datapoint->lon) power->add_gx_value(QString("%1").arg(datapoint->watts).toStdString());
            }
        }
        // torque
        if (ride->areDataPresent()->nm) {
            GxSimpleArrayDataPtr torque = CreateGxSimpleArrayData("torque");
            schema->add_gx_simplearraydata(torque);

            // now create a GxSimpleArrayData
            foreach (RideFilePoint *datapoint, ride->dataPoints()) {
                if (datapoint->lat && datapoint->lon) torque->add_gx_value(QString("%1").arg(datapoint->nm).toStdString());
            }
        }
        // headwind
        if (ride->areDataPresent()->headwind) {
            GxSimpleArrayDataPtr headwind = CreateGxSimpleArrayData("headwind");
            schema->add_gx_simplearraydata(headwind);

            // now create a GxSimpleArrayData
            foreach (RideFilePoint *datapoint, ride->dataPoints()) {
                if (datapoint->lat && datapoint->lon) headwind->add_gx_value(QString("%1").arg(datapoint->headwind).toStdString());
            }
        }

    }

    // add start and end marker
    PlacemarkPtr start = CreateStartPlacemark(start_lat, start_lon);
    folder->add_feature(start);
    PlacemarkPtr end = CreateEndPlacemark(end_lat, end_lon);
    folder->add_feature(end);

    // Create KML document
    kmldom::KmlPtr kml = kml_factory->CreateKml();
    kml->set_feature(document);


    // make sure the google extensions are added in!
    kmlbase::Attributes gxxmlns22;
    gxxmlns22.SetValue("gx", "http://www.google.com/kml/ext/2.2");
    kml->MergeXmlns(gxxmlns22);

    // Serialize
    if (!file.open(QIODevice::WriteOnly)) return(false);
    file.write(kmldom::SerializePretty(kml).data());
    file.close();
    return(true);
}
