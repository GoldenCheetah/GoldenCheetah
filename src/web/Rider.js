/*
 * Copyright (c) 2011 Greg Lonnon (greg.lonnon@gmail.com)
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


// This the rider class...
// It can take a JSON file and turn it into a rider.
// or (in the future...) take live input from the RealtimeWindow


function Rider_createRoute()
{
    var path = new Array();
    var bounds = new google.maps.LatLngBounds();
    var routeLon = this.riderBridge.routeLon;
    var routeLat = this.riderBridge.routeLat;
    var length = routeLon.length;

    for(var i = 0; i < length ; i++)
    {
        var lon = routeLon[i];
        var lat = routeLat[i];
        if(lon == 0) continue;
        if(lat == 0) continue;
        var coord = new google.maps.LatLng(lat,lon);
        path.push(coord);
        bounds.extend(coord);
    }
    var polyline = new google.maps.Polyline();
    polyline.setPath(path);
    return [polyline,bounds,path];
}

function Rider_validTime(time)
{
    if((time > 0) && (time <= this.routeLength))
  {
      return time;
  }
  else
  {
      return 0;
  }
}

function Rider_setTime(time)
{
    this.time = this.validTime(time);
}

function Rider_isDone(time)
{
    if(this.routeLength < time)
    {
        return true;
    }
    return false;
}

function Rider_computeHeading()
{
    var pos1 = this.position();
    var pos0 = pos1 -1;
    var coord1 = this.route[2][pos1];
    var coord0 = this.route[2][pos0];
    return google.maps.geometry.spherical.computeHeading(coord0,coord1);
}

function Rider_coord()
{
    return this.route[2][this.position()];
}

function Rider_position()
{
    return this.riderBridge.position;
}

function Rider(riderBridge)
{
    this.riderBridge = riderBridge;
    this.setTime = Rider_setTime;
    this.createRoute = Rider_createRoute;
    this.route = this.createRoute();
    this.routeLength = this.route[2].length;
    this.isDone = Rider_isDone;
    this.computeHeading = Rider_computeHeading;
    this.validTime = Rider_validTime;
    this.coord = Rider_coord;
    this.position = Rider_position;
}
