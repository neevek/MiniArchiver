package net.neevek.miniarchiver;

import java.io.*;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;

/**
 * Created with IntelliJ IDEA.
 * User: neevek
 * Date: 8/8/13
 * Time: 9:32 PM
 *
 * This class implements a tool that packs up a directory of files
 * into an archive file, and uses GZIP to compress the archive file,
 * which is actually what the command "tar zcf archive" does.
 *
 * The reason I write this tool is to provide the similar functionality
 * that "tar" provides with no need to include a "tar" library.
 *
 * Note that the file format of the archive that this implementation
 * uses is far simpler than that of the tar's. currently it is used only
 * used in my personal projects.
 */
public class MiniArchiver {
    private final static short MAX_FILE_PATH_LEN = 0xffff >> 1;
    private final static int DIR_MARK_BIT = 1 << 15;
    private final static int VERSION = 1;
    private final static int BYTE_BUF_SIZE = 1024 * 4;


    public static void main(String[] args) {
        archive("/Users/neevek/Desktop/html", "/Users/neevek/Desktop/a.mar", false);
        unarchive("/Users/neevek/Desktop/a.mar", "/Users/neevek/Desktop/a");
    }

    // ************* Archive related code *************

    public static void archive (String rootPath, String outputFile, boolean incCurDir) {
        File rootDir = new File(rootPath);
        if (!rootDir.exists())
            throw new RuntimeException("Directory not found: " + rootPath);
        if (!rootDir.isDirectory())
            throw new RuntimeException("Root path is not a directory: " + rootPath);

        rootPath = rootDir.getAbsolutePath();

        int pathStartIndex = incCurDir ? rootPath.lastIndexOf('/') + 1 : rootPath.length() + 1;

        DataOutputStream dos = null;
        try {
            dos = new DataOutputStream(new GZIPOutputStream(new FileOutputStream(outputFile)));
            dos.writeShort(VERSION);
            archiveInternal(rootDir, dos, pathStartIndex);
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            closeCloseable(dos);
        }
    }

    public static void archiveInternal (File curFile, DataOutputStream dos, int pathStartIndex) throws IOException {
        if (curFile.isDirectory()) {
            // for the rootDir, if "incCurDir==false", we should not archive the root dir
            if (pathStartIndex < curFile.getAbsolutePath().length())
                archiveDir(curFile, dos, pathStartIndex);

            File files[] = curFile.listFiles();
            for (File f : files) {
                if (f.isDirectory()) {
                    archiveInternal(f, dos, pathStartIndex);
                } else {
                    archiveFile(f, dos, pathStartIndex);
                }
            }
        } else {
            archiveFile(curFile, dos, pathStartIndex);
        }
    }

    public static void archiveDir (File curDir, DataOutputStream dos, int pathStartIndex) throws IOException {
        String path = curDir.getAbsolutePath().substring(pathStartIndex);
        byte[] pathBytes = path.getBytes("utf-8");
        checkFilePathLength(pathBytes.length);

        dos.writeShort(DIR_MARK_BIT | (short)pathBytes.length);
        dos.write(pathBytes);
    }

    public static void archiveFile (File curFile, DataOutputStream dos, int pathStartIndex) throws IOException {
        if (!curFile.exists())
            return;

        String path = curFile.getAbsolutePath().substring(pathStartIndex);
        byte[] pathBytes = path.getBytes("utf-8");
        checkFilePathLength(pathBytes.length);

        dos.writeShort((short) pathBytes.length);
        dos.write(pathBytes);
        dos.writeInt((int) curFile.length());
        writeFile(curFile, dos);

    }

    private static void checkFilePathLength(int pathByteLength) {
        if (pathByteLength > MAX_FILE_PATH_LEN) {
            throw new RuntimeException("File path too long: " + pathByteLength);
        }
    }

    public static void writeFile (File curFile, DataOutputStream dos) throws IOException {
        InputStream is = null;
        try {
            is = new FileInputStream(curFile);
            byte buf[] = getThreadSafeByteBuffer();
            int lenRead;
            while ((lenRead = is.read(buf)) != -1) {
                dos.write(buf, 0, lenRead);
            }
        } finally {
            closeCloseable(is);
        }
    }


    // ************* Unarchive related code *************

    public static void unarchive (String archiveFilePath, String outputDirPath) {
        File archiveFile = new File(archiveFilePath);
        if (!archiveFile.exists()) {
            throw new RuntimeException("File not found: " + archiveFilePath);
        }
        if (!archiveFile.isFile()) {
            throw new RuntimeException("Not a file: " + archiveFilePath);
        }

        File outputDir = new File(outputDirPath);
        outputDir.mkdirs();

        DataInputStream dis = null;
        try {
            dis = new DataInputStream(new GZIPInputStream(new FileInputStream(archiveFile)));
            // ignore the version number
            short version = dis.readShort();

            unarchiveInternal(dis, outputDir);
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            closeCloseable(dis);
        }
    }

    public static void unarchiveInternal (DataInputStream dis, File outputDir) throws IOException {
        while (true) {
            int filePathLength = safeReadShort(dis);
            if (filePathLength == -1)
                return; // reach the end of the archive

            if ((filePathLength & DIR_MARK_BIT) > 0) {
                unarchiveDir(dis, outputDir, (short) (filePathLength & MAX_FILE_PATH_LEN));
            } else {
                unarchiveFile(dis, outputDir, (short) filePathLength);
            }
        }
    }

    public static void unarchiveDir (DataInputStream dis, File outputDir, short dirPathLength) throws IOException {
        byte buf[] = new byte[dirPathLength];
        int lenRead = 0;
        while (lenRead < dirPathLength) {
            lenRead += dis.read(buf, lenRead, dirPathLength - lenRead);
        }

        String dirPath = new String(buf, "utf-8");
        new File(outputDir, dirPath).mkdir();
    }

    public static void unarchiveFile (DataInputStream dis, File outputDir, short filePathLength) throws IOException {
        byte buf[];
        if (filePathLength > BYTE_BUF_SIZE)
            buf = new byte[filePathLength];
        else
            buf = getThreadSafeByteBuffer();

        int lenRead = 0;
        while (lenRead < filePathLength) {
            lenRead += dis.read(buf, lenRead, filePathLength - lenRead);
        }

        String filePath = new String(buf, 0, filePathLength, "utf-8");

        int fileLength = dis.readInt();
        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream(new File(outputDir, filePath));
            buf = getThreadSafeByteBuffer();

            int totalRead = 0;
            while (totalRead < fileLength) {
                lenRead = dis.read(buf, 0, Math.min(BYTE_BUF_SIZE, fileLength - totalRead));
                fos.write(buf, 0, lenRead);

                totalRead += lenRead;
            }
        } finally {
            closeCloseable(fos);
        }
    }

    // ************* Utility methods *************

    private final static ThreadLocal<byte[]> threadSafeByteBuf = new ThreadLocal<byte[]>();
    public static byte[] getThreadSafeByteBuffer () {
        byte[] buf = threadSafeByteBuf.get();
        if(buf == null) {
            buf = new byte[BYTE_BUF_SIZE];
            threadSafeByteBuf.set(buf);
        }
        return buf;
    }

    public static void closeCloseable(Closeable obj) {
        try {
            if (obj != null)
                obj.close();
        } catch (IOException e) { }
    }

    private static short safeReadShort (InputStream is) throws IOException {
        int first = is.read();
        int second = is.read();
        if (first == -1 || second == -1)
            return -1;
        return (short)((first << 8) + second);
    }
}
