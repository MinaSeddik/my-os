
#include <filesystem.h>

#include <system.h>
#include <fat32.h>
#include <iso9660.h>
#include <ata.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <debug.h>

extern struct ide_device ide_devices[4];


static FileHandler* file_handlers[MAX_FILE_HANDLERS];


FileSystem* filesystems = NULL;
FS_Tree* root = NULL;


typedef struct _DirStack
{
	char* name;
	struct _DirStack* next; 
} DirStack;

FS_Tree* create_node_entry(char* path, char* node_name);
FS_Tree* walk_internal_node(FS_Tree* current_node, DirStack* nodes);
void add_child(FS_Tree* parent, FS_Tree* child);
FS_Tree* search_child(FS_Tree* parent, char* node_name);
void detect_attached_storage();
void register_cdrom(int dev_num);
int register_harddisk(int device_num);
void build_dir_tree(FS_Tree* current_node);
FS_Tree* load_directory(char* path);
char* is_dir_exist(char* path);
void retrive_node_from_driver(FS_Tree* parent);
FS_FileEntry* list_dir(char* path);

int init_fileSystems()
{
	root = (FS_Tree*) malloc(sizeof(FS_Tree));

//printf("inside init_fileSystems\n");
	root->dirty = 0;
	root->name = (char*) malloc(sizeof(char) * 2);
	strcpy(root->name,"/");
	root->parent = NULL;
	root->child_dirs = NULL;
	root->files = NULL;
	root->responsible_driver = NULL;
	root->lbapos = root->device = root->size = 0;
	root->next = NULL;
	root->type = SYSTEM_DIR_NODE;

//printf("creating_drive_entry for /dev \n");
	FS_Tree* dev_node = create_node_entry("/", "dev");
//printf("creating_drive_entry for /dev = 0x%x \n", dev_node);

	if (dev_node == NULL)	return -1;
	dev_node->type = SYSTEM_DIR_NODE;

	detect_attached_storage();

	return 0;
	
}

FS_Tree* create_node_entry(char* path, char* node_name)
{
	FS_Tree* dir_node = NULL;

//printf("create_node_entry for path = %s and node_name = %s\n", path, node_name);

	if ( path == NULL || path[0] != '/' )
		return dir_node;

	FS_Tree* parent_level = root;
	if ( strcmp(path,"/" ) )
		parent_level = load_directory(path);
	
//printf("create_node_entry here 1\n");

	FS_Tree *tmp_level = NULL;
	if ( parent_level && (tmp_level = search_child(parent_level, node_name)) )	// search if the name of the node is already found
		parent_level = NULL;	

//printf("create_node_entry here 2\n");
	if ( parent_level )
	{

		//----------------------------------------------------------------------
		// here you should create the actual directory first then get its lba
		// then add it the the file system tree
		//----------------------------------------------------------------------


		dir_node = (FS_Tree*) malloc(sizeof(FS_Tree));

		dir_node->dirty = 1;			// need to be write on the disk
		dir_node->name = (char*) malloc(strlen(node_name) + 1);
		strcpy(dir_node->name,node_name);
		add_child(parent_level, dir_node);
		dir_node->child_dirs = NULL;
		dir_node->files = NULL;
		dir_node->responsible_driver = parent_level->responsible_driver;
		dir_node->lbapos = dir_node->size = 0;
		dir_node->device = 0;
		dir_node->next = NULL;
		dir_node->type = NORMAL_DIR_NODE;


		// add entry to files
		time_t* time = (time_t*) malloc(sizeof(time_t));
		local_time(time);
//printf("create_node_entry here 1 \n");		
		FS_FileEntry* entry = (FS_FileEntry*) malloc(sizeof(FS_FileEntry));
		entry->filename = (char*) malloc(strlen(node_name) + 1);
		strcpy(entry->filename, node_name);

//printf("create_node_entry here 2 \n");		

		entry->year = time->year + 2000;		
		entry->month = time->month;
		entry->day = time->day;
		entry->hour = time->hour;
		entry->min = time->minute;
		entry->sec = time->second;	

		entry->lbapos = 0;
		entry->device = dir_node->device;
		entry->size = 0;
		entry->flags = FS_DIRECTORY;		// directory type

		entry->directory = parent_level;	// containing directory
		entry->next = NULL;

//printf("create_node_entry here 3 parent_level->files = %x  node_name = %s\n", parent_level->files, parent_level->name);		

		// add it to the tree
		
		if ( ! parent_level->files )
			parent_level->files = entry;
		else
		{
			FS_FileEntry* tmp = parent_level->files;
			while ( tmp->next )
			{
				
				tmp = tmp->next;
			}
//printf("***> entry name = %s \n", tmp->filename);
			tmp->next = entry;
		}

		if ( time )	free(time);
	}

	return dir_node;

}

FS_Tree* walk_internal_node(FS_Tree* current_node, DirStack* nodes)
{
//printf("walk_internal_node here1  current_node_name = %s\n", current_node->name);

	if ( nodes == NULL)	return NULL;
	
	DirStack* node = nodes;
	FS_Tree* parent = current_node, *level = parent;

	while ( node && parent )
	{

//printf("walk_internal_node XXX  parent = %s, node_name = %s\n", parent->name, node->name);

		retrive_node_from_driver(parent);

		level = search_child(parent, node->name);

		parent = level;
		
		node = node->next;
	}

//printf("walk_internal_node here2 return  level name = %s\n", level->name);	
	return level;
}


