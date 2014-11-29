package com.ridelogger;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import com.dsi.ant.plugins.antplus.pcc.defines.DeviceType;
import com.dsi.ant.plugins.antplus.pcc.defines.RequestAccessResult;
import com.dsi.ant.plugins.antplus.pccbase.MultiDeviceSearch;
import com.dsi.ant.plugins.antplus.pccbase.MultiDeviceSearch.MultiDeviceSearchResult;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.preference.MultiSelectListPreference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;

public class SettingsActivity extends PreferenceActivity {   
    
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
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            
            addPreferencesFromResource(R.xml.ant_settings);
            mMultiSelectListPreference = (MultiSelectListPreference) findPreference(getString(R.string.PREF_PAIRED_ANTS));
            mMultiSelectListPreference.setEnabled(false);

            setupAnt();

            Timer timer = new Timer();
            final Handler handler = new Handler();
            
            timer.schedule(
                new TimerTask() {
                    @Override  
                    public void run() {
                        handler.post(new Runnable() {
    
                            public void run() {
                                mMultiSelectListPreference.setEnabled(true);
                            }
                        });
                    }
                },
                5000
            );      
        }

        /**
         * try to pair some ant+ devices
         */
        protected void setupAnt() {
            MultiDeviceSearch.SearchCallbacks        mCallback;
            MultiDeviceSearch.RssiCallback           mRssiCallback;
            final ArrayList<MultiDeviceSearchResult> foundDevices = new ArrayList<MultiDeviceSearchResult>();
            
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
    }
}
