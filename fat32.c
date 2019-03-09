

#include <filesystem.h>
#include <fat32.h>
#include <ata.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <debug.h>

extern FileSystem* filesystems;

#define CLUSTER_TO_SECTOR(info, start_cluster)		info->start_lba + info->reserved_sectors + ( info->fat_count * info->sectors_per_fat ) + ( (start_cluster - 2) * info->sectors_per_cluster )

fat32* 		fat32_entry_point(int device_num, int part_id, u32_t start_lba, u32_t part_len);
FS_FileEntry* 	fat32_parse_directory(FS_Tree* node, fat32* info, u32_t start_cluster);
u32_t 		FAT32_getNextCluster(fat32* info, u32_t cluster);
u8_t 		FAT32_calcChecksum(u8_t *pName);
u32_t 		FAT32_allocateCluster(fat32* info);
u32_t 		FAT32_freeClustersChain(fat32* info, u32_t start_cluster);


u32_t FAT32_getNextCluster(fat32* info, u32_t cluster)
{
	u32_t fat_start_sector = info->start_lba + info->reserved_sectors;
	int numOfEntries = info->bytes_per_sector / 4;
	u32_t cluster_start_sector = fat_start_sector + ( cluster / numOfEntries );
	int cluster_offset =  cluster % numOfEntries;

	u32_t* buffer = (u32_t*) malloc(info->bytes_per_sector);
	memset(buffer, 0, info->bytes_per_sector);

//printf("FAT32_getNextCluster  info->sectors_per_cluster = %d\n", info->sectors_per_cluster);

	if ( !ide_read_sectors(info->drivenumber, 1, cluster_start_sector, (u32_t)buffer) )
	{
		printf("FAT-32: Could get next Cluster %d  %d !\n", cluster_start_sector, info->sectors_per_cluster);
		if (buffer) 		free(buffer);
		return EOC;
	}

	u32_t next_cluster = buffer[cluster_offset]&EOC;
	if (buffer) 		free(buffer);

	return next_cluster;
	
}

u32_t FAT32_getClusterByIndex(fat32* info, u32_t start_cluster, int cluster_index /* cluster number nth in the chain */)
{
	u32_t fat_start_sector = info->start_lba + info->reserved_sectors;
	int numOfEntries = info->bytes_per_sector / 4, index = 0, cluster_offset;
	u32_t next_cluster = start_cluster, cluster_start_sector, tmp;
	u32_t* buffer = (u32_t*) malloc(info->bytes_per_sector);

	while ( next_cluster < 0x0FFFFFF0 && index < cluster_index )
	{	
		cluster_start_sector = fat_start_sector + ( next_cluster / numOfEntries );
		cluster_offset =  next_cluster % numOfEntries;

		memset(buffer, 0, info->bytes_per_sector);

//printf("FAT32_getNextCluster  info->sectors_per_cluster = %d\n", info->sectors_per_cluster);

		if ( !ide_read_sectors(info->drivenumber, 1, cluster_start_sector, (u32_t)buffer) )
		{
			printf("FAT-32: Could get next Cluster %d %d when loading directory!\n", cluster_start_sector, info->sectors_per_cluster);
			if (buffer) 		free(buffer);
			return EOC;
		}

		do{
			next_cluster = buffer[cluster_offset]&EOC;
			index++;
		
			tmp = fat_start_sector + ( next_cluster / numOfEntries );
			cluster_offset =  next_cluster % numOfEntries;
		}while(next_cluster < 0x0FFFFFF0 && index < cluster_index && cluster_start_sector == tmp );

	}


	if (buffer) 		free(buffer);

	return next_cluster;
	
}

u32_t FAT32_allocateCluster(fat32* info)
{
	
	u32_t free_cluster = EOC, last_cluster;	//, next_free_cluster = EOC;
	u32_t fat_start_sector = info->start_lba + info->reserved_sectors;
	int numOfEntries = info->bytes_per_sector / 4;		// entries per sector
	int fat_sector = 0,fat_sector_offset = 0, i, found = 0;

	last_cluster = info->fs_info->next_free_cluster;

//printf("FAT32_allocateCluster .. free_cluster = %d\n",free_cluster);
//printf("FAT32_getNextCluster  info->sectors_per_cluster = %d\n", info->sectors_per_cluster);
	u32_t* buffer = (u32_t*) malloc(info->bytes_per_sector);
	fat_sector = last_cluster / numOfEntries;
	fat_sector_offset = (last_cluster % numOfEntries) + 1;
	if ( fat_sector_offset >= numOfEntries )
	{
		fat_sector++;
		fat_sector_offset = 0;
	}

//	fat_sector = 0;
//	fat_sector_offset = 0;

	// find the next free cluster
	while ( !found && fat_sector < info->sectors_per_fat )
	{
		memset(buffer, 0, info->bytes_per_sector);

		if ( !ide_read_sectors(info->drivenumber, 1, fat_start_sector + fat_sector, (u32_t)buffer) )
		{
			printf("FAT-32: Could get next Sector 0x%x+0x%x when Scanning FAT!\n", fat_start_sector + fat_sector, info->sectors_per_cluster);
			if (buffer) 		free(buffer);
			return EOC;
		}


		for(i=fat_sector_offset;i<numOfEntries;++i)
		{
			// just for safty	
			if ( ( fat_sector == 0 && i == 0 ) || (fat_sector == 0 && i == 1) ) 	continue;

			if ( buffer[i] == 0 )
			{
				//next_free_cluster = (fat_sector * numOfEntries) + i;
				free_cluster = (fat_sector * numOfEntries) + i;

				// set it as a last cluster ( allocate it)
				buffer[i] = EOC;
				
				if ( !ide_write_sectors(info->drivenumber, 1, fat_start_sector + fat_sector, (u32_t)buffer) )
				{
					printf("FAT-32: Could get next Sector 0x%x+0x%x when Scanning FAT!\n", fat_start_sector + fat_sector, info->sectors_per_cluster);
					if (buffer) 		free(buffer);
					return EOC;
				}


				found = 1;
				break;
			}
			
		}
 
		fat_sector++;
		fat_sector_offset = 0;
	}

	
	info->fs_info->free_cluster_count--;
//	info->fs_info->next_free_cluster = next_free_cluster;
	info->fs_info->next_free_cluster = free_cluster;

	// syncronize FS info
	synchronizeFS_Info(info);
	
	if (buffer) 		free(buffer);

	return free_cluster;


}

