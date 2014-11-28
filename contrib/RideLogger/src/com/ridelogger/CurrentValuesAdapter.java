package com.ridelogger;

import java.util.LinkedHashMap;
import java.util.Map.Entry;

import android.graphics.Typeface;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

public class CurrentValuesAdapter extends BaseAdapter {

    public StartActivity                 context;
    public LinkedHashMap<String, TextView> tvs = new LinkedHashMap<String, TextView>();

    public CurrentValuesAdapter(StartActivity c) {
        context = c;
    }
    
    
    @Override
    public int getCount() {
        return tvs.size();
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
        if (convertView == null) {  // if it's not recycled, initialize some attributes
            return (TextView) tvs.values().toArray()[position];
        } else {
            return (TextView) convertView;
        }
    }
    
    
    public void update(LinkedHashMap<String, String> currentValues) {
        for (Entry<String, String> entry : currentValues.entrySet()) {
            String key = entry.getKey();
            
            String value = String.format("%.2f", Float.valueOf(entry.getValue())) + " " +  key.toLowerCase();
            
            
            if(tvs.containsKey(key)) {
                tvs.get(key).setText(value);
            } else {
                TextView tv = new TextView(context);
                tv.setTextAppearance(context, android.R.attr.textAppearanceLarge);
                tv.setTypeface(null, Typeface.BOLD);
                tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
                tv.setText(value);
                
                tvs.put(key, tv);
            }
        }
        this.notifyDataSetChanged();
    }
}