FS_Tree* search_child(FS_Tree* parent, char* node_name)
{
	if ( parent == NULL )	return NULL;

//printf("search_child here1 parent_name = %s, node_name = %s\n", parent->name, node_name);

	
	
//printf("search_child here1 tmp = %x \n", tmp);

	if ( !strcmp(node_name,"."))	return parent;

	if ( !strcmp(node_name,".."))	return parent->parent;

	FS_Tree* tmp = parent->child_dirs;

	if ( parent->type != SYSTEM_DIR_NODE && tmp == NULL )	
		retrive_node_from_driver(tmp);	

	while ( tmp != NULL )
	{
		if ( !strcmp(node_name, tmp->name) )	return tmp;
		tmp = tmp->next;
	}

	return NULL;
}

void retrive_node_from_driver(FS_Tree* parent)
{
	
	if ( parent->child_dirs && parent->files )	return;

//printf("retrive_node_from_driver  get_dir = 0x%x\n", parent->responsible_driver->getDirectory );

	if ( parent->type != SYSTEM_DIR_NODE )
	{

		FS_FileEntry* childs = parent->responsible_driver->getDirectory(parent);	

		if ( childs )
		{
			parent->files = childs;
		
			build_dir_tree(parent);
		
		}
	}
	
}


void add_child(FS_Tree* parent, FS_Tree* child)
{

	child->parent = parent;

	if ( parent->child_dirs == NULL )
		parent->child_dirs = child;
	else
	{
		FS_Tree* node = parent->child_dirs;
		while ( node->next != NULL )	node = node->next;
		node->next = child;
		
	}

}

void remove_child(FS_Tree* parent, char* dir_name)
{

	if ( parent->child_dirs == NULL )	return; 

	FS_Tree* tmp = NULL;

	// if it is the first node in the list
	if ( !strcmp(parent->child_dirs->name, dir_name) )
	{
		tmp = parent->child_dirs;
		parent->child_dirs = parent->child_dirs->next;
	}
	else
	{
		FS_Tree* node = parent->child_dirs;
		while ( node->next )
		{
			if ( !strcmp(node->next->name, dir_name) ) 
			{
				tmp = node->next;
				node->next = tmp->next;
				break;
			}
			node = node->next;
		}	
	}

	if (tmp->name) 	free(tmp->name);
	if (tmp) 	free(tmp);

}


void detect_attached_storage()
{
	int i;
//printf("detect_attached_storage here 1 ...\n");

	for (i = 0; i < 4; i++)
	{
		if ( ide_devices[i].reserved == 1 && ide_devices[i].type == IDE_ATA)	// Device exists and Hard-disk drive
			register_harddisk(i);
		
		else if ( ide_devices[i].reserved == 1 && ide_devices[i].type == IDE_ATAPI)	// Device exists and cdrom drive
			register_cdrom(i);
	}	
}


int register_harddisk(int device_num)
{
	printf("try to mount harddisk, device number = %d\n",device_num);

	// read the master boot record for that drive
	MBR* mbr = (MBR*) malloc(sizeof(MBR));
	memset(mbr, 0, sizeof(MBR));

	if ( !ide_read_sectors(device_num, 1, 0 /* start lba = 0 ; master boot record*/, (u32_t)mbr) )
	{
		printf("Could not read Master boot Record!\n");
		if (mbr) 	free(mbr);
		return 0;
	}

//	dump_hexa(mbr, 512);
//	while(1);

	if ( mbr->signature == BOOT_SIGNATURE )
	{
		int i;
		for( i=0;i<4;i++)
		{
		
			if ( mbr->partition[i].system_id == FAT32_SIG || mbr->partition[i].system_id == FAT32_SIG_LBA )
			{
				printf("Drive : %d   Found FAT-32 Patition # %d\n",device_num, i);
/*
				printf("boot indicator 0x%x\n", mbr->partition[i].boot_indicator);
				printf("starting head %d\n", mbr->partition[i].starting_head);
				printf("starting sector %d\n", mbr->partition[i].starting_sector&0x3F);
				u16_t cylind = (mbr->partition[i].starting_sector&0xC0) <<2; 
				cylind|= mbr->partition[i].starting_cylinder;
				printf("starting cylinder %d\n", cylind);
				printf("system Id 0x%x\n", mbr->partition[i].system_id);
				printf("ending head %d\n", mbr->partition[i].ending_head);
				printf("ending sector %d\n", mbr->partition[i].ending_sector&0x3F);
				cylind = (mbr->partition[i].ending_sector&0xC0) <<2; 
				cylind|= mbr->partition[i].ending_cylinder;
				printf("ending cylinder %d\n", cylind);
				printf("start lba %d\n", mbr->partition[i].start_lba);
				printf("total sectors %d\n\n", mbr->partition[i].total_sectors);
*/
				//while(1);
				register_fat32_partition(device_num, i+1, mbr->partition[i].start_lba, mbr->partition[i].total_sectors);
			}
			else if ( mbr->partition[i].system_id == EXT2_SIG )
			{
				printf("Drive : %d   Found ext2 Patition # %d\n",device_num, i);

				register_ext2_partition(device_num, i+1, mbr->partition[i].start_lba, mbr->partition[i].total_sectors);
			}
		}

	}


	if (mbr) 	free(mbr);

	return 0;
}

