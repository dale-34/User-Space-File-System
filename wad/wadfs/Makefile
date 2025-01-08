daemon: daemon.cpp 
	g++ -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 daemon.cpp -o wadfs -lfuse -L ../libWad -lWad -w