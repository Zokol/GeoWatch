package com.example.geowatch2;

import java.io.IOException;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.util.UUID;

import com.example.geowatch2.R;

import android.os.Bundle;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.EditText;

public class MainActivity extends Activity {

	public final static String EXTRA_MESSAGE = "com.example.myfirstapp.MESSAGE";
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        init_bt();
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }
    
    /** Called when user clicks Send-button **/
    public void sendMessage(View view){
    	// Do something with the message
    	//Intent intent = new Intent(this, DisplayMessageActivity.class);
    	EditText editText = (EditText) findViewById(R.id.edit_message);
    	String message = editText.getText().toString();
    	//String text = "s;60;-22";
		try {
			send_data(message.getBytes("UTF-8"));
		} catch (UnsupportedEncodingException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    	//intent.putExtra(EXTRA_MESSAGE, message);
    	//startActivity(intent);
    }
    
    BluetoothAdapter bluetooth = null;
	private BluetoothSocket socket = null;
	final UUID SERIAL_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"); //UUID for serial connection
	private final static int REQUEST_ENABLE_BT = 1;
	BluetoothDevice device = null;
	
	public void init_bt(){
		bluetooth = BluetoothAdapter.getDefaultAdapter();
		if (bluetooth == null) {
		    // Device does not support Bluetooth
		    finish(); //exit
		} else {
			if (!bluetooth.isEnabled()) {
				//make sure the device's bluetooth is enabled
				Intent enableBluetooth = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
				startActivityForResult(enableBluetooth, REQUEST_ENABLE_BT);
			}
			else{
				//bluetooth is enabled and usable, get remote device
				device = bluetooth.getRemoteDevice("07:12:05:17:60:02"); //get remote device by mac, we assume these two devices are already paired
			}
		}
	}
	
	public void send_data(byte[] data){
		OutputStream out = null;
		try {
		    socket = device.createInsecureRfcommSocketToServiceRecord(SERIAL_UUID);
		} catch (IOException e) {}

		try {           
			Log.i("MainActivity", "GeoWatch2.socket created: " + socket);
		    socket.connect();
		    Log.i("MainActivity", "GeoWatch2.socket connected: " + socket);
		    out = socket.getOutputStream();
		    Log.i("MainActivity", "GeoWatch2.out created: " + out);
		    //now you can use out to send output via out.write
		    out.write(data);
		} catch (IOException e) {}
	}
}
