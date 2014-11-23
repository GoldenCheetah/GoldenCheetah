package com.ridelogger;

import java.io.IOException;
import java.io.OutputStream;
import java.util.zip.GZIPOutputStream;

public class GzipWriter extends GZIPOutputStream {
    public GzipWriter(OutputStream os) throws IOException {
        super(os);
    }

    public void write(String data) throws IOException {
        write(data.getBytes());
    }
}
