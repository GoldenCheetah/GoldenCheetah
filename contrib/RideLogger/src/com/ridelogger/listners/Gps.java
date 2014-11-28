package com.ridelogger.listners;

import java.util.LinkedHashMap;
import com.ridelogger.RideService;
import android.content.Context;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;

/**
 * Gps 
 * @author henry
 * Listen and log gps events
 */
public class Gps extends Base<Gps>
{
    public Gps(RideService mContext) 
    {
        super(mContext);
        
        LocationManager locationManager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);

        //listen to gps events and log them
        LocationListener locationListener = new LocationListener() {
                public void onLocationChanged(Location location) {
                    LinkedHashMap<String, String> map = new LinkedHashMap<String, String>();
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