void register_fat32_partition(int device_num, int part_id, u32_t start_lba, u32_t total_sectors)
{
	char dev_ch[2]={0};
	char part_id_str[5];

	switch(device_num)
	{
		case 0:		dev_ch[0] = 'a';		break;
		case 1:		dev_ch[0] = 'b';		break;
		case 2:		dev_ch[0] = 'c';		break;
		case 3:		dev_ch[0] = 'd';		break;
		default:	dev_ch[0] = 'x';
	}

	char* path = (char*) malloc(sizeof(char) * 10);

	strcpy(path,"hd");
	strcat(path,dev_ch);
	itoa(part_id, part_id_str, 10);
	strcat(path,part_id_str);

	FS_FileEntry* fat32_part_driver = (FS_FileEntry*) malloc( sizeof(FS_FileEntry));
	fat32_part_driver->filename = (char*) malloc(strlen(path) + 1);
	strcpy(fat32_part_driver->filename, path);
	fat32_part_driver->year = fat32_part_driver->month = fat32_part_driver->day = fat32_part_driver->hour = fat32_part_driver->min = fat32_part_driver->sec = 0;
	fat32_part_driver->lbapos = fat32_part_driver->size = 0;	// only for initialization
	fat32_part_driver->device = device_num;
	fat32_part_driver->flags = 0;		// as ordinary file type
	fat32_part_driver->directory = root;
	fat32_part_driver->next = NULL;

	// add it to the tree
	FS_Tree* dev_node = load_directory("/dev");
	if ( !dev_node )
	{
		printf("ERROR : Can't get /dev node ...\n");
		return;
	}
	
	if ( ! dev_node->files )
		dev_node->files = fat32_part_driver;
	else
	{
		FS_FileEntry* tmp = dev_node->files;
//printf("register_fat32_partition : tmp->name = %s  next = 0x%x...\n",tmp->filename, tmp->next);
		while ( tmp->next )	tmp = tmp->next;
		tmp->next = fat32_part_driver;
	}
//while(1);
	FileSystem* fat32_fs = mount_fat32(device_num, part_id, start_lba, total_sectors);

//printf("register_fat32_partition here 1 fat32_fs = 0x%x...\n", fat32_fs);

	if ( fat32_fs )
	{
//printf("register_fat32_partition create_node_entry :- %s ...\n", path);
		FS_Tree* fat32_part_mount_node = create_node_entry("/", path);	// create /hdXN node

		if ( fat32_part_mount_node )
		{
//printf("rregister_fat32_partition here 3 ...\n");
			fat32_part_mount_node->responsible_driver = fat32_fs;

			fat32* fat32_info = (fat32*) fat32_fs->opaque;
//printf("register_fat32_partition here 4...\n");
			fat32_part_mount_node->lbapos = fat32_info->root_dir_cluster;		// lba position here is represent cluster number
			fat32_part_mount_node->size = 0;		

			FS_FileEntry* entries = fat32_info->root;
			fat32_part_mount_node->files = entries;
//printf("register_fat32_partition here 5...\n");
			build_dir_tree(fat32_part_mount_node);
//printf("register_fat32_partition here 6...\n");
		}
	}

//printf("register_fat32_partition here 7...\n");
	if ( path )	free(path);
//printf("register_fat32_partition here 8...\n");
	
	return ;

}


void register_ext2_partition(int device_num, int part_id, u32_t start_lba, u32_t total_sectors)
{
	char dev_ch[2]={0};
	char part_id_str[5];

	switch(device_num)
	{
		case 0:		dev_ch[0] = 'a';		break;
		case 1:		dev_ch[0] = 'b';		break;
		case 2:		dev_ch[0] = 'c';		break;
		case 3:		dev_ch[0] = 'd';		break;
		default:	dev_ch[0] = 'x';
	}

	char* path = (char*) malloc(sizeof(char) * 10);

	strcpy(path,"hd");
	strcat(path,dev_ch);
	itoa(part_id, part_id_str, 10);
	strcat(path,part_id_str);

	FS_FileEntry* ext2_part_driver = (FS_FileEntry*) malloc( sizeof(FS_FileEntry));
	ext2_part_driver->filename = (char*) malloc(strlen(path) + 1);
	strcpy(ext2_part_driver->filename, path);
	ext2_part_driver->year = ext2_part_driver->month = ext2_part_driver->day = ext2_part_driver->hour = ext2_part_driver->min = ext2_part_driver->sec = 0;
	ext2_part_driver->lbapos = ext2_part_driver->size = 0;	// only for initialization
	ext2_part_driver->device = device_num;
	ext2_part_driver->flags = 0;		// as ordinary file type
	ext2_part_driver->directory = root;
	ext2_part_driver->next = NULL;

	// add it to the tree
	FS_Tree* dev_node = load_directory("/dev");
	if ( !dev_node )
	{
		printf("ERROR : Can't get /dev node ...\n");
		return;
	}
	
	if ( ! dev_node->files )
		dev_node->files = ext2_part_driver;
	else
	{
		FS_FileEntry* tmp = dev_node->files;
//printf("register_ext2_partition : tmp->name = %s  next = 0x%x...\n",tmp->filename, tmp->next);
		while ( tmp->next )	tmp = tmp->next;
		tmp->next = ext2_part_driver;
	}

	FileSystem* ext2_fs = mount_ext2(device_num, part_id, start_lba, total_sectors);

//printf("register_ext2_partition here 1 ext2_fs = 0x%x...\n", ext2_fs);

	if ( ext2_fs )
	{
//printf("register_ext2_partition create_node_entry :- %s ...\n", path);
		FS_Tree* ext2_part_mount_node = create_node_entry("/", path);	// create /hdXN node

		if ( ext2_part_mount_node )
		{
//printf("register_ext2_partition here 3 ...\n");
			ext2_part_mount_node->responsible_driver = ext2_fs;

			ext2* ext2_info = (ext2*) ext2_fs->opaque;
//printf("register_ext2_partition here 4...\n");
			ext2_part_mount_node->lbapos = ext2_info->root_dir_inode;		// lba position here is represent inode number
			ext2_part_mount_node->size = 0;		

			FS_FileEntry* entries = ext2_info->root;
			ext2_part_mount_node->files = entries;
//printf("register_ext2_partition here 5...\n");
			build_dir_tree(ext2_part_mount_node);
//printf("register_ext2_partition here 6...\n");
		}
	}

//printf("register_ext2_partition here 7...\n");
	if ( path )	free(path);
//printf("register_ext2_partition here 8...\n");
	
	return ;

}


