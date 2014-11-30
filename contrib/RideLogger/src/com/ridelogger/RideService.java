package com.ridelogger;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.EnumSet;
import java.util.LinkedHashMap;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.TimeZone;
import java.util.Timer;
import java.util.TimerTask;

import com.dsi.ant.plugins.antplus.pcc.defines.DeviceType;
import com.dsi.ant.plugins.antplus.pcc.defines.RequestAccessResult;
import com.dsi.ant.plugins.antplus.pccbase.MultiDeviceSearch;
import com.dsi.ant.plugins.antplus.pccbase.MultiDeviceSearch.MultiDeviceSearchResult;
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
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.support.v4.app.NotificationCompat;
import android.telephony.PhoneNumberUtils;
import android.telephony.SmsManager;


/**
 * RideService
 * @author Chet Henry
 * Performs ride logging from sensors as an android service
 */
public class RideService extends Service
{    
    public static final int notifyID     = 1;                  //Id of the notification in the top android bar that this class creates and alters   
    
    
    public static final int SECS         = 0;
    public static final int KPH          = 1;
    public static final int ALTITUDE     = 2;
    public static final int bearing      = 3;
    public static final int gpsa         = 4;
    public static final int LAT          = 5;
    public static final int LON          = 6;
    public static final int HR           = 7;
    public static final int WATTS        = 8;
    public static final int NM           = 9;
    public static final int CAD          = 10;
    public static final int KM           = 11;
    public static final int LTE          = 12;
    public static final int RTE          = 13;
    public static final int SNPLC        = 14;
    public static final int SNPR         = 15;
    public static final int ms2x         = 16;
    public static final int ms2y         = 17;
    public static final int ms2z         = 18;
    public static final int temp         = 19;
    public static final int uTx          = 20;
    public static final int uTy          = 21;
    public static final int uTz          = 22;
    public static final int press        = 23;
    public static final int lux          = 24;
    
    public static CharSequence[] KEYS    = {
        "SECS", 
        "KPH", 
        "ALTITUDE", 
        "bearing", 
        "gpsa", 
        "LAT", 
        "LON",
        "HR",
        "WATTS",
        "NM",
        "CAD",
        "KM",
        "LTE",
        "RTE",
        "SNPLC",
        "SNPR",
        "ms2x",
        "ms2y",
        "ms2z",
        "temp",
        "uTx",
        "uTy",
        "uTz",
        "press",
        "lux"
    };
    
    public static final int TOTALSENSORS = RideService.KEYS.length;
    
    public float[]           currentValues = new float[RideService.TOTALSENSORS]; //float array of current values
    public float[]           snoopedValues = new float[RideService.TOTALSENSORS]; //float array of snooped values
    public GzipWriter        buf;                           //writes to log file buffered
    public long              startTime;                     //start time of the ride
    public boolean           rideStarted   = false;         //have we started logging the ride
    public Boolean           snoop         = false;         //should we log others ant+ devices
    private Set<String>      pairedAnts;                    //list of ant devices to pair with
    private Timer            timer;                         //timer class to control the periodic messages
    private Timer            timerUI;                       //timer class to control the periodic messages
    private String           emergencyNumbuer;              //the number to send the messages to

                                        
    public LinkedHashMap<String, Base<?>> sensors   = new LinkedHashMap<String, Base<?>>();
                                                              //All other Android sensor class
    MultiDeviceSearch                     mSearch;              //Ant+ device search class to init connections
    MultiDeviceSearch.SearchCallbacks     mCallback;            //Ant+ device class to setup sensors after they are found
    MultiDeviceSearch.RssiCallback        mRssiCallback;        //Ant+ class to do something with the signal strength on device find
    
    
    
    final Messenger mMessenger = new Messenger(new IncomingHandler()); // Target we publish for clients to send messages to IncomingHandler.
    Messenger replyTo;
    
    class IncomingHandler extends Handler { // Handler of incoming messages from clients.
        @Override
        public void handleMessage(Message msg) {
            if(timerUI != null) {
                timerUI.cancel();
            }
            
            timerUI = new Timer();
            
            if(msg.replyTo != null) {
                replyTo = msg.replyTo;
                timerUI.scheduleAtFixedRate(
                    new TimerTask() {              
                        @Override  
                        public void run() {                    
                            Message msg = Message.obtain(null, 2, 0, 0);
                            Bundle bundle = new Bundle();
                            bundle.putSerializable("currentValues", currentValues);
                            msg.setData(bundle);
                            try {
                                replyTo.send(msg);
                            } catch (RemoteException e) { }
                        }
                    },
                    1000, 
                    1000
                ); //every second update the screen
            }
        }
    }
    
