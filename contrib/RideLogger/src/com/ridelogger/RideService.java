package com.ridelogger;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.TimeZone;

import com.dsi.ant.plugins.antplus.pcc.defines.DeviceType;
import com.dsi.ant.plugins.antplus.pcc.defines.RequestAccessResult;
import com.dsi.ant.plugins.antplus.pccbase.MultiDeviceSearch;
import com.dsi.ant.plugins.antplus.pccbase.MultiDeviceSearch.MultiDeviceSearchResult;
import com.ridelogger.R;
import com.ridelogger.listners.Gps;
import com.ridelogger.listners.HeartRate;
import com.ridelogger.listners.Power;
import com.ridelogger.listners.Sensors;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.app.TaskStackBuilder;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Environment;
import android.os.IBinder;
import android.support.v4.app.NotificationCompat;

public class RideService extends Service
{
    public static  BufferedWriter      buf;
    public static  long                start_time;
    public static  Map<String, String> current_values;
    public         boolean             rideStarted = false;
    
    public static  HeartRate           hr;
    public static  Power               w;
    public static  Gps                 gps;
    public static  Sensors             sensors;
    
    MultiDeviceSearch                  mSearch;
    MultiDeviceSearch.SearchCallbacks  mCallback;
    MultiDeviceSearch.RssiCallback     mRssiCallback;
    
    public int                        notifyID = 1;
    NotificationManager               mNotificationManager;
    
    public String                     file_name = "";
    SharedPreferences settings;
    
    /**
     * 
     * @return BufferedWriter
     */
    public static BufferedWriter getBuf() { 
        return buf; 
    }
    
    /**
     * 
     * @return start_time
     */
    public static long getStartTime() { 
        return start_time; 
    }
    
    
    /**
     * 
     * @return start_time
     */
    public static Map<String, String> getCurrentValues() { 
        return current_values; 
    }
    
    
    /**
     * 
     * @return w
     */
    public static Power getPower() { 
        return w; 
    }
    
    /**
     * 
     * @return w
     */
    public static void setPower(Power pw) { 
        w = pw;
        if(w == null) {
            w = pw;
        }
    }
    
    /**
     * 
     * @return hr
     */
    public static HeartRate getHeartRate() { 
        return hr; 
    }
    

    public static void setHeartRate(HeartRate phr) {
        if(hr == null) {
            hr = phr;
        }
    }

    
    public static void setGps(Gps pgps) {
        if(gps == null) {
            gps = pgps;
        }
    }
    
    public static Gps getGps() {
        return gps;
    }
    
    
    public static void setSensors(Sensors psens) {
        if(sensors == null) {
            sensors = psens;
        }
    }
    
