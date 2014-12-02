package com.ridelogger.listners;

import com.ridelogger.GzipWriter;
import com.ridelogger.RideService;

import java.io.IOException;

/**
 * Base
 * @author Chet Henry
 * Base sensor class that has methods to time stamp are write to buffer
 */
public class Base<T>
{
    public GzipWriter  buf;
    public long        startTime;
    
    public RideService context;
        
    public Base(RideService mContext) {
        context       = mContext;
        buf           = context.buf; //shared file buffer object
    }
    
    
    public void writeData(int key, float value)
    {
        if(context.currentValues[key] != value) {
            context.currentValues[RideService.SECS] = getTs();
            context.currentValues[key]              = value;

            try {
                synchronized (buf) {
                    buf.write(",{");
                    
                    buf.write("\"");
                    buf.write((String) RideService.KEYS[RideService.SECS]);
                    buf.write("\":");
                    buf.write(String.format("%f", context.currentValues[RideService.SECS]));
                    
                    buf.write(",\"");
                    buf.write((String) RideService.KEYS[key]);
                    buf.write("\":");
                    buf.write(String.format("%f", value));
                    
                    buf.write("}");
                }
            } catch (IOException e) {}
        }
    }
    
    
    public void writeData(int[] keys, float[] values)
    {
        context.currentValues[RideService.SECS] = getTs();

        try {
            synchronized (buf) {
                buf.write(",{");
                
                buf.write("\"");
                buf.write((String) RideService.KEYS[RideService.SECS]);
                buf.write("\":");
                buf.write(String.format("%f", context.currentValues[RideService.SECS]));
                
                int i = 0;
                for (int key : keys) {
                    context.currentValues[key] = values[i];
                    buf.write(",\"");
                    buf.write((String) RideService.KEYS[key]);
                    buf.write("\":");
                    buf.write(String.format("%f", context.currentValues[i]));
                    i++;
                }

                buf.write("}");
            }
        } catch (IOException e) {}
    }
    
    
    public void writeSnoopedData(int key, float value)
    {
        if(context.currentValues[key] != value) {
            context.snoopedValues[RideService.SECS] = getTs();
            context.snoopedValues[key]              = value;
        }
    }
    
    
    public void writeSnoopedData(int[] keys, float[] values)
    {
        context.snoopedValues[RideService.SECS] = getTs();
        synchronized (context.snoopedValues) {
            int i = 0;
            for (int key : keys) {
                context.snoopedValues[key] = values[i];
                i++;
            }
        }
    }
    
    
    public void alterCurrentData(int key, float value)
    {
        synchronized (context.currentValues) {
            context.currentValues[RideService.SECS] = getTs();
            context.currentValues[key] = value;
            writeCurrentData();
        }
    }
    
    
    public void alterSnoopedData(int key, float value)
    {
        synchronized (context.snoopedValues) {
            context.snoopedValues[RideService.SECS] = getTs();
            context.snoopedValues[key] = value;
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
            
            writeCurrentData();
        }
    }
    
    
    public void alterSnoopedData(int[] keys, float[] values)
    {
        synchronized (context.snoopedValues) {
            context.snoopedValues[RideService.SECS] = getTs();
            
            int i = 0;
            for (int key : keys) {
                context.snoopedValues[key] = values[i];
                i++;
            }
        }
    }
    
    
    public void writeCurrentData()
    {
        try {
            synchronized (buf) {
                buf.write(",{");
                buf.write("\"");
                buf.write((String) RideService.KEYS[0]);
                buf.write("\":");
                buf.write(String.format("%f", context.currentValues[0]));
                
                for (int i = 1; i < context.currentValues.length; i++) {
                    buf.write(",\"");
                    buf.write((String) RideService.KEYS[i]);
                    buf.write("\":");
                    buf.write(String.format("%f", context.currentValues[i]));
                }

                buf.write("}");
            }
        } catch (IOException e) {}
    }
    
    
    //get current time stamp
    public float getTs() {
        return (float) ((System.currentTimeMillis() - context.startTime) / 1000.0);   
    }
    
    
    //Clean up my listeners here
    public void onDestroy() {}
}


