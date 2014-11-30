package com.ridelogger.listners;

import java.util.Timer;
import java.util.TimerTask;

import com.ridelogger.R;
import com.ridelogger.RideService;

import android.content.Context;
import android.content.SharedPreferences;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.preference.PreferenceManager;

/**
 * Sensors
 * @author Chet Henry
 * Listen to android sensor events and log them
 */
public class Sensors extends Base<Object>
{
    public static final double  CRASHMAGNITUDE = 30.0;
    private SensorManager       mSensorManager;    
    private Sensor              mLight;
    private Sensor              mAccel;
    private Sensor              mPress;
    private Sensor              mTemp;
    private Sensor              mField;
    
    private SensorEventListener luxListner;
    private SensorEventListener accelListner;
    private SensorEventListener pressListner;
    private SensorEventListener tempListner;
    private SensorEventListener fieldListner;
    
    public Sensors(RideService mContext) 
    {
        super(mContext);
        
        mSensorManager = (SensorManager) mContext.getSystemService(Context.SENSOR_SERVICE);
        
        mLight         = mSensorManager.getDefaultSensor(Sensor.TYPE_LIGHT);
        mAccel         = mSensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION);
        mPress         = mSensorManager.getDefaultSensor(Sensor.TYPE_PRESSURE);
        mTemp          = mSensorManager.getDefaultSensor(Sensor.TYPE_AMBIENT_TEMPERATURE);
        mField         = mSensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
          
        if(mLight != null) {
          luxListner = new SensorEventListener() {
              @Override
              public final void onAccuracyChanged(Sensor sensor, int accuracy) {}
              
              @Override
              public final void onSensorChanged(SensorEvent event) {
                  // The light sensor returns a single value.
                  // Many sensors return 3 values, one for each axis.
                  alterCurrentData(RideService.lux,  event.values[0]);
              }
            };
            
            mSensorManager.registerListener(luxListner,   mLight, 3000000);
        }
        
        if(mAccel != null) {
            SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(context);
            if(settings.getBoolean(context.getString(R.string.PREF_DETECT_CRASH), false)) {
                accelListner = new SensorEventListener() {
                    private boolean crashed = false;
                    private Timer   timer   = new Timer();
                    private double[] St     = new double[3];
        
                    @Override
                    public final void onAccuracyChanged(Sensor sensor, int accuracy) {}
                      
                    @Override
                    public final void onSensorChanged(SensorEvent event) {                
                        int[] keys = {
                                RideService.ms2x,
                                RideService.ms2y,
                                RideService.ms2z
                        };

                        alterCurrentData(keys, event.values);
                        
                        if(St.length == 0) {
                            St[0] = event.values[0];
                            St[1] = event.values[1];
                            St[2] = event.values[2];
                        }
                        
                        St[0] = 0.6 * event.values[0] + 0.4 * St[0];
                        St[1] = 0.6 * event.values[1] + 0.4 * St[1];
                        St[2] = 0.6 * event.values[2] + 0.4 * St[2];
                        
                        double amag = Math.sqrt(St[0]*St[0] + St[1]*St[1] + St[2]*St[2]);
                        
                        if(amag > CRASHMAGNITUDE && !crashed) {
                            crashed = true;
                            context.phoneCrash(amag);

                            if(!Float.isNaN(context.currentValues[RideService.KPH])) {
                                timer.schedule(
                                    new TimerTask() {              
                                        @Override  
                                        public void run() {
                                            //if we are traveling less then 1km/h at 5 seconds after crash detection
                                            // confirm the crash
                                            if(1.0 > context.currentValues[RideService.KPH]) {
                                                context.phoneCrashConfirm();
                                            } else {
                                                crashed = false;
                                                context.phoneHome();
                                            }
                                        }  
                                    }, 
                                    5000
                                ); //in five sec reset
                            } else {
                                timer.schedule(
                                    new TimerTask() {              
                                        @Override  
                                        public void run() {
                                            crashed = false;
                                        }  
                                    }, 
                                    180000
                                ); //in three min reset
                            }
                        }
                    }
                };
            } else {
                accelListner = new SensorEventListener() {
                    @Override
                    public final void onAccuracyChanged(Sensor sensor, int accuracy) {}
                      
                    @Override
                    public final void onSensorChanged(SensorEvent event) {                                        
                        int[] keys = {
                                RideService.ms2x,
                                RideService.ms2y,
                                RideService.ms2z
                        };
                        
                        alterCurrentData(keys, event.values);
                    }
                };
            }

            mSensorManager.registerListener(accelListner, mAccel, SensorManager.SENSOR_DELAY_NORMAL);
        }
        
        if(mPress != null) {
            pressListner = new SensorEventListener() {
              @Override
              public final void onAccuracyChanged(Sensor sensor, int accuracy) {}
              
              @Override
              public final void onSensorChanged(SensorEvent event) {
                  // The light sensor returns a single value.
                  // Many sensors return 3 values, one for each axis.
                  alterCurrentData(RideService.press,  event.values[0]);
              }
            };
            
            mSensorManager.registerListener(pressListner, mPress, 3000000);
        }
        
        if(mTemp != null) {
            tempListner = new SensorEventListener() {
              @Override
              public final void onAccuracyChanged(Sensor sensor, int accuracy) {}
              
              @Override
              public final void onSensorChanged(SensorEvent event) {
                  // The light sensor returns a single value.
                  // Many sensors return 3 values, one for each axis.
                  alterCurrentData(RideService.temp,  event.values[0]);
              }
            };
            
            mSensorManager.registerListener(tempListner,  mTemp,  3000000);
        }
        
        if(mField != null) {
            fieldListner = new SensorEventListener() {
              @Override
              public final void onAccuracyChanged(Sensor sensor, int accuracy) {}
              
              @Override
              public final void onSensorChanged(SensorEvent event) {                  
                  int[] keys = {
                          RideService.uTx,
                          RideService.uTy,
                          RideService.uTz
                  };
                  
                  alterCurrentData(keys, event.values);
              }
            };
            
            mSensorManager.registerListener(fieldListner, mField, SensorManager.SENSOR_DELAY_NORMAL);
        }
    }
    
    @Override
    public void onDestroy()
    {
        if(luxListner != null) {
            mSensorManager.unregisterListener(luxListner);
        }
        if(accelListner != null) {
            mSensorManager.unregisterListener(accelListner);
        }
        if(pressListner != null) {
            mSensorManager.unregisterListener(pressListner);
        }
        if(tempListner != null) {
            mSensorManager.unregisterListener(tempListner);
        }
        if(fieldListner != null) {
            mSensorManager.unregisterListener(fieldListner);
        }
    }
}