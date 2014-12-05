package com.ridelogger;

import java.util.Set;

import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.preference.PreferenceManager;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.RelativeLayout;
import android.widget.TextView;

public class CurrentValuesAdapter extends BaseAdapter {
    private StartActivity    context;
    
    private int[]            keys;
    private String[]         values    = new String[RideService.KEYS.length];
    private String[]         keyLabels = new String[RideService.KEYS.length];
    private int              size      = 20;
    private boolean          imperial  = false;

    private GridView layout;

    public CurrentValuesAdapter(StartActivity c, GridView tlayout) {
        context                    = c;
        layout                     = tlayout;
        
        layout.setBackgroundColor(Color.BLACK);
        layout.setVerticalSpacing(4);
        layout.setHorizontalSpacing(4);
        
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(context);
        Set<String> sensors        = settings.getStringSet(context.getString(R.string.PREF_TRACKING_SENSORS), null);
        size                       = Integer.valueOf(settings.getString(context.getString(R.string.PREF_TRACKING_SIZE), "20"));
        imperial                   = settings.getBoolean(context.getString(R.string.PREF_TRACKING_IMPERIAL_UNITS), false);
        
        
        if(sensors != null && sensors.size() > 0) {
            keys = new int[sensors.size()];
            int i = 0;
            for(String sensor : sensors) {
                keys[i]   = Integer.parseInt(sensor);
                i++;
            }
        } else {
            keys = new int[RideService.KEYS.length];
            
            for (int i = 0; i < RideService.KEYS.length; i++) {
                keys[i] = i;
            }
        }
        
        
        for (int i = 0; i < RideService.KEYS.length; i++) {
            values[i] = "0.0";
        }
        
        updateKeyLables();
        
        settings.registerOnSharedPreferenceChangeListener(
            new SharedPreferences.OnSharedPreferenceChangeListener() {
                @Override
                public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String pkey) {
                    if(pkey == context.getString(R.string.PREF_TRACKING_SIZE)) {
                        size = Integer.valueOf(sharedPreferences.getString(context.getString(R.string.PREF_TRACKING_SIZE), "20"));
                        setupWidth();
                        notifyDataSetChanged();
                    } else if (pkey == context.getString(R.string.PREF_TRACKING_SENSORS)) {
                        Set<String> sensors = sharedPreferences.getStringSet(context.getString(R.string.PREF_TRACKING_SENSORS), null);
                        keys = new int[sensors.size()];
                        int i = 0;
                        for(String sensor : sensors) {
                            keys[i] = Integer.parseInt(sensor);
                            i++;
                        }
                        
                        layout.setAdapter(CurrentValuesAdapter.this);
                    } else if (pkey == context.getString(R.string.PREF_TRACKING_IMPERIAL_UNITS)) {
                        imperial = sharedPreferences.getBoolean(pkey, false);
                        
                        updateKeyLables();
                        notifyDataSetChanged();
                    }
                }
            }
        );
    }
    
    private void updateKeyLables() {
        if(!imperial) {
            for(int key : keys) {
                keyLabels[key] = RideService.KEYS[key].toString().toLowerCase();
            }
        } else {
            for(int key : keys) {
                switch (key) {
                    case RideService.ALTITUDE:
                        keyLabels[key] = "ft";
                        break;
                        
                    case RideService.KPH:
                        keyLabels[key] = "mph";
                        break;
                        
                    case RideService.KM:
                        keyLabels[key] = "m";
                        break;
        
                    default:
                        keyLabels[key] = RideService.KEYS[key].toString().toLowerCase();
                        break;
                }
            }
        }
    }
    
    private RelativeLayout newRelativeLayout(int key) {
        RelativeLayout view = new RelativeLayout(context);
        view.setBackgroundColor(Color.WHITE);
        
        TextView valueView = newValueTv(key);
        TextView keyView   = newKeyTv(key);
        
        RelativeLayout.LayoutParams valueLayoutParams = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.WRAP_CONTENT,
                RelativeLayout.LayoutParams.WRAP_CONTENT
        );
        
        valueLayoutParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
        valueLayoutParams.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
        
        view.addView(valueView, valueLayoutParams);
        
        RelativeLayout.LayoutParams keyLayoutParams = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.WRAP_CONTENT,
                RelativeLayout.LayoutParams.WRAP_CONTENT
        );
        
        keyLayoutParams.addRule(RelativeLayout.BELOW, valueView.getId());
        keyLayoutParams.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        keyLayoutParams.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
        
        view.addView(keyView, keyLayoutParams);
        
        return view;
    }
    
    
    private TextView newValueTv(int key){
        TextView tv = new TextView(context);
        tv.setTextAppearance(context, android.R.attr.textAppearanceLarge);
        tv.setTypeface(Typeface.MONOSPACE, Typeface.BOLD);
        tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, size);
        tv.setText(String.format("%.1f", 0.0));
        tv.setId(key + 1);
        return tv;
    }
    
    
    private TextView newKeyTv(int key){
        TextView tv = new TextView(context);
        tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        tv.setBackgroundColor(Color.LTGRAY);
        tv.setTypeface(Typeface.MONOSPACE, Typeface.BOLD);
        tv.setPadding(4, 1, 4, 1);
        
        if(!imperial) {
            tv.setText(keyLabels[key]);
        } else {
            switch (key) {
                case RideService.ALTITUDE:
                    tv.setText("ft");
                    break;
                    
                case RideService.KPH:
                    tv.setText("mph");
                    break;
                    
                case RideService.KM:
                    tv.setText("miles");
                    break;
    
                default:
                    tv.setText(keyLabels[key]);
                    break;
            }
        }
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
            return newRelativeLayout(keys[position]);
        } else {
            RelativeLayout view = (RelativeLayout) convertView;
            TextView valueView  = (TextView) (view.getChildAt(0));
            TextView valueKey   = (TextView) (view.getChildAt(1));
            
            if(valueView.getText() != values[keys[position]])
                valueView.setText(values[keys[position]]);
            
            valueView.setTextSize(TypedValue.COMPLEX_UNIT_SP, size);
            
            if(valueKey.getText() != keyLabels[keys[position]])
                valueKey.setText(keyLabels[keys[position]]);
            
            return convertView;
        }
    }
    
    
    public void update(float[] current_float_values) {
        for (int key: keys) {
            if(!imperial) {
                values[key] = String.format("%.1f", current_float_values[key]);
            } else {
                switch (key) {
                    case RideService.ALTITUDE:
                        values[key] = String.format("%.1f", current_float_values[key] * 3.28084);
                        break;
                    
                    case RideService.KPH:
                        values[key] = String.format("%.1f", current_float_values[key] * 0.621371);
                        break;

                    case RideService.KM:
                        values[key] = String.format("%.1f", current_float_values[key] * 0.621371);
                        break;
        
                    default:
                        values[key] = String.format("%.1f", current_float_values[key]);
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
    private void setupWidth() {
        TextView tv = new TextView(context);
        tv.setTextAppearance(context, android.R.attr.textAppearanceLarge);
        tv.setTypeface(Typeface.MONOSPACE, Typeface.BOLD);
        tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, size);
        tv.setText(String.format("%.1f", 1111.11));
        
        Rect bounds = new Rect();
        tv.getPaint().getTextBounds("9999.9", 0, "9999.9".length(), bounds);

        layout.setColumnWidth(bounds.width() + 16);
    }
}
