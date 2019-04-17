// This file contains some starter code form cs3650, spring 2019 
// This also references Cristos slides from cs 5600

#ifndef STRUCTS
#define STRUCTS

#include <unistd.h>
#include <sys/types.h>

//inodes -> file or directory
typedef struct inode {
    mode_t mode; //modes
    int flags; //file or directory flag
    off_t size;  // size of the file
    int num_blocks; // blocks allocated to this file
    int blocks_off; // offset for data blocks
    int link_count; //how many hard links point to this file

} inode;

//entry in a directory
typedef struct entry {
    int node_off; // node
    int name_off; // name
} ent;

//directories contain a list of entries, # of entries, and the directory offset.
typedef struct directory {
    int ents_off;  // entry list
    int inum;  // number of entries
    int  node_off; // the node offset
} directory;

typedef struct block {    
    int root_node_off; //the root node offset     
    //bitmap things for free data blocks and free inodes
    int inode_bitmap_size; 
    int inode_bitmap_off; 
    int data_bitmap_off; 
    int data_bitmap_size;
    // data (the idea of ext is to use bitmaps to manipulate this) 
    int inode_num;       
    int inodes_off;     
    int data_num;          
    int data_blocks_off;   

} block;

#endif         
