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
import com.ridelogger.listners.Base;
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

/**
 * RideService
 * @author Chet Henry
 * Performs ride logging from sensors as an android service
 */
public class RideService extends Service
{
    public static  BufferedWriter       buf;                  //writes to log file buffered
    public static  long                 startTime;            //start time of the ride
    public static  Map<String, String>  currentValues;        //hash of current values
    public         boolean              rideStarted = false;  //have we started logging the ride
    
    public static  Map<String, Base<?>> sensors     = new HashMap<String, Base<?>>();
                                                              //All other Android sensor class
    MultiDeviceSearch                   mSearch;              //Ant+ device search class to init connections
    MultiDeviceSearch.SearchCallbacks   mCallback;            //Ant+ device class to setup sensors after they are found
    MultiDeviceSearch.RssiCallback      mRssiCallback;        //Ant+ class to do something with the signal strength on device find
    
    public int                          notifyID = 1;         //Id of the notification in the top android bar that this class creates and alters
    
    public String                       fileName = "";        //File where the ride will go
    SharedPreferences                   settings;             //Object to load our setting from android's storage
    public Boolean                      snoop    = false;     //should we log others ant+ devices
    Set<String>                         pairedAnts;           //list of ant devices to pair with
    
    /**
     * starts the ride on service start
     */
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        startRide();
        return Service.START_NOT_STICKY;
    }
    
    /**
     * sets android service settings
     */
    @Override
    public IBinder onBind(Intent arg0) {
        return null;
    }
    
    
    /**
     * sets android service settings
     */
    @Override
    public boolean onUnbind (Intent intent) {
        return true;
    }
    
    
    /**
     * stop the ride on service stop
     */
    @Override
    public void onDestroy() {
        stopRide();
        super.onDestroy();
    }
    
    /**
     * start a ride if there is not one started yet
     */
    protected void startRide() {
        if(rideStarted) return;
        
        startTime     = System.currentTimeMillis();
        fileName      = "ride-" + startTime + ".json";
        currentValues = new HashMap<String, String>();
        
        SimpleDateFormat f = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss");
        f.setTimeZone(TimeZone.getTimeZone("UTC"));
        
        String utc   = f.format(new Date(startTime));
        
        Calendar cal = Calendar.getInstance();
        cal.setTimeInMillis(startTime);
        
        String month                 = cal.getDisplayName(Calendar.MONTH, Calendar.LONG, Locale.US);
        String week_day              = cal.getDisplayName(Calendar.DAY_OF_WEEK, Calendar.LONG, Locale.US);
        String year                  = Integer.toString(cal.get(Calendar.YEAR));
        settings                     = getSharedPreferences(StartActivity.PREFS_NAME, 0);
        String rider_name            = settings.getString(StartActivity.RIDER_NAME, "");
        final Set<String> pairedAnts = settings.getStringSet(StartActivity.PAIRED_ANTS, null);
         
        currentValues.put("SECS", "0.0");
        
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
                "ride-" + startTime + ".json"
            );
            
            try {
                dir.mkdirs();
                file.createNewFile();
                buf = new BufferedWriter(new FileWriter(file, true));
                buf.write(rideHeadder);
                
                sensors.put("GPS",            new Gps(this));
                sensors.put("AndroidSensors", new Sensors(this));
                
                mCallback = new MultiDeviceSearch.SearchCallbacks(){
                    public void onDeviceFound(final MultiDeviceSearchResult deviceFound)
                    {
                        if (!deviceFound.isAlreadyConnected())  {
                            if(pairedAnts == null || pairedAnts.contains(Integer.toString(deviceFound.getAntDeviceNumber()))) {
                                launchConnection(deviceFound, false);
                            } else if (snoop) {
                                launchConnection(deviceFound, true);
                            }
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
        
        //build the notification in the top android drawer
        NotificationCompat.Builder mBuilder = new NotificationCompat
            .Builder(this)
            .setSmallIcon(R.drawable.ic_launcher)
            .setContentTitle("Ride On")
            .setContentText("Building ride: " + fileName + " Click to stop ride.")
            .setProgress(0, 0, true)
            .setContentIntent(
                TaskStackBuilder
                    .create(this)
                    .addParentStack(StartActivity.class)
                    .addNextIntent(new Intent(this, StartActivity.class))
                    .getPendingIntent(0, PendingIntent.FLAG_UPDATE_CURRENT)
            );

        startForeground(notifyID, mBuilder.build());
    }
   
    
    //stop the ride and clean up resources
    protected void stopRide() {
        if(!rideStarted) return;

        for (Map.Entry<String, Base<?>> entry : sensors.entrySet()) {                   
            entry.getValue().onDestroy();
        }

        //stop the Ant+ search
        mSearch.close();
        
        try {
            buf.write("]}}");
            buf.close();
        } catch (IOException e) {}
        
        rideStarted = false;
        NotificationManager mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        mNotificationManager.cancel(notifyID);
    }
    
    
    //remove snooped sensors if they are not longer in range
    public void releaseSnoopedSensor(Base<?> sensor) {
        sensors.remove(sensor);
    }
    
    
    //launch ant+ connection
    public void launchConnection(MultiDeviceSearchResult result, Boolean snoop) {
        switch (result.getAntDeviceType()) {
            case BIKE_CADENCE:
                break;
            case BIKE_POWER:
                sensors.put(String.valueOf(result.getAntDeviceNumber()), new Power(result, this, snoop));
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
                sensors.put(String.valueOf(result.getAntDeviceNumber()), new HeartRate(result, this, snoop));            
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


