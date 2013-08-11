package net.neevek.miniarchiver;

import java.io.IOException;
import java.io.InputStream;

/**
 * Created with IntelliJ IDEA.
 * User: xiejm
 * Date: 8/9/13
 * Time: 10:59 AM
 */
public class MiniArchiveEntryInputStream extends InputStream {
    private InputStream is;
    private int limitLength;

    public MiniArchiveEntryInputStream (InputStream is, int limitLength) {
        this.is = is;
        this.limitLength = limitLength;
    }

    public int read() throws IOException {
        if (limitLength > 0) {
            int b = is.read();
            --limitLength;
            return b;
        }
        return -1;
    }

    @Override
    public int read(byte[] buffer) throws IOException {
        return read(buffer, 0, buffer.length);
    }

    @Override
    public long skip(long byteCount) throws IOException {
        return 0;
    }

    @Override
    public int read(byte[] buffer, int offset, int length) throws IOException {
        if (limitLength == 0)
            return -1;

        int lenRead = is.read(buffer, offset, Math.min(limitLength, length));
        limitLength -= lenRead;
        return lenRead;
    }

    @Override
    public int available() throws IOException {
        return limitLength;
    }
}
