#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "ext2_fs.h"

#define SUPERBLOCK_OFFSET   1024

int fd;
struct ext2_super_block sb;
struct ext2_group_desc* gd;
struct ext2_inode in;

unsigned int blocks_count, inodes_count, log_block_size, blocks_per_group, inodes_per_group, groups_count;                                                  //CHANGED

void superblockSum(){
    ssize_t bytesRead;
    bytesRead = pread(fd, &sb, sizeof(struct ext2_super_block), SUPERBLOCK_OFFSET);
    if (bytesRead == -1){
        fprintf(stderr, "ERROR: pread() failed trying to read superblock\n");
        exit(1);
    }
    
    blocks_count = sb.s_blocks_count;
    inodes_count = sb.s_inodes_count;
    log_block_size = EXT2_MIN_BLOCK_SIZE << sb.s_log_block_size;    //CHANGED
    blocks_per_group = sb.s_blocks_per_group;
    inodes_per_group = sb.s_inodes_per_group;
    
    fprintf(stdout, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",
            blocks_count, inodes_count, log_block_size, sb.s_inode_size, blocks_per_group, inodes_per_group, sb.s_first_ino);                   //CHANGED
}

void blockSum(){
    unsigned int n;                                                 //CHANGED
    groups_count = 1 + (blocks_count / blocks_per_group);
    
    gd = malloc(sizeof(struct ext2_group_desc) * groups_count);
    if (gd == NULL){
        fprintf(stderr, "ERROR: malloc() failed trying to allocate group descriptors\n");
        exit(1);
    }
    
    ssize_t bytesRead;
    bytesRead = pread(fd, gd, sizeof(struct ext2_group_desc) * groups_count, SUPERBLOCK_OFFSET + log_block_size);
    if (bytesRead == -1){
        fprintf(stderr, "ERROR: pread() failed trying to read group\n");
        exit(1);
    }
    
    for (n = 0; n < groups_count; n++){
        fprintf(stdout, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n",
                n, blocks_count, inodes_count, gd[n].bg_free_blocks_count, gd[n].bg_free_inodes_count, gd[n].bg_block_bitmap, gd[n].bg_inode_bitmap, gd[n].bg_inode_table);
    }
    
}

void directorySum(struct ext2_inode in, int inNum){
    struct ext2_dir_entry *dir = malloc(sizeof(struct ext2_dir_entry));
    if (dir == NULL){
        fprintf(stderr, "ERROR: malloc() failed trying to allocate for directory summary\n");
        exit(1);
    }
    
    int inital, n;
    unsigned int place;
    for (n = 0; n < EXT2_NDIR_BLOCKS; n++){
        inital = in.i_block[n] * log_block_size;
        place = 0;
        
        while (1) {
            ssize_t bytesRead;
            bytesRead = pread(fd, dir, sizeof(struct ext2_dir_entry), inital + place);
            if (bytesRead == -1){
                fprintf(stderr, "ERROR: pread() failed trying to read directory\n");
                exit(1);
            }
            
            if (dir->inode != 0){
                fprintf(stdout, "DIRENT,%d,%d,%d,%d,%d,'%s'\n",
                        inNum, place, dir->inode, dir->rec_len, dir->name_len, dir->name);
            } else{
                break;
            }
            place += dir->rec_len;
            if (place >= log_block_size){
                break;
            }
            struct ext2_dir_entry *temp = dir + dir->rec_len;
            dir = temp;
        }
    }
}