    public static Sensors getSensors() {
        return sensors;
    }
    
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        startRide();
        return Service.START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent arg0) {
        return null;
    }
    
    public boolean onUnbind (Intent intent) {
        return true;
    }
    
    @Override
    public void onDestroy() {
        stopRide();
        super.onDestroy();
    }
    
    protected void startRide() {
        if(rideStarted) return;
        
        start_time     = System.currentTimeMillis();
        file_name      = "ride-" + start_time + ".json";

        current_values = new HashMap<String, String>();
        
        SimpleDateFormat f = new SimpleDateFormat("yyyy/MMM/dd HH:mm:ss");
        f.setTimeZone(TimeZone.getTimeZone("UTC"));
        String utc = f.format(new Date(start_time));
        
        Calendar cal = Calendar.getInstance();
        cal.setTimeInMillis(start_time);
        
        String month      = cal.getDisplayName(Calendar.MONTH, Calendar.LONG, Locale.US);
        String week_day   = cal.getDisplayName(Calendar.DAY_OF_WEEK, Calendar.LONG, Locale.US);
        String year       = Integer.toString(cal.get(Calendar.YEAR));
        settings          = getSharedPreferences(StartActivity.PREFS_NAME, 0);
        String rider_name = settings.getString(StartActivity.RIDER_NAME, "");
        final Set<String> pairedAnts = settings.getStringSet(StartActivity.PAIRED_ANTS, null);
        
        
        current_values.put("SECS", "0.0");
        
        String rideHeadder = "{" +
            "\"RIDE\":{" +
                "\"STARTTIME\":\"" + utc + " UTC\"," +
                "\"RECINTSECS\":1," +
                "\"DEVICETYPE\":\"Android\"," +
                "\"IDENTIFIER\":\"\"," +
                "\"TAGS\":{" +
                    "\"Athlete\":\"" + rider_name + "\"," +
                    "\"Calendar Text\":\"Auto Recored Android Ride\"," +
                    "\"Change History\":\"\"," +
                    "\"Data\":\"\"," +
                    "\"Device\":\"\"," +
                    "\"Device Info\":\"\"," +
                    "\"File Format\":\"\"," +
                    "\"Filename\":\"\"," +
                    "\"Month\":\"" + month +"\"," +
                    "\"Notes\":\"\"," +
                    "\"Objective\":\"\"," +
                    "\"Sport\":\"Bike\"," +
                    "\"Weekday\":\"" + week_day + "\"," +
                    "\"Workout Code\":\"\"," +
                    "\"Year\":\"" + year + "\"" +
                "}," +
                "\"SAMPLES\":[{\"SECS\":0}";
        if (Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState())) {
            File dir = new File(
                Environment.getExternalStoragePublicDirectory(
                    Environment.DIRECTORY_DOCUMENTS
                ), 
                "Rides"
            );
            
            File file = new File(
                Environment.getExternalStoragePublicDirectory(
                    Environment.DIRECTORY_DOCUMENTS
                ) + "/Rides", 
                "ride-" + start_time + ".json"
            );
            
            try {
                dir.mkdirs();
                file.createNewFile();
                buf = new BufferedWriter(new FileWriter(file, true));
                buf.write(rideHeadder);
                
                if(gps == null) {
                    gps = new Gps(this);
                }   
                
                if(sensors == null) {
                    sensors = new Sensors(this);
                }
                
                mCallback = new MultiDeviceSearch.SearchCallbacks(){
                    public void onDeviceFound(final MultiDeviceSearchResult deviceFound)
                    {
                        if (!deviceFound.isAlreadyConnected() && (pairedAnts == null || pairedAnts.contains(Integer.toString(deviceFound.getAntDeviceNumber()))))  {
                            launchConnection(deviceFound);
                        }
                    }

                    @Override
                    public void onSearchStopped(RequestAccessResult arg0) {}
                };
                
                mRssiCallback = new MultiDeviceSearch.RssiCallback() {
                    @Override
                    public void onRssiUpdate(final int resultId, final int rssi){}
                };

                // start the multi-device search
                mSearch = new MultiDeviceSearch(this, EnumSet.allOf(DeviceType.class), mCallback, mRssiCallback);
            } catch (IOException e) {}
             
        }
        rideStarted = true;
        
        
        NotificationCompat.Builder mBuilder =
                new NotificationCompat.Builder(this)
                .setSmallIcon(R.drawable.ic_launcher)
                .setContentTitle("Ride On")
                .setContentText("Building ride: " + file_name + " Click to stop ride.");
        mBuilder.setProgress(0, 0, true);
        // Creates an explicit intent for an Activity in your app
        Intent resultIntent = new Intent(this, StartActivity.class);

        // The stack builder object will contain an artificial back stack for the
        // started Activity.
        // This ensures that navigating backward from the Activity leads out of
        // your application to the Home screen.
        TaskStackBuilder stackBuilder = TaskStackBuilder.create(this);
        // Adds the back stack for the Intent (but not the Intent itself)
        stackBuilder.addParentStack(StartActivity.class);
        // Adds the Intent that starts the Activity to the top of the stack
        stackBuilder.addNextIntent(resultIntent);
        PendingIntent resultPendingIntent = stackBuilder.getPendingIntent(0, PendingIntent.FLAG_UPDATE_CURRENT);
        mBuilder.setContentIntent(resultPendingIntent);
        if(mNotificationManager == null) {
            mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        }

        startForeground(notifyID, mBuilder.build());
    }
   
    
    protected void stopRide() {
        if(!rideStarted) return;
                
        if(w != null) {
            w.onDestroy();
        }
        
        if(hr != null) {
            hr.onDestroy();
        }
        
        if(gps != null) {
            gps.onDestroy();
        }
        
        if(sensors != null) {
            sensors.onDestroy();
        }
        
        mSearch.close();
        
        try {
            buf.write("]}}");
            buf.close();
        } catch (IOException e) {}
        
        rideStarted = false;
        mNotificationManager.cancel(notifyID);
    }
    
    
    public void launchConnection(MultiDeviceSearchResult result)
    {
        switch (result.getAntDeviceType())
        {
            case BIKE_CADENCE:
                break;
            case BIKE_POWER:
                if(w == null) {
                    w = new Power(result, this);
                }
                break;
            case BIKE_SPD:
                break;
            case BIKE_SPDCAD:
                break;
            case BLOOD_PRESSURE:
                break;
            case ENVIRONMENT:
                break;
            case WEIGHT_SCALE:
                break;
            case HEARTRATE:
                if(hr == null) {
                    hr = new HeartRate(result, this);
                }                 
                break;
            case STRIDE_SDM:
                break;
            case FITNESS_EQUIPMENT:
                break;
            case GEOCACHE:
            case CONTROLLABLE_DEVICE:
                break;
            case UNKNOWN:
                break;
            default:
                break;
        }
    }
}


