package com.ridelogger.listners;

import com.ridelogger.GzipWriter;
import com.ridelogger.RideService;

import java.io.IOException;
import java.math.BigDecimal;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;

/**
 * Base
 * @author Chet Henry
 * Base sensor class that has methods to time stamp are write to buffer
 */
public class Base<T>
{
    public static GzipWriter          buf;
    public static long                startTime;
    public static Map<String, String> currentValues;
    
    public RideService                context;
        
    public Base(RideService mContext) {
        init(mContext);
    }
    
    //setup references to buffer and current values and context
    public void init(RideService mContext) {
        context       = mContext;
        buf           = context.buf;           //shared file buffer object
        currentValues = context.currentValues; //shared currentValues object
    }
    
    
    public void writeData(String key, String value)
    {
        if(!currentValues.containsKey(key) || currentValues.get(key) != value) {
            String ts = getTs();
            currentValues.put("SECS", ts);
            currentValues.put(key, value);

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
        String ts = getTs();
        currentValues.put("SECS", ts);

        try {
            synchronized (buf) {
                buf.write(",{");
                
                buf.write("\"");
                buf.write("SECS");
                buf.write("\":");
                buf.write(ts);
                
                for (Map.Entry<String, String> entry : map.entrySet()) {
                    String key   = entry.getKey();
                    String value = entry.getValue();
                    
                    buf.write(",\"");
                    buf.write(key);
                    buf.write("\":");
                    buf.write(value);
                    
                    currentValues.put(key, value);
                }

                buf.write("}");
            }
        } catch (IOException e) {}
    }
    
    
    public void alterCurrentData(String key, String value)
    {
        synchronized (currentValues) {
            currentValues.put("SECS", getTs());
            currentValues.put(key, value);
            writeCurrentData();
        }

    }
    
    
    public void alterCurrentData(Map<String, String> map)
    {
        synchronized (currentValues) {
            currentValues.put("SECS", getTs());
            
            for (Map.Entry<String, String> entry : map.entrySet()) {               
                currentValues.put(entry.getKey(), entry.getValue());
            }
            
            writeCurrentData();
        }
    }
    
    
    public void writeCurrentData()
    {
        try {
            synchronized (buf) {
                buf.write(",{");
                
                Iterator<Entry<String, String>> it = currentValues.entrySet().iterator();
                if(it.hasNext()) {
                    buf.write("\"");
                    Entry<String, String> entry = it.next();
                    buf.write(entry.getKey());
                    buf.write("\":");
                    buf.write(entry.getValue());
                }
                
                while (it.hasNext()) {
                    Entry<String, String> entry = it.next();
                    buf.write(",\"");
                    buf.write(entry.getKey());
                    buf.write("\":");
                    buf.write(entry.getValue());
                }

                buf.write("}");
            }
        } catch (IOException e) {}
    }
    
    //get current time stamp
    public String getTs() {
        return reduceNumberToString((double) (System.currentTimeMillis() - context.startTime) / 1000.0);   
    }
    
    
    //reduce number data types to consistently formatted strings
    public static String reduceNumberToString(double d)
    {
        if(d == (long) d)
            return String.format("%d",(long)d);
        else
            return String.format("%f", d);
    }
    
    
    //reduce number data types to consistently formatted strings
    public static String reduceNumberToString(float d)
    {
        if(d == (long) d)
            return String.format("%d",(long)d);
        else
            return String.format("%f", d);
    }
    
    
    //reduce number data types to consistently formatted strings
    public static String reduceNumberToString(BigDecimal d)
    {
        return String.format("%s", d.toPlainString());
    }
    
    
    //Clean up my listeners here
    public void onDestroy() {}
}


