package com.ridelogger;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Set;
import java.util.TimeZone;
import java.util.Timer;
import java.util.TimerTask;

import com.dsi.ant.plugins.antplus.pcc.defines.DeviceType;
import com.ridelogger.formats.BaseFormat;
import com.ridelogger.formats.JsonFormat;
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

    public GzipWriter          buf;                                                //writes to log file buffered
    public long                startTime;                                          //start time of the ride
    public float[]             currentValues = new float[RideService.KEYS.length]; //float array of current values
    private Messenger          mMessenger    = null;                               //class to send back current values if needed
    private boolean            rideStarted   = false;                              //have we started logging the ride
    private int                sensor_index  = 0;                                  //current index of sensors
    private String             emergencyNumbuer;                                   //the number to send the messages to
    private Timer              timer;                                              //timer class to control the periodic messages
    private Timer              timerUI;                                            //timer class to control the periodic messages
    private Base<?>[]          sensors;                                            //list of sensors tracking
    public BaseFormat<?>       fileFormat;

    
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
        mMessenger = new Messenger(new Handler() { // Handler of incoming messages from clients.
            Messenger replyTo;
            @Override
            public void handleMessage(Message msg) {
                if(timerUI != null) {
                    timerUI.cancel();
                    timerUI = null;
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
                                } catch (RemoteException e) {}
                            }
                        },
                        1000, 
                        1000
                    ); //every second update the screen
                }
            }
        }); // Target we publish for clients to send messages to IncomingHandler.
        
        return mMessenger.getBinder();
    }
    
    
    /**
     * releases the timer that sends messages to the app
     */
    @Override
    public boolean onUnbind (Intent intent) {
        if(timerUI != null) {
            timerUI.cancel();
            timerUI = null;
        }
        
        mMessenger = null;
        
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
        
        SharedPreferences settings  = PreferenceManager.getDefaultSharedPreferences(this);
        emergencyNumbuer            = settings.getString(getString(R.string.PREF_EMERGENCY_NUMBER), "");
        currentValues[SECS]         = (float) 0.0;
        
        startTime                   = System.currentTimeMillis();
        
        
        Date startDate              = new Date(startTime);        
        
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
        
        final String fileName       = filef.format(startDate) + ".json.gz";

        if (Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState())) {
            fileFormat = new JsonFormat(this);
            fileFormat.createFile();
            fileFormat.writeHeader();
            
            final Set<String> pairedAnts = settings.getStringSet(getString(R.string.PREF_PAIRED_ANTS), null);
            
            if(pairedAnts != null && !pairedAnts.isEmpty()){
                sensors = new Base<?>[pairedAnts.size() + 2];
                for(String deviceNumber: pairedAnts) {
                    DeviceType deviceType =  DeviceType.getValueFromInt(settings.getInt(deviceNumber, 0));
                    switch (deviceType) {
                        case BIKE_CADENCE:
                            break;
                        case BIKE_POWER:
                            sensors[sensor_index++] = new Power(Integer.valueOf(deviceNumber), this);
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
                            sensors[sensor_index++] = new HeartRate(Integer.valueOf(deviceNumber), this);            
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
            } else {
                sensors = new Base<?>[4];
                sensors[sensor_index++] = new HeartRate(0, this);
                sensors[sensor_index++] = new Power(0, this);
            }
            
            sensors[sensor_index++] = new Gps(this);
            sensors[sensor_index++] = new Sensors(this);
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
        
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(this);
        if(settings.getBoolean(getString(R.string.PREF_PHONE_HOME), false)) {
            phoneStop();
        }

        for(Base<?> sensor: sensors) {
            if(sensor != null) {
                sensor.onDestroy();
            }
        }
        
        //stop the phoneHome timer if we need to.
        if(timer != null) {
            timer.cancel();
            timer = null;
        }
        
        if(timerUI != null) {
            timerUI.cancel();
            timerUI = null;
        }
        
        fileFormat.writeFooter();
        
        rideStarted = false;
        NotificationManager mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        mNotificationManager.cancel(notifyID);
    }
}


