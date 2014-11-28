package com.ridelogger;

import android.preference.MultiSelectListPreference;
import android.util.AttributeSet;
import android.app.Dialog;
import android.content.Context;

public class DynamicMultiSelectListPreference extends MultiSelectListPreference {
    public DynamicMultiSelectListPreference(Context context) {
        super(context);
    }
    
    public DynamicMultiSelectListPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void refresh() {
        Dialog test = getDialog();
        if(test != null && test.isShowing()) {
            test.dismiss();
            showDialog(null);
        }
    }
}