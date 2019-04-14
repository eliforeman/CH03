//based on cs3650 starter code

#ifndef NUFS_STORAGE_H
#define NUFS_STORAGE_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "slist.h"
#include "structs.h"

//helper functions + main functions for storage.c / nufs.c
void storage_init(const char* path);
int storage_stat(const char* path, struct stat* st);
void* sblock_pointer(int offset);
int acessHelper(const char* path);
const char* get_data(const char* path);
int make_file(const char *path, mode_t mode);
int storage_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
ent* get_file_data(const char* path);
directory* root();
int storage_rename(const char *from_path, const char *to_path);

#endif
