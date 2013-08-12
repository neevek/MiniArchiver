package net.neevek.miniarchiver;

import java.io.*;

/**
 * Created with IntelliJ IDEA.
 * User: xiejm
 * Date: 8/9/13
 * Time: 9:42 AM
 */
public class MiniArchiverUtil {

    public static void closeCloseable(Closeable obj) {
        try {
            if (obj != null)
                obj.close();
        } catch (IOException e) { }
    }

    public static short safeReadShort (DataInput is) throws IOException {
        try {
            return readLittleEndianShort(is);
        } catch (EOFException e) { }

        return -1;
    }

    public static void writeLittleEndianShort (short n, DataOutput dos) throws IOException {
        n = (short)((n >> 8) & 0xff | (n << 8) & 0xff00);
        dos.writeShort(n);
    }

    public static void writeLittleEndianInt (int n, DataOutput dos) throws IOException {
        n = ((n >>> 24) & 0xff) | ((n >>> 8) & 0xff00) | ((n << 8) & 0xff0000) | ((n << 24) & 0xff000000);
        dos.writeInt(n);
    }

    public static short readLittleEndianShort (DataInput dis) throws IOException {
        short n = dis.readShort();
        n = (short)((n >> 8) & 0xff | (n << 8) & 0xff00);
        return n;
    }

    public static int readLittleEndianInt (DataInput dis) throws IOException {
        int n = dis.readInt();
        n = ((n >>> 24) & 0xff) | ((n >>> 8) & 0xff00) | ((n << 8) & 0xff0000) | ((n << 24) & 0xff000000);
        return n;
    }
}
