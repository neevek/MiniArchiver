package net.neevek.miniarchiver;

import java.io.Closeable;
import java.io.DataInput;
import java.io.EOFException;
import java.io.IOException;

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
            return is.readShort();
        } catch (EOFException e) { }

        return -1;
    }
}
