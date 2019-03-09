#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include <types.h>


#define FILESYSTEM_PREM_READ_ONLY		0X01
#define FILESYSTEM_PREM_READ_WRITE		0X02


#define FS_NAME_MAX_LEN		8
#define ISO9660_FS		"ISO9660"
#define FAT32_FS		"FAT32"
#define EXT2_FS			"EXT2"

#define BOOT_SIGNATURE		0xAA55

#define FAT32_SIG		0X0B
#define FAT32_SIG_LBA		0X0C

#define EXT2_SIG		0X83

#define SYSTEM_DIR_NODE		0
#define DEVICE_DIR_NODE		1
#define NORMAL_DIR_NODE		2

/* File flags for FS_DirectoryEntry */
#define FS_DIRECTORY	0x00000001	/* Entry is a directory */


/* File Open flags */
#define _O_APPEND   0x00000001
#define _O_CREAT    0x00000002
#define _O_RDONLY   0x00000004
#define _O_WRONLY   0x00000008
#define _O_RDWR     (_O_WRONLY|_O_RDONLY)

#define MAX_PATH		1024
#define MAX_FILE_HANDLERS	1024

#define MAX_READ_BUFF_LEN	4096 * 10
#define MAX_WRITE_BUFF_LEN	4096 * 10

#define SEEK_SET 		0
#define SEEK_CUR 		1
#define SEEK_END		2


typedef void* (*GET_DIRECTORY)(void* );
typedef int (*CREATE_FILE_NODE)(void* );
typedef int (*READ_FILE)(void*, u32_t, u32_t, u8_t*);
typedef int (*WRITE_FILE)(void*, u32_t, u32_t, u8_t*);
typedef int (*REMOVE_FILE)(void*);

typedef struct _FileSystem
{
	char name[FS_NAME_MAX_LEN];
	int device_num;

	GET_DIRECTORY getDirectory;
	CREATE_FILE_NODE createFile;
	READ_FILE readFile;
	WRITE_FILE writeFile;
	REMOVE_FILE removeFile;

	void* opaque;		/* Opaque data (driver specific data) */

	int permission;

	struct _FileSystem* next;
	
} FileSystem;


typedef struct _FS_FileEntry
{
	char* filename;		/* Entry name */
	u16_t year;		/* Creation year */
	u8_t month;		/* Creation month */
	u8_t day;		/* Creation day */
	u8_t hour;		/* Creation hour */
	u8_t min;		/* Creation minute */
	u8_t sec;		/* Creation second */
	u32_t lbapos;		/* On-device position of file (driver specific, e.g. for iso9660
				   it is a raw sector, but for fat32 it is a cluster number) */
	u32_t device;		/* Device ID (driver specific, for ide devices it is the index) */
	u32_t size;		/* File size in bytes */
	u32_t flags;		/* File flags (FS_*) */

	struct _FS_Tree* directory;	/* Containing directory */
	struct _FS_FileEntry* next;	/* Next entry */
} FS_FileEntry;


typedef struct _FS_Tree
{
	u8_t dirty;					/* If this is set, the directory needs to be (re-)fetched from the filesystem driver */
	char* name;					/* Directory name */
	struct _FS_Tree* parent;			/* Parent directory name */
	struct _FS_Tree* child_dirs;			/* Linked list of child directories */
	FS_FileEntry* files;			/* List of files (this also includes directories with FS_DIRECTORY bit set) */
	FileSystem* responsible_driver;		/* The driver responsible for handling this directory */
	u32_t lbapos;					/* Directory LBA position (driver specific) */
	u32_t device;					/* Directory device ID (driver specific) */
	u32_t size;					/* Directory size (usually meaningless except to drivers) */
	struct _FS_Tree* next;				/* Next entry for iterating as a linked list (enumerating child directories) */

	u8_t	type;					/* Type of the Node*/
} FS_Tree;


/* ISO9660 structure. This maps an ISO9660 filesystem to a linked
 * list of VFS entries which can be used in the virtual filesystem.
*/

/* drivers specific data*/
typedef struct _iso9660
{
	u32_t drivenumber;
	int joliet;
	char* volume_name;
	u32_t pathtable_lba;
	u32_t rootextent_lba;
	u32_t rootextent_len;
	FS_FileEntry* root;
} iso9660;

typedef struct _FSInfo		// for FAT-32 file system
{
	u32_t signature;
	u8_t reserved[480];
	u32_t signature_2;
	u32_t free_cluster_count;
	u32_t next_free_cluster;
	u8_t _reserved[12];
	u16_t unknown;
	u16_t boot_sig;
} __attribute__((packed)) FSInfo;

typedef struct _fat32
{
	u32_t drivenumber;
	char* partition_label;
	int partition_id;
	int start_lba;
	int partition_len;
	u16_t reserved_sectors;
	u32_t FSInfo_sector_number;
	u16_t bytes_per_sector;
	u8_t sectors_per_cluster;
	u8_t fat_count;
	u32_t sectors_per_fat;	
	u32_t root_dir_cluster;
	FSInfo* fs_info;
	FS_FileEntry* root;
} fat32;


typedef struct _ext2		// for ext2 file-system
{
	u32_t drivenumber;
	char* partition_label;
	int partition_id;
	int start_lba;
	int partition_len;
	u32_t supperblock_block_number;
	u16_t bytes_per_sector;
	u16_t sectors_per_block;
	u32_t bytes_per_block;
	u32_t reserved_blocks;
	u32_t total_inodes;
	u32_t total_blocks;	
	u32_t num_of_blocks_per_blockgroup;
	u32_t num_of_inodes_per_blockgroup;

	u16_t first_non_reserved_inode;
	u32_t size_of_inode_structure;

	u8_t* group_desc_table;
	int blockgroup_count;

	u8_t* superblock;

	u32_t root_dir_inode;
	FS_FileEntry* root;
} ext2;


typedef struct _FileHandler
{
	int mode;
	FS_FileEntry* file;
	
	u8_t* inBuff;
	int inBuffSize;

	u8_t* outBuff;
	int outBuffSize;
	int outBuffPos;

	u32_t seek_pos;

} FileHandler;

typedef struct _PartitionTableEntry
{
	u8_t boot_indicator;
	u8_t starting_head;
	u8_t starting_sector;
	u8_t starting_cylinder;
	u8_t system_id;
	u8_t ending_head;
	u8_t ending_sector;
	u8_t ending_cylinder;
	u32_t start_lba;
	u32_t total_sectors;	
} __attribute__((packed)) PartitionTableEntry;

typedef struct _MBR
{
	u8_t boot_code[446];
	PartitionTableEntry partition[4];
	u16_t signature;
} __attribute__((packed)) MBR;



int init_fileSystems();
void register_fat32_partition(int device_num, int part_id, u32_t start_lba, u32_t total_sectors);
void register_ext2_partition(int device_num, int part_id, u32_t start_lba, u32_t total_sectors);
void register_cdrom(int dev_num);


/*----------------------------------------------------------*/
/* file operation */
/*----------------------------------------------------------*/
int _open(const char *filename, int oflag);

int _read(int fd, void *buffer, u32_t count);

int _write(int fd, void *buffer, u32_t count);

int _flush(int fd);

void _close(int fd);

int _eof(int fd);

long _lseek(int fd, long offset, int origin);

long _tell(int fd);

u32_t file_length(int fd);


int _mkdir(const char *filename);
int _rm(const char *filename);
/*----------------------------------------------------------*/
/*----------------------------------------------------------*/
/*----------------------------------------------------------*/

int fs_read_file(FS_FileEntry* file, u32_t start, u32_t length, u8_t* buffer);



#endif