void register_cdrom(int dev_num)
{
	char dev_ch[2]={0};

	switch(dev_num)
	{
		case 0:		dev_ch[0] = 'a';		break;
		case 1:		dev_ch[0] = 'b';		break;
		case 2:		dev_ch[0] = 'c';		break;
		case 3:		dev_ch[0] = 'd';		break;
		default:	dev_ch[0] = 'x';
	}

	char* path = (char*) malloc(sizeof(char) * 15);

	strcpy(path,"cdrom");
	strcat(path,dev_ch);

	FS_FileEntry* cdrom_driver = (FS_FileEntry*) malloc( sizeof(FS_FileEntry));
	cdrom_driver->filename = (char*) malloc(strlen(path) + 1);
	strcpy(cdrom_driver->filename, path);
	cdrom_driver->year = cdrom_driver->month = cdrom_driver->day = cdrom_driver->hour = cdrom_driver->min = cdrom_driver->sec = 0;
	cdrom_driver->lbapos = cdrom_driver->size = 0;
	cdrom_driver->device = dev_num;
	cdrom_driver->flags = 0;		// as ordinary file type
	cdrom_driver->directory = root;
	cdrom_driver->next = NULL;

	// add it to the tree
	FS_Tree* dev_node = load_directory("/dev");
	if ( !dev_node )
	{
		printf("ERROR : Can't get /dev node ...\n");
		return;
	}
	
	if ( ! dev_node->files )
		dev_node->files = cdrom_driver;
	else
	{
		FS_FileEntry* tmp = dev_node->files;
		while ( !tmp->next )	tmp = tmp->next;
		tmp->next = cdrom_driver;
	}

	FileSystem* iso_fs =  mount_iso9660(dev_num);

//printf("register_cdrom here 1 iso_fs = 0x%x...\n", iso_fs);

	if ( iso_fs )
	{
//printf("register_cdrom create_node_entry :- %s ...\n", path);
		FS_Tree* cdrom_mount_node = create_node_entry("/", path);	// create /cdromX node

		if ( cdrom_mount_node )
		{
//printf("register_cdrom here 3 ...\n");
			cdrom_mount_node->responsible_driver = iso_fs;

			iso9660* iso_info = (iso9660*) iso_fs->opaque;
//printf("register_cdrom here 4...\n");
			cdrom_mount_node->lbapos = iso_info->rootextent_lba;
			cdrom_mount_node->size = iso_info->rootextent_len;

			FS_FileEntry* entries = iso_info->root;
			cdrom_mount_node->files = entries;
//printf("register_cdrom here 5...\n");
			build_dir_tree(cdrom_mount_node);
//printf("register_cdrom here 6...\n");
		}
	}

//printf("register_cdrom here 7...\n");
	if ( path )	free(path);

}


void build_dir_tree(FS_Tree* current_node)
{

	if ( current_node == NULL ) return;

//printf("build_dir_tree ...\n");

	FS_FileEntry* entries = current_node->files;

	while( entries )
	{
	
//printf("name = %s type = %d size = %d pos = %d\n", entries->filename, entries->flags, entries->size, entries->lbapos);

		if ( entries->flags )	// this file is directory type
		{
			FS_Tree* child = (FS_Tree*) malloc(sizeof(FS_Tree));

			child->dirty = 1;
			child->name = (char*) malloc(strlen(entries->filename) + 1);
			strcpy(child->name,entries->filename);
			add_child(current_node, child);
			child->child_dirs = NULL;
			child->files = NULL;
			child->responsible_driver = current_node->responsible_driver;
			child->lbapos = entries->lbapos;
			child->size = entries->size;
			child->device = current_node->device;
			child->next = NULL;
			child->type = NORMAL_DIR_NODE;

		} 
		entries->directory = current_node;

		entries = entries->next;
	}

	
}

char* is_dir_exist(char* vpath)
{
	FS_Tree* current_node = load_directory(vpath);

	char* path = NULL;
	if ( current_node )
	{
		path = (char*) malloc(MAX_PATH);
		memset(path, 0, MAX_PATH);

		DirStack* nodes = NULL, *node = NULL;
		FS_Tree* tmp = current_node;

		while( strcmp(tmp->name,"/") )
		{
		
			node = (DirStack*) malloc(sizeof(DirStack));
			node->name = (char*) malloc(strlen(tmp->name) + 1);	
			strcpy(node->name,tmp->name);
			node->next = nodes;
			nodes = node;

			tmp = tmp->parent;
		}

		// build path
		strcpy(path,"/");
		node = nodes;
		while ( node )
		{
			strcat(path,node->name);
			strcat(path,"/");
			node = node->next;
		}

		// free nodes
		node = nodes;
		while ( node )
		{
			nodes = node->next;

			if( node->name )	free(node->name);
			if( node )	free(node);

			node = nodes;
		}
		
		
	}

	return path;
}



