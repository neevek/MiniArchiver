MiniArchiver
============

A tool that packs up a directory of files into an archive file, and uses GZIP to compress the archive file, which is actually what the command "tar zcf archive.tar.gz dir" does.

Archive file that this tool creates stores only the raw content of the files. Meta information of the files, such as permission, owner, group information are **NOT** stored, symbolic links are silently **ignored**. Since less information is stored, the final archive tends to be a little bit smaller than that created with `tar`.

Currently MiniArchiver does not offer any means of checking the validity of content stored in the archive.
