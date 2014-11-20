package com.ridelogger.listners;

import java.util.HashMap;
import java.util.Map;

import com.ridelogger.RideService;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;


/**
 * Sensors
 * @author Chet Henry
 * Listen to android sensor events and log them
 */
public class Sensors extends Base<Object>
{
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
                  alterCurrentData("lux",  reduceNumberToString(event.values[0]));
              }
            };
            
            mSensorManager.registerListener(luxListner,   mLight, 3000000);
        }
        if(mAccel != null) {
            accelListner = new SensorEventListener() {
              @Override
              public final void onAccuracyChanged(Sensor sensor, int accuracy) {}
              
              @Override
              public final void onSensorChanged(SensorEvent event) {                
                  Map<String, String> map = new HashMap<String, String>();
                  map.put("ms2x", reduceNumberToString(event.values[0]));
                  map.put("ms2y", reduceNumberToString(event.values[1]));
                  map.put("ms2z", reduceNumberToString(event.values[2]));
                  alterCurrentData(map);
              }
            };
            
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
                  alterCurrentData("press",  reduceNumberToString(event.values[0]));
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
                  alterCurrentData("temp",  reduceNumberToString(event.values[0]));
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
                  Map<String, String> map = new HashMap<String, String>();
                  map.put("uTx", reduceNumberToString(event.values[0]));
                  map.put("uTy", reduceNumberToString(event.values[1]));
                  map.put("uTz", reduceNumberToString(event.values[2]));
                  alterCurrentData(map);
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