FS_Tree* load_directory(char* path)
{

//printf("load_directory 1 ... path = %s\n", path);


	FS_Tree* dir_node = NULL;

	if ( path == NULL || path[0] != '/' )
		return NULL;

	if ( ! strcmp(path,"/"))
		return root;		// root element already loaded

	char* copy = (char*) malloc(strlen(path));
	strcpy(copy, path+1);

	if( copy[strlen(copy)-1] == '/' )
		copy[strlen(copy)-1] = 0;

	DirStack* nodes = NULL, *node = NULL, *tmp = NULL;
	char* level = copy , *ptr;
	int level_count = 1;

	for( ptr = copy;*ptr;++ptr)
	{
//printf("load_directory 2 ... \n");
		if ( *ptr == '/' )
		{
			*ptr = 0;
			node = (DirStack*) malloc(sizeof(DirStack));
			node->name = (char*) malloc(strlen(level) + 1);
			strcpy(node->name,level);
			node->next = NULL;
			level = ptr+1;

			if ( nodes == NULL )
			{	
				nodes = node;
			}
			else
			{
				tmp = nodes;
				while ( tmp->next != NULL )	tmp = tmp->next;
				tmp->next = node;
			}

			++level_count;
		}
	}
	
//printf("load_directory 3 ... level = %s\n", level);

	node = (DirStack*) malloc(sizeof(DirStack));
	node->name = (char*) malloc(strlen(level) + 1);
	strcpy(node->name,level);
	node->next = NULL;
	if ( ! nodes )
		nodes = node;
	else
	{
		tmp = nodes;
		while ( tmp->next != NULL )	tmp = tmp->next;
		tmp->next = node;
	}

//printf("create_node_entry here1  node->name = %s, level count = %d\n", node->name, level_count);

//printf("load_directory 2 ... \n");
	FS_Tree* parent_level = NULL;
	parent_level = walk_internal_node(root,nodes);
//printf("load_directory 3 ... parent_level = 0x%x\n", parent_level);

	// free temp virtual path
	if ( copy )	free(copy);

	// free nodes stack
	node = nodes;
	while ( node )
	{
		nodes = node->next;
		if ( node->name )	free(node->name);
		if ( node )	free(node);
		node = nodes;

	}

	return parent_level;	
}

FS_FileEntry* list_dir(char* path)
{
	FS_Tree* dir_node = load_directory(path);

//printf("list_dir path = %s\n", path);

//printf("list_dir dir_node = 0x%x\n", dir_node);

	if ( ! dir_node )	return NULL;

	retrive_node_from_driver(dir_node);

//printf("list_dir files = 0x%x\n", dir_node->files);

	return dir_node->files;

	
}

FS_Tree* get_file_parent_directory(char* path)
{

	FS_Tree* dir_node = NULL;

//printf("get_file_parent_directory  1 ...\n");

	if ( path == NULL || path[0] != '/' || !strcmp(path,"/") || path[strlen(path)-1] == '/' )
		return NULL;

	char* copy = (char*) malloc(strlen(path) + 1);
	strcpy(copy, path);

	char* file_name = strrchr(copy,'/');
//printf("get_file_parent_directory  3 ...\n");
	if ( file_name )
	{
		// test if the parent is root directory
		if ( file_name == copy )	dir_node = root;
		else
		{
			*file_name = 0;
			dir_node = load_directory(copy);
			if ( !dir_node->files )	retrive_node_from_driver(dir_node);
		}
	}
//printf("get_file_parent_directory  4 ...\n");
	if (copy)	free(copy);

	return dir_node;	
}

FS_FileEntry* get_file_info(char* path)
{


//printf("get_file_info 1 ... path = \"%s\"\n", path);


	FS_FileEntry* file_info = NULL,* tmp = NULL;

	if ( path == NULL || path[0] != '/' || !strcmp(path,"/") || path[strlen(path)-1] == '/' )
		return NULL;

	char* copy = (char*) malloc(strlen(path) + 1);
	strcpy(copy, path);

	char* file_name = strrchr(copy,'/');

	if ( file_name )
	{

		*file_name = 0;
		file_name++;
//printf("get_file_info 1 ... file_name = %s\n", file_name);
//printf("get_file_info 1 ... copy = %s\n", copy);
		FS_Tree* dir_node = load_directory(copy);
//printf("get_file_info 1 ... dir_node = %x\n", dir_node);
		if ( dir_node )
		{
//printf("get_file_info 1 ... dir_node->files = %x\n", dir_node->files);
			if ( !dir_node->files )	retrive_node_from_driver(dir_node);

			tmp = dir_node->files;

			while ( tmp )
			{
//printf("get_file_info 1 ... file name = %x\n", tmp->filename);
				if ( !strcmp(tmp->filename, file_name) )	
				{	
					file_info = tmp;
					break;
				}
				tmp = tmp->next;
			}
		
		}
	}


	if (copy)	free(copy);
	
	return file_info;
		
}

