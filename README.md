# RAMDISK-in-memory-filesystem
Implemented a in-memory file system using the fuse library. Implemented own way of maintaining files and folders in the file system and provided features to create, append, list, delete files and folders.
Capable of following file system calls like:
open, read, write, close (read only, write only, read and write, append modes supported)
opendir, readdir and closedir
unlink, rmdir ,create, truncate etc.
