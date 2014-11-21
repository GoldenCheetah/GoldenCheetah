package com.ridelogger.listners;

import java.util.Map;
import com.dsi.ant.plugins.antplus.pcc.defines.DeviceState;
import com.dsi.ant.plugins.antplus.pccbase.PccReleaseHandle;
import com.dsi.ant.plugins.antplus.pccbase.AntPluginPcc.IDeviceStateChangeReceiver;
import com.dsi.ant.plugins.antplus.pccbase.AntPluginPcc.IPluginAccessResultReceiver;
import com.dsi.ant.plugins.antplus.pccbase.MultiDeviceSearch.MultiDeviceSearchResult;
import com.ridelogger.RideService;


/**
 * Ant
 * @author Chet Henry
 * Listen to and log Ant+ events base class
 */
public class Ant extends Base<Object>
{
    public PccReleaseHandle<?>            releaseHandle;    //Handle class
    public IPluginAccessResultReceiver<?> mResultReceiver;  //Receiver class
    public Boolean                        snooped  = false; //should we snoop others connections?
    public String                         prefix   = "";    //prefix log messages with this
    public Ant                            that     = null;                           
    //setup listeners and logging 
    public Ant(MultiDeviceSearchResult result, RideService mContext) {
        super(mContext);
    }
    
    public Ant(MultiDeviceSearchResult result, RideService mContext, Boolean psnoop) {
        super(mContext);
        that = this;
        if(psnoop) {
            snooped  = true;
            prefix = "SNOOPED-";
        }
    }
    
    
    IDeviceStateChangeReceiver mDeviceStateChangeReceiver = new IDeviceStateChangeReceiver()
    {
        @Override
        public void onDeviceStateChange(final DeviceState newDeviceState){
            //if we lose a device zero out its values
            if(newDeviceState.equals(DeviceState.DEAD)) {
                zeroReadings();
                if(snooped) {
                    releaseHandle.close(); // release ourselves if snooped
                    context.releaseSnoopedSensor(that);
                }
            }
        }
    };

    @Override
    public void onDestroy()
    {
        if(releaseHandle != null) {
            releaseHandle.close();
        }
    }
    
    
    @Override
    public void writeData(String key, String value)
    {
        super.writeData(prefix + key, value);
    }
    
    
    @Override
    public void writeData(Map<String, String> map)
    {
        if(prefix != "") {
            for (Map.Entry<String, String> entry : map.entrySet()) {
                map.remove(entry);
                map.put(prefix + entry.getKey(), entry.getValue());
            }
        }
        
        super.writeData(map);
    }
    
    
    @Override
    public void writeData(String key, String value)
    {
        super.writeData(prefix + key, value);
    }
    
    
    @Override
    public void writeData(Map<String, String> map)
    {
        if(prefix != "") {
            for (Map.Entry<String, String> entry : map.entrySet()) {
                map.remove(entry);
                map.put(prefix + entry.getKey(), entry.getValue());
            }
        }
        
        super.writeData(map);
    }


    @Override
    public void alterCurrentData(String key, String value)
    {
        super.alterCurrentData(prefix + key, value);
    }
    
    
    @Override
    public void alterCurrentData(Map<String, String> map)
    {
        if(prefix != "") {
            for (Map.Entry<String, String> entry : map.entrySet()) {         
                map.remove(entry);
                map.put(prefix + entry.getKey(), entry.getValue());
            }
        }
        
        super.alterCurrentData(map);
    }
    
    
    public void zeroReadings() {}
}


