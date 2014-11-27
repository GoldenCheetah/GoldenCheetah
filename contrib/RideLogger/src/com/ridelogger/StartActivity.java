package com.ridelogger;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.HashSet;

import com.dsi.ant.plugins.antplus.pcc.defines.DeviceType;
import com.dsi.ant.plugins.antplus.pcc.defines.RequestAccessResult;
import com.dsi.ant.plugins.antplus.pccbase.MultiDeviceSearch;
import com.dsi.ant.plugins.antplus.pccbase.MultiDeviceSearch.MultiDeviceSearchResult;

import android.app.ActivityManager;
import android.app.ActivityManager.RunningServiceInfo;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.text.InputType;
import android.view.View;
import android.widget.EditText;
import android.widget.Toast;

public class StartActivity extends FragmentActivity
{
    Intent rsi;
    public static final String PREFS_NAME        = "RideLogger";
    public static final String RIDER_NAME        = "RiderName";
    public static final String EMERGENCY_NUMBER  = "EmergencyNumbuer";
    public static final String DETECT_CRASH      = "DetectCrash";
    public static final String PHONE_HOME        = "PhoneHome";
    public static final String PAIRED_ANTS       = "PairedAnts";
    public static final String PHONE_HOME_PERIOD = "PhoneHomePeriod";
    
    SharedPreferences settings;
    MultiDeviceSearch mSearch;
    
    /**
     * start up our class
     */
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {    
        super.onCreate(savedInstanceState);   
        rsi                              = new Intent(this, RideService.class);
        settings                         = getSharedPreferences(PREFS_NAME, 0);

        if(settings.getString(RIDER_NAME, "") == "") {
            setupSettings();
        } else {
            toggleRide();
        }
    }
    
