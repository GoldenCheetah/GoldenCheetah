package com.ridelogger;

import java.io.IOException;
import java.io.OutputStream;
import java.util.zip.GZIPOutputStream;

public class GzipWriter extends GZIPOutputStream {
    public GzipWriter(OutputStream os) throws IOException {
        super(os);
    }

    public void write(String data) throws IOException {
        super.write(data.getBytes());
    }
    
    public void write(float data) throws IOException {
        super.write(Float.floatToIntBits(data));
    }

    public void write(CharSequence data) throws IOException {
        byte[] barr = new byte[data.length()];
        for (int i = 0; i < barr.length; i++) {
            barr[i] = (byte) data.charAt(i);
        }
        
        super.write(barr);
    }
}
