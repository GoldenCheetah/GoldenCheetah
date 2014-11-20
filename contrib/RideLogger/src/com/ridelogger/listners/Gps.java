package com.ridelogger.listners;

import java.util.HashMap;
import java.util.Map;

import android.content.Context;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;

public class Gps extends Base
{
    public Gps(Context mContext) 
    {
        super(mContext);
        
        LocationManager locationManager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);

        
        LocationListener locationListener = new LocationListener() {
                public void onLocationChanged(Location location) {
                    Map<String, String> map = new HashMap<String, String>();
                    map.put("ALTITUDE", reduceNumberToString(location.getAltitude()) );
                    map.put("KPH",      reduceNumberToString(location.getSpeed())    );
                    map.put("bearing",  reduceNumberToString(location.getBearing())  );
                    map.put("gpsa",     reduceNumberToString(location.getAccuracy()) );
                    map.put("LAT",      reduceNumberToString(location.getLatitude()) );
                    map.put("LON",      reduceNumberToString(location.getLongitude()));
                    alterCurrentData(map);
                }
            
                @Override
                public void onStatusChanged(String provider, int status, Bundle extras) {}
            
                @Override
                public void onProviderEnabled(String provider) {}
            
                @Override
                public void onProviderDisabled(String provider) {}
          };
          
          locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 0, 0, locationListener);
    }
}