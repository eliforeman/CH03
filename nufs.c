//based on cs3650 starter code

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <bsd/string.h>
#include <assert.h>
#include <stdlib.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "storage.h"
#include "structs.h"

// implementation for: man 2 access
// Checks if a file exists.
int
nufs_access(const char *path, int mask)
{
    int rv = 0;
    printf("access(%s, %04o)\n", path, mask);
    if (acessHelper(path) == -1) {
      return -ENOENT;
    }
    else {
      rv;
    }
}

// implementation for: man 2 stat
// gets an object's attributes (type, permissions, size, etc)
int
nufs_getattr(const char *path, struct stat *st)
{
    printf("getattr(%s)\n", path);
    int rv = storage_stat(path, st);
    if (rv == -1) {
        return -ENOENT;
    }
    else {
        return 0;
    }
}

// implementation for: man 2 readdir
// lists the contents of a directory
int
nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi)
{
    printf("readdir(%s)\n", path);
    struct stat st;
    storage_stat(path, &st);
    filler(buf, ".", &st, 0);
    // root
    directory* dir = root();
    if(dir->inum > 1) {
	//entry pointer to current entry
        ent* cur_ent = (ent*)(sblock_pointer(dir->ents_off + (64 + sizeof(ent))));
        for (int i = 0; i < dir->inum - 1; i++) {
            cur_ent = (ent*)(sblock_pointer(dir->ents_off + ((i + 1) * (64 + sizeof(ent)))));
            struct stat temp;
            char* full_path = malloc(256); 
            //two char pointer together
	    strcpy(full_path, path);
            strcat(full_path, (char*)sblock_pointer(cur_ent->name_off));

            storage_stat(full_path, &temp);
            filler(buf, (char*)sblock_pointer(cur_ent->name_off), &temp, 0);
            free(full_path);
        }
    }

    return 0;
}


// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
int
nufs_mknod(const char *path, mode_t mode, int flag)
{

    printf("mknod(%s, %04o)\n", path, mode);
   /* if(flag == 1){
    printf("make dir!");
    int rv = make_dir(path, mode);
    	if (rv == -1) {
            return -ENOENT;
    	}
    	else {
            return 0;
    	}
    }
    else{
    printf("make file!");
    int rv = make_file(path,mode);
        if(rv == -1){
		return -ENOENT;
	}
	else{
		return 0;
	}
    }*/
    int rv = make_file(path,mode);
    if(rv == -1){
	    return -ENOENT;
    }
    else{
	    return 0;
    }
}
//rename!!
int
nufs_rename(const char *from, const char *to)
{
    printf("rename(%s => %s)\n", from, to);
    int rv = storage_rename(from, to);
    if (rv == -1) {
        return -ENOENT;
    }
    else {
        return 0;
    }
}

int
nufs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("read(%s, %ld bytes, @%ld)\n", path, size, offset);
    const char* data = get_data(path);

    int len = strlen(data) + 1;
    if (size < len) {
        len = size;
    }

    strncpy(buf, data, len);
    return len;
}

int
nufs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("write(%s, buf: %s, %ld bytes, @%ld)\n", path, buf, size, offset);
    int rv = storage_write(path, buf, size, offset, fi);
    return (int)size;
}
// remove a directory from the block
/*int
nufs_rmdir(const char *path){
	int rv = -1;
	return rv;
}
*/

// most of the following callbacks implement
// another system call; see section 2 of the manual
int
nufs_mkdir(const char *path, mode_t mode)
{
    int rv = nufs_mknod(path, mode | 040000, 1);
    printf("mkdir(%s) -> %d\n", path, rv);
    return rv;
}


int
nufs_link(const char *from, const char *to)
{
    int rv = -1;
    printf("link(%s => %s) -> %d\n", from, to, rv);
    return rv;
}


void
nufs_init_ops(struct fuse_operations* ops)
{
    memset(ops, 0, sizeof(struct fuse_operations));
    ops->access   = nufs_access;
    ops->getattr  = nufs_getattr;
    ops->readdir  = nufs_readdir;
    ops->mknod    = nufs_mknod;
    ops->rename   = nufs_rename;
    ops->read     = nufs_read;
    ops->write    = nufs_write;
    //new stuff
    ops->mkdir = nufs_mkdir;
    //ops->rmdir = nufs_rmdir;
    ops->link = nufs_link;
   // ops->unlink = nufs_unlink;
};

struct fuse_operations nufs_ops;

int
main(int argc, char *argv[])
{
    assert(argc > 2 && argc < 6);
    storage_init(argv[--argc]);
    nufs_init_ops(&nufs_ops);
    return fuse_main(argc, argv, &nufs_ops, NULL);
}
