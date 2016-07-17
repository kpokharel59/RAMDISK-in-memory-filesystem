all:
	gcc -Wall rmfs.c `pkg-config fuse --cflags --libs` -o ramdisk
