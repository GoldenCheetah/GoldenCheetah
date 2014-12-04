package com.ridelogger;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningServiceInfo;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.support.v4.app.FragmentActivity;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.GridView;
import android.widget.Toast;

public class StartActivity extends FragmentActivity
{
    Intent rsi;
    
    GridView layout;
        
    private RunningServiceInfo service;
    
    Messenger mService = null;
    boolean mIsBound;
    BroadcastReceiver mReceiver;
    private MenuItem startMenu;
    private MenuItem stopMenu;
    
    final Messenger mMessenger = new Messenger(new IncomingHandler());
    
    private CurrentValuesAdapter currentValuesAdapter;
    
    class IncomingHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            updateValues(msg.getData());
        }
    }
    
    private ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
            mService = new Messenger(service);
            try {
                Message msg = Message.obtain();
                msg.replyTo = mMessenger;
                mService.send(msg);
            } catch (RemoteException e) {
                // In this case the service has crashed before we could even do anything with it
            }
        }

        public void onServiceDisconnected(ComponentName className) {
            // This is called when the connection with the service has been unexpectedly disconnected - process crashed.
            mService = null;
        }
    };

    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu items for use in the action bar
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.start_activity, menu);
        return super.onCreateOptionsMenu(menu);
    }

    
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle presses on the action bar items
        if(item.getItemId() == R.id.Settings) {
            setupSettings();
        } else if(item.getItemId() == R.id.Start) {
            startRide();
        } else {
            stopRide();
        }
        
        return true;
    }

    
    /**
     * start up our class
     */
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {    
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_dashboard);
        rsi             = new Intent(this, RideService.class);
        
        layout = (GridView) findViewById(R.id.LayoutData);
        currentValuesAdapter = new CurrentValuesAdapter(this);
        layout.setAdapter(currentValuesAdapter);
        layout.setColumnWidth(currentValuesAdapter.getWidth());
        layout.setBackgroundColor(Color.BLACK);
        layout.setVerticalSpacing(4);
        layout.setHorizontalSpacing(4);
    }
    
    
    @Override
    protected void onResume() {
        super.onResume();
        bindToService();
    }

    @Override
    protected void onPause() {
        super.onPause();
        unBindToService();
    }
    
    @Override
    protected void onStop() {
        // TODO Auto-generated method stub
        super.onStop();
    }
    
    
    /**
     * setup the settings for the user
     */
    public void setupSettings() {
        Intent intent = new Intent(this, SettingsActivity.class);
        startActivity(intent);
    }

    
    /**
     * stop ride and clean up references 
     */
    private void stopRide() {
        Toast toast = Toast.makeText(getApplicationContext(), getString(R.string.stopping_ride), Toast.LENGTH_LONG);
        toast.show();
        this.stopService(rsi);
        finish();
    }
    
    
    /**
     * tell the service to start sending us messages with current values
     */
    public void bindToService() {
        service = getServiceRunning(RideService.class);
        if(service != null) {
            bindService(new Intent(StartActivity.this, RideService.class), mConnection, Context.BIND_AUTO_CREATE);
            mIsBound = true;
        }
    }
    
    
    /**
     * tell the service to stop sending us messages with current values
     */
    public void unBindToService() {
        if (mIsBound) {
            // Detach our existing connection.
            unbindService(mConnection);
            mIsBound = false;
        }
    }
    
    
    /**
     * update the text fields with current values
     * @param bundle 
     */
    public void updateValues(Bundle bundle) {
        float[] currentValues = (float[]) bundle.getSerializable("currentValues");
        currentValuesAdapter.update(currentValues);       
    }
    
    
    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);
        
        service   = getServiceRunning(RideService.class);
        startMenu = menu.findItem(R.id.Start);
        stopMenu  = menu.findItem(R.id.Stop);
        if(service == null) {
            startMenu.setVisible(true);
            stopMenu.setVisible(false);
        } else {
            startMenu.setVisible(false);
            stopMenu.setVisible(true);
        }
        return true;
    }
    

    /**
     * start the ride and notify the user of success
     */
    private void startRide() {
        this.startService(rsi);
        
        startMenu.setVisible(false);
        stopMenu.setVisible(true);
        
        bindToService();
        
        Toast toast = Toast.makeText(getApplicationContext(), getString(R.string.starting_ride), Toast.LENGTH_LONG);
        toast.show();
    }
    
    
    /**
     * is a service running or not
     * @param serviceClass
     * @return
     */
    private RunningServiceInfo getServiceRunning(Class<?> serviceClass) {
        ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        for (RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
            if (serviceClass.getName().equals(service.service.getClassName())) {
                return service;
            }
        }
        return null;
    }
}