    /**
     * starts the ride on service start
     */
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        startRide();
        return Service.START_NOT_STICKY;
    }
    
    
    /**
     * returns the messenger to talk to the app with
     */
    @Override
    public IBinder onBind(Intent arg0) {
        return mMessenger.getBinder();
    }
    
    
    /**
     * releases the timer that sends messages to the app
     */
    @Override
    public boolean onUnbind (Intent intent) {
        if(timerUI != null) {
            timerUI.cancel();
        }
        
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
        final String fileName      = "ride-" + startTime + ".json.gzip";
        
        SimpleDateFormat f = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss");
        f.setTimeZone(TimeZone.getTimeZone("UTC"));
        
        String utc   = f.format(new Date(startTime));
        
        Calendar cal = Calendar.getInstance();
        cal.setTimeInMillis(startTime);
        
        String month     = cal.getDisplayName(Calendar.MONTH, Calendar.LONG, Locale.US);
        String weekDay   = cal.getDisplayName(Calendar.DAY_OF_WEEK, Calendar.LONG, Locale.US);
        String year      = Integer.toString(cal.get(Calendar.YEAR));
        
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(this);
        emergencyNumbuer           = settings.getString(getString(R.string.PREF_EMERGENCY_NUMBER), "");
        pairedAnts                 = settings.getStringSet(getString(R.string.PREF_PAIRED_ANTS), null);
         
        currentValues[SECS] = (float) 0.0;
        
        String rideHeadder = "{" +
            "\"RIDE\":{" +
                "\"STARTTIME\":\"" + utc + " UTC\"," +
                "\"RECINTSECS\":1," +
                "\"DEVICETYPE\":\"Android\"," +
                "\"IDENTIFIER\":\"\"," +
                "\"TAGS\":{" +
                    "\"Athlete\":\"" + settings.getString(getString(R.string.PREF_RIDER_NAME), "") + "\"," +
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
                    "\"Weekday\":\"" + weekDay + "\"," +
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
                fileName
            );
            
            
            try {
                dir.mkdirs();
                file.createNewFile();
                file.setReadable(true);
                file.setWritable(true);
                
                OutputStream bufWriter = new BufferedOutputStream(new FileOutputStream(file));
                buf = new GzipWriter(bufWriter);
                
                buf.write(rideHeadder);
                
                sensors.put("GPS",            new Gps(this));
                sensors.put("AndroidSensors", new Sensors(this));
                
                if(pairedAnts != null && !pairedAnts.isEmpty()){
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
                }
            } catch (IOException e) {}
             
        }
        rideStarted = true;
        
        if(settings.getBoolean(getString(R.string.PREF_PHONE_HOME), false)) {
            timer = new Timer();
            int period = Integer.parseInt(settings.getString(getString(R.string.PREF_PHONE_HOME_PERIOD), "10"));
            
            timer.scheduleAtFixedRate(
                new TimerTask() {              
                    @Override  
                    public void run() {
                        phoneHome();
                    }  
                },
                60000 * period, 
                60000 * period
            ); //every ten min let them know where you are at
            phoneStart();
        }
        
        //build the notification in the top android drawer
        NotificationCompat.Builder mBuilder = new NotificationCompat
            .Builder(this)
            .setSmallIcon(R.drawable.ic_launcher)
            .setContentTitle(getString(R.string.ride_on))
            .setContentText(getString(R.string.building_ride) + " " + fileName + " " + getString(R.string.click_to_stop))
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
    
    
    /**
     * let a love one know where you are at about every 10 min
     */
    public void phoneCrash(double mag) {
        smsHome(
                getString(R.string.crash_warning) + "\n" + getLocationLink()
                + "\n " + getString(R.string.crash_magnitude) + ": " + String.valueOf(mag)
        );
    }
    
    
    /**
     * confirm the crash if we are not moving
     */
    public void phoneCrashConfirm() {
        smsHome(getString(R.string.crash_confirm) + "!\n" + getLocationLink());
    }
    
   
    /**
     * let them know we are starting
     */
    public void phoneStart() {
        smsWithLocation(getString(R.string.ride_start_sms));
    }
    
    
    /**
     * let them know we are stopping
     */
    public void phoneStop() {
        smsWithLocation(getString(R.string.ride_stop_sms));
    }
    
    
    /**
     * send an sms with location
     */
    public void smsWithLocation(String body) {        
        smsHome(body + "\n " + getLocationLink());
    }
    
    
    /**
     * let a love one know where you are at about every 10 min
     */
    public void phoneHome() {
        smsWithLocation(getString(R.string.riding_ok_sms));
    }
    
    
    /**
     * send a sms message
     */
    public void smsHome(String body) {
        SmsManager smsManager = SmsManager.getDefault();
        if(emergencyNumbuer != null && PhoneNumberUtils.isWellFormedSmsAddress(emergencyNumbuer)) {
            smsManager.sendTextMessage(emergencyNumbuer, null, body, null, null);
        }
    }
    
    public String getLocationLink() {
        if(currentValues[LAT] != 0.0 || currentValues[LON] != 0.0) {
            return "https://www.google.com/maps/place/" + currentValues[LAT] + "," + currentValues[LON];
        }
        return getString(R.string.crash_unknow_location);
    }
    
    
    //stop the ride and clean up resources
    protected void stopRide() {
        if(!rideStarted) return;
        
        phoneStop();

        for (Map.Entry<String, Base<?>> entry : sensors.entrySet()) {                   
            entry.getValue().onDestroy();
        }

        //stop the Ant+ search
        if(mSearch != null) {
            mSearch.close();
        }
        
        //stop the phoneHome timer if we need to.
        if(timer != null) {
            timer.cancel();
        }
        
        if(timerUI != null) {
            timerUI.cancel();
        }
        
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