int create_file_handler(FS_FileEntry* file_info, int oflag)
{

	int first_free = 0, fd = -1;

//printf("create_file_handler here 1 0x%x\n", file_handlers[first_free]);


	while ( first_free < MAX_FILE_HANDLERS )
	{
		if ( !file_handlers[first_free] )
		{
//printf("create_file_handler here 2 0x%x\n", file_handlers[first_free]);
			fd = first_free;
			break;	
		}
		first_free++;
	}

	if ( fd >= 0 )
	{
//printf("create_file_handler here 3\n");
		FileHandler* handler = (FileHandler*) malloc(sizeof(FileHandler));

		handler->mode = oflag;
		handler->file = file_info;
		handler->seek_pos = 0;

		if ( handler->mode | _O_RDONLY )	// open file for read
		{
			handler->inBuff = (char*) malloc(MAX_READ_BUFF_LEN);
			handler->inBuffSize = MAX_READ_BUFF_LEN;
		}
		else
		{
			handler->inBuff = NULL;
			handler->inBuffSize = 0;	
		}
	
		if ( handler->mode | _O_WRONLY )	// open file for write
		{
			handler->outBuff = (char*) malloc(MAX_WRITE_BUFF_LEN);
			handler->outBuffSize = MAX_WRITE_BUFF_LEN;
			handler->outBuffPos = 0;
		}
		else
		{
			handler->outBuff = NULL;
			handler->outBuffSize = 0;
			handler->outBuffPos = 0;	
		}

		file_handlers[fd] = handler;
	}
//printf("create_file_handler here 4 fd = %d\n", fd);
	return fd;
}


void destroy_file_handler(int fd)
{
	FileHandler* handler = file_handlers[fd];
	file_handlers[fd] = NULL;

	if ( handler )
	{

		if ( handler->inBuff )	free(handler->inBuff);

		if ( handler->outBuff )	free(handler->outBuff);

		free(handler);
	}
	
}


int _mkdir(const char *filename)
{
	if ( !filename )	return -1;
	
	FS_FileEntry* file_info = get_file_info(filename);

	// if it is exist 
	if ( file_info )	return -1;
	
	FS_Tree* dir_node = get_file_parent_directory(filename); 
	if ( ! dir_node )	return -1;	// invalid path
		
	// check if the file system has write permission
	FileSystem* file_system = dir_node->responsible_driver;
	if ( dir_node->type != SYSTEM_DIR_NODE && file_system && file_system->permission & FILESYSTEM_PREM_READ_WRITE && file_system->createFile )
	{
			
		// create node and append it to the tree
		FS_FileEntry* file_entry = (FS_FileEntry*) malloc(sizeof(FS_FileEntry));
		memset(file_entry, 0, sizeof(FS_FileEntry));

		char* file_name = strrchr(filename,'/');
		if ( !file_name )
		{
			if(file_entry)	free(file_entry);
			return -1;
		}
		file_name++;
		int file_len = strlen(file_name);
		file_entry->filename = (char*) malloc(file_len+1);
		strcpy(file_entry->filename, file_name);
	
		time_t now;
		local_time(&now);
		file_entry->year = now.year + 2000;
		file_entry->month = now.month;
		file_entry->day = now.day;
		file_entry->hour = now.hour;
		file_entry->min = now.minute;
		file_entry->sec = now.second;
		file_entry->lbapos = 0;		// it will be updated in createFile system call
		file_entry->device = dir_node->device;
		file_entry->size = 0;
		file_entry->flags = FS_DIRECTORY;
		file_entry->directory = dir_node;

		int ret = file_system->createFile(file_entry);

		if ( ret == -1 )
		{
			if(file_entry->filename)	free(file_entry->filename);
			if(file_entry)	free(file_entry);
			return -1;
		}

		// append to the list
		file_entry->next = dir_node->files;
		dir_node->files = file_entry;


		// add to directory tree
		char *dir_name = strrchr(filename, '/');
		dir_name++;
//printf("Dir Name = %s\n", dir_name);

		FS_Tree* child = (FS_Tree*) malloc(sizeof(FS_Tree));
		child->dirty = 1;
		child->name = (char*) malloc(strlen(dir_name) + 1);
		strcpy(child->name,dir_name);
		add_child(dir_node, child);
		child->child_dirs = NULL;
		child->files = NULL;
		child->responsible_driver = dir_node->responsible_driver;
		child->lbapos = file_entry->lbapos;
		child->size = file_entry->size;
		child->device = dir_node->device;
		child->next = NULL;
		child->type = NORMAL_DIR_NODE;


	}
	
	return 0;
}


int _rm(const char *filename)
{
	if ( !filename )	return -1;
	
	FS_FileEntry* file_info = get_file_info(filename);

	// if it is exist 
	if ( !file_info )	return -1;

	FS_Tree* dir_node = get_file_parent_directory(filename); 
	if ( ! dir_node )	return -1;	// invalid path
		
	// check if the file system has write permission
	FileSystem* file_system = dir_node->responsible_driver;
//printf("inside _rm dir_node->type = %d\n", dir_node->type);
	if ( dir_node->type != SYSTEM_DIR_NODE && file_system && file_system->permission & FILESYSTEM_PREM_READ_WRITE && file_system->removeFile )
	{
			
		// check if it is a directory, if not empty return error
		if (file_info->flags == FS_DIRECTORY )
		{
			FS_Tree* parent_dir_node = load_directory(filename);
			if ( parent_dir_node && !parent_dir_node->files )
				retrive_node_from_driver(parent_dir_node);	// to load it's child nodes

			FS_Tree* child_node = dir_node->child_dirs;	
			while ( child_node )
			{	
				// the directory is NOT empty
				if( !strcmp(child_node->name, file_info->filename) && child_node->files)		return -2;
				child_node = child_node->next;
			}	
		}
	
//printf("inside _rm before ...\n");
		int ret = file_system->removeFile(file_info);
//printf("inside _rm after ...\n");


		if ( ret < 0 )		return -1;

		// remove from the list
		FS_FileEntry* file_entry = NULL;
		if ( file_info == dir_node->files )	// first node in the list
			dir_node->files = file_info->next;
		else
		{
			file_entry = dir_node->files;
			while ( file_entry )
			{
				if ( file_entry->next == file_info )
				{	
					file_entry->next = file_info->next;
					break;
				}
				file_entry = file_entry->next;
			}
		}
		
		
		// if directory, free directory node
		if (file_info->flags == FS_DIRECTORY )
			remove_child(dir_node, file_info->filename);

		// free the file node
		if (file_info->filename) 	free(file_info->filename);
		if (file_info) 	free(file_info);
	}
	
	return 0;
}



