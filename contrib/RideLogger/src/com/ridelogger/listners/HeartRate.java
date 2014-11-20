package com.ridelogger.listners;

import com.dsi.ant.plugins.antplus.pcc.AntPlusHeartRatePcc;
import com.dsi.ant.plugins.antplus.pcc.AntPlusHeartRatePcc.DataState;
import com.dsi.ant.plugins.antplus.pcc.AntPlusHeartRatePcc.IHeartRateDataReceiver;
import com.dsi.ant.plugins.antplus.pcc.defines.DeviceState;
import com.dsi.ant.plugins.antplus.pcc.defines.EventFlag;
import com.dsi.ant.plugins.antplus.pcc.defines.RequestAccessResult;
import com.dsi.ant.plugins.antplus.pccbase.AntPluginPcc.IPluginAccessResultReceiver;
import com.dsi.ant.plugins.antplus.pccbase.MultiDeviceSearch.MultiDeviceSearchResult;
import com.ridelogger.RideService;
import java.math.BigDecimal;
import java.util.EnumSet;

/**
 * HeartRate
 * @author Chet Henry
 * Listen to and log Ant+ HearRate events
 */
public class HeartRate extends Ant
{
    public HeartRate(MultiDeviceSearchResult result, RideService mContext) {
        super(result, mContext);
        releaseHandle = AntPlusHeartRatePcc.requestAccess(context, result.getAntDeviceNumber(), 0, mResultReceiver, mDeviceStateChangeReceiver);
    }
    
    
    public HeartRate(MultiDeviceSearchResult result, RideService mContext, Boolean psnoop) {
        super(result, mContext, psnoop);
        releaseHandle = AntPlusHeartRatePcc.requestAccess(context, result.getAntDeviceNumber(), 0, mResultReceiver, mDeviceStateChangeReceiver);
    }
    
    
    public IPluginAccessResultReceiver<AntPlusHeartRatePcc> mResultReceiver = new IPluginAccessResultReceiver<AntPlusHeartRatePcc>() {
        //Handle the result, connecting to events on success or reporting failure to user.
        @Override
        public void onResultReceived(AntPlusHeartRatePcc result, RequestAccessResult resultCode, DeviceState initialDeviceState)
        {
            if(resultCode == com.dsi.ant.plugins.antplus.pcc.defines.RequestAccessResult.SUCCESS) {
                result.subscribeHeartRateDataEvent(
                    new IHeartRateDataReceiver() {
                        @Override
                        public void onNewHeartRateData(final long estTimestamp, EnumSet<EventFlag> eventFlags, final int computedHeartRate, final long heartBeatCount, final BigDecimal heartBeatEventTime, final DataState dataState) {
                            alterCurrentData("HR", reduceNumberToString(computedHeartRate));
                        }
                    }
                );
            }
        }
    };
    
    
    @Override
    public void zeroReadings()
    {
        alterCurrentData("HR", "0");
    }
}