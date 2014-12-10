package com.ridelogger.listners;

import com.dsi.ant.plugins.antplus.pcc.defines.DeviceState;
import com.dsi.ant.plugins.antplus.pccbase.PccReleaseHandle;
import com.dsi.ant.plugins.antplus.pccbase.AntPluginPcc.IDeviceStateChangeReceiver;
import com.dsi.ant.plugins.antplus.pccbase.AntPluginPcc.IPluginAccessResultReceiver;
import com.ridelogger.RideService;


/**
 * Ant
 * @author Chet Henry
 * Listen to and log Ant+ events base class
 */
public abstract class Ant extends Base<Object>
{
    protected PccReleaseHandle<?>            releaseHandle;    //Handle class
    public    IPluginAccessResultReceiver<?> mResultReceiver;  //Receiver class
    protected int                            deviceNumber = 0;
    
    
    //setup listeners and logging 
    public Ant(int pDeviceNumber, RideService mContext)
    {
        super(mContext);
        deviceNumber = pDeviceNumber;
    }
    
    
    public IDeviceStateChangeReceiver mDeviceStateChangeReceiver = new IDeviceStateChangeReceiver()
    {
        @Override
        public void onDeviceStateChange(final DeviceState newDeviceState){
            //if we lose a device zero out its values
            if(newDeviceState.equals(DeviceState.DEAD)) {
                zeroReadings();
            }
        }
    };
    
    
    abstract protected void requestAccess();

    
    @Override
    public void onDestroy()
    {
        if(releaseHandle != null) {
            releaseHandle.close();
        }
    }
}


