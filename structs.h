// This file contains some starter code form cs3650, spring 2019 
// This also references Cristos slides from cs 5600

#ifndef STRUCTS
#define STRUCTS

#include <unistd.h>
#include <sys/types.h>


typedef struct inode {
    mode_t mode; //modes
    int flags; //file or directory flag
    off_t size;  // size of the file
    int num_blocks; // blocks allocated to this file
    int blocks_off; // offset for data blocks

} inode;

typedef struct entry {
    int node_off; // node offset
    int name_off; // name of offset.
} ent;

typedef struct directory {
    int ents_off;  // entry list
    int inum;  // number of entries
    int  node_off; // the node offset
} directory;

typedef struct block {    
    int root_node_off; //the root node offset     
    //bitmap things for data blocks and inodes
    int inode_bitmap_size; 
    int inode_bitmap_off; 
    int data_bitmap_off; 
    int data_bitmap_size;
    // data  
    int inode_num;       
    int inodes_off;     
    int data_num;          
    int data_blocks_off;   

} block;

#endif         