u32_t FAT32_freeClustersChain(fat32* info, u32_t start_cluster)
{
	
	if ( start_cluster == 0 )	return start_cluster;

	u32_t next_free = start_cluster;
	u32_t fat_start_sector = info->start_lba + info->reserved_sectors;
	int numOfEntries = info->bytes_per_sector / 4;		// entries per sector
	int fat_sector = 0,fat_sector_offset = 0, i;
	int this_fat_sector;

//printf("FAT32_freeCluster .. next_free = %d\n",next_free);

	u32_t* buffer = (u32_t*) malloc(info->bytes_per_sector);
	fat_sector = next_free / numOfEntries;
	fat_sector_offset = next_free % numOfEntries;

//printf("FAT32_freeClustersChain next_free = %d\n", next_free);
//printf("FAT32_freeClustersChain info->sectors_per_cluster = %d\n", info->sectors_per_cluster);
	// find the next free cluster
	while ( next_free < 0x0FFFFFF0 && fat_sector < info->sectors_per_fat )
	{
		memset(buffer, 0, info->bytes_per_sector);

		if ( !ide_read_sectors(info->drivenumber, 1, fat_start_sector + fat_sector, (u32_t)buffer) )
		{
			printf("FAT-32: Could get next Sector 0x%x+0x%x when Scanning FAT!\n", fat_start_sector + fat_sector, info->sectors_per_cluster);
			if (buffer) 		free(buffer);
			return EOC;
		}


		do{
			next_free = buffer[fat_sector_offset];
//printf("******* ---->> next_free = %d\n", next_free);
			buffer[fat_sector_offset] = 0;		// free this cluster
			info->fs_info->free_cluster_count++;

			if ( next_free < 0x0FFFFFF0 && next_free < info->fs_info->next_free_cluster ) 
				info->fs_info->next_free_cluster = next_free - 1;		// in the fact, it is the last allocated	
			
			this_fat_sector = next_free / numOfEntries;
			fat_sector_offset = next_free % numOfEntries;
		}while ( next_free < 0x0FFFFFF0 && this_fat_sector == fat_sector );

		
		if ( !ide_write_sectors(info->drivenumber, 1, fat_start_sector + fat_sector, (u32_t)buffer) )
		{
			printf("FAT-32: Could get next Sector 0x%x+0x%x when Scanning FAT!\n", fat_start_sector + fat_sector, info->sectors_per_cluster);
			if (buffer) 		free(buffer);
			return EOC;
		}

		fat_sector = this_fat_sector;

	}

	// syncronize FS info
	synchronizeFS_Info(info);
	
	if (buffer) 		free(buffer);

	return next_free;


}

void synchronizeFS_Info(fat32* info)
{

	u32_t info_sector = info->start_lba + info->FSInfo_sector_number;
	if ( !ide_write_sectors(info->drivenumber, 1, info_sector, (u32_t)info->fs_info) )
	{
		printf("FAT-32: Could not Write LBA sectors 0x%x+0x%x when synchronize FS_Info!\n", info_sector, 1);
	}
	
}

