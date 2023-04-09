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
// #include "file.h"
dir_node * new_directory_node(char* f_name, uint32_t size, uint16_t firstBlock, uint8_t type, uint8_t perm, time_t time) {
    dir_node* res = (dir_node*)malloc(sizeof(dir_node));
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
    // res->first_dir_node =NULL;
    // res->last_dir_node =NULL;

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
    uint32_t fat_size = 0;
    // # of FAT entries = block size * number of blocks in FAT / 2
    res->entry_size = res->block_size * res->block_num;
    fat_size += res->entry_size;
    res->entry_size /= 2;
    res->free_entries = res->entry_size -2;
    res->data_size = (res->free_entries + 1) * (res->block_size);

    fat_size += res->data_size;

    int fs_fd = open(f_name, O_RDWR | O_TRUNC | O_CREAT, 0644);
    if(fs_fd == -1) {
        perror("open file");
        return NULL;
    }

    // use ftruncate to make the fatsize == block_num * block size
    if(ftruncate(fs_fd, (fat_size)) == -1) {
        perror("file truncate");
        return NULL;
    }
    

    res->block_arr = (uint16_t*) mmap(NULL, (fat_size), PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);
    res->directory_starting_index = (uint32_t) res->entry_size;
    res->dblock_starting_index = (uint32_t) (res->directory_starting_index); 
    res->dblock_starting_index += (uint32_t) (res->block_size)/2; 


    // res->block_arr[res->directory_starting_index] = (uint16_t) 's';
    //visualizeation
    // res->block_arr[(res->directory_starting_index)] =  0XDDDD;
    // res->block_arr[(res->dblock_starting_index)] =  0XBBBB;


    //first block stored FS information by LSB and MSB
                                    //LSB               MSB
    res->block_arr[0] = (uint16_t) block_num << 8 | block_size;

    //second block is the root directory
    res->block_arr[1] = ROOT;
    return res;
}   

FAT* mount_fat(char* f_name) {
    printf("here in mount_fat function\n");
    //file system file descriptor
    int fs_fd;
    if ((fs_fd = open(f_name, O_RDONLY, 0644)) == -1) {
        perror("open");
        return NULL;
    }

   //read the first bytes as blocksize
    uint8_t block_size = 0;
    uint32_t actual_block_size= 0;
    if (read(fs_fd, &block_size, sizeof(uint8_t)) == -1) {
        perror("read");
        return NULL;
    }

    // read the next byte as number of blocks
    uint8_t numBlocks = 0;
    if (read(fs_fd, &numBlocks, sizeof(uint8_t)) == -1) {
        perror("read");
        return NULL;
    }
    printf("here before the new content\n");
    
    if (block_size == 0) {
        actual_block_size = 256;
    }
    else if (block_size == 1) {
        actual_block_size = 512;
    } else if(block_size == 2) {
        actual_block_size = 1024;
    } else if(block_size == 3) {
        actual_block_size = 2048;
    } else if(block_size == 4) {
        actual_block_size = 4096;
    }


    // TODO: read direcory entry information:
    uint32_t entry_size = 0;
    // # of FAT entries = block size * number of blocks in FAT / 2
    printf("blocksize : %d\n", actual_block_size);
    printf("numBlocks : %d\n", numBlocks);
    entry_size = actual_block_size * numBlocks;
    int max_filenum = (int)actual_block_size/SIZE_DIRECTORY_ENTRY;
    //get how many entries are in the directory block:
    int count = 0;
    for(int i = 0 ; i < max_filenum; i++) {
        lseek(fs_fd, entry_size + SIZE_DIRECTORY_ENTRY * i, SEEK_SET);
        uint8_t* buf = malloc(sizeof(uint8_t));
        if (read(fs_fd, buf, 1) == -1) {
            perror("read");
            return NULL;
        }
        if((uint8_t)buf[0] == 0){
            free(buf);
            break;
        }
        printf("%d\n", (uint8_t)buf[0]);
        count++;
        free(buf);
    }

    printf("checking %d\n", count);
    directory_entry **entry_arr = malloc(count * sizeof(directory_entry*));
    printf("here before the first for loop for creating entry array\n");

    for(int i = 0 ; i < count; i++) {
        printf("%d\n",i);
        entry_arr[i] = malloc(sizeof(directory_entry));
        lseek(fs_fd, entry_size + SIZE_DIRECTORY_ENTRY * i, SEEK_SET);
        uint8_t buffer[sizeof(directory_entry)];
        for (int j = 0; j < sizeof(directory_entry); j++){
            buffer[j] = 0x00;
        }
        
        if (read(fs_fd, buffer, SIZE_DIRECTORY_ENTRY) == -1) {
            perror("read");
            return NULL;
        }
        memcpy(entry_arr[i], buffer, sizeof(buffer));
    }
    printf("here after the first for loop for creating entry array\n");
    dir_node* head = NULL; 
    dir_node* curr_node = NULL; 
    for (size_t i = 0; i < count; i++) {
        if (entry_arr[i] == NULL) {
            break;
        }
        dir_node* new_node = malloc(sizeof(dir_node));
        new_node->dir_entry = entry_arr[i];
        new_node->next = NULL;
        if (head == NULL) { 
            head = new_node; 
            curr_node = head;
        } else {
            curr_node->next = new_node; 
            curr_node = new_node; 
        }
    }




    // printf("here after the first new read\n");
    // count++;
    // dir_node dummy = {NULL, NULL};
    // dir_node iter = {NULL, NULL};
    // iter.dir_entry = (directory_entry*)malloc(sizeof(directory_entry));
    // dummy.next = &iter;
    // Allocate memory for iter
    // printf("here before while loop\n");
    // printf("the first entry read: %s\n", curr_entry->name);
    // memcpy(iter.dir_entry, curr_entry,sizeof(dir_node*));
    // printf("here after memcpy\n");

    // while(iter.dir_entry != NULL || count==block_size/64) {
    //     printf("here before first memcpy\n");
    //     memcpy(iter.dir_entry, curr_entry,64);
    //     printf("here after first memcpy\n");
    //     curr_entry = NULL;
    //     if (read(fs_fd, &curr_entry, sizeof(directory_entry)) == -1) {
    //         perror("read");
    //         return NULL;
    //     }
    //     iter = *iter.next;
    // }
    // printf("here after the all new read\n");

    // if (close(fs_fd) == -1) {
    //     perror("close");
    //     return NULL;
    // }
    // printf("here in before makefat\n");


    // make this fat table
    FAT *res = make_fat(f_name, numBlocks, block_size);

    if (res == NULL) {
        printf("error: Failed to load FAT\n");
        return NULL;
    }
    res->first_dir_node = head;
    dir_node* curr = res->first_dir_node;
    while(curr != NULL) {
        if(write_directory_to_block(*curr->dir_entry,res)== FAILURE) {
            printf("error: write directory entry to block");
            return res;
        }
        curr = curr->next;
    }
    return res;
}

