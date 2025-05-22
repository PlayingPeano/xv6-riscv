#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#if defined(__linux__)
  #include <endian.h>
#elif defined(__APPLE__)
  #include <libkern/OSByteOrder.h>
  #define le16toh(x) OSSwapLittleToHostInt16(x)
  #define le32toh(x) OSSwapLittleToHostInt32(x)
  #define le64toh(x) OSSwapLittleToHostInt64(x)
#else
  #error "unsupported platform"
#endif

#define UINT32_SIZE sizeof(uint32_t)
#define UINT16_SIZE sizeof(uint16_t)

#define SB_OFFSET 1024
#define SB_SIZE 1024

#define SB_BC_OFFSET 4
#define SB_LOG_BS_OFFSET 24
#define SB_BPG_OFFSET 32
#define SB_IPG_OFFSET 40
#define SB_RL_OFFSET 76
#define SB_IS_OFFSET 88

#define I_SZIE 4
#define I_BLOCK 40
#define I_DIR_ACL 108

struct superblock {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t block_size;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    uint32_t rev_level;
    uint16_t inode_size;
    uint32_t block_groups_count;
};

struct block_descriptor {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t used_dirs_count;
    uint16_t pad;
    uint32_t bg_reserved[3];
};

struct inode {
    uint64_t size;
    uint32_t pointers[15];
};


void read_superblock(FILE* file, struct superblock* sb) {
    uint8_t sb_data[SB_SIZE];
    fseek(file, SB_OFFSET, SEEK_SET);
    fread(sb_data, SB_SIZE, 1, file);

    uint32_t tmp32;

    memcpy(&tmp32, sb_data, UINT32_SIZE);
    sb->inodes_count = le32toh(tmp32);

    memcpy(&tmp32, sb_data + SB_BC_OFFSET, UINT32_SIZE);
    sb->blocks_count = le32toh(tmp32);

    memcpy(&tmp32, sb_data + SB_LOG_BS_OFFSET, UINT32_SIZE);
    sb->block_size = 1024u << le32toh(tmp32);

    memcpy(&tmp32, sb_data + SB_BPG_OFFSET, UINT32_SIZE);
    sb->blocks_per_group = le32toh(tmp32);
    
    memcpy(&tmp32, sb_data + SB_IPG_OFFSET, UINT32_SIZE);
    sb->inodes_per_group = le32toh(tmp32);

    memcpy(&tmp32, sb_data + SB_RL_OFFSET, UINT32_SIZE);
    sb->rev_level = le32toh(tmp32);

    if (sb->rev_level < 1)
        sb->inode_size = 128;
    else {
        uint16_t tmp16;
        memcpy(&tmp16, sb_data + SB_IS_OFFSET, UINT16_SIZE);
        sb->inode_size = le16toh(tmp16);
    }

    sb->block_groups_count = (sb->blocks_count + sb->blocks_per_group - 1) / sb->blocks_per_group;
}

void read_block_descriptor(FILE* file, uint32_t block_group, uint32_t bgdt, struct block_descriptor* bd) {
    fseek(file, bgdt + block_group * sizeof(*bd), SEEK_SET);
    fread(bd, sizeof(*bd), 1, file);

    bd->block_bitmap = le32toh(bd->block_bitmap);
    bd->inode_bitmap = le32toh(bd->inode_bitmap);
    bd->inode_table = le32toh(bd->inode_table);
    bd->free_blocks_count = le16toh(bd->free_blocks_count);
    bd->free_inodes_count = le16toh(bd->free_inodes_count);
    bd->used_dirs_count = le16toh(bd->used_dirs_count);
}

void read_node(FILE* file, uint32_t inode_pa, struct inode* inode) {
    uint32_t lower_size, upper_size;

    fseek(file, inode_pa + I_SZIE, SEEK_SET);
    fread(&lower_size, UINT32_SIZE, 1, file);

    fseek(file, inode_pa + I_DIR_ACL, SEEK_SET);
    fread(&upper_size, UINT32_SIZE, 1, file);

    inode->size = ((uint64_t)le32toh(upper_size) << 32) | le32toh(lower_size);

    fseek(file, inode_pa + I_BLOCK, SEEK_SET);
    fread(inode->pointers, UINT32_SIZE, 15, file);
    for (size_t i = 0; i < 15; ++i) {
        inode->pointers[i] = le32toh(inode->pointers[i]);
    }
}

int main(int argc, char* argv[]) {
    struct superblock sb;
    struct block_descriptor bd;
    struct inode inode;

    FILE* file = fopen(argv[1], "rb");
    uint32_t inode_addr = strtoul(argv[2], NULL, 0) - 1;

    read_superblock(file, &sb);

    uint32_t block_group = inode_addr / sb.inodes_per_group;
    uint32_t bgdt = (sb.block_size == 1024 ? 2 : 1) * sb.block_size;
    read_block_descriptor(file, block_group, bgdt, &bd);

    uint32_t inode_index = inode_addr % sb.inodes_per_group;
    uint32_t inode_block = (inode_index * sb.inode_size) / sb.block_size;
    uint32_t inode_block_offset = (inode_index * sb.inode_size) % sb.block_size;
    uint32_t inode_pa = (bd.inode_table + inode_block) * sb.block_size + inode_block_offset;
    read_node(file, inode_pa, &inode);

    uint8_t* b_data = malloc(sb.block_size);
    size_t rem_size = inode.size;
    for (size_t i = 0; i < 12 && rem_size > 0; ++i) {
        if (inode.pointers[i] == 0)
            continue;
        size_t to_read = sb.block_size > rem_size ? rem_size : sb.block_size;
        fseek(file, (uint64_t)inode.pointers[i] * sb.block_size, SEEK_SET);
        fread(b_data, to_read, 1, file);
        fwrite(b_data, 1, to_read, stdout);
    }

    free(b_data);
    fclose(file);
    exit(0);
}