FS_FileEntry* fat32_parse_directory(FS_Tree* node, fat32* info, u32_t start_cluster)
{
	
	FS_FileEntry* list = NULL;
	FAT32_Dir_Entry* fat32_entry = NULL;
	FAT32_LFN_Entry* longFile_entry = NULL;

//printf("inside fat32_parse_directory ... \n");
	int cluster_size = info->sectors_per_cluster * info->bytes_per_sector;
	u32_t next_cluster = start_cluster;
	u8_t* dirbuffer = (u8_t*) malloc(cluster_size);
	char* fileName = NULL;
	int reach_end_marker = 0, i, j, serial, file_len, file_index;
	
	FS_FileEntry* current_entry = NULL;
	while ( next_cluster < 0x0FFFFFF0 )
	{

//printf("inside fat32_parse_directory ... next_cluster = %d\n", next_cluster);

		memset(dirbuffer, 0, cluster_size);

		if ( !ide_read_sectors(info->drivenumber, info->sectors_per_cluster, CLUSTER_TO_SECTOR(info, next_cluster), (u32_t)dirbuffer) )
		{
			printf("FAT-32: Could not read LBA sectors 0x%x+0x%x when loading directory!\n", next_cluster, info->sectors_per_cluster);
			if (dirbuffer) 		free(dirbuffer);
			return NULL;
		}

		for (i=0;i<cluster_size;i+=FAT32_ENTRY_RECORD_LEN)
		{
			

			// 1) check for end marker of the directory scanner and check weather this entry is valid or deleted and not . , .. entries
			//if ( fat32_entry->name[0] != 0 && fat32_entry->name[0] != FAT32_DELETED_ENTRY && fat32_entry->name[0] != FAT32_DOT_ENTRY)
			if ( dirbuffer[i] != 0 && dirbuffer[i] != FAT32_DELETED_ENTRY && dirbuffer[i] != FAT32_DOT_ENTRY)
			{

				fat32_entry = (FAT32_Dir_Entry*) (dirbuffer + i);
//printf("*** i = %d \n", i);
//dump_hexa(fat32_entry, 32);
/*
			char ch = (char) fat32_entry->name[0];
printf("*** fat32_entry->name[0] = %2x   len = %d ch = %2x   %2x \n", fat32_entry->name[0], sizeof(fat32_entry->name[0]) , ch, dirbuffer[i]);
*/

				// 2) check if the entry is volume lable
				if ( fat32_entry->file_attr == FAT32_VOL_LABEL_ENTRY )	continue;	// ignore volume table

//dump_hexa(fat32_entry, 32);
//sleep(3000);
//while(1);
				// 3) check the type of the entry
				// 3.1)	LFN entry		; long file name
				if ( fat32_entry->file_attr == FAT32_LFN_ENTRY )
				{

					// cast the structure
//printf("FAT-32: inside long file name entry ... \n");
//dump_hexa(fat32_entry, 32);
//while(1);
					longFile_entry = (FAT32_LFN_Entry*) fat32_entry;
					serial = longFile_entry->serial_number & 0x0F;
					if (longFile_entry->serial_number & 0x40)
					{
						// free the last name
						if (fileName) 	free(fileName);
						fileName = NULL;
						file_len = (serial * 13 ) + 1;
						fileName = (char*) malloc(file_len);
						memset(fileName, 0, file_len);
						
					}
					file_index = 13 * (serial-1);

					for(j=0;j<5;j++)	fileName[file_index++] = longFile_entry->name1[j];			
					for(j=0;j<6;j++)	fileName[file_index++] = longFile_entry->name2[j];	
					for(j=0;j<2;j++)	fileName[file_index++] = longFile_entry->name3[j];	

		
				}
				else	// 3.2) normal record 8.3 format
				{
		
					current_entry = (FS_FileEntry*) malloc(sizeof(FS_FileEntry));
					memset(current_entry, 0, sizeof(FS_FileEntry));
					current_entry->filename = (char*) malloc(file_len);
					memset(current_entry->filename, 0, file_len);
					strcpy(current_entry->filename, fileName);
//printf("fat32_parse_directory file name = %s\n", current_entry->filename);
//dump_hexa(current_entry->filename, file_len);
//dump_hexa(fat32_entry, 32);
//printf("inside fat32_parse_directory ... file name = %s   ch[0] = %2x\n", current_entry->filename, (char)fat32_entry->name[0]);


					current_entry->year = ((fat32_entry->creation_date & 0xFE00) >> 9 ) + 127;
					current_entry->month = (fat32_entry->creation_date & 0x01E0) >> 5;
					current_entry->day = fat32_entry->creation_date & 0x001F;
					current_entry->hour = (fat32_entry->creation_time & 0xF800) >> 11;
					current_entry->min = (fat32_entry->creation_time & 0x07E0) >> 5;
					current_entry->sec = fat32_entry->creation_time & 0x001F;

					current_entry->lbapos = ((u32_t)fat32_entry->cluster_hi << 16) | (u32_t)fat32_entry->cluster_lo;
					current_entry->device = info->drivenumber;
					current_entry->size = fat32_entry->file_size;
//printf("fat32_parse_directory size = %d\n", current_entry->size);

//printf("inside fat32_parse_directory ... file attr = %x\n", fat32_entry->file_attr);
		
//printf("fat32_parse_directory file name = %s  cluster number = %d\n", current_entry->filename, current_entry->lbapos);

			
					if ( fat32_entry->file_attr == 0x10 )	// is directory
						current_entry->flags |= FS_DIRECTORY;	// directory file

					current_entry->directory = node;

					// append to the list
					current_entry->next = list;
					list = current_entry;
					
				}
			}
			else if ( dirbuffer[i] == 0 )
			{
				reach_end_marker = 1;
				break;			// break for loop
			}
		
		}	// end of for loop
		
		if ( reach_end_marker ) 	break;

		// get next cluster
		next_cluster = FAT32_getNextCluster(info, next_cluster);

	}

	if (dirbuffer) 		free(dirbuffer);
	if (fileName) 	free(fileName);

//while(1);
	return list;
}


fat32* fat32_entry_point(int device_num, int part_id, u32_t start_lba, u32_t part_len)
{
	
	fat32* driver_info = (fat32*) malloc(sizeof(fat32));
	memset(driver_info , 0, sizeof(fat32));
	driver_info->drivenumber = device_num;
	driver_info->partition_id = part_id;
	driver_info->start_lba = start_lba;
	driver_info->partition_len = part_len;		// len in sectors units


	PartitionBootRecord* pb = (PartitionBootRecord*) malloc(sizeof(PartitionBootRecord));
	memset(pb , 0, sizeof(PartitionBootRecord));

	if ( !ide_read_sectors(device_num, 1, start_lba, (u32_t)pb) )
	{
		printf("Could not read Partition boot Record!\n");
		if (driver_info) 	free(driver_info);
		if (pb) 	free(pb);
		return NULL;
	}

	if ( pb->signature == BOOT_SIGNATURE && ( pb->sig == 0x28 || pb->sig == 0x29 ) )
	{

/*
		printf("bytes_per_sector = %d\n", pb->bytes_per_sector);
		printf("sectors_per_cluster = %d\n", pb->sectors_per_cluster);
		printf("fat_count = %d\n", pb->fat_count);
		printf("start_lba = %d\n", pb->start_lba);
		printf("sectors_per_fat = %d\n", pb->sectors_per_fat);
		printf("root_dir_cluster = %d\n", pb->root_dir_cluster);
		printf("FSInfo_sector_number = %d\n", pb->FSInfo_sector_number);
		printf("label = %s\n", pb->label);
		printf("sys_id_string = %s\n", pb->sys_id_string);
		printf("signature = 0x%x\n\n", pb->signature);
*/

		driver_info->start_lba = pb->start_lba;
		driver_info->bytes_per_sector = pb->bytes_per_sector;
		driver_info->reserved_sectors = pb->reserved_sectors;
		driver_info->sectors_per_cluster = pb->sectors_per_cluster;
		driver_info->fat_count = pb->fat_count;
		driver_info->sectors_per_fat = pb->sectors_per_fat;	
		driver_info->root_dir_cluster = pb->root_dir_cluster;
		driver_info->FSInfo_sector_number = pb->FSInfo_sector_number;

		driver_info->partition_label = (char*) malloc(12);
		memset(driver_info->partition_label, 0, 12);
		strncpy(driver_info->partition_label, pb->label, 11);

		// read fat32 FSInfo sector
		driver_info->fs_info = (FSInfo*) malloc(sizeof(FSInfo));
		if ( !ide_read_sectors(device_num, 1, driver_info->start_lba + driver_info->FSInfo_sector_number, (u32_t)driver_info->fs_info) || driver_info->fs_info->boot_sig != BOOT_SIGNATURE || driver_info->fs_info->signature != FSINFO_SIGNATURE_1 || driver_info->fs_info->signature_2 != FSINFO_SIGNATURE_2 )
		{
			printf("Could not read FS Information of the partition!\n");
			if (driver_info->fs_info) 	free(driver_info->fs_info);
			if (driver_info) 	free(driver_info);
			if (pb) 	free(pb);
			return NULL;
		}
/*
		else
		{
			printf("signature = 0x%x\n", driver_info->fs_info->signature);
			printf("signature_2 = 0x%x\n", driver_info->fs_info->signature_2);
			printf("free_cluster_count = %d\n", driver_info->fs_info->free_cluster_count);
			printf("next_free_cluster = %d\n", driver_info->fs_info->next_free_cluster);
			printf("boot signature = 0x%x\n", driver_info->fs_info->boot_sig);

		}
*/
		driver_info->root = fat32_parse_directory(NULL, driver_info, driver_info->root_dir_cluster);


	}
	else
	{
		printf("Invalid Partition boot Record!\n");
		if (driver_info) 	free(driver_info);
		if (pb) 	free(pb);
		return NULL;	
	}

	if (pb) 	free(pb);	

	return driver_info;
}





