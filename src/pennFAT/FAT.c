#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include "FAT.h"
#include "macro.h"

dir_node * new_directory_node(char* f_name, uint32_t size, uint16_t firstBlock, uint8_t type, uint8_t perm, time_t time) {
    dir_node* res = (dir_node*)malloc(sizeof(directory_entry));
    res->next = NULL;
    directory_entry* res_directory_entry = (directory_entry*)malloc(sizeof(directory_entry));
    res->dir_entry = res_directory_entry;
    res_directory_entry->firstBlock=firstBlock;
    res_directory_entry->mtime=time;
    res_directory_entry->perm=perm;
    res_directory_entry->size=size;

    for (size_t i = 0; i<32;i++) {
        res_directory_entry->name[i] = '\0';
    }
    strcpy(res_directory_entry->name,f_name);

    return res;
}

void free_directory_node(dir_node *node) {
    free(node->dir_entry);
    free(node);
}

FAT* make_fat(char* f_name, uint8_t block_num, uint8_t block_size) {
    if (block_num > 32 || block_num < 1) {
        perror("number of blocks needs to be within 1-32");
        return NULL;
    }
    if (block_size > 4 || block_size < 0) {
        perror("block size needs to be within 0-4");
        return NULL;
    }
    FAT* res = (FAT*) malloc(sizeof(FAT));
    res->file_num = 0;
    res->first_dir_node =NULL;
    res->last_dir_node =NULL;

    int len = strlen(f_name);
    res->f_name = (char*) malloc(len * sizeof(char) + 1);
    strcpy(res->f_name, f_name);
    res->f_name[len] =  '\0';
    res->block_num = block_num;

    if (block_size == 0) {
        res->block_size = 256;
    }
    else if (block_size == 1) {
        res->block_size = 512;
    } else if(block_size == 2) {
        res->block_size = 1024;
    } else if(block_size == 3) {
        res->block_size = 2048;
    } else if(block_size == 4) {
        res->block_size = 4096;
    }

    // # of FAT entries = block size * number of blocks in FAT / 2
    res->entry_num = res->block_size * res->block_num;
    res->entry_num /= 2;
    res->free_blocks = res->entry_num -2;
    int fs_fd = open(f_name, O_RDWR | O_TRUNC | O_CREAT, 0644);
    if(fs_fd == -1) {
        perror("open file");
        return NULL;
    }

    //use ftruncate to make the fatsize == block_num * block size
    if(ftruncate(fs_fd, (res->block_num * res->block_size)) == -1) {
        perror("file truncate");
        return NULL;
    }
    

    res->block_arr = (uint16_t*) mmap(NULL, (res->block_num * res->block_size), 
        PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);
    
    if (res->block_arr == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }

    //first block stored FS information by LSB and MSB
                                    //LSB               MSB
    res->block_arr[0] = (uint16_t) block_num << 8 | block_size;

    //second block is the root directory
    res->block_arr[1] = 0xFFFF;
    return res;
}   

void free_fat(FAT** fat){
    struct FAT *curr_fat = *fat;
    if (curr_fat == NULL)   return;

    while (curr_fat->first_dir_node != NULL) {
        dir_node *curr = curr_fat->first_dir_node;
        curr_fat->first_dir_node = curr->next;
        free_directory_node(curr);
    }

    if (munmap(curr_fat->block_arr, curr_fat->block_num * curr_fat->block_num) == -1) {
        perror("munmap");
        return;
    }

    free(curr_fat);
}
    