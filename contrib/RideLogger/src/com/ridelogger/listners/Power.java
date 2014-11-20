package com.ridelogger.listners;

import android.content.Context;

import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.CalculatedWheelDistanceReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.CalculatedWheelSpeedReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.DataSource;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.ICalculatedCrankCadenceReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.ICalculatedPowerReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.ICalculatedTorqueReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.IInstantaneousCadenceReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.IPedalPowerBalanceReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.IPedalSmoothnessReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.IRawCrankTorqueDataReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.IRawPowerOnlyDataReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.IRawWheelTorqueDataReceiver;
import com.dsi.ant.plugins.antplus.pcc.AntPlusBikePowerPcc.ITorqueEffectivenessReceiver;
import com.dsi.ant.plugins.antplus.pcc.defines.DeviceState;
import com.dsi.ant.plugins.antplus.pcc.defines.EventFlag;
import com.dsi.ant.plugins.antplus.pcc.defines.RequestAccessResult;
import com.dsi.ant.plugins.antplus.pccbase.AntPluginPcc.IDeviceStateChangeReceiver;
import com.dsi.ant.plugins.antplus.pccbase.AntPluginPcc.IPluginAccessResultReceiver;
import com.dsi.ant.plugins.antplus.pccbase.MultiDeviceSearch.MultiDeviceSearchResult;

import java.math.BigDecimal;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.Map;

/**
 * Base class to connects to Heart Rate Plugin and display all the event data.
 */
public class Power extends Base
{
    
    public Power(MultiDeviceSearchResult result, Context mContext) {
        super(result, mContext);
        releaseHandle = AntPlusBikePowerPcc.requestAccess(context, result.getAntDeviceNumber(), 0, mResultReceiver, mDeviceStateChangeReceiver);
    }
    
    BigDecimal wheelCircumferenceInMeters = new BigDecimal("2.07");
    