int _open(const char *filename, int oflag)
{

	if ( !filename )	return -1;

//printf("_open here 1\n");

	FS_FileEntry* file_info = get_file_info(filename);

	// if it exists but it is a directory
	if ( file_info && ( file_info->flags == FS_DIRECTORY /*|| file_info->flags == DEVICE_DIR_NODE */) )	return -1;

/*
printf("_open dir_node  oflag = %x \n", oflag);
printf("_open dir_node  _O_WRONLY = %x \n", _O_WRONLY);
printf("_open dir_node  oflag | _O_WRONLY = %x \n", oflag | _O_WRONLY);
*/

	// if the file not found, try to create it if oflag is write mode
	if ( !file_info && (oflag & _O_WRONLY) )
	{	

//printf("_open file not found and we try to create it ... \n");
//while(1);
		FS_Tree* dir_node = get_file_parent_directory(filename); 
		if ( ! dir_node )	return -1;	// invalid path
//printf("_open dir_node  name = %s \n", dir_node->name);		
		// check if the file system has write permission
		FileSystem* file_system = dir_node->responsible_driver;
		if ( dir_node->type != SYSTEM_DIR_NODE && file_system && file_system->permission | FILESYSTEM_PREM_READ_WRITE && file_system->createFile )
		{
			// create node and append it to the tree
			FS_FileEntry* file_entry = (FS_FileEntry*) malloc(sizeof(FS_FileEntry));
			memset(file_entry, 0, sizeof(FS_FileEntry));

			char* file_name = strrchr(filename,'/');
			if ( !file_name )
			{
				if(file_entry)	free(file_entry);
				return -1;
			}
			file_name++;
			int file_len = strlen(file_name);
			file_entry->filename = (char*) malloc(file_len+1);
			strcpy(file_entry->filename, file_name);
//printf("_open file name = %s\n", file_entry->filename);	
			time_t now;
			local_time(&now);
			file_entry->year = now.year + 2000;
			file_entry->month = now.month;
			file_entry->day = now.day;
			file_entry->hour = now.hour;
			file_entry->min = now.minute;
			file_entry->sec = now.second;
		
			file_entry->lbapos = 0;
			file_entry->device = dir_node->device;
			file_entry->size = 0;
					
			file_entry->directory = dir_node;

			int ret = file_system->createFile(file_entry);
//int ret = -1;
			if ( ret == -1 )
			{
				if(file_entry->filename)	free(file_entry->filename);
				if(file_entry)	free(file_entry);
				return -1;
			}

			// append to the list
			file_entry->next = dir_node->files;
			dir_node->files = file_entry;

			file_info = file_entry;

		}
		
		
	}


//printf("_open here 2 file_info = 0x%x, name = %s\n", file_info, file_info->filename );

	return file_info ? create_file_handler(file_info, oflag) : -1;
}

void _close(int fd)
{
	// sanity check
	if ( fd < 0 || fd >= MAX_FILE_HANDLERS || !file_handlers[fd] )
		return ;

	// flush first if the file is write enable
	if ( file_handlers[fd]->mode | _O_WRONLY && file_handlers[fd]->outBuffSize && file_handlers[fd]->outBuff )
	_flush(fd);

	return destroy_file_handler(fd);
	
}

int _read(int fd, void *buffer, u32_t count)
{

	int rb = 0;
	int data_len = 0;
	u8_t* data_buffer = buffer;

	// sanity check
	if ( fd < 0 || fd >= MAX_FILE_HANDLERS || !file_handlers[fd] )
		return -1;

	// check if file is read enabled
	if ( (! (file_handlers[fd]->mode | _O_RDONLY )) || !file_handlers[fd]->inBuffSize || !file_handlers[fd]->inBuff )
		return -1;

	// if the current offset beyond the file size
	if ( file_handlers[fd]->seek_pos >= file_handlers[fd]->file->size || count == 0 )
		return 0;

	// justify the file offset with the size
	if ( file_handlers[fd]->seek_pos + count >= file_handlers[fd]->file->size )
		count = file_handlers[fd]->file->size - file_handlers[fd]->seek_pos;

	int read_bytes = 0;
	if ( count > file_handlers[fd]->inBuffSize )
	{

		while ( count > 0 )
		{
			data_len = count > file_handlers[fd]->inBuffSize ? file_handlers[fd]->inBuffSize : count;
			rb = fs_read_file(file_handlers[fd]->file, file_handlers[fd]->seek_pos, data_len, file_handlers[fd]->inBuff);
			if ( rb == -1 )	return read_bytes;

			file_handlers[fd]->seek_pos+= rb;
			memcpy(data_buffer + read_bytes, file_handlers[fd]->inBuff, rb);
			count-= rb;
			read_bytes+= rb;
		
		}
	}
	else
	{
		read_bytes = fs_read_file(file_handlers[fd]->file, file_handlers[fd]->seek_pos, count, file_handlers[fd]->inBuff);
		if ( read_bytes == -1 )	return read_bytes;

		file_handlers[fd]->seek_pos+= read_bytes;	
		memcpy(buffer, file_handlers[fd]->inBuff, read_bytes);
	
	}	


	return read_bytes;

}


