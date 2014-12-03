package com.ridelogger;

import java.util.Set;

import android.content.SharedPreferences;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.preference.PreferenceManager;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.RelativeLayout;
import android.widget.TextView;

public class CurrentValuesAdapter extends BaseAdapter {
    public StartActivity    context;
    
    public int[]            keys;
    public TextView[]       keyTvs    = new TextView[RideService.TOTALSENSORS];
    public TextView[]       valuesTvs = new TextView[RideService.TOTALSENSORS];
    public RelativeLayout[] lls       = new RelativeLayout[RideService.TOTALSENSORS];
    public int              size      = 20;
    public boolean          imperial  = false;
    public int              count     = 0;
    public static SharedPreferences.OnSharedPreferenceChangeListener spChanged;

    public CurrentValuesAdapter(StartActivity c) {
        context                    = c;
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(context);
        Set<String> sensors        = settings.getStringSet(context.getString(R.string.PREF_TRACKING_SENSORS), null);
        size                       = Integer.valueOf(settings.getString(context.getString(R.string.PREF_TRACKING_SIZE), "20"));
        imperial                   = settings.getBoolean(context.getString(R.string.PREF_TRACKING_IMPERIAL_UNITS), false);
        
        
        if(sensors != null && sensors.size() > 0) {
            keys = new int[sensors.size()];
            int i = 0;
            for(String sensor : sensors) {
                keys[i] = Integer.parseInt(sensor);
                initRelativeLayout(keys[i]);
                i++;
            }
        } else {
            keys = new int[RideService.KEYS.length];
            
            for (int i = 0; i < RideService.KEYS.length; i++) {
                keys[i] = i;
                initRelativeLayout(keys[i]);
            }
        }
        
        settings.registerOnSharedPreferenceChangeListener(
            new SharedPreferences.OnSharedPreferenceChangeListener() {
                @Override
                public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String pkey) {
                    if(pkey == context.getString(R.string.PREF_TRACKING_SIZE)) {
                        size = Integer.valueOf(sharedPreferences.getString(context.getString(R.string.PREF_TRACKING_SIZE), "20"));
                        for (int key: keys) {
                            valuesTvs[key].setTextSize(TypedValue.COMPLEX_UNIT_SP, size);
                        }
                        
                        context.layout.setColumnWidth(getWidth());
                        
                        notifyDataSetChanged();
                    } else if (pkey == context.getString(R.string.PREF_TRACKING_SENSORS)) {
                        Set<String> sensors = sharedPreferences.getStringSet(context.getString(R.string.PREF_TRACKING_SENSORS), null);
                        keys = new int[sensors.size()];
                        int i = 0;
                        for(String sensor : sensors) {
                            keys[i] = Integer.parseInt(sensor);
                            initRelativeLayout(keys[i]);
                            i++;
                        }

                        context.layout.setAdapter(CurrentValuesAdapter.this);
                    } else if (pkey == context.getString(R.string.PREF_TRACKING_IMPERIAL_UNITS)) {
                        imperial = sharedPreferences.getBoolean(pkey, false);
                        
                        if(!imperial) {
                            for(int key : keys) {
                                keyTvs[key].setText(RideService.KEYS[key].toString().toLowerCase());
                            }
                        } else {
                            for(int key : keys) {
                                switch (key) {
                                    case RideService.ALTITUDE:
                                        keyTvs[key].setText("ft");
                                        break;
                                        
                                    case RideService.KPH:
                                        keyTvs[key].setText("mph");
                                        break;
                                        
                                    case RideService.KM:
                                        keyTvs[key].setText("m");
                                        break;
                        
                                    default:
                                        keyTvs[key].setText(RideService.KEYS[key].toString().toLowerCase());
                                        break;
                                }
                            }
                        }
                        
                        notifyDataSetChanged();
                    }
                }
            }
        );
    }
    
    public void initRelativeLayout(int key) {
        lls[key] = new RelativeLayout(context);
        initValueTv(key);
        initKeyTv(key);
        
        RelativeLayout.LayoutParams valueLayoutParams = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.WRAP_CONTENT,
                RelativeLayout.LayoutParams.WRAP_CONTENT
        );
        
        valueLayoutParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
        valueLayoutParams.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
        
        lls[key].addView(valuesTvs[key], valueLayoutParams);
        
        RelativeLayout.LayoutParams keyLayoutParams = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.WRAP_CONTENT,
                RelativeLayout.LayoutParams.WRAP_CONTENT
        );
        
        keyLayoutParams.addRule(RelativeLayout.BELOW, valuesTvs[key].getId());
        keyLayoutParams.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        keyLayoutParams.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
        
        lls[key].addView(keyTvs[key], keyLayoutParams);
    }
    
    
    public void initValueTv(int key){
        valuesTvs[key] = new TextView(context);
        valuesTvs[key].setTextAppearance(context, android.R.attr.textAppearanceLarge);
        valuesTvs[key].setTypeface(null, Typeface.BOLD);
        valuesTvs[key].setTextSize(TypedValue.COMPLEX_UNIT_SP, size);
        valuesTvs[key].setText(String.format("%.2f", 0.0));
        valuesTvs[key].setId(key + 1);
    }
    
    
    public void initKeyTv(int key){
        keyTvs[key] = new TextView(context);
        keyTvs[key].setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        
        if(!imperial) {
            keyTvs[key].setText(RideService.KEYS[key].toString().toLowerCase());
        } else {
            switch (key) {
                case RideService.ALTITUDE:
                    keyTvs[key].setText("ft");
                    break;
                    
                case RideService.KPH:
                    keyTvs[key].setText("mph");
                    break;
                    
                case RideService.KM:
                    keyTvs[key].setText("miles");
                    break;
    
                default:
                    keyTvs[key].setText(RideService.KEYS[key].toString().toLowerCase());
                    break;
            }
        }
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
            return (RelativeLayout) lls[keys[position]];
        } else {
            return (RelativeLayout) convertView;
        }
    }
    
    
    public void update(float[] values) {
        for (int key: keys) {
            if(!imperial) {
                valuesTvs[key].setText(String.format("%.2f", values[key]));
            } else {
                switch (key) {
                    case RideService.ALTITUDE:
                        valuesTvs[key].setText(String.format("%.2f", values[key] * 3.28084));
                        break;
                    
                    case RideService.KPH:
                        valuesTvs[key].setText(String.format("%.2f", values[key] * 0.621371));
                        break;

                    case RideService.KM:
                        valuesTvs[key].setText(String.format("%.2f", values[key] * 0.621371));
                        break;
        
                    default:
                        valuesTvs[key].setText(String.format("%.2f", values[key]));
                        break;
                }
            }
        }
        
        notifyDataSetChanged();
    }

    /**
     * returns the size of the cell basted on 8 chars wide and the configured size param
     * @return int
     */
    public int getWidth() {
        TextView tv = new TextView(context);
        tv.setTextAppearance(context, android.R.attr.textAppearanceLarge);
        tv.setTypeface(null, Typeface.BOLD);
        tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, size);
        tv.setText(String.format("%.2f", 1111.11));
        
        Rect bounds = new Rect();
        tv.getPaint().getTextBounds("9999.99", 0, "9999.99".length(), bounds);
        
        return bounds.width();
    }
}