FileSystem* mount_fat32(int device_num, int part_id, int start_lba, int part_len)
{

	printf("Try to mount FAT-32 partition # %d, device number = %d\n",part_id, device_num);

	FileSystem* filesystem = (FileSystem*) malloc(sizeof(FileSystem));
	memset(filesystem, 0, sizeof(FileSystem));

	strcpy(filesystem->name, FAT32_FS);
	filesystem->device_num = device_num;
	filesystem->permission = FILESYSTEM_PREM_READ_WRITE;

	filesystem->readFile = fat32_read_file;
	filesystem->writeFile = fat32_write_file;
	filesystem->getDirectory = fat32_get_directory;		// to list directory
	filesystem->createFile = fat32_create_fat_entry;
	filesystem->removeFile = fat32_remove_fat_entry;

	fat32* driver_info = fat32_entry_point(device_num, part_id, start_lba, part_len);	

	if ( driver_info == NULL )
	{
		printf("Error in trying to mount FAT-32 file system, device number = %d\n",device_num);
		if ( filesystem ) 	free(filesystem);
		filesystem = NULL;
	}
	else
	{
		filesystem->opaque = (void *) driver_info;
		filesystem->next = filesystems;
		filesystems = filesystem;
	}


	return filesystem;	

}

int fat32_read_file(void* file, u32_t start_offset /* start offset */, u32_t length, u8_t* buffer)
{
	FS_FileEntry* file_entry = (FS_FileEntry*) file;
	fat32* info = (fat32*) file_entry->directory->responsible_driver->opaque;

	u32_t next_cluster;	// start cluster number
	int remaining_bytes = length, cluster_count = 0, cluster_offset, data_len, read_bytes = 0;
	int cluster_size = info->bytes_per_sector * info->sectors_per_cluster;

	u8_t* cluster_data = (u8_t*) malloc(cluster_size);

//printf("fat32_read_file start_offset  = %d length = %d\n", start_offset, length);

	int cluster_index = start_offset / cluster_size;
	cluster_offset = start_offset % cluster_size;
	next_cluster = FAT32_getClusterByIndex(info, file_entry->lbapos, cluster_index);

//printf("fat32_read_file (***) file_entry->lbapos = %d\n", file_entry->lbapos);	
//printf("fat32_read_file (***) next_cluster = %d\n", next_cluster);

	int len = length;
	if ( cluster_offset )
	{
		cluster_count++;
		len = len - (cluster_size - cluster_offset);
	}
	cluster_count+= len / cluster_size;
	if ( len % cluster_size ) cluster_count++;
//printf("fat32_read_file start_offset  = %d length = %d cluster_count = %d\n", start_offset, length, cluster_count);	
	while( remaining_bytes > 0 && cluster_count > 0 && next_cluster < 0x0FFFFFF0 )
	{
		// read cluster
		//memset(cluster_data, 0, cluster_size);

//printf("fat32_read_file next_cluster = %d\n", next_cluster);
//if (next_cluster == 0 ) while(1);

		if ( !ide_read_sectors(info->drivenumber, info->sectors_per_cluster, CLUSTER_TO_SECTOR(info, next_cluster), (u32_t)cluster_data) )
		{
			printf("FAT-32: Could not read LBA sectors %d  %d when reading file!\n", next_cluster, info->sectors_per_cluster);
			if (cluster_data) 		free(cluster_data);
			return -1;
		}
		data_len = cluster_size - cluster_offset;
		if ( remaining_bytes < data_len )	data_len = remaining_bytes;
		memcpy(buffer+read_bytes, cluster_data+cluster_offset, data_len);

		read_bytes+= data_len;
		remaining_bytes-= data_len;
		cluster_offset = 0;

		// get next cluster
		next_cluster = FAT32_getNextCluster(info, next_cluster);
		cluster_count--;

//printf("remaining_bytes = %d next_cluster = %x cluster_count = %d\n", remaining_bytes, next_cluster, cluster_count);
	} 
	
	if (cluster_data)	free(cluster_data);	

	return read_bytes;		
}