    IDeviceStateChangeReceiver mDeviceStateChangeReceiver = new IDeviceStateChangeReceiver()
    {
        @Override
        public void onDeviceStateChange(final DeviceState newDeviceState){}
    };

    
    protected IPluginAccessResultReceiver<AntPlusBikePowerPcc> mResultReceiver = new IPluginAccessResultReceiver<AntPlusBikePowerPcc>() {
        //Handle the result, connecting to events on success or reporting failure to user.
        @Override
        public void onResultReceived(AntPlusBikePowerPcc result, RequestAccessResult resultCode, DeviceState initialDeviceState)
        {
            if(resultCode == com.dsi.ant.plugins.antplus.pcc.defines.RequestAccessResult.SUCCESS) {
                result.subscribeCalculatedPowerEvent(new ICalculatedPowerReceiver() {
                        @Override
                        public void onNewCalculatedPower(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final DataSource dataSource, final BigDecimal calculatedPower) {
                            alterCurrentData("WATTS", reduceNumberToString(calculatedPower));
                        }
                    }
                );

                result.subscribeCalculatedTorqueEvent(
                    new ICalculatedTorqueReceiver() {
                        @Override
                        public void onNewCalculatedTorque(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final DataSource dataSource, final BigDecimal calculatedTorque) {
                            alterCurrentData("NM", reduceNumberToString(calculatedTorque));
                        }
                    }
                );

                result.subscribeCalculatedCrankCadenceEvent(
                    new ICalculatedCrankCadenceReceiver() {
                        @Override
                        public void onNewCalculatedCrankCadence(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final DataSource dataSource, final BigDecimal calculatedCrankCadence) {
                            alterCurrentData("RPM", reduceNumberToString(calculatedCrankCadence));
                        }
                    }
                );

                result.subscribeCalculatedWheelSpeedEvent(
                    new CalculatedWheelSpeedReceiver(wheelCircumferenceInMeters) {
                        @Override
                        public void onNewCalculatedWheelSpeed(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final DataSource dataSource, final BigDecimal calculatedWheelSpeed)
                        {
                            alterCurrentData("KMH", reduceNumberToString(calculatedWheelSpeed));
                        }
                    }
                );

                result.subscribeCalculatedWheelDistanceEvent(
                    new CalculatedWheelDistanceReceiver(wheelCircumferenceInMeters) {
                        @Override
                        public void onNewCalculatedWheelDistance(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final DataSource dataSource, final BigDecimal calculatedWheelDistance) 
                        {
                            alterCurrentData("KM", reduceNumberToString(calculatedWheelDistance));
                        }
                    }
                );

                result.subscribeInstantaneousCadenceEvent(
                    new IInstantaneousCadenceReceiver() {
                        @Override
                        public void onNewInstantaneousCadence(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final DataSource dataSource, final int instantaneousCadence)
                        {
                            alterCurrentData("RPM", reduceNumberToString(instantaneousCadence));
                        }
                    }
                );

                result.subscribeRawPowerOnlyDataEvent(
                    new IRawPowerOnlyDataReceiver() {
                        @Override
                        public void onNewRawPowerOnlyData(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final long powerOnlyUpdateEventCount, final int instantaneousPower, final long accumulatedPower)
                        {
                            alterCurrentData("WATTS", reduceNumberToString(instantaneousPower));
                        }
                    }
                );

                result.subscribePedalPowerBalanceEvent(
                    new IPedalPowerBalanceReceiver() {
                        @Override
                        public void onNewPedalPowerBalance(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final boolean rightPedalIndicator, final int pedalPowerPercentage)
                        {
                            alterCurrentData("LTE", reduceNumberToString(pedalPowerPercentage));
                        }
                    }
                );

                result.subscribeRawWheelTorqueDataEvent(
                    new IRawWheelTorqueDataReceiver() {
                        @Override
                        public void onNewRawWheelTorqueData(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final long wheelTorqueUpdateEventCount, final long accumulatedWheelTicks, final BigDecimal accumulatedWheelPeriod, final BigDecimal accumulatedWheelTorque)
                        {
                            alterCurrentData("NM", reduceNumberToString(accumulatedWheelTorque));
                        }
                    }
                );

                result.subscribeRawCrankTorqueDataEvent(
                    new IRawCrankTorqueDataReceiver() {
                        @Override
                        public void onNewRawCrankTorqueData(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final long crankTorqueUpdateEventCount, final long accumulatedCrankTicks, final BigDecimal accumulatedCrankPeriod, final BigDecimal accumulatedCrankTorque)
                        {
                            alterCurrentData("NM", reduceNumberToString(accumulatedCrankTorque));
                        }
                    }
                );

                result.subscribeTorqueEffectivenessEvent(
                    new ITorqueEffectivenessReceiver() {
                        @Override
                        public void onNewTorqueEffectiveness(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final long powerOnlyUpdateEventCount, final BigDecimal leftTorqueEffectiveness, final BigDecimal rightTorqueEffectiveness)
                        {                            
                            Map<String, String> map = new HashMap<String, String>();
                            map.put("LTE", reduceNumberToString(leftTorqueEffectiveness));
                            map.put("RTE", reduceNumberToString(rightTorqueEffectiveness));
                            alterCurrentData(map);
                        }
    
                    }
                );

                result.subscribePedalSmoothnessEvent(new IPedalSmoothnessReceiver() {
                        @Override
                        public void onNewPedalSmoothness(final long estTimestamp, final EnumSet<EventFlag> eventFlags, final long powerOnlyUpdateEventCount, final boolean separatePedalSmoothnessSupport, final BigDecimal leftOrCombinedPedalSmoothness, final BigDecimal rightPedalSmoothness)
                        {
                            Map<String, String> map = new HashMap<String, String>();
                            map.put("SNPLC", reduceNumberToString(leftOrCombinedPedalSmoothness));
                            map.put("SNPR",  reduceNumberToString(rightPedalSmoothness));
                            alterCurrentData(map);
                        }
                    }
                );
            }
        }
    };
}


