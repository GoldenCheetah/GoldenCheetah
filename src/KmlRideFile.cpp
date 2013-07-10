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

static int kmlFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "kml", "Google Earth KML", new KmlFileReader());
//
// Utility functions
//
static const char kDotIcon[] =
    "http://maps.google.com/mapfiles/kml/shapes/shaded_dot.png";


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

static PlacemarkPtr CreateGxTrackPlacemark(string name, GxTrackPtr tracks) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();

  PlacemarkPtr placemark = kml_factory->CreatePlacemark();
  placemark->set_name(name);
  placemark->set_id(name);

  placemark->set_geometry(tracks);
  return placemark;
}

static GxTrackPtr CreateGxTrack(string name) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  GxTrackPtr track = kml_factory->CreateGxTrack();
  track->set_id(name);
  return track;
}

static SchemaPtr CreateSchema(string name) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  SchemaPtr schema = kml_factory->CreateSchema();
  schema->set_id(name);
  schema->set_name(name);
  return schema;
}

static GxSimpleArrayFieldPtr CreateGxSimpleArrayField(string name) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  GxSimpleArrayFieldPtr field = kml_factory->CreateGxSimpleArrayField();
  field->set_type("float");
  field->set_name(name);
  field->set_displayname(name);
  return field;
}

static IconStylePtr CreateIconStyle(double scale) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  IconStyleIconPtr icon = kml_factory->CreateIconStyleIcon();
  icon->set_href(kDotIcon);
  IconStylePtr icon_style = kml_factory->CreateIconStyle();
  icon_style->set_icon(icon);
  icon_style->set_scale(scale);
  return icon_style;
}

static LabelStylePtr CreateLabelStyle(double scale) {
  LabelStylePtr label_style = KmlFactory::GetFactory()->CreateLabelStyle();
  label_style->set_scale(scale);
  return label_style;
}

static PairPtr CreatePair(int style_state, double icon_scale) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  PairPtr pair = kml_factory->CreatePair();
  pair->set_key(style_state);
  StylePtr style = kml_factory->CreateStyle();
  style->set_iconstyle(CreateIconStyle(icon_scale));
  // Hide the label in normal style state, visible in highlight.
  style->set_labelstyle(CreateLabelStyle(
      style_state == kmldom::STYLESTATE_NORMAL ? 0 : 1 ));
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
  style_map->add_pair(CreatePair(kmldom::STYLESTATE_NORMAL, 0.1));
  style_map->add_pair(CreatePair(kmldom::STYLESTATE_HIGHLIGHT, 0.3));
  return style_map;
}

//
// Serialise the ride
//
bool
KmlFileReader::writeRideFile(Context *, const RideFile * ride, QFile &file) const
{
    // Create a new DOM document and setup styles et al
    kmldom::KmlFactory* kml_factory = kmldom::KmlFactory::GetFactory();
    kmldom::DocumentPtr document = kml_factory->CreateDocument();
    const char* kRadioFolderId = "radio-folder-style";
    document->add_styleselector(CreateRadioFolder(kRadioFolderId));
    document->set_styleurl(std::string("#") + kRadioFolderId);
    const char* kStyleMapId = "style-map";
    document->add_styleselector(CreateStyleMap(kStyleMapId));
    document->set_name("Golden Cheetah");

    // add the schema elements for each data series
    SchemaPtr schemadef = CreateSchema("schema");
    // gx:SimpleArrayField ...
    if (ride->areDataPresent()->cad)
        schemadef->add_gx_simplearrayfield(CreateGxSimpleArrayField("cadence"));
    if (ride->areDataPresent()->hr)
        schemadef->add_gx_simplearrayfield(CreateGxSimpleArrayField("heartrate"));
    if (ride->areDataPresent()->watts)
        schemadef->add_gx_simplearrayfield(CreateGxSimpleArrayField("power"));
    if (ride->areDataPresent()->nm)
        schemadef->add_gx_simplearrayfield(CreateGxSimpleArrayField("torque"));
    if (ride->areDataPresent()->headwind)
        schemadef->add_gx_simplearrayfield(CreateGxSimpleArrayField("headwind"));
    document->add_schema(schemadef);

    // setup trip folder (shown on lhs of google earth
    FolderPtr folder = kmldom::KmlFactory::GetFactory()->CreateFolder();
    folder->set_name("Bike Rides");
    document->add_feature(folder);

    // Create a track for the entire ride
    GxTrackPtr track = CreateGxTrack("Entire Ride");
    PlacemarkPtr placemark = CreateGxTrackPlacemark(QString("Bike %1")
                             .arg(ride->startTime().toString()).toStdString(), track);
    folder->add_feature(placemark);

    //
    // Basic Data -- Lat/Lon/Alt and Timestamp
    //
    foreach (RideFilePoint *datapoint, ride->dataPoints()) {

        // lots of arsing around with dates 
        QDateTime timestamp(ride->startTime().addSecs(datapoint->secs));
        string stdctimestamp = timestamp.toString(Qt::ISODate).toStdString() + "Z"; //<< 'Z' fixes crash!
        kmlbase::DateTime *when = kmlbase::DateTime::Create(stdctimestamp.data());
        if (datapoint->lat && datapoint->lon) track->add_when(when->GetXsdDateTime());
    }

    // <when> loop through the entire ride
    foreach (RideFilePoint *datapoint, ride->dataPoints()) {
        if (datapoint->lat && datapoint->lon) track->add_gx_coord(kmlbase::Vec3(datapoint->lon, datapoint->lat, datapoint->alt));
    }

    //
    // Extended Data -- cadence, heartrate, power, torque, headwind
    //
    ExtendedDataPtr extended = CreateExtendedData();
    track->set_extendeddata(extended);
    SchemaDataPtr schema = CreateSchemaData("schema");
    extended->add_schemadata(schema);

    // power
    if (ride->areDataPresent()->watts) {
        GxSimpleArrayDataPtr power = CreateGxSimpleArrayData("power");
        schema->add_gx_simplearraydata(power);

        // now create a GxSimpleArrayData
        foreach (RideFilePoint *datapoint, ride->dataPoints()) {
            if (datapoint->lat && datapoint->lon) power->add_gx_value(QString("%1").arg(datapoint->watts).toStdString());
        }
    }
    // cadence
    if (ride->areDataPresent()->cad) {
        GxSimpleArrayDataPtr cadence = CreateGxSimpleArrayData("cadence");
        schema->add_gx_simplearraydata(cadence);

        // now create a GxSimpleArrayData
        foreach (RideFilePoint *datapoint, ride->dataPoints()) {
            if (datapoint->lat && datapoint->lon) cadence->add_gx_value(QString("%1").arg(datapoint->cad).toStdString());
        }
    }
    // heartrate
    if (ride->areDataPresent()->hr) {
        GxSimpleArrayDataPtr heartrate = CreateGxSimpleArrayData("heartrate");
        schema->add_gx_simplearraydata(heartrate);

        // now create a GxSimpleArrayData
        foreach (RideFilePoint *datapoint, ride->dataPoints()) {
            if (datapoint->lat && datapoint->lon) heartrate->add_gx_value(QString("%1").arg(datapoint->hr).toStdString());
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
