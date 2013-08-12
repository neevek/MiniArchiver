MiniArchiver
============

A tool that packs up a directory of files into an archive file, and uses GZIP to compress the archive file, which is actually what the command "tar zcf archive" does.

Archive files that this tool creates store only the raw content of the files. meta information of the files, such as permission, owner, group information are *NOT* stored.
