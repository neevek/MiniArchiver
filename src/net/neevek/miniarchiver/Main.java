package net.neevek.miniarchiver;

public class Main {

	/**
	 * @param args 
	 */
	public static void main(String[] args) {
		String cmd = args[0];
		if(args.length < 3){
			System.out.println("Usage: ");
			System.out.println("\t archive <rootPath> <outputFile> [compress=true] [incCurDir=false]");
			System.out.println("\t unarchive <archiveFilePath> <outputDirPath>");
		}else if("archive".equalsIgnoreCase(cmd)){
			String rootPath = args[1];
			String outputFile = args[2];
			Boolean compress = args.length>=4 ? new Boolean(args[3]) : true; 
			Boolean incCurDir = args.length>=5 ? new Boolean(args[4]) : false;
			System.out.println(String.format("archive %s to %s with compress=%s, incCurDir=%s", rootPath, outputFile, compress, incCurDir));
			MiniArchiver.archive(rootPath, outputFile, compress, incCurDir);
			System.out.println("archive success");
		}else if("unarchive".equalsIgnoreCase(cmd)){
			String archiveFilePath = args[1]; 
			String outputDirPath = args[2];
			System.out.println(String.format("unarchive %s to %s ", archiveFilePath, outputDirPath));
			MiniArchiver.unarchive(archiveFilePath, outputDirPath);
			System.out.println("unarchive success");
		}else{
			System.err.println("Unknow cmd type: " + cmd);
		}
	}

}
