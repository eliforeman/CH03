//conceptual inspriation from cs3650 spring2019 and
//cs5600 cristo slides

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "storage.h"
#include "slist.h"
#include "util.h"
#include "structs.h"

//super block 
static block* s_block = 0;

//bit functions

/*
    Go through the bits until a 0 is found.
*/
int
get_bit_index(char* bits, int size)
{
    int count = 0;
    char temp = *(bits);
    for (int i = 0; i < size; i++) {

        if (count >= 8) {
            count = 0;
            temp = *(bits + (i + 1));
        }
        else {
            if (!(((temp >> count) & 1) ^ 0)) {
                return count + (i * 8);
            }
            i--;
            count += 1;
        }
    }
    return -1;
}

void
set_bit(char* bits, int size, int val, int index)
{
    int char_index = index / 8;
    int bit_index = index % 8;
    char temp = *(bits + char_index);
    if (val) {
        // set bit
        temp |= 1 << bit_index;
    }
    else {
        // undo
        temp &= ~(1 << bit_index);
    }
    *(bits + char_index) = temp;
}

/*
    super block pointer given some offset.
*/
void*
sblock_pointer(int offset) {
    return ((void*)s_block) + offset;
}


//main function to init my data

void
storage_init(const char* path)
{
    printf("Store file system data in: %s\n", path);
    int fd = open(path, O_CREAT | O_RDWR, 0644);
    assert(fd != -1);
    int rv = ftruncate(fd, 1024*1024);
    assert(rv == 0);
    //super block things
    s_block = mmap(0, 1024 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(s_block != MAP_FAILED);
    if(s_block->inode_bitmap_size != 4) {
        s_block->inode_bitmap_size = 4;
        s_block->inode_bitmap_off =  sizeof(block);
        s_block->data_bitmap_size = 6;
        s_block->data_bitmap_off = s_block->inode_bitmap_off + s_block->inode_bitmap_size;
        s_block->inode_num = 20;
        s_block->inodes_off = s_block->data_bitmap_off + s_block->data_bitmap_size;
        s_block->data_num = 48;
        s_block->data_blocks_off = s_block->inodes_off + (s_block->inode_num * (sizeof(inode) + (sizeof(int) * 15)));
        s_block->root_node_off = s_block->inodes_off;
	//init the inodes stuff
        inode* i_node = (inode*)sblock_pointer(s_block->root_node_off);
        i_node->mode = 040755;
        i_node->size = 4096;
        i_node->num_blocks = 1;
        i_node->flags = 0;
        i_node->blocks_off = s_block->root_node_off + sizeof(inode);
        int* temp_pointer = (int*)sblock_pointer(i_node->blocks_off);
        *(temp_pointer) = s_block->data_blocks_off;

        directory* dir = (directory*)sblock_pointer(*(temp_pointer));
        dir->node_off = s_block->root_node_off;
        dir->inum = 1;
        dir->ents_off = s_block->data_blocks_off + sizeof(directory);
        ent* root_dirent = (ent*)sblock_pointer(dir->ents_off);
        root_dirent->name_off = dir->ents_off + sizeof(ent);
        char* name = (char*)sblock_pointer(root_dirent->name_off);
        strcpy(name, "/");

        root_dirent->node_off = s_block->root_node_off;

        set_bit((char*)sblock_pointer(s_block->inode_bitmap_off), s_block->inode_bitmap_size, 1, 0);
        set_bit((char*)sblock_pointer(s_block->data_bitmap_off), s_block->data_bitmap_size, 1, 0);
    }
}
//make files
int
make_file(const char *path, mode_t mode) {
    printf("making file: %s, %04o\n", path, mode);
    int index = get_bit_index((char*)sblock_pointer(s_block->inode_bitmap_off), s_block->inode_bitmap_size);
    set_bit((char*)sblock_pointer(s_block->inode_bitmap_off), s_block->inode_bitmap_size, 1, index);

    inode* node = (inode*)(sblock_pointer(s_block->inodes_off) + (index * (sizeof(inode) + (sizeof(int) * 15))));
    node->mode = mode;
    node->size = 0;
    node->num_blocks = 0;
    node->flags = 1; 
    node->blocks_off = sizeof(inode) + (s_block->inodes_off + (index * (sizeof(inode) + (sizeof(int) * 15))));

    int* temp_pointer = (int*)sblock_pointer(((inode*)sblock_pointer(s_block->root_node_off))->blocks_off);

    directory* root = (directory*)sblock_pointer(*(temp_pointer));
    ent* new_ent = (ent*)(sblock_pointer(root->ents_off + (root->inum * (sizeof(ent)+64))));
    new_ent->node_off = s_block->inodes_off + (index * (sizeof(inode) + (sizeof(int) * 15)));
    new_ent->name_off = root->ents_off + ((root->inum + 2) * (sizeof(ent)+64));
    slist* path_name = s_split(path, '/');
    char* name = (char*)sblock_pointer(new_ent->name_off);
    strcpy(name, path_name->next->data);
    root->inum += 1;

    return 0;
}

// point to the file datas entry
ent*
get_file_data(const char* path) {
    //start with a pointer and root
    int* temp_pointer = (int*)sblock_pointer(((inode*)sblock_pointer(s_block->root_node_off))->blocks_off);
    directory* root = (directory*)sblock_pointer(*(temp_pointer));
    if (streq(path, "/")) {
        return (ent*)sblock_pointer(root->ents_off);
    }
    slist* path_list = s_split(path,  '/');

    path_list = path_list->next; 
    char* current = path_list->data;
    directory* temp = root;
    ent* temp_ent = (ent*)sblock_pointer(temp->ents_off + (sizeof(ent)+64));
    
    while (path_list != NULL) {
        for(int i = 0; i < temp->inum - 1; i++) {
            if(streq(current, (char*)sblock_pointer(temp_ent->name_off))) {
                ent* cur_ent = temp_ent;

                if(path_list->next == 0) {
                    return cur_ent;
                }
                else {
                    inode* targetNode = (inode*)sblock_pointer(cur_ent->node_off);

                    // check, based on the flags, if this is a file or directory.
                    if(targetNode->flags == 0) {
                        path_list = path_list->next;
                        temp_pointer = (int*)sblock_pointer(targetNode->blocks_off);
                        temp = (directory*)sblock_pointer(*(temp_pointer));
                        temp_ent = (ent*)sblock_pointer(temp->ents_off);
                        i = 0;
                        break;
                    }
                    else {
                        printf("%s is an invalid path\n", path);
                        return 0;
                    }
                }
            }
            temp_ent = (ent*)((void*)(temp_ent) + (sizeof(ent)+64));
        }
        path_list = path_list->next;
    }

    printf("%s does not exist\n", path);
    return 0;
}

//write
int
storage_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    
    //4k offest 
    int off = offset % 4096;
    int block_num = offset / 4096;

    ent* data = get_file_data(path);
    if (!data || size >= 4096) {
        return -1;
    }

    inode* node = (inode*)sblock_pointer(data->node_off);

    int* temp_pointer = 0;

    // add mem blocks to the node if necessary
    if((offset + size)/4096 >= node->num_blocks) {
        for(int i = 0; i < 1; i++) {
            // get index from bitmap
            int index = get_bit_index((char*)sblock_pointer(s_block->data_bitmap_off), s_block->data_bitmap_size);
            set_bit((char*)sblock_pointer(s_block->data_bitmap_off), s_block->data_bitmap_size, 1, index);

            // set the block offeset in the node
            temp_pointer = ((int*)sblock_pointer(node->blocks_off)) + node->num_blocks;
            *(temp_pointer) = (index * 4096) + s_block->data_blocks_off;
            node->num_blocks += 1;
        }
    }
    temp_pointer = (int*)sblock_pointer(node->blocks_off);
    int* roottemp = (int*)sblock_pointer(s_block->data_blocks_off);

    // get the data from the block we are writting to
    temp_pointer = ((int*)sblock_pointer(node->blocks_off)) + block_num;
    char* file_data =  (char*)sblock_pointer(*(temp_pointer));

    // write the data
    for (int i = 0; i < size; i++) {
        *(file_data + off) = *(buf + i);
        off += 1;
    }
    temp_pointer = (int*)sblock_pointer(node->blocks_off);

    node->size = node->size - offset + size;

    return 0;
}