int fat32_write_file(void* file, u32_t start_offset /* start offset */, u32_t length, u8_t* buffer)
{
	FS_FileEntry* file_entry = (FS_FileEntry*) file;
	if ( start_offset > file_entry->size )		return -1;

	
	FS_Tree* dir_node = file_entry->directory;
	fat32* info = (fat32*) file_entry->directory->responsible_driver->opaque;

	u32_t next_cluster, perior_cluster;
	int remaining_bytes = length, cluster_count = 0, cluster_offset, data_len,  writtenbytes = 0;
	int cluster_size = info->bytes_per_sector * info->sectors_per_cluster;
	u8_t* cluster_data = (u8_t*) malloc(cluster_size);
//printf("fat32_write_file start_offset = %d length = %d\n", start_offset, length);
	if( file_entry->size == 0 && file_entry->lbapos == 0)	// the file is empty
	{
		next_cluster = FAT32_allocateCluster(info);
		file_entry->lbapos = next_cluster;
//		printf("fat32_write_file after allocation start_cluster = %d .\n", next_cluster);
		update_entry_in_directory_table(info, dir_node->lbapos, FAT32_DIR_TABLE_ENTRY_UPDATE, file_entry->filename, FIELD_CLUSTER_NUM, &next_cluster);
	}
//printf("fat32_write_file (***) 222\n");
	int cluster_index = start_offset / cluster_size;
	cluster_offset = start_offset % cluster_size;
	next_cluster = perior_cluster = file_entry->lbapos;
	if( cluster_index )	
	{
		perior_cluster = FAT32_getClusterByIndex(info, file_entry->lbapos, cluster_index - 1);
		next_cluster = FAT32_getNextCluster(info, perior_cluster);
	}
	
//printf("fat32_write_file (***) 333 next_cluster = %d \n", next_cluster);	
	if ( cluster_offset )
	{
//printf("fat32_write_file (***) 444 next_cluster = %d \n", next_cluster);
		// read cluster
		//memset(cluster_data, 0, cluster_size);
		if ( !ide_read_sectors(info->drivenumber, info->sectors_per_cluster, CLUSTER_TO_SECTOR(info, next_cluster), (u32_t)cluster_data) )
		{
			printf("FAT-32: Could not read LBA sectors 0x%x+0x%x when reading file!\n", next_cluster, info->sectors_per_cluster);
			if (cluster_data) 		free(cluster_data);
			return -1;
		}

		data_len = cluster_size - cluster_offset;
		if ( remaining_bytes < data_len )	data_len = remaining_bytes;
		memcpy(cluster_data+cluster_offset, buffer+writtenbytes, data_len);
		writtenbytes+= data_len;
		remaining_bytes-= data_len;
		cluster_offset = 0;
		if ( !ide_write_sectors(info->drivenumber, info->sectors_per_cluster, CLUSTER_TO_SECTOR(info, next_cluster), (u32_t)cluster_data) )
		{
			printf("FAT-32: Could not read LBA sectors 0x%x+0x%x when reading file!\n", next_cluster, info->sectors_per_cluster);
			if (cluster_data) 		free(cluster_data);
			return -1;
		}
		
		// get next cluster
		perior_cluster = next_cluster;
		next_cluster = FAT32_getNextCluster(info, perior_cluster);
	}

	cluster_count = remaining_bytes / cluster_size;
	if ( remaining_bytes % cluster_size ) cluster_count++;
//printf("fat32_write_file (***) 555 cluster_count = %d \n", cluster_count);	
	while( remaining_bytes > 0 && cluster_count > 0 )
	{
		
		// check if we reach to the end of cluster chain of the file
		if ( next_cluster >= 0x0FFFFFF0 )	// it is the last cluster in the file chain
		{
			next_cluster = FAT32_allocateCluster(info);
			//printf("try to aloc next_cluster = %d \n", next_cluster);

			// add it to the linked list of the directory table
			FAT32_appendCluster(info, perior_cluster, next_cluster);
		}

		data_len = remaining_bytes < cluster_size ? remaining_bytes : cluster_size;
//printf("fat32_write_file (***) 555 data_len = %d \n", data_len);
		memcpy(cluster_data, buffer+writtenbytes, data_len);
		writtenbytes+= data_len;
		remaining_bytes-= data_len;

		if ( !ide_write_sectors(info->drivenumber, info->sectors_per_cluster, CLUSTER_TO_SECTOR(info, next_cluster), (u32_t)cluster_data) )
		{
			printf("FAT-32: Could not read LBA sectors 0x%x+0x%x when reading file!\n", next_cluster, info->sectors_per_cluster);
			if (cluster_data) 		free(cluster_data);
			return -1;
		}

		perior_cluster = next_cluster;
		next_cluster = FAT32_getNextCluster(info, perior_cluster);

		cluster_count--;
	} 
	
	// update file size
	file_entry->size+= writtenbytes;
	update_entry_in_directory_table(info, dir_node->lbapos, FAT32_DIR_TABLE_ENTRY_UPDATE, file_entry->filename, FIELD_FILE_SIZE, &(file_entry->size));

	if (cluster_data)	free(cluster_data);	

	return writtenbytes;		
}



void* fat32_get_directory(void* t)
{
	FS_Tree* dir_node = (FS_Tree*) t;

	FS_FileEntry* childs = fat32_parse_directory(dir_node, dir_node->responsible_driver->opaque, dir_node->lbapos /* start cluster number */ );

	return ((void*) childs);	
}


