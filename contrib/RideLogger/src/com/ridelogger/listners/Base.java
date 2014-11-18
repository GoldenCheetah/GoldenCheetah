package com.ridelogger.listners;

import android.content.Context;

import com.dsi.ant.plugins.antplus.pcc.defines.DeviceState;
import com.dsi.ant.plugins.antplus.pccbase.PccReleaseHandle;
import com.dsi.ant.plugins.antplus.pccbase.AntPluginPcc.IDeviceStateChangeReceiver;
import com.dsi.ant.plugins.antplus.pccbase.AntPluginPcc.IPluginAccessResultReceiver;
import com.dsi.ant.plugins.antplus.pccbase.MultiDeviceSearch.MultiDeviceSearchResult;
import com.ridelogger.RideService;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.Map;

/**
 * Base class to connects to Heart Rate Plugin and display all the event data.
 */
public class Base 
{
    public static BufferedWriter      buf;
    public static long                start_time;
    public static Map<String, String> current_values;
    public PccReleaseHandle<?>        releaseHandle;
    public Context                    context;
    
    public Base(MultiDeviceSearchResult result, Context mContext) {
        init(mContext);
    }
    
    public Base(Context mContext) {
        init(mContext);
    }
    
    public void init(Context mContext) {
        buf            = RideService.getBuf();
        start_time     = RideService.getStartTime();
        current_values = RideService.getCurrentValues();
        context        = mContext;
    }
    
    IDeviceStateChangeReceiver mDeviceStateChangeReceiver = new IDeviceStateChangeReceiver() {
        @Override
        public void onDeviceStateChange(final DeviceState newDeviceState){}
    };
    
    public IPluginAccessResultReceiver<?> mResultReceiver;
    
    public void writeData(String key, String value)
    {
        if(!current_values.containsKey(key) || current_values.get(key) != value) {
            String ts = String.valueOf((double) (System.currentTimeMillis() - start_time) / 1000.0);
            current_values.put("SECS", ts);
            current_values.put(key, value);

            try {
                synchronized (buf) {
                    buf.write(",{");
                    
                    buf.write("\"");
                    buf.write("SECS");
                    buf.write("\":");
                    buf.write(ts);
                    
                    buf.write(",\"");
                    buf.write(key);
                    buf.write("\":");
                    buf.write(value);
                    
                    buf.write("}");
                }
            } catch (IOException e) {}
        }
    }
    
    
    public void writeData(Map<String, String> map)
    {
            String ts = String.valueOf((double) (System.currentTimeMillis() - start_time) / 1000.0);
            current_values.put("SECS", ts);

            try {
                synchronized (buf) {
                    buf.write(",{");
                    
                    buf.write("\"");
                    buf.write("SECS");
                    buf.write("\":");
                    buf.write(ts);
                    
                    for (Map.Entry<String, String> entry : map.entrySet())
                    {
                        String key   = entry.getKey();
                        String value = entry.getValue();
                        
                        buf.write(",\"");
                        buf.write(key);
                        buf.write("\":");
                        buf.write(value);
                        
                        current_values.put(key, value);
                    }

                    buf.write("}");
                }
            } catch (IOException e) {}
    }
    
    
    public void onDestroy()
    {
        if(releaseHandle != null) {
            releaseHandle.close();
        }
    }
}


