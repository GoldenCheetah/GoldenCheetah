package com.ridelogger.listners;

import com.ridelogger.RideService;

/**
 * Base
 * @author Chet Henry
 * Base sensor class that has methods to time stamp are write to buffer
 */
public class Base<T>
{    
    public RideService context;
        
    public Base(RideService mContext) {
        context       = mContext;
    }


    public void alterCurrentData(int key, float value)
    {
        synchronized (context.currentValues) {
            context.currentValues[RideService.SECS] = getTs();
            context.currentValues[key] = value;
            context.fileFormat.writeValues();
        }
    }

    
    public void alterCurrentData(int[] keys, float[] values)
    {
        synchronized (context.currentValues) {
            context.currentValues[RideService.SECS] = getTs();
            
            int i = 0;
            for (int key : keys) {
                context.currentValues[key] = values[i];
                i++;
            }
            
            context.fileFormat.writeValues();
        }
    }
    
    
    //get current time stamp
    public float getTs() {
        return (float) ((System.currentTimeMillis() - context.startTime) / 1000.0);   
    }
    
    
    //Clean up my listeners here
    public void onDestroy() {}
    
    //zero any of my values
    public void zeroReadings() {}
}