void indirectSum(int inNum, int level, int bNum, int offset){
    int indirectNum = 0, logicOff = 0;
    int block[log_block_size];
    int place = (bNum - 1) * log_block_size;
    int blockDivNum = log_block_size / sizeof(int);
    int blockDivNumSquared = blockDivNum * blockDivNum;
    
    unsigned int n;
    for (n = 0; n < log_block_size; n++){
        block[n] = 0;
    }
    
    ssize_t bytesRead;
    bytesRead = pread(fd, block, log_block_size, SUPERBLOCK_OFFSET + place);
    if (bytesRead == -1){
        fprintf(stderr, "ERROR: pread() failed trying to read indirect blocks\n");
        exit(1);
    }
    
    for (n = 0; n < (unsigned int)blockDivNum; n++){
        if (block[n] != 0){
            switch(level){
                case 1:
                indirectNum = bNum;
                logicOff = offset + n;
                break;
                case 2:
                indirectNum = bNum + n;
                logicOff = offset + blockDivNum + EXT2_IND_BLOCK;
                break;
                case 3:
                indirectNum = bNum + n;
                logicOff = offset + blockDivNum + EXT2_IND_BLOCK + blockDivNumSquared;
                break;
            }
            
            fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                    inNum, level, logicOff, indirectNum, block[n]);
            
            if (level == 2){
                indirectSum( inNum, level - 1, block[n], offset + EXT2_IND_BLOCK + n + blockDivNum);
            } else if (level == 3){
                indirectSum( inNum,  level - 1, block[n], n + blockDivNumSquared);
            }
        }
    }
}


void calculateTime(time_t inputTime, char* returnString){      //CHANGED A LOT
    char* stringTime = ctime(&inputTime);
    
    char secondMonth = '0';
    char firstMonth = '1';
    
    if (strncmp(stringTime + 4, "Feb", 3) == 0){
        firstMonth = '2';
    }else if (strncmp(stringTime + 4, "Mar", 3) == 0){
        firstMonth = '3';
    }else if (strncmp(stringTime + 4, "Apr", 3) == 0){
        firstMonth = '4';
    }else if (strncmp(stringTime + 4, "May", 3) == 0){
        firstMonth = '5';
    }else if (strncmp(stringTime + 4, "Jun", 3) == 0){
        firstMonth = '6';
    }else if (strncmp(stringTime + 4, "Jul", 3) == 0){
        firstMonth = '7';
    }else if (strncmp(stringTime + 4, "Aug", 3) == 0){
        firstMonth = '8';
    }else if (strncmp(stringTime + 4, "Sep", 3) == 0){
        firstMonth = '9';
    }else if (strncmp(stringTime + 4, "Oct", 3) == 0){
        secondMonth = '1';
        firstMonth = '0';
    }else if (strncmp(stringTime + 4, "Nov", 3) == 0){
        secondMonth = '1';
        firstMonth = '1';
    }else if (strncmp(stringTime + 4, "Dec", 3) == 0){
        secondMonth = '1';
        firstMonth = '2';
    }
    
    returnString[0] = secondMonth;
    returnString[1] = firstMonth;
    
    returnString[2] = returnString[5] = '/';
    
    returnString[4] = stringTime[9];
    returnString[6] = stringTime[22];
    returnString[7] = stringTime[23];
    
    if (stringTime[8] == ' '){
        returnString[3] = '0';
        returnString[8] = stringTime[8];
    }else{
        returnString[3] = stringTime[8];
        returnString[8] = ' ';
    }
    
    if (strncpy(&returnString[9], &stringTime[11], 8) < 0){
        fprintf(stderr, "ERROR: Requires 1 argument\n");
        exit(1);
    }
    returnString[17] = '\0';
}

void freeBlockSum(){
    unsigned int n;
    for(n = 0; n < groups_count; n++) {
        int size_bm = (gd[n].bg_inode_bitmap - gd[n].bg_block_bitmap)*log_block_size;
        char bm[size_bm];
        
        pread(fd, bm, size_bm, gd[n].bg_block_bitmap * log_block_size);
        char eight_bits;
        
        unsigned int block = 1 + (n * sb.s_blocks_per_group);
        unsigned int i;
        for (i = 0; i < (blocks_count>>3); i++){
            eight_bits = bm[i];
            int j;
            for (j = 0; j < 8; j++){
                if (!(eight_bits & 1)){
                    fprintf(stdout, "BFREE,%u\n", block);
                }
                eight_bits = eight_bits >> 1;
                block++;
            }
        }
    }
}