int fat32_create_fat_entry(void* t)
{
	FS_FileEntry* file = (FS_FileEntry*) t;
	FS_Tree* dir_node = file->directory;
	fat32* info = (fat32*) dir_node->responsible_driver->opaque;
	u8_t fileName_checkSum = 0;
	int i, ret = 0;
	
	FAT32_Dir_Entry* fat32_entry = (FAT32_Dir_Entry*) malloc(sizeof(FAT32_Dir_Entry));
	memset(fat32_entry, 0, sizeof(FAT32_Dir_Entry));

	// 8.3 file name format
	memset(fat32_entry->name, 0x20, 8);
	memset(fat32_entry->ext, 0x20, 2);

	int len = strlen(file->filename);
	char* file_name = (char*) malloc(len+1);
	memset(file_name, 0, len+1);
	strcpy(file_name, file->filename);

	// convert it to upper letter
	for(i=0;i<len;++i)
		file_name[i] = toupper(file_name[i]);

	char* file_ext = strrchr(file_name, '.');
	if( file_ext && !(file->flags & FS_DIRECTORY) )		// has extension and it isn't a directory
	{
		*file_ext = 0;
		file_ext++;
		len = strlen(file_ext);
		len = ( len > 3 ) ? 3 : len;
		strncpy(fat32_entry->ext, file_ext, len); 
	}

	len = strlen(file_name);
	len = ( len > 8 ) ? 8 : len;
	strncpy(fat32_entry->name, file_name, len);

	// set file type
	fat32_entry->file_attr = ( file->flags & FS_DIRECTORY ) ? 0x10 : 0x20;


	// reformat the creation date/time according to FAT-32 format
	fat32_entry->creation_date = (file->year - 1980) << 9;
	fat32_entry->creation_date|= file->month << 5;
	fat32_entry->creation_date|= file->day;
	fat32_entry->creation_time = file->hour << 11;
	fat32_entry->creation_time|= file->min << 5;
	fat32_entry->creation_time|= file->sec;

	// if the file is of directory type, so create empty directory table for it and update cluster number
	fat32_entry->cluster_hi = fat32_entry->cluster_lo = 0;

	fat32_entry->file_size = 0;


	// define file type ( ordinary file or directory )
	if ( file->flags & FS_DIRECTORY )	// is directory
	{		
		// create raw cluster and update cluster_hi and cluster_lo
		u32_t cluster = FAT32_allocateCluster(info);

		if ( cluster < 0x0FFFFFF0 )
		{

			int cluster_size = info->sectors_per_cluster * info->bytes_per_sector;
			u8_t* buffer = (u8_t*) malloc(cluster_size);
			memset(buffer, 0, cluster_size);
			ret = FAT32_saveCluster(info, cluster, buffer);

			// add it to the linked list of the directory table
			//FAT32_appendCluster(info, 0 , cluster);		// cluster is the first cluster

			// set the cluster number in the fat32 entry record
			fat32_entry->cluster_hi = (u16_t) ( cluster >> 16);
			fat32_entry->cluster_lo = (u16_t) cluster;

			// set cluster number in the directory structure tree
			file->lbapos = cluster;

			if ( buffer ) 	free(buffer);

		}
	} 

	// calculate check sum to be used in LFN entries
	u8_t* pName = (u8_t*) malloc(11);
	strncpy(pName, fat32_entry->name, 8);
	strncpy(pName+8, fat32_entry->ext, 3);
	fileName_checkSum = FAT32_calcChecksum(pName);
	if ( pName ) 	free(pName);


	len = strlen(file->filename);
	int LFN_entriesCount = len / 13;
	if ( len % 13 ) ++LFN_entriesCount;

	// prepare unicode string
	int uniNameLen = 13 * LFN_entriesCount;
	u16_t* uniName = (u16_t*) malloc(uniNameLen * sizeof(u16_t));	
	for(i = 0;i<len;++i)	uniName[i] = (u16_t) file->filename[i];
	if ( i < uniNameLen )	uniName[i++] = (u16_t) 0;
	while ( i < uniNameLen )	uniName[i++] = -1;//0xFFFF;

	FAT32_LFN_Entry* longFile_entry = (FAT32_LFN_Entry*) malloc(sizeof(FAT32_LFN_Entry));

	int uniNameIndex;
	for(i=LFN_entriesCount;i>0;i--)
	{

		uniNameIndex = (i-1) * 13;

		longFile_entry->name1[0] = uniName[uniNameIndex++];
		longFile_entry->name1[1] = uniName[uniNameIndex++];
		longFile_entry->name1[2] = uniName[uniNameIndex++];
		longFile_entry->name1[3] = uniName[uniNameIndex++];
		longFile_entry->name1[4] = uniName[uniNameIndex++];

		longFile_entry->name2[0] = uniName[uniNameIndex++];
		longFile_entry->name2[1] = uniName[uniNameIndex++];
		longFile_entry->name2[2] = uniName[uniNameIndex++];
		longFile_entry->name2[3] = uniName[uniNameIndex++];
		longFile_entry->name2[4] = uniName[uniNameIndex++];
		longFile_entry->name2[5] = uniName[uniNameIndex++];

		longFile_entry->name3[0] = uniName[uniNameIndex++];
		longFile_entry->name3[1] = uniName[uniNameIndex++];
	
		// set serial number
		longFile_entry->serial_number = i;
		if ( i == LFN_entriesCount )	longFile_entry->serial_number|= 0x40;
		
		longFile_entry->attr = FAT32_LFN_ENTRY;
		longFile_entry->type = 0;
		longFile_entry->check_sum = fileName_checkSum;
		longFile_entry->fCluster = 0;

		ret = create_new_entry_in_directory_table(info, dir_node->lbapos, longFile_entry);

		if ( ret == -1 ) break;

	}

	if (ret >= 0 )
		ret = create_new_entry_in_directory_table(info, dir_node->lbapos, fat32_entry);

	if ( file_name ) 	free(file_name);
	if ( uniName ) 	free(uniName);
	if ( longFile_entry ) 	free(longFile_entry);
	if ( fat32_entry ) 	free(fat32_entry);

	return ret;
}


u8_t FAT32_calcChecksum(u8_t *pName)
{
   	int i;
   	u8_t sum = 0;
 
   	for (i = 11; i; i--)
      	sum = ((sum & 1) << 7) + (sum >> 1) + *pName++;
 
   	return sum;
}


