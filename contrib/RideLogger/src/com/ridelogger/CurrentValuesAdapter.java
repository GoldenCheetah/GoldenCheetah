package com.ridelogger;

import java.util.Set;

import android.content.SharedPreferences;
import android.graphics.Typeface;
import android.preference.PreferenceManager;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

public class CurrentValuesAdapter extends BaseAdapter {
    public StartActivity context;
    public int           count  = 0;
    public int[]         keys;
    public TextView[]    tvs    = new TextView[RideService.TOTALSENSORS];
    public int           size   = 20;
    public static SharedPreferences.OnSharedPreferenceChangeListener spChanged;

    public CurrentValuesAdapter(StartActivity c) {
        context                    = c;
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(context);
        Set<String> sensors        = settings.getStringSet(context.getString(R.string.PREF_TRACKING_SENSORS), null);
        size                       = Integer.valueOf(settings.getString(context.getString(R.string.PREF_TRACKING_SIZE), "20"));
        
        if(sensors != null && sensors.size() > 0) {
            keys = new int[sensors.size()];
            int i = 0;
            for(String sensor : sensors) {
                keys[i] = Integer.parseInt(sensor);
                i++;
            }
        } else {
            keys = new int[RideService.KEYS.length];
            
            for (int i = 0; i < RideService.KEYS.length; i++) {
                keys[i] = i;
            }
        }
        
        for (int key: keys) {
            tvs[key] = getNewTv(key);
        }
        
        settings.registerOnSharedPreferenceChangeListener(
            new SharedPreferences.OnSharedPreferenceChangeListener() {
                @Override
                public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String pkey) {
                    if(pkey == context.getString(R.string.PREF_TRACKING_SIZE)) {
                        size = Integer.valueOf(sharedPreferences.getString(context.getString(R.string.PREF_TRACKING_SIZE), "20"));
                        for (int key: keys) {
                            tvs[key].setTextSize(TypedValue.COMPLEX_UNIT_SP, size);
                        }
                    } else if (pkey == context.getString(R.string.PREF_TRACKING_SENSORS)) {
                        Set<String> sensors = sharedPreferences.getStringSet(context.getString(R.string.PREF_TRACKING_SENSORS), null);
                        keys = new int[sensors.size()];
                        int i = 0;
                        for(String sensor : sensors) {
                            keys[i] = Integer.parseInt(sensor);
                            i++;
                        }
                        tvs = new TextView[RideService.TOTALSENSORS];
                        for (int key: keys) {
                            tvs[key] = getNewTv(key);
                        }
                        
                        context.layout.setAdapter(CurrentValuesAdapter.this);
                    }
                }
            }
        );
    }
    
    public TextView getNewTv(int key){
        TextView tv = new TextView(context);
        
        tv.setTextAppearance(context, android.R.attr.textAppearanceLarge);
        tv.setTypeface(null, Typeface.BOLD);
        tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, size);
        tv.setText(String.format("%.2f", 0.0) + " " +  RideService.KEYS[key].toString().toLowerCase());
        
        return tv;
    }
    
    
    @Override
    public int getCount() {
        return keys.length;
    }
    

    @Override
    public Object getItem(int position) {
        return null;
    }
    

    @Override
    public long getItemId(int position) {
        return 0;
    }
    

    @Override    
    public View getView(int position, View convertView, ViewGroup parent) {        
        if (convertView == null) {
            return (TextView) tvs[keys[position]];
        } else {
            return (TextView) convertView;
        }
    }
    
    
    public void update(float[] values) {
        for (int key: keys) {
            tvs[key].setText(String.format("%.2f", values[key]) + " " +  RideService.KEYS[key].toString().toLowerCase());
        }
        
        notifyDataSetChanged();
    }
}
