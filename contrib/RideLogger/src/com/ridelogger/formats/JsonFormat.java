package com.ridelogger.formats;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;

import com.ridelogger.R;
import com.ridelogger.RideService;

import android.content.SharedPreferences;
import android.preference.PreferenceManager;

public class JsonFormat extends BaseFormat<Object> {
    public JsonFormat(RideService rideService) {
        super(rideService);
        subExt = ".json";
    }
    
    public void writeHeader() {
        SharedPreferences settings  = PreferenceManager.getDefaultSharedPreferences(context);
        Date startDate              = new Date(context.startTime);        
        
        SimpleDateFormat startTimef = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss");
        SimpleDateFormat filef      = new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss");
        SimpleDateFormat month      = new SimpleDateFormat("MMMMM");
        SimpleDateFormat year       = new SimpleDateFormat("yyyy");
        SimpleDateFormat day        = new SimpleDateFormat("EEEEE");
        
        startTimef.setTimeZone(TimeZone.getTimeZone("UTC"));
        filef.setTimeZone(TimeZone.getTimeZone("UTC"));     
        month.setTimeZone(TimeZone.getTimeZone("UTC"));     
        year.setTimeZone(TimeZone.getTimeZone("UTC"));      
        day.setTimeZone(TimeZone.getTimeZone("UTC")); 
        
        try {
            buf.write("{" +
                    "\"RIDE\":{" +
                        "\"STARTTIME\":\"" + startTimef.format(startDate) + " UTC\"," +
                        "\"RECINTSECS\":1," +
                        "\"DEVICETYPE\":\"Android\"," +
                        "\"IDENTIFIER\":\"\"," +
                        "\"TAGS\":{" +
                            "\"Athlete\":\"" + settings.getString(context.getString(R.string.PREF_RIDER_NAME), "") + "\"," +
                            "\"Calendar Text\":\"Auto Recored Android Ride\"," +
                            "\"Change History\":\"\"," +
                            "\"Data\":\"\"," +
                            "\"Device\":\"\"," +
                            "\"Device Info\":\"\"," +
                            "\"File Format\":\"\"," +
                            "\"Filename\":\"\"," +
                            "\"Month\":\"" + month.format(startDate) +"\"," +
                            "\"Notes\":\"\"," +
                            "\"Objective\":\"\"," +
                            "\"Sport\":\"Bike\"," +
                            "\"Weekday\":\"" + day.format(startDate) + "\"," +
                            "\"Workout Code\":\"\"," +
                            "\"Year\":\"" + year.format(startDate) + "\"" +
                        "}," +
                        "\"SAMPLES\":[{\"SECS\":0}");
        } catch (Exception e) {}
    }
    
    
    public void writeValues() {
        try {
            synchronized (buf) {                    
                buf.write(",{");
                buf.write("\"");
                buf.write(RideService.KEYS[0]);
                buf.write("\":");
                buf.write(String.format("%f", context.currentValues[0]));
                
                for (int i = 1; i < context.currentValues.length; i++) {
                    buf.write(",\"");
                    buf.write(RideService.KEYS[i]);
                    buf.write("\":");
                    buf.write(String.format("%f", context.currentValues[i]));
                }

                buf.write("}");
            }
        } catch (Exception e) {}
    }
    
    
    public void writeFooter() {
        try {
            buf.write("]}}");
            
        } catch (Exception e) {}
        super.writeFooter();
    }
}
