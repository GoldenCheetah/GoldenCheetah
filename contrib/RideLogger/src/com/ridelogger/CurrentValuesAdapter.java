package com.ridelogger;

import android.graphics.Typeface;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

public class CurrentValuesAdapter extends BaseAdapter {

    public StartActivity                 context;
    
    public TextView[] tvs = new TextView[RideService.TOTALSENSORS];

    public CurrentValuesAdapter(StartActivity c) {
        context = c;
        
        for (int i = 0; i < RideService.TOTALSENSORS; i++) {
            tvs[i] = new TextView(context);
            tvs[i].setTextAppearance(context, android.R.attr.textAppearanceLarge);
            tvs[i].setTypeface(null, Typeface.BOLD);
            tvs[i].setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
            tvs[i].setText(String.format("%.2f", (float) 0.0) + " " +  RideService.KEYS[i].toString().toLowerCase());
        }
    }
    
    
    @Override
    public int getCount() {
        return tvs.length;
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
            return (TextView) tvs[position];
        } else {
            return (TextView) convertView;
        }
    }
    
    
    public void update(float[] values) {
        for (int i = 0; i < values.length; i++) {
            if(tvs[i] != null) {
                tvs[i].setText(String.format("%.2f", values[i]) + " " +  RideService.KEYS[i].toString().toLowerCase());
            } else {
                tvs[i] = new TextView(context);
                tvs[i].setTextAppearance(context, android.R.attr.textAppearanceLarge);
                tvs[i].setTypeface(null, Typeface.BOLD);
                tvs[i].setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
                tvs[i].setText(String.format("%.2f", values[i]) + " " +  RideService.KEYS[i].toString().toLowerCase());
            }
        }
        
        this.notifyDataSetChanged();
    }
}
