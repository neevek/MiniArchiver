package net.neevek.miniarchiver;

import java.io.*;

/**
 * Created with IntelliJ IDEA.
 * User: xiejm
 * Date: 8/9/13
 * Time: 11:50 AM
 */
public class Test {
    public static void main(String[] args) throws Exception {
        // testLocateFile();
       testArchiveAndUnarchive();
    }

    private static void testLocateFile() throws IOException {

        long ts = System.currentTimeMillis();
        MiniArchiver.locateFile("/Users/xiejm/Desktop/html.zip", "version", new OnArchivedFileLocatedListener() {
            @Override
            public void onLocated(String name, int fileLength, boolean isDirectory) {
                System.out.println(">>>>>>>>>>>>>>>>> FOUND FILE: " + name + ", " + fileLength + ", " + isDirectory);
            }

            @Override
            public void onStreamPrepared(InputStream is, int fileLength) {
                byte[] buf = new byte[fileLength];
                int lenRead = 0;
                try {
                    while ((lenRead = is.read(buf, lenRead, fileLength - lenRead)) != -1);
                    System.out.println(">>>>>>>>>>>>>>>>>>> data: " + new String(buf, "utf-8"));
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }

            @Override
            public void onFileNotFound() {
                System.out.println(">>>>>>>>>> file not found");
            }
        });

        System.out.println(">>>>>>>>>>>>>>>>>>>>>> time: " + (System.currentTimeMillis() - ts));
    }

    public static void testArchiveAndUnarchive() {
//        MiniArchiver.archive("/Users/neevek/Desktop/testmini/html", "/Users/neevek/Desktop/testmini/html2.mar", true, false);
        //MiniArchiver.unarchive("/Users/neevek/Desktop/music.ma.gz", "D:\\Workspace\\Code\\9game\\libs\\MiniArchiver\\dist\\html");
        MiniArchiver.archive("D:\\Workspace\\Code\\9game\\libs\\MiniArchiver\\test\\dev\\", "D:\\Workspace\\Code\\9game\\libs\\MiniArchiver\\test\\html2.gz",true, false );
//        MiniArchiver.unarchive("/Users/xiejm/Desktop/html.mar.gz", "/Users/xiejm/Desktop/myhtml");

//        MiniArchiver.archive("/Users/xiejm/Desktop/html", "/Users/xiejm/Desktop/html.mar", true, false);
//        MiniArchiver.unarchive("/Users/xiejm/Desktop/html.mar", "/Users/xiejm/Desktop/aaa");
    }

}
