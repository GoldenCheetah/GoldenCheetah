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
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.text.InputType;
import android.widget.EditText;
import android.widget.Toast;

public class StartActivity extends FragmentActivity
{
    Intent rsi;
    public static final String PREFS_NAME = "RideLogger";
    public static final String RIDER_NAME = "RiderName";
    public static final String PAIRED_ANTS = "PairedAnts";
    SharedPreferences settings;
    AlertDialog dialog;
    public MultiDeviceSearch mSearch;
    
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {    
        super.onCreate(savedInstanceState);   
        rsi = new Intent(this, RideService.class);
        
        settings = getSharedPreferences(PREFS_NAME, 0);
        String rider_name = settings.getString(RIDER_NAME, "");
        
        if(rider_name == "") {
            // 1. Instantiate an AlertDialog.Builder with its constructor
            AlertDialog.Builder builder = new AlertDialog.Builder(this);

            // 2. Chain together various setter methods to set the dialog characteristics
            builder.setMessage("What is your Golder Cheata Rider Name")
                   .setTitle("Chose Rider Name");
            
            // Set up the input
            final EditText input = new EditText(this);
            // Specify the type of input expected; this, for example, sets the input as a password, and will mask the text
            input.setInputType(InputType.TYPE_CLASS_TEXT);
            builder.setView(input);

            
            builder.setPositiveButton("Set", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int id) {
                    String name = input.getText().toString();
                    if(name != "" && name != null) {
                        SharedPreferences.Editor editor = settings.edit();
                        editor.putString(RIDER_NAME, name);
                        editor.commit();
                        setupAnt();
                    }
                }
            });

            // 3. Get the AlertDialog from create()
            dialog = builder.create();
            dialog.show();
        } else {
            toggleRide();
            finish();
        }
    }
    
    protected void setupAnt() {
        
        MultiDeviceSearch.SearchCallbacks        mCallback;
        MultiDeviceSearch.RssiCallback           mRssiCallback;
        final ArrayList<MultiDeviceSearchResult> foundDevices = new ArrayList<MultiDeviceSearchResult>();
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
    
    protected void selectDevicesDialog(final ArrayList<MultiDeviceSearchResult> foundDevices) {
        final ArrayList<Integer>      mSelectedItems     = new ArrayList<Integer>();  // Where we track the selected items
        final ArrayList<CharSequence> foundDevicesString = new ArrayList<CharSequence>();
        
        for(MultiDeviceSearchResult device : foundDevices) { 
            foundDevicesString.add(device.getAntDeviceType() + " - " + device.getDeviceDisplayName());
        }
        CharSequence[] foundDevicesCharSeq = foundDevicesString.toArray(new CharSequence[foundDevicesString.size()]);
        
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        
        // Set the dialog title
        builder.setTitle("Select Paired Ant Devices")
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
           .setPositiveButton("Pair", new DialogInterface.OnClickListener() {
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
                   finish();
               }
           });

        dialog = builder.create();
        dialog.show();
    }
    
    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        super.onDestroy();
    }

    
    private void stopRide() {
        Toast toast = Toast.makeText(getApplicationContext(), "Stoping Ride!", Toast.LENGTH_LONG);
        toast.show();
        this.stopService(rsi);
    }
    

    protected void toggleRide() {
        if(!isServiceRunning(RideService.class)) {
            startRide();
        } else {
            stopRide();
        }
    }
    

    private void startRide() {
        if(!isServiceRunning(RideService.class)) {
            this.startService(rsi);
        }
        Toast toast = Toast.makeText(getApplicationContext(), "Starting Ride!", Toast.LENGTH_LONG);
        toast.show();
    }
    
    
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