int create_new_entry_in_directory_table(fat32* info, u32_t start_cluster, void* entry)
{
	int ret = 0;
	int cluster_size = info->sectors_per_cluster * info->bytes_per_sector;
	u32_t next_cluster = start_cluster;
	u8_t* dirbuffer = (u8_t*) malloc(cluster_size);
	int written = 0, i;
	memset(dirbuffer, 0, cluster_size);
	
	while ( !written && next_cluster < 0x0FFFFFF0 )
	{

		if ( !ide_read_sectors(info->drivenumber, info->sectors_per_cluster, CLUSTER_TO_SECTOR(info, next_cluster), (u32_t)dirbuffer) )
		{
			printf("FAT-32: Could not read LBA sectors %d %d when loading directory table!\n", next_cluster, info->sectors_per_cluster);
			if (dirbuffer) 		free(dirbuffer);
			return -1;
		}

		for (i=0;i<cluster_size;i+=FAT32_ENTRY_RECORD_LEN)
		{
			if ( dirbuffer[i] == 0 )	// empty entry
			{
				memcpy(dirbuffer+i, entry, FAT32_ENTRY_RECORD_LEN);
				ret = FAT32_saveCluster(info, next_cluster, dirbuffer);
				written = 1;
				break;
			}	// end if 

		}	// end for loop	

		next_cluster = FAT32_getNextCluster(info, next_cluster);

	}	// end while loop


	if( !written )	// it doesn't written, there is no room for it
	{
		next_cluster = FAT32_allocateCluster(info);
		if ( next_cluster < 0x0FFFFFF0 )
		{
			memset(dirbuffer, 0, cluster_size);
			memcpy(dirbuffer, entry, FAT32_ENTRY_RECORD_LEN);
			ret = FAT32_saveCluster(info, next_cluster, dirbuffer);

			// add it to the linked list of the directory table
			FAT32_appendCluster(info, start_cluster, next_cluster);

		}


	}

	if (dirbuffer) free(dirbuffer);	
	
	return ret;
}

int FAT32_saveCluster(fat32* info, u32_t cluster, u8_t* buffer)
{
	
	if ( !ide_write_sectors(info->drivenumber, info->sectors_per_cluster, CLUSTER_TO_SECTOR(info, cluster), (u32_t)buffer) )
	{
		printf("FAT-32: Could not Write LBA sectors 0x%x+0x%x when saving cluster!\n", cluster, info->sectors_per_cluster);
		return -1;
	}

	return 0;
}

int FAT32_appendCluster(fat32* info, u32_t start_cluster, u32_t new_cluster)
{

	u32_t next_cluster = start_cluster, cluster = start_cluster;

	u32_t cluster_start_sector, fat_start_sector;
	int cluster_offset,numOfEntries = info->bytes_per_sector / 4;
	u32_t* buffer = (u32_t*) malloc(info->bytes_per_sector);


	fat_start_sector = info->start_lba + info->reserved_sectors;

	if( next_cluster != 0 )
	{

		// append cluster in the FAT table
		while ( next_cluster < 0x0FFFFFF0 )
		{
			cluster = next_cluster;
			next_cluster = FAT32_getNextCluster(info, cluster);
		}

		// make the cluster to point to the new cluster
		cluster_start_sector = fat_start_sector + ( cluster / numOfEntries );
		cluster_offset =  cluster % numOfEntries;
		memset(buffer, 0, info->bytes_per_sector);

		if ( !ide_read_sectors(info->drivenumber, 1, cluster_start_sector, (u32_t)buffer) )
		{
			printf("FAT-32: Could get next Cluster 0x%x+0x%x when FAT32_appendCluster!\n", cluster_start_sector, info->sectors_per_cluster);
			if (buffer) 		free(buffer);
			return -1;
		}	
		buffer[cluster_offset] = new_cluster;
		if ( !ide_write_sectors(info->drivenumber, 1, cluster_start_sector, (u32_t)buffer) )
		{
			printf("FAT-32: Could get next Cluster 0x%x+0x%x when FAT32_appendCluster\n", cluster_start_sector, info->sectors_per_cluster);
			if (buffer) 		free(buffer);
			return -1;
		}
	}

/*	
	// set the new_cluster as EOC
	cluster_start_sector = fat_start_sector + ( new_cluster / numOfEntries );
	cluster_offset =  new_cluster % numOfEntries;	
	memset(buffer, 0, info->bytes_per_sector);
	if ( !ide_read_sectors(info->drivenumber, 1, cluster_start_sector, (u32_t)buffer) )
	{
		printf("FAT-32: Could get next Cluster 0x%x+0x%x when loading directory!\n", cluster_start_sector, info->sectors_per_cluster);
		if (buffer) 		free(buffer);
		return -1;
	}
	buffer[cluster_offset] = EOC;
	if ( !ide_write_sectors(info->drivenumber, 1, cluster_start_sector, (u32_t)buffer) )
	{
		printf("FAT-32: Could get next Cluster 0x%x+0x%x when loading directory!\n", cluster_start_sector, info->sectors_per_cluster);
		if (buffer) 		free(buffer);
		return -1;
	}
*/
	if (buffer) 		free(buffer);
	return 0;
			
}


