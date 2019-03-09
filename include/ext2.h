#ifndef _EXT2_H
#define _EXT2_H

#include <types.h>

#define EXT2_SUPPERBLOCK_SIGNATURE		0xEF53
#define ROOT_DIRECTORY_INODE_NUMBER		2


#define BLOCKNUM_BLOCK_BITMAP		1
#define BLOCKNUM_INODE_BITMAP		2
#define BLOCKNUM_INODE_TABLE		3


#define INODE_TYPE_FIFO			0x1000 	
#define INODE_CHAR_DEVICE		0x2000
#define INODE_DIRECTORY			0x4000
#define INODE_BLOCK_DEVICE		0x6000
#define INODE_REGILAR_FILE		0x8000
#define INODE_SYMB_LINK			0xA000
#define INODE_UNIX_SOCKET		0xC000 

#define EXT2_FILE_UNKNOWN_TYPE		0
#define EXT2_FILE_REGULAR_FILE		1
#define EXT2_FILE_DIRECTORY		2
#define EXT2_FILE_CHAR_DEVICE		3
#define EXT2_FILE_BLOCK_DEVICE		4
#define EXT2_FILE_FIFO			5
#define EXT2_FILE_SOCKET		6
#define EXT2_FILE_SYMBOLIC_LINK		7


#define DIR_FILE_OFFSET sizeof(u32_t) + sizeof(u16_t) + sizeof(u8_t) + sizeof(u8_t)

typedef struct _Ext2_SupperBlock
{
	u32_t total_inodes;
	u32_t total_blocks;
	u32_t supperblock_reserved_blocks;
	u32_t total_unalloc_blocks;
	u32_t total_unalloc_inodes;
	u32_t supperblock_block_number;
	u32_t block_size;
	u32_t fragment_size;
	u32_t num_of_blocks_per_blockgroup;
	u32_t num_of_fragment_per_blockgroup;
	u32_t num_of_inodes_per_blockgroup;
	u32_t last_mount_time;
	u32_t last_written_time;
	u16_t num_of_mounted_times;
	u16_t num_of_mounted_allowed;
	u16_t ext2_signature;
	u16_t fs_state;
	u16_t action_in_error;
	u16_t minor_version;
	u32_t last_consistency_check;
	u32_t interval_between_forced_consistency;
	u32_t os_id;
	u32_t major_version;
	u16_t user_id;
	u16_t group_id;

	u32_t first_non_reserved_inode;
	u16_t size_of_inode_structure;	
	u16_t blockgroup_of_supperblock;
	u32_t optional_features;
	u32_t required_features;
	u32_t features_not_supported;
	u8_t file_system_id[16];
	char volume_name[16];
	char path_volume_last_mounted[64];
	u32_t compression_algorithm;
	u8_t number_of_blocks_to_preallocate_for_files;
	u8_t number_of_blocks_to_preallocate_for_directories;
	u16_t unused;
	u8_t journal_id[16];
	u32_t journal_inode;
	u32_t journal_device;
	u32_t head_of_orphan_inode_list;
	u8_t _unused[788];
} __attribute__((packed)) Ext2_SupperBlock;


typedef struct _Block_Group_Descriptor
{
	u32_t block_bitmap;
	u32_t inode_bitmap;
	u32_t start_block_inodeTable;
	u16_t free_blocks;
	u16_t free_inodes;
	u16_t dir_count;
	u8_t  unused[14];
} __attribute__((packed)) Block_Group_Descriptor;


typedef struct _Inode
{
	u16_t permission;
	u16_t user_id;
	u32_t size_lo;
	u32_t last_access_time;
	u32_t creation_time;
	u32_t last_mod_time;
	u32_t delete_time;
	u16_t group_id;
	u16_t hard_link_count;
	u32_t sectors_count;
	u32_t flags;
	u32_t os_val;

	u32_t direct_block[12];
/*	u32_t direct_block_0;
	u32_t direct_block_1;
	u32_t direct_block_2;
	u32_t direct_block_3;
	u32_t direct_block_4;
	u32_t direct_block_5;
	u32_t direct_block_6;
	u32_t direct_block_7;
	u32_t direct_block_8;
	u32_t direct_block_9;
	u32_t direct_block_10;
	u32_t direct_block_11;
*/
	u32_t singly_indirect_block;
	u32_t doubly_indirect_block;
	u32_t triply_indirect_block;
	u32_t generation_number;
	u32_t facl;	// Extended attribute block (File ACL)
	u32_t size_hi;
	u32_t block_of_fragment;
	u8_t os_specific_val[12];
} __attribute__((packed)) Inode;

FileSystem* mount_ext2(int device_num, int part_id, int start_lba, int part_len);

int ext2_create_ext_entry(void* t);		// used to create file or directory
int ext2_remove_ext_entry(void* t);		// used to remove file or directory
int ext2_read_file(void* file, u32_t start, u32_t length, u8_t* buffer);
int ext2_write_file(void* file, u32_t start, u32_t length, u8_t* buffer);
void* ext2_get_directory(void* t);		// list directory

#endif


