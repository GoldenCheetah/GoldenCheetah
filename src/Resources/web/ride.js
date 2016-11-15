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

"use strict";

// MAP

function Map_init()
{
    var mapOptions = {
        mapTypeId: google.maps.MapTypeId.ROADMAP
    };
    this.map = new google.maps.Map(document.getElementById(this.id),mapOptions);
    var retArray = this.rider.route;
    var routePolyline = retArray[0];
    var mapBounds = retArray[1];
    routePolyline.setMap(this.map);
    this.map.fitBounds(mapBounds);
    this.marker = new google.maps.Marker();
    this.marker.setMap(this.map);
}

function Map_update()
{
    var curPosition = this.rider.position();
    if(curPosition == this.position){
        return;
    }
    var coord = this.rider.coord();
    if(coord.lat() == 0) return;
    if(coord.lng() == 0) return;
    this.map.panTo(coord);
    this.marker.setPosition(coord);
}

function Map(rider)
{
    this.rider = rider;
    this.map = null;
    this.marker = null;
    this.update = Map_update;
    this.init = Map_init;
    this.position = 0;
}

function MapFactory(id)
{
    if(typeof(GCRider) != "undefined")
    {
        // we are running inside GC
        GCRider.time = 1;
        var map = new Map(new Rider(GCRider));
        map.id = id;
        myArray.push(map);
    }
}

// STREET VIEW
function StreetView_init()
{
    var streetOptions = {
        addressControl : false,
        panControl : false,
        linksControl : false,
        scrollwheel : false,
        zoomControl : false
    };
    this.street = new google.maps.StreetViewPanorama(document.getElementById(this.id),streetOptions);
}

function StreetView_update()
{
    var curPosition = this.rider.position();
    if(curPosition == this.position  || curPosition == 0) {
        return;
    }
    this.position = curPosition;
    var h = this.rider.computeHeading();
    this.street.setPov({ heading: h, pitch: 10, zoom: 1 });
    this.street.setPosition(this.rider.coord());
    this.street.setVisible(true);
}

function StreetView(rider)
{
    this.rider = rider;
    this.update = StreetView_update;
    this.init = StreetView_init;
    this.position = 0;
}

function StreetViewFactory(id)
{
    if(typeof(GCRider) != "undefined")
    {
        // we are running inside GC
        GCRider.time = 1;
        var street = new StreetView(new Rider(GCRider));
        street.id = id;
        myArray.push(street);
    }
}

function animateLoop(eventCount)
{
    try
    {
        var done;
        for(var i = 0; i < myArray.length; i++) {
            myArray[i].update();
            if(done == false)
            {
                done = myArray[i].rider.isDone;
            }
        }
        eventCount = eventCount + 1;
        if(done != true)
        {
            setTimeout("animateLoop(" + eventCount+ ")",1000);
        }
    }
    catch(err) {
        // dump everything into a string
        var debugStr;
        for(var p in err) {
            debugStr += "property: " + p + " value: [" + err[p] + "]\n";
        }
        debugDebug += "toString(): " + " value: [" + err.toString() + "]";
        alert(debugStr);
    }
}