int
acessHelper(const char* path)
{
    ent* data = get_file_data(path);

    if (!data) {
        printf("file doesnt exist\n");
        return -1;
    }
    else {
        printf("file does exist\n");
        return 0;
    }
}

directory*
root()
{
    return (directory*)sblock_pointer(s_block->data_blocks_off);
}

int
storage_stat(const char* path, struct stat* st)
{
    ent* data = get_file_data(path);
    if (!data) {
        return -1;
    }

    inode* targetNode = (inode*)sblock_pointer(data->node_off);

    memset(st, 0, sizeof(struct stat));
    if (targetNode->flags == 1) {
        st->st_mode = S_IFREG | targetNode->mode;
    }
    if (targetNode->flags == 0) {
        st->st_mode = S_IFDIR | targetNode->mode;
    }
    if (targetNode) {
        st->st_size = targetNode->size;
    }
    else {
        st->st_size = 0;
    }
    return 0;
}


//used in read, get data from blocks
const char*
get_data(const char* path)
{
    ent* data = get_file_data(path);
    if (!data) {
        return 0;
    }

    inode* node = (inode*)sblock_pointer(data->node_off);
    size_t size = node->size;
    int num_blocks = node->num_blocks;
    char* buf = malloc(num_blocks * 4096);
    int* temp_pointer = 0;
    // copy all bytes from all blocks.
    for(int i = 0; i < num_blocks; i++) {
        temp_pointer = ((int*)sblock_pointer(node->blocks_off)) + i;
        char* temp = (char*)sblock_pointer(*(temp_pointer));
        for(int j = 0; j < 4096; j++) {
            *(buf + j + (i * 4096)) = *(temp + j);
        }
    }
    return buf;
}

/*rename*/
int
storage_rename(const char *from_path, const char *to_path)
{
    ent* from_data = get_file_data(from_path);
    char* from_name = (char*)sblock_pointer(from_data->name_off);

    slist* path_name = s_split(to_path, '/');
    strcpy(from_name, path_name->next->data);

    return 0;
}

