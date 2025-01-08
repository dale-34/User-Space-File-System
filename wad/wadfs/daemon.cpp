#include <iostream>
#include <unistd.h>
#include <fuse.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include "../libWad/Wad.h"

#define FUSE_USE_VERSION 30
using namespace std;

Wad* myWad;

static int getattr_callback(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    Wad *myWad = ((Wad*)(fuse_get_context()->private_data));
    if(myWad->isDirectory(path)) {
        stbuf->st_mode = S_IFDIR | 0777;
        stbuf->st_nlink = 2;
        return 0;
    }
    if(myWad->isContent(path)) {
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = myWad->getSize(path);
        return 0;
    }
    return -ENOENT;
}

static int mknod_callback(const char *path, mode_t mode, dev_t rdev) {
	Wad *myWad = ((Wad*)(fuse_get_context()->private_data));
    myWad->createFile(path);
	return 0;
}

static int mkdir_callback(const char *path, mode_t mode){
	Wad *myWad = ((Wad*)(fuse_get_context()->private_data));
    myWad->createDirectory(path);
	return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {
    Wad *myWad = ((Wad*)(fuse_get_context()->private_data));
    return myWad->getContents(path, buf, size, offset);
}


static int write_callback( const char *path, const char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {
    Wad *myWad = ((Wad*)(fuse_get_context()->private_data));
    return myWad->writeToFile(path, buf, size, offset);
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi) {
    
    (void) offset;
    (void) fi;
    filler(buf, ".", NULL, 0); 
    filler(buf, "..", NULL, 0); 

    Wad *myWad = ((Wad*)(fuse_get_context()->private_data));
    vector<string> contents;
    myWad->getDirectory(path, &contents);

    for (const auto &entry : contents) {
        filler(buf, entry.c_str(), NULL, 0); 
    }

    return 0;   
}


static struct fuse_operations operations = {
    .getattr = getattr_callback,
    .mknod = mknod_callback,
    .mkdir = mkdir_callback, 
    .read = read_callback,
    .write = write_callback,
    .readdir = readdir_callback,
};

int main (int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Not enough arguements" << endl;
        exit(EXIT_SUCCESS);
    }

    string wadPath = argv[argc-2];

    if (wadPath.at(0) != '/') {
        wadPath = string(get_current_dir_name()) + "/" + wadPath;
    }

    myWad = Wad::loadWad(wadPath); 
    argv[argc - 2] = argv[argc - 1];
    argc--;

    return fuse_main(argc, argv, &operations, myWad);
}