    public void setupSettings() {
        final Runnable askAntRunnable = new Runnable() {
            @Override
            public void run() {
                final Runnable setupAntRunnable = new Runnable() {
                    @Override
                    public void run() {
                        setupAnt();
                    }
                };
                
                final Runnable skipAntRunnable = new Runnable() {
                    @Override
                    public void run() {
                        toggleRide();
                    }
                };
                
                promptDialogBoolean(getString(R.string.ant_setup_title), getString(R.string.ant_setup_note), setupAntRunnable, skipAntRunnable);
            }
        };
        
        final Runnable setupPhoneHomePeriod = new Runnable() {
            @Override
            public void run() {
                promptDialogPhone(
                    getString(R.string.sms_period_dialog_title), 
                    getString(R.string.sms_period_dialog_note),
                    PHONE_HOME_PERIOD,
                    askAntRunnable
                );
            }
        };
        
        final Runnable setupPhoneHomeRunnable = new Runnable() {
            @Override
            public void run() {
                promptDialogBoolean(
                    getString(R.string.emergency_contact_dialog_title), 
                    getString(R.string.emergency_contact_dialog_note),
                    PHONE_HOME,
                    setupPhoneHomePeriod
                );
            }
        };
        
        final Runnable setupDetectCrashRunnable = new Runnable() {
            @Override
            public void run() {
                promptDialogBoolean(
                    getString(R.string.crash_detection_dialog_title), 
                    getString(R.string.crash_detection_dialog_note),
                    DETECT_CRASH,
                    setupPhoneHomeRunnable
                );
            }
        };
        
        Runnable setupEmergencyContactRunnable = new Runnable() {
            @Override
            public void run() {
                promptDialogPhone(
                    getString(R.string.emergency_contact_number_dialog_title), 
                    getString(R.string.emergency_contact_number_dialog_note),
                    EMERGENCY_NUMBER,
                    setupDetectCrashRunnable
                );
            }
        };
        
        promptDialogInput(
            getString(R.string.gc_rider_name_dialog_title), 
            getString(R.string.gc_rider_name_dialog_note),
            RIDER_NAME,
            setupEmergencyContactRunnable
        );
    }
    
    
    /**
     * TextBox Dialog Prompt
     * @param title
     * @param message
     * @param settingSaveKey
     * @param callBack
     */
    public void promptDialogInput(String title, String message, final String settingSaveKey, final Runnable callBack) {
     // Set up the input
        final EditText input = new EditText(this);
        
        // Specify the type of input expected; this, for example, sets the input as a password, and will mask the text
        input.setInputType(InputType.TYPE_CLASS_TEXT);
        input.setText(settings.getString(settingSaveKey, ""));
        
        OnClickListener positiveClick = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                String value = input.getText().toString();
                if(value != "" && value != null) {
                    SharedPreferences.Editor editor = settings.edit();
                    editor.putString(settingSaveKey, value).commit();
                    callBack.run();
                }
            }
        };
        promptDialog(title, message, input, positiveClick);
    }
    
    
    /**
     * TextBox Dialog Prompt
     * @param title
     * @param message
     * @param settingSaveKey
     * @param callBack
     */
    public void promptDialogPhone(String title, String message, final String settingSaveKey, final Runnable callBack) {
     // Set up the input
        final EditText input = new EditText(this);
        // Specify the type of input expected; this, for example, sets the input as a password, and will mask the text
        input.setInputType(InputType.TYPE_CLASS_PHONE);
        input.setText(settings.getString(settingSaveKey, ""));
        
        OnClickListener positiveClick = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                String value = input.getText().toString();
                if(value != "" && value != null) {
                    SharedPreferences.Editor editor = settings.edit();
                    editor.putString(settingSaveKey, value).commit();
                    callBack.run();
                }
            }
        };
        promptDialog(title, message, input, positiveClick);
    }
    

    /**
     * Boolean dialog prompt
     * @param title
     * @param message
     * @param settingSaveKey
     * @param positiveCallback
     */
    public void promptDialogBoolean(String title, String message, Runnable positiveCallback, Runnable negativeCallback) {               
       promptDialogBoolean(title, message, null, positiveCallback, negativeCallback);
    }
    
    
    /**
     * Boolean dialog prompt
     * @param title
     * @param message
     * @param settingSaveKey
     * @param positiveCallback
     */
    public void promptDialogBoolean(String title, String message, String settingSaveKey, Runnable positiveCallback) {               
       promptDialogBoolean(title, message, settingSaveKey, positiveCallback, positiveCallback);
    }
    
    
    /**
     * Boolean dialog prompt
     * @param title
     * @param message
     * @param settingSaveKey
     * @param positiveCallback
     */
    public void promptDialogBoolean(String title, String message, Runnable positiveCallback, Runnable negativeCallback, String positiveButtonLabel, String negativeButtonLabel) {               
       promptDialogBoolean(title, message, null, positiveCallback, negativeCallback, positiveButtonLabel, negativeButtonLabel);
    }
    
    
    /**
     * Boolean dialog prompt
     * @param title
     * @param message
     * @param settingSaveKey
     * @param positiveCallback
     */
    public void promptDialogBoolean(String title, String message, String settingSaveKey, Runnable positiveCallback, Runnable negativeCallback) {               
       promptDialogBoolean(title, message, settingSaveKey, positiveCallback, negativeCallback, getString(R.string.boolean_dialog_yes), getString(R.string.boolean_dialog_no));
    }
    
    
    /**
     * Boolean dialog prompt
     * @param title
     * @param message
     * @param settingSaveKey
     * @param positiveCallback
     * @param negativeCallback
     */
    public void promptDialogBoolean(String title, String message, final String settingSaveKey, final Runnable positiveCallback, final Runnable negativeCallback, String positiveButtonLabel, String negativeButtonLabel) {        
        OnClickListener positiveClick = new DialogInterface.OnClickListener() {
           public void onClick(DialogInterface dialog, int id) {
                   if(settingSaveKey != null) {
                       SharedPreferences.Editor editor = settings.edit();
                       editor.putBoolean(settingSaveKey, true).commit();
                   }
                   positiveCallback.run();
           }
       };
       
       OnClickListener negativeClick = new DialogInterface.OnClickListener() {
           public void onClick(DialogInterface dialog, int id) {
               if(settingSaveKey != null) {
                   SharedPreferences.Editor editor = settings.edit();
                   editor.putBoolean(settingSaveKey, false).commit();
               }
               negativeCallback.run();
           }
       };
       
       promptDialog(title, message, null, positiveClick, positiveButtonLabel, negativeClick, negativeButtonLabel);
    }
    
    
    /**
     * very general dialog prompt without positiveLabel
     * @param title
     * @param message
     * @param view
     * @param positiveClick
     */
    public void promptDialog(
            String          title, 
            String          message,
            View            view,
            OnClickListener positiveClick
    ) {
        promptDialog(title, message, view, positiveClick, null, null, null);
    }
    
    
    /**
     * very general dialog prompt
     * @param title
     * @param message
     * @param view
     * @param positiveClick
     * @param positiveLabel
     */
    public void promptDialog(
              String          title, 
              String          message,
              View            view,
              OnClickListener positiveClick,
              String          positiveLabel,
              OnClickListener negativeClick,
              String          negativeLabel
    ) {
        if(positiveLabel == null) positiveLabel = getString(R.string.save_setting_button);
        
        AlertDialog.Builder builder = new AlertDialog.Builder(this);

        // 2. Chain together various setter methods to set the dialog characteristics
        builder.setMessage(message)
            .setTitle(title)
            .setPositiveButton(positiveLabel, positiveClick);
        
        if(view != null) {
            builder.setView(view);
        }
        
        if(negativeClick != null) {
            if(negativeLabel == null) negativeLabel = getString(R.string.skip_setting_button);
            
            builder.setNegativeButton(negativeLabel, negativeClick);
        }
         
         AlertDialog dialog = builder.create();

        // 3. Get the AlertDialog from create()
        dialog.show();
    }
    
    
    /**
     * try to pair some ant+ devices
     */
    protected void setupAnt() {
        MultiDeviceSearch.SearchCallbacks        mCallback;
        MultiDeviceSearch.RssiCallback           mRssiCallback;
        final ArrayList<MultiDeviceSearchResult> foundDevices = new ArrayList<MultiDeviceSearchResult>();
        selectDevicesDialog(foundDevices);
        mCallback = new MultiDeviceSearch.SearchCallbacks(){
            public void onDeviceFound(final MultiDeviceSearchResult deviceFound)
            {
                if(!foundDevices.contains(deviceFound)) {
                    foundDevices.add(deviceFound);
                    selectDevicesDialog(foundDevices);
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
    
    
    /**
     * dialog of soon to be paired ant devices
     * @param foundDevices
     */
    protected void selectDevicesDialog(final ArrayList<MultiDeviceSearchResult> foundDevices) {
        final ArrayList<Integer>      mSelectedItems     = new ArrayList<Integer>();  // Where we track the selected items
        final ArrayList<CharSequence> foundDevicesString = new ArrayList<CharSequence>();
        
        for(MultiDeviceSearchResult device : foundDevices) { 
            foundDevicesString.add(device.getAntDeviceType() + " - " + device.getDeviceDisplayName());
        }
        CharSequence[] foundDevicesCharSeq = foundDevicesString.toArray(new CharSequence[foundDevicesString.size()]);
        
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        
        // Set the dialog title
        builder.setTitle(getString(R.string.ant_pair_dialog_title))
            // Specify the list array, the items to be selected by default (null for none),
            // and the listener through which to receive callbacks when items are selected
           .setMultiChoiceItems(foundDevicesCharSeq, null, new DialogInterface.OnMultiChoiceClickListener() {
               @Override
               public void onClick(DialogInterface dialog, int which, boolean isChecked) {
                   if (isChecked) {
                       // If the user checked the item, add it to the selected items
                       mSelectedItems.add(which);
                   } else if (mSelectedItems.contains(which)) {
                       // Else, if the item is already in the array, remove it 
                       mSelectedItems.remove(Integer.valueOf(which));
                   }
               }
           })
           // Set the action buttons
           .setPositiveButton(getString(R.string.ant_pair_dialog_button_label), new DialogInterface.OnClickListener() {
               @Override
               public void onClick(DialogInterface dialog, int id) {
                   ArrayList<String> ids = new ArrayList<String>();
                   for(Integer index : mSelectedItems) { 
                       ids.add(Integer.toString(foundDevices.get(index).getAntDeviceNumber()));
                   }
                   
                   SharedPreferences.Editor editor = settings.edit();
                   editor.putStringSet(PAIRED_ANTS, new HashSet<String>(ids));
                   editor.commit();
                   toggleRide();
               }
           });

        AlertDialog dialog = builder.create();
        dialog.show();
    }
    
    @Override
    protected void onDestroy() {
        if(mSearch != null) mSearch.close();
        super.onDestroy();
        finish();
    }
    

    /**
     * stop ride and clean up references 
     */
    private void stopRide() {
        Toast toast = Toast.makeText(getApplicationContext(), getString(R.string.stopping_ride), Toast.LENGTH_LONG);
        toast.show();
        this.stopService(rsi);
    }
    

    /**
     * start or stop ride
     */
    protected void toggleRide() {
        if(!isServiceRunning(RideService.class)) {
            final Runnable startAndCloseRunnable = new Runnable() {
                @Override
                public void run() {
                    startRide();
                    finish();
                }
            };
            
            final Runnable settingsRunnable = new Runnable() {
                @Override
                public void run() {
                    setupSettings();
                }
            };
            
            promptDialogBoolean(
                getString(R.string.ride_start_title),
                getString(R.string.ride_start_note), 
                startAndCloseRunnable,
                settingsRunnable,
                getString(R.string.start_and_close),
                getString(R.string.edit_settings)
            );            
        } else {
            final Runnable viewRunnable = new Runnable() {
                @Override
                public void run() {}
            };
            
            final Runnable stopRunnable = new Runnable() {
                @Override
                public void run() {
                    stopRide();
                    finish();
                }
            };
            
            promptDialogBoolean(
                getString(R.string.ride_stop_title),
                getString(R.string.ride_stop_note), 
                stopRunnable,
                viewRunnable,
                getString(R.string.stop_and_close),
                getString(R.string.view)
            );
        }
    }
    

    /**
     * start the ride and notify the user of success
     */
    private void startRide() {
        if(!isServiceRunning(RideService.class)) {
            this.startService(rsi);
        }
        Toast toast = Toast.makeText(getApplicationContext(), getString(R.string.starting_ride), Toast.LENGTH_LONG);
        toast.show();
    }
    
    
    /**
     * is a service running or not
     * @param serviceClass
     * @return
     */
    private boolean isServiceRunning(Class<?> serviceClass) {
        ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        for (RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
            if (serviceClass.getName().equals(service.service.getClassName())) {
                return true;
            }
        }
        return false;
    }
}