int fs_read_file(FS_FileEntry* file, u32_t offset, u32_t length, u8_t* buffer)
{

	FileSystem* file_system = NULL;
	int read_count = -1;

	if ( file && file->directory && file->directory->responsible_driver )
	{
		file_system = file->directory->responsible_driver;

		read_count = file_system->readFile(file, offset, length, buffer);
	}

	return read_count;
}


int _write(int fd, void *buffer, u32_t count)
{

	int remainig_bytes = 0;

	// sanity check
	if ( fd < 0 || fd >= MAX_FILE_HANDLERS || !file_handlers[fd] )
		return -1;

	// check if file is write enabled
	if ( (! (file_handlers[fd]->mode | _O_WRONLY )) || !file_handlers[fd]->outBuffSize || !file_handlers[fd]->outBuff )
		return -1;
		
	remainig_bytes = file_handlers[fd]->outBuffSize - file_handlers[fd]->outBuffPos;
	u32_t bytes = count;
	u8_t* data_buffer = (u8_t*) buffer;
	int buff_pos = 0, wb, writen_bytes = 0;
	
	while( bytes >= remainig_bytes )
	{
		memcpy(file_handlers[fd]->outBuff + file_handlers[fd]->outBuffPos, data_buffer + buff_pos, remainig_bytes);	
		buff_pos+= remainig_bytes;
		bytes-= remainig_bytes;
		file_handlers[fd]->outBuffPos+= remainig_bytes;

		// write to the disk
		wb = fs_write_file(file_handlers[fd]->file, file_handlers[fd]->seek_pos, file_handlers[fd]->outBuffPos, file_handlers[fd]->outBuff);
		
		file_handlers[fd]->seek_pos+= wb;
		writen_bytes+= wb;
		remainig_bytes = file_handlers[fd]->outBuffSize; 

		if ( file_handlers[fd]->outBuffPos >= file_handlers[fd]->outBuffSize )
			file_handlers[fd]->outBuffPos = 0;
	}
	
	if ( remainig_bytes > bytes )
	{
		// put the data in the buffer without writing to the disk
		memcpy(file_handlers[fd]->outBuff + file_handlers[fd]->outBuffPos, data_buffer + buff_pos , bytes);
		file_handlers[fd]->outBuffPos+= bytes;		// need to be flushed
		
		writen_bytes+= bytes;
	}

	return writen_bytes;
}


int fs_write_file(FS_FileEntry* file, u32_t offset, u32_t length, u8_t* buffer)
{

	FileSystem* file_system = NULL;
	int write_count = -1;

	if ( file && file->directory && file->directory->responsible_driver )
	{
		file_system = file->directory->responsible_driver;

		write_count = file_system->writeFile(file, offset, length, buffer);
	}

	return write_count;
}


int _flush(int fd)
{
	// sanity check
	if ( fd < 0 || fd >= MAX_FILE_HANDLERS || !file_handlers[fd] )
		return -1;
	
	int wb = 0;

	if ( file_handlers[fd]->outBuffPos > 0 )
	{
		// write to the disk
		wb = fs_write_file(file_handlers[fd]->file, file_handlers[fd]->seek_pos, file_handlers[fd]->outBuffPos, file_handlers[fd]->outBuff);
		
		file_handlers[fd]->seek_pos+= wb;
		file_handlers[fd]->outBuffPos = 0;
	}

	return wb;
	
}


int _eof(int fd)
{
	// sanity check
	if ( fd < 0 || fd >= MAX_FILE_HANDLERS || !file_handlers[fd] )
		return -1;

	return file_handlers[fd]->seek_pos >= file_handlers[fd]->file->size;
}

long _lseek(int fd, long offset, int origin)
{
	// sanity check
	if ( fd < 0 || fd >= MAX_FILE_HANDLERS || !file_handlers[fd] )
		return -1;

	switch(origin)
	{
		case SEEK_SET:
			file_handlers[fd]->seek_pos = offset % file_handlers[fd]->file->size;
			break;
		case SEEK_END:
			offset = offset < 0 ? -offset : offset;
			file_handlers[fd]->seek_pos = file_handlers[fd]->file->size - offset;
			if ( file_handlers[fd]->seek_pos < 0 )
				file_handlers[fd]->seek_pos = 0;
			break;
		default :
			file_handlers[fd]->seek_pos+= offset;
			if ( file_handlers[fd]->seek_pos < 0 )
				file_handlers[fd]->seek_pos = 0;
			break;
	}

	return file_handlers[fd]->seek_pos;

}

long _tell(int fd)
{
	// sanity check
	if ( fd < 0 || fd >= MAX_FILE_HANDLERS || !file_handlers[fd] )
		return -1;

	return file_handlers[fd]->seek_pos;

}


u32_t file_length(int fd)
{
	// sanity check
	if ( fd < 0 || fd >= MAX_FILE_HANDLERS || !file_handlers[fd] )
		return -1;

	return file_handlers[fd]->file->size;
}

