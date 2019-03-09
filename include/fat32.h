#ifndef _FAT32_H
#define _FAT32_H

#include <types.h>

#define FSINFO_SIGNATURE_1 		0x41615252
#define FSINFO_SIGNATURE_2 		0x61417272


#define FAT32_ENTRY_RECORD_LEN		32
#define FAT32_DELETED_ENTRY		0xE5
#define FAT32_DOT_ENTRY			0x2E
#define FAT32_VOL_LABEL_ENTRY		0x08
#define FAT32_LFN_ENTRY			0x0F

#define EOC				0x0FFFFFFF

#define FAT32_DIR_TABLE_ENTRY_UPDATE	0x1
#define FAT32_DIR_TABLE_ENTRY_REMOVE	0x2

#define FIELD_CLUSTER_NUM		100		
#define FIELD_MODIFIED_DATE		200
#define FIELD_FILE_SIZE			300


typedef struct _PartitionBootRecord
{
	u8_t jmp_code[3];
	char OEM[8];
	u16_t bytes_per_sector;
	u8_t sectors_per_cluster;
	u16_t reserved_sectors;
	u8_t fat_count;
	u16_t dir_entries;
	u16_t total_sectors;
	u8_t media_desc;
	u16_t sectors_per_fat16;	
	u16_t sectors_per_track;	
	u16_t media_sides;
	u32_t start_lba;
	u32_t media_sectors_count;
	u32_t sectors_per_fat;	
	u16_t flags;
	u16_t fat_version;
	u32_t root_dir_cluster;
	u16_t FSInfo_sector_number;
	u16_t backup_boot_cluster;
	u8_t reserved[12];
	u8_t drive_number;
	u8_t reserved_2;
	u8_t sig;
	u32_t serial_number;
	char label[11];
	char sys_id_string[8];
	u8_t boot_code[420];
	u16_t signature;
} __attribute__((packed)) PartitionBootRecord;


typedef struct _FAT32_Dir_Entry
{
	char name[8];
	char ext[3];
	u8_t file_attr;
	u8_t reserved;
	u8_t creation_time_ms;
	u16_t creation_time;
	u16_t creation_date;
	u16_t last_access_date;
	u16_t cluster_hi;
	u16_t last_modification_time;
	u16_t last_modification_date;
	u16_t cluster_lo;
	u32_t file_size;	
} __attribute__((packed)) FAT32_Dir_Entry;


typedef struct _FAT32_LFN_Entry		// long file name
{
	u8_t serial_number;
	u16_t name1[5];
	u8_t attr;
	u8_t type;
	u8_t check_sum;
	u16_t name2[6];
	u16_t fCluster;
	u16_t name3[2];	
} __attribute__((packed)) FAT32_LFN_Entry;


FileSystem* mount_fat32(int device_num, int part_id, int start_lba, int part_len);

int fat32_create_fat_entry(void* t);		// used to create file or directory
int fat32_remove_fat_entry(void* t);		// used to remove file or directory
int fat32_read_file(void* file, u32_t start, u32_t length, u8_t* buffer);
int fat32_write_file(void* file, u32_t start, u32_t length, u8_t* buffer);
void* fat32_get_directory(void* t);		// list directory

#endif


