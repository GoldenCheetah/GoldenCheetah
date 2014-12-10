package com.ridelogger.listners;

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
                    int[] keys = {
                            RideService.ALTITUDE,
                            RideService.KPH,
                            RideService.bearing,
                            RideService.gpsa,
                            RideService.LAT,
                            RideService.LON
                    };
                    
                    float[] values = {
                            (float) location.getAltitude(),
                            location.getSpeed(),    
                            location.getBearing(),  
                            location.getAccuracy(), 
                            (float) location.getLatitude(), 
                            (float) location.getLongitude()
                    };
                    
                    alterCurrentData(keys, values);
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