int update_entry_in_directory_table(fat32* info, u32_t start_cluster, int mode /**/, char* file_name, int updated_field, void* field_value)
{

	int cluster_size = info->sectors_per_cluster * info->bytes_per_sector;

	u32_t next_cluster, perior_cluster = 0;
	u8_t* dirbuffer = (u8_t*) malloc(cluster_size);
	int i, ret = 0, entry_updated = 0, lfn_entries_deleted = 0, len, matched = 0, matched_parts = 0;

	FAT32_Dir_Entry* fat32_entry = NULL;
	FAT32_LFN_Entry* lfn_entry = NULL;
	
	// bulid unide string file name
	len = strlen(file_name);
	int LFN_entriesCount = len / 13;
	if ( len % 13 ) ++LFN_entriesCount;

	// prepare unicode string
	int uniNameLen = 13 * LFN_entriesCount;
	u16_t* uniName = (u16_t*) malloc(uniNameLen * sizeof(u16_t));	
	for(i = 0;i<len;++i)	uniName[i] = (u16_t) file_name[i];
	if ( i < uniNameLen )	uniName[i++] = (u16_t) 0;
	while ( i < uniNameLen )	uniName[i++] = -1;//0xFFFF;

//printf("update_entry_in_directory_table   start_cluster = %d\n", start_cluster);

	next_cluster = start_cluster;
	while ( !entry_updated && next_cluster < 0x0FFFFFF0 )
	{

		memset(dirbuffer, 0, cluster_size);

		if ( !ide_read_sectors(info->drivenumber, info->sectors_per_cluster, CLUSTER_TO_SECTOR(info, next_cluster), (u32_t)dirbuffer) )
		{
			printf("FAT-32: Could not read LBA sectors 0x%x+0x%x when update_entry!\n", next_cluster, info->sectors_per_cluster);
			if (dirbuffer) 		free(dirbuffer);
			return NULL;
		}

		for (i=0;i<cluster_size;i+=FAT32_ENTRY_RECORD_LEN)
		{
			// check for LFN
			if ( dirbuffer[i] == 0 ) 	break;		// end of directory

			if ( dirbuffer[i] != FAT32_DELETED_ENTRY && dirbuffer[i+0x0B] == FAT32_LFN_ENTRY )
			{
				lfn_entry = (FAT32_LFN_Entry*) (dirbuffer + i);
				matched = compare_record(lfn_entry, uniName + ((LFN_entriesCount - matched_parts - 1) *13) );

				matched_parts = matched ? ++matched_parts : 0;
//printf("^^^^^ i = %d   matched_parts = %d\n", i, matched_parts);
//dump_hexa(lfn_entry, 32);
			}
			else if ( matched_parts == LFN_entriesCount ) 	// record entry
			{
//printf("$$$ i = %d\n", i);
				fat32_entry = (FAT32_Dir_Entry*) (dirbuffer + i);
				
				// remove entry mode 
				if ( mode == FAT32_DIR_TABLE_ENTRY_REMOVE )
				{
					fat32_entry->name[0] = FAT32_DELETED_ENTRY;
//printf("**** i = %d\n", i);
//dump_hexa(fat32_entry, 32);
					// before saving this cluster try to remove LFN entries as much as u can
					while ( i > 0 && !lfn_entries_deleted)
					{
	
						i-= FAT32_ENTRY_RECORD_LEN;
						lfn_entry = (FAT32_LFN_Entry*) (dirbuffer + i);
//printf("**** i = %d\n", i);
//dump_hexa(lfn_entry, 32);	
						// last entry
						if ( lfn_entry->serial_number & 0x40 )	lfn_entries_deleted = 1;
					
						lfn_entry->serial_number = FAT32_DELETED_ENTRY;	
	
					}
				}
				else if ( mode == FAT32_DIR_TABLE_ENTRY_UPDATE )	// update entry mode
				{
					u32_t value = *((u32_t*)field_value);
					switch (updated_field)	
					{
						case FIELD_CLUSTER_NUM:
							fat32_entry->cluster_lo = (u16_t) value;
							fat32_entry->cluster_hi = (u16_t) (value >> 16 );
						break;
						case FIELD_MODIFIED_DATE:
							// to be done later
						break;
						case FIELD_FILE_SIZE:
							fat32_entry->file_size = value;
						break;
					}
				}
				else
				{	
					// invalid mode
					if (dirbuffer) 		free(dirbuffer);
					if ( uniName ) 	free(uniName);

					return -1;	
				}
		
				// save back the cluster		
				ret = FAT32_saveCluster(info, next_cluster, dirbuffer);
	
				entry_updated = 1;
				break;

			}

			perior_cluster = next_cluster;

		}	// end for loop
	
		next_cluster = FAT32_getNextCluster(info, next_cluster);
	
	}	// end while loop


	// the remaing long file entries for the deletion mode
	if ( mode == FAT32_DIR_TABLE_ENTRY_REMOVE && !lfn_entries_deleted && perior_cluster && entry_updated )
	{	
		memset(dirbuffer, 0, cluster_size);

		if ( !ide_read_sectors(info->drivenumber, info->sectors_per_cluster, CLUSTER_TO_SECTOR(info, perior_cluster), (u32_t)dirbuffer) )
		{
			printf("FAT-32: Could not read LBA sectors 0x%x+0x%x when when update_entry!\n", perior_cluster, info->sectors_per_cluster);
			if (dirbuffer) 		free(dirbuffer);
			return NULL;
		}

		for (i = cluster_size - FAT32_ENTRY_RECORD_LEN; i >= 0 && !lfn_entries_deleted ;i-=FAT32_ENTRY_RECORD_LEN)
		{
			// check for LFN
			if ( dirbuffer[i+0x0B] != FAT32_LFN_ENTRY ) 	break;
			lfn_entry = (FAT32_LFN_Entry*) (dirbuffer + i);
					
			// last entry
			if ( lfn_entry->serial_number & 0x40 )	lfn_entries_deleted = 1;
				
			lfn_entry->serial_number = FAT32_DELETED_ENTRY;	

		}

		// save back the cluster		
		ret = FAT32_saveCluster(info, perior_cluster, dirbuffer);
	
	}

	if (dirbuffer) 		free(dirbuffer);
	if ( uniName ) 	free(uniName);

	return 0;

}

int fat32_remove_fat_entry(void* t)
{
	FS_FileEntry* file = (FS_FileEntry*) t;
	FS_Tree* dir_node = file->directory;		// parent directory
	u32_t file_cluster = file->lbapos;
	
	fat32* info = (fat32*) dir_node->responsible_driver->opaque;	

	int ret = update_entry_in_directory_table(info, dir_node->lbapos, FAT32_DIR_TABLE_ENTRY_REMOVE, file->filename, 0, NULL);

//	printf("fat32_remove_fat_entry ret = %d   file_cluster = %d\n", ret, file_cluster);

	// now remove the chain clusters of the file itself
	if ( ret >= 0 && file_cluster )	// if not empty file
		FAT32_freeClustersChain(info, file_cluster);

	return ret;
}



int compare_record(FAT32_LFN_Entry* lfn_enty, u16_t* uniName)
{
	int i=0, j=0;

	for (i=0;i<5;i++)
		if ( lfn_enty->name1[i] != uniName[j++] )	return 0;

	for (i=0;i<6;i++)
		if ( lfn_enty->name2[i] != uniName[j++] )	return 0;

	for (i=0;i<2;i++)
		if ( lfn_enty->name3[i] != uniName[j++] )	return 0;

	return 1;
	
}

