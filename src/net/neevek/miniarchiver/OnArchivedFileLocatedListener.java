package net.neevek.miniarchiver;

import java.io.InputStream;

/**
 * Created with IntelliJ IDEA.
 * User: xiejm
 * Date: 8/9/13
 * Time: 12:57 PM
 */
public interface OnArchivedFileLocatedListener {
    void onLocated (String name, int fileLength, boolean isDirectory);
    void onStreamPrepared(InputStream is, int fileLength);
    void onFileNotFound ();
}
