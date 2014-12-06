package com.ridelogger;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;
import java.util.Set;
import java.util.Timer;
import java.util.TimerTask;

import com.dsi.ant.plugins.antplus.pcc.defines.DeviceType;
import com.dsi.ant.plugins.antplus.pcc.defines.RequestAccessResult;
import com.dsi.ant.plugins.antplus.pccbase.MultiDeviceSearch;
import com.dsi.ant.plugins.antplus.pccbase.MultiDeviceSearch.MultiDeviceSearchResult;

import android.app.ActivityManager;
import android.app.ActivityManager.RunningServiceInfo;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.preference.MultiSelectListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;

public class SettingsActivity extends PreferenceActivity {
    
    static ArrayList<MultiDeviceSearchResult> foundDevices = new ArrayList<MultiDeviceSearchResult>();
    
    /**
     * This fragment contains a second-level set of preference that you
     * can get to by tapping an item in the first preferences fragment.
     */
    public static class GeneralFragment extends PreferenceFragment {
        
        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            // Load the preferences from an XML resource
            addPreferencesFromResource(R.xml.general_settings);
        }
        
        
        @Override
        public void onResume() {
            // TODO Auto-generated method stub
            super.onResume();
            
            if(((SettingsActivity) getActivity()).getServiceRunning(RideService.class) != null) {
                Preference pref = findPreference(getString(R.string.PREF_RIDER_NAME));
                pref.setEnabled(false);
                
                pref = findPreference(getString(R.string.PREF_EMERGENCY_NUMBER));
                pref.setEnabled(false);
            }
        }
    }
    
    
    /**
     * This fragment contains a second-level set of preference that you
     * can get to by tapping an item in the first preferences fragment.
     */
    public static class SensorsFragment extends PreferenceFragment {        
        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            addPreferencesFromResource(R.xml.sensors_settings);
            MultiSelectListPreference mMultiSelectListPreference = (MultiSelectListPreference) findPreference(getString(R.string.PREF_TRACKING_SENSORS));
            mMultiSelectListPreference.setEntries( RideService.KEYS );
            
            CharSequence[] keys = new CharSequence[RideService.KEYS.length];
            
            for (int i = 0; i < RideService.KEYS.length; i++) {
                keys[i] = String.valueOf(i);
            }
            mMultiSelectListPreference.setEntryValues(keys);
        }
    }
    
    
    @Override
    public boolean isValidFragment(String fragment) {
        return true;
    }
    
    
    /**
     * This fragment contains a second-level set of preference that you
     * can get to by tapping an item in the first preferences fragment.
     */
    public static class AntFragment extends PreferenceFragment {
        private MultiDeviceSearch mSearch;
        private MultiSelectListPreference mMultiSelectListPreference;
        
        @Override
        public void onConfigurationChanged(Configuration newConfig) {
            // TODO Auto-generated method stub
            super.onConfigurationChanged(newConfig);
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            
            addPreferencesFromResource(R.xml.ant_settings);
            mMultiSelectListPreference = (MultiSelectListPreference) findPreference(getString(R.string.PREF_PAIRED_ANTS));
        }
        
        @Override
        public void onResume() {
            // TODO Auto-generated method stub
            super.onResume();
            initAntSearch();
        }

        
        /**
         * search for ant+ if we are not recording
         */
        public void initAntSearch() {
            mMultiSelectListPreference.setEnabled(false);
            if(((SettingsActivity) getActivity()).getServiceRunning(RideService.class) == null) {
                mMultiSelectListPreference.setSummary(R.string.searching_for_ants);
                
                setupAnt();

                Timer timer = new Timer();
                final Handler handler = new Handler();
                
                timer.schedule(
                    new TimerTask() {
                        @Override  
                        public void run() {
                            if(mMultiSelectListPreference.getEntryValues().length > 0) {
                                handler.post(new Runnable() {
                                    public void run() {
                                        mMultiSelectListPreference.setEnabled(true);
                                        mMultiSelectListPreference.setSummary(R.string.found_ants);
                                    }
                                });
                            } else {
                                handler.post(new Runnable() {
                                    public void run() {
                                        mMultiSelectListPreference.setEnabled(true);
                                        mMultiSelectListPreference.setSummary(R.string.no_found_ants);
                                    }
                                });
                            }
                        }
                    },
                    5000
                );      
            }
        }
        

        /**
         * try to pair some ant+ devices
         */
        protected void setupAnt() {
            MultiDeviceSearch.SearchCallbacks        mCallback;
            MultiDeviceSearch.RssiCallback           mRssiCallback;
            
            updateList(foundDevices);
            
            mCallback = new MultiDeviceSearch.SearchCallbacks(){
                public void onDeviceFound(final MultiDeviceSearchResult deviceFound)
                {
                    if(!foundDevices.contains(deviceFound)) {
                        foundDevices.add(deviceFound);
                        updateList(foundDevices);
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
            mSearch = new MultiDeviceSearch(this.getActivity(), EnumSet.allOf(DeviceType.class), mCallback, mRssiCallback);
        }
        
        
        /**
         * dialog of soon to be paired ant devices
         * @param foundDevices
         */
        protected void updateList(final ArrayList<MultiDeviceSearchResult> foundDevices) {
            ArrayList<CharSequence> foundDevicesString = new ArrayList<CharSequence>();
            ArrayList<CharSequence> foundDevicesValues = new ArrayList<CharSequence>();
            
            for(MultiDeviceSearchResult device : foundDevices) { 
                foundDevicesString.add(device.getAntDeviceType() + ": " + String.valueOf(device.getAntDeviceNumber()));
            }
            
            for(MultiDeviceSearchResult device : foundDevices) { 
                foundDevicesValues.add(String.valueOf(device.getAntDeviceNumber()));
            }
            
            if(mMultiSelectListPreference != null) {
                mMultiSelectListPreference.setEntries(
                    foundDevicesString.toArray(new CharSequence[foundDevicesString.size()])
                );
                mMultiSelectListPreference.setEntryValues(
                    foundDevicesValues.toArray(new CharSequence[foundDevicesValues.size()])
                );
            }
        }
        
        
        @Override
        public void onDestroy() {
            if(mSearch != null) mSearch.close();
            super.onDestroy();
        }
    }
    
    /**
     * Populate the activity with the top-level headers.
     */
    @Override
    public void onBuildHeaders(List<Header> target) {
        loadHeadersFromResource(R.layout.settings, target);
        
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(this);
        settings.registerOnSharedPreferenceChangeListener(
                new SharedPreferences.OnSharedPreferenceChangeListener() {
                    @Override
                    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String pkey) {
                        if (pkey == getString(R.string.PREF_PAIRED_ANTS)) {
                            Set<String> sensors = sharedPreferences.getStringSet(pkey, null);
                            if(sensors != null && sensors.size() > 0) {
                                Editor editor = sharedPreferences.edit();
                                for(MultiDeviceSearchResult result : foundDevices) {
                                    sensors.contains(String.valueOf(result.getAntDeviceNumber()));
                                    editor.putInt(String.valueOf(result.getAntDeviceNumber()), result.getAntDeviceType().getIntValue());
                                }
                                editor.commit();
                            }
                        }
                    }
                }
            );
    }
    
    
    /**
     * is a service running or not
     * @param serviceClass
     * @return
     */
    public RunningServiceInfo getServiceRunning(Class<?> serviceClass) {
        ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        for (RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
            if (serviceClass.getName().equals(service.service.getClassName())) {
                return service;
            }
        }
        return null;
    }
}