void freeInodeSum(){
    char eight_bits;
    unsigned int n;
    for(n = 0; n < groups_count; n++) {
        int ibm_size = (gd[n].bg_inode_table - gd[n].bg_inode_bitmap)*log_block_size;
        char ibm[ibm_size];
        
        pread(fd, ibm, ibm_size, gd[n].bg_inode_bitmap*log_block_size);
        
        unsigned int inode_i = 1 + n * inodes_per_group; //CHANGED
        unsigned int i;
        int j;
        for (i = 0; i < inodes_count>>3; i++){
            eight_bits = ibm[i];
            for(j = 0; j < 8; j++){
                if (!(eight_bits & 1)){
                    fprintf(stdout, "IFREE,%u\n", inode_i);
                }
                eight_bits = eight_bits >> 1;
                inode_i++;
            }
        }
        
        char atime[18];
        char ctime[18];
        char mtime[18];
        __u16 mode;
        char file_type;
        
        const time_t seven_hours = 25200;
        unsigned int inode_n = 1;
        __u16 file_type_code;
        
        for (i = 0; i < (inodes_count >> 3); i++){
            eight_bits = ibm[i];
            for(j = 0; j < 8; j++, inode_n++){
                if ((eight_bits & 1)){
                    pread(fd, &in, sizeof(in), (gd[n].bg_inode_table*log_block_size)+(inode_n-1)*sizeof(in));
                    if (in.i_mode != 0){
                        file_type_code = in.i_mode & 0xF000;
                        mode = in.i_mode & 0x0FFF;
                        
                        switch (file_type_code){
                            case 0x4000:
                                file_type = 'd';
                                break;
                            case 0x8000:
                                file_type = 'f';
                                break;
                            case 0xA000:
                                file_type = 's';
                                break;
                            default:
                                file_type = '?';
                        }
                        time_t aInput = in.i_atime+seven_hours;
                        calculateTime(aInput, atime);
                        time_t cInput = in.i_ctime+seven_hours;
                        calculateTime(cInput, ctime);
                        time_t mInput = in.i_mtime+seven_hours;
                        calculateTime(mInput, mtime);
                        
                        if (file_type != 's'){
                            fprintf(stdout, "INODE,%u,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                                    inode_n, file_type, mode, in.i_uid, in.i_gid,
                                    in.i_links_count, ctime, mtime, atime,
                                    in.i_size, in.i_blocks,
                                    in.i_block[0], in.i_block[1], in.i_block[2],
                                    in.i_block[3], in.i_block[4], in.i_block[5],
                                    in.i_block[6], in.i_block[7], in.i_block[8],
                                    in.i_block[9], in.i_block[10], in.i_block[11],
                                    in.i_block[12], in.i_block[13], in.i_block[14]);
                        }else{
                            fprintf(stdout, "INODE,%u,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u,%u\n",
                                    inode_n, file_type, mode, in.i_uid, in.i_gid,
                                    in.i_links_count, ctime, mtime, atime,
                                    in.i_size, in.i_blocks, in.i_block[0]);
                        }
                        if (file_type == 'd'){
                            directorySum(in, inode_n);
                        }
                        
                        if (in.i_block[EXT2_IND_BLOCK] != 0){
                            indirectSum(inode_n, 1, in.i_block[EXT2_IND_BLOCK], EXT2_IND_BLOCK);
                        }
                        if (in.i_block[EXT2_DIND_BLOCK] != 0){
                            indirectSum(inode_n, 2, in.i_block[EXT2_DIND_BLOCK], 0);
                        }
                        if (in.i_block[EXT2_TIND_BLOCK] != 0){
                            indirectSum(inode_n, 3, in.i_block[EXT2_TIND_BLOCK], 0);
                        }
                        
                    }
                }
            }
        }
    }
}


int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "ERROR: Requires 1 argument\n");
        exit(1);
    }
    const char *name = argv[1];
    fd = open(name, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR: open() failed\n");
        exit(1);
    }
    
    superblockSum();
    blockSum();
    freeBlockSum();
    freeInodeSum();
    
    exit(0);
}

