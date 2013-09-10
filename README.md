MiniArchiver
============

A tool that packs up a directory of files into an archive file, and uses GZIP to compress the archive file, which is actually what the command "tar zcf archive.tar.gz dir" does.

Archive file that this tool creates stores only the raw content of the files. Meta information of the files, such as permission, owner and group information are **NOT** stored, symbolic links are silently **ignored**. Since less information is stored, the final archive tends to be a little bit smaller than that created with `tar`.

Currently MiniArchiver does not offer any means of checking the validity of content stored in the archive.

Installation
============

    cd c
    make && make install

Usage
=====

    usage: ma [options] ...

    optins:
        -c: archive
        -x: unarchive
        -z: use gzip compression when archiving
        -f: archive file(for archiving or unarchiving)
        -C: output directory for unarchiving
        -0: no compression but keep gzip header, this option is applied only when -z is specified.
        -1 to -9: compress faster to compress better, this option is applied only when -z is specified.
        -v: verbose

    examples:
        ma -czf foo.ma.gz dir1 dir2 file1 file2
        ma -xf foo.ma.gz -C outdir
        ma -c dir1 dir2 | gzip -c > foo.ma.gz

    version: v0.0.1, all right reserved @neevek