void free_fat(FAT* fat){
    struct FAT *curr_fat = fat;
    if (curr_fat == NULL)   return;

    while (curr_fat->first_dir_node != NULL) {
        dir_node *curr = curr_fat->first_dir_node;
        curr_fat->first_dir_node = curr->next;
        free_directory_node(curr);
    }

    if (munmap(curr_fat->block_arr, curr_fat->block_size * curr_fat->block_num) == -1) {
        perror("munmap");
        return;
    }

    free(curr_fat);
}

int write_directory_to_block(directory_entry en, FAT* fat) {
    // find a spot in file system
    uint16_t index = 0;
    while(fat->block_arr[fat->directory_starting_index + index] != ZERO) {
        //if the index is non-zero, jump to the next directory block
        //each directory entry is 64 bytes, and each array index is 2 bytes as it is uint16_t type
        // thus increment by 32
        index += 32;
    }
    // return failure if out of directory block bound
    if (fat->directory_starting_index + index >= fat->dblock_starting_index) {
        printf("no directory space anymore\n");
        return FAILURE;
    }
    // writing the directory entry struct into the directory block
    directory_entry* entry_ptr = (directory_entry*) &fat->block_arr[fat->directory_starting_index+index];
    *entry_ptr = en;
    return SUCCESS;
}

int delete_directory_from_block(directory_entry en, FAT* fat) {
    // find a spot in file system
    uint16_t index = 0;
    while(fat->directory_starting_index + index < fat->dblock_starting_index) {
        //if the index is non-zero, jump to the next directory block
        //each directory entry is 64 bytes, and each array index is 2 bytes as it is uint16_t type
        // thus increment by 32
        directory_entry* curr_entry = (directory_entry*) &fat->block_arr[fat->directory_starting_index+index];
        if(strcmp(curr_entry->name, en.name) == 0) {
            for (int i = 0; i<32; i++) {
                fat->block_arr[fat->directory_starting_index + index + i] = ZERO;
            }
            return SUCCESS;
        }
        index += 32;
    }
    printf("file not found\n");
    return FAILURE;
}