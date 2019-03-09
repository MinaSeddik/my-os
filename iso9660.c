

#include <filesystem.h>
#include <iso9660.h>
#include <ata.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


extern FileSystem* filesystems;

#define CD_SECTOR_LEN	2048
#define VERIFY_ISO9660(n) (n->standardidentifier[0] == 'C' && n->standardidentifier[1] == 'D' \
				&& n->standardidentifier[2] == '0' && n->standardidentifier[3] == '0' && n->standardidentifier[4] == '1')

/* File flags for FS_DirectoryEntry */
#define FS_DIRECTORY	0x00000001	/* Entry is a directory */


void parse_boot(iso9660* info,u8_t* buffer);
void parse_VPD(iso9660* info,u8_t* buffer);
void parse_PVD(iso9660* info,u8_t* buffer);
void parse_SVD(iso9660* info,u8_t* buffer);




FS_FileEntry* ParseDirectory(FS_Tree* node, iso9660* info, u32_t start_lba, u32_t lengthbytes)
{
	u8_t* dirbuffer = (u8_t*) malloc(lengthbytes);
	int j;

//printf("inside ParseDirectory ... \n");

	memset(dirbuffer, 0, lengthbytes);

	if ( !ide_read_sectors(info->drivenumber, lengthbytes / CD_SECTOR_LEN, start_lba, (u32_t)dirbuffer) )
	{
		printf("ISO9660: Could not read LBA sectors 0x%x+0x%x when loading directory!\n", start_lba, lengthbytes / CD_SECTOR_LEN);
		free(dirbuffer);
		return NULL;
	}

	u32_t dir_items = 0;
	FS_FileEntry* list = NULL;

	// Iterate each of the entries in this directory, enumerating files
	u8_t* walkbuffer = dirbuffer;
	int entrycount = 0;
	while (walkbuffer < dirbuffer + lengthbytes)
	{
		entrycount++;
		ISO9660_File* fentry = (ISO9660_File*)walkbuffer;

		if (fentry->length == 0)	break;

//printf("entrycount = %d\n", entrycount);

		FS_FileEntry* thisentry = (FS_FileEntry*) malloc(sizeof(FS_FileEntry));

//printf("entry len = %d start_lba = %d\n", fentry->filename_length, start_lba);
/*
		if ( fentry->filename_length == 1 && fentry->filename[0] == 0 )		// current dir
		{
			thisentry->filename = (char*) malloc(2);
			thisentry->filename[0] = '.';
			thisentry->filename[1] = 0;

		}
		else if ( fentry->filename_length == 1 && fentry->filename[0] == 1 )	// parent dir
		{
			thisentry->filename = (char*) malloc(3);
			thisentry->filename[0] = '.';
			thisentry->filename[1] = '.';
			thisentry->filename[2] = 0;

		}
*/		

		if ( entrycount > 2 )
		{

			if (info->joliet == 0)
			{
				thisentry->filename = (char*) malloc(fentry->filename_length + 1);
				j = 0;
				char* ptr = fentry->filename;
				// Stop at end of string or at ; which seperates the version id from the filename.
				// We don't want the version ids.
				for (; j < fentry->filename_length && *ptr != ';'; ++ptr)
				//thisentry->filename[j++] = *ptr;
				thisentry->filename[j++] = tolower(*ptr);
				thisentry->filename[j] = 0;
	
				/* Filenames ending in '.' are not allowed */
				if (thisentry->filename[j - 1] == '.')
					thisentry->filename[j - 1] = 0;

			}
			else
			{
				// Parse joliet filename, 16-bit unicode UCS-2
				thisentry->filename = (char*) malloc((fentry->filename_length / 2) + 1);
				j = 0;
				char* ptr = fentry->filename;
				for (; j < fentry->filename_length / 2; ptr += 2)
				{
					if (*ptr != 0)
						thisentry->filename[j++] = '?';
					else
						thisentry->filename[j++] = *(ptr + 1);
				}
				thisentry->filename[j] = 0;
			}


			thisentry->year = fentry->recording_date.years_since_1900 + 1900;
			thisentry->month = fentry->recording_date.month;
			thisentry->day = fentry->recording_date.day;
			thisentry->hour = fentry->recording_date.hour;
			thisentry->min = fentry->recording_date.minute;
			thisentry->sec = fentry->recording_date.second;
			thisentry->device = info->drivenumber;
			thisentry->lbapos = fentry->extent_lba_lsb;
			thisentry->size = fentry->data_length_lsb;
			thisentry->flags = 0;		// ordinary file
			thisentry->directory = node;

			if (fentry->file_flags & 0x02)
				thisentry->flags |= FS_DIRECTORY;	// directory file
			dir_items++;
			thisentry->next = list;
			list = thisentry;
		}

		walkbuffer += fentry->length;
	}

	if ( dirbuffer) free(dirbuffer);

	return list;
}




void* iso_entry_point(u32_t drivenumber)
{

	u8_t* buffer = (u8_t*) malloc(CD_SECTOR_LEN);
	memset(buffer, 0, CD_SECTOR_LEN);

	iso9660* driver_info = (iso9660*) malloc(sizeof(iso9660));
	driver_info->drivenumber = drivenumber;

	u32_t VolumeDescriptorPos = PVD_LBA;	// 0x10
//printf("iso_entry_point here 1 ... drivenumber = %d\n",drivenumber);
	while (1)
	{

//printf("iso_entry_point here X ... \n");
		if ( !ide_read_sectors(drivenumber, 1, VolumeDescriptorPos++, (u32_t)buffer) )
		{
			printf("ISO9660: Could not read LBA sector 0x%x !\n", VolumeDescriptorPos);
			if ( driver_info )	free(driver_info);
			driver_info = NULL;
			break;
		}

		u8_t VolumeDescriptorID = buffer[0];

//printf("iso_entry_point VolumeDescriptorID = %d\n",VolumeDescriptorID);

		if (VolumeDescriptorID == 0xFF)		// Volume descriptor terminator
			break;
		else if (VolumeDescriptorID == 0x00)	// Boot volume descriptor
			parse_boot(driver_info, buffer);
		else if (VolumeDescriptorID == 0x01)	// Primary volume descriptor
			parse_PVD(driver_info, buffer);
		else if (VolumeDescriptorID == 0x02)	// Supplementary volume descriptor
			parse_SVD(driver_info, buffer);
		else if (VolumeDescriptorID == 0x03)	// Volume partition descriptor
			parse_VPD(driver_info, buffer);
		else if (VolumeDescriptorID >= 0x04 && VolumeDescriptorID <= 0xFE)	// Reserved and unknown ID
			printf("ISO9660: WARNING: Unknown volume descriptor 0x%x at LBA 0x%x!\n", VolumeDescriptorID, VolumeDescriptorPos);

	}

	if ( buffer )	free(buffer);

	return driver_info;
}

void parse_boot(iso9660* info,u8_t* buffer)
{
	// todo later
}

void parse_VPD(iso9660* info,u8_t* buffer)
{
	// todo later
}

void parse_PVD(iso9660* info,u8_t* buffer)
{
	PVD* pvd = (PVD*)buffer;

//printf("parse_PVD here 1 ... \n");
	if (!VERIFY_ISO9660(pvd))
	{
		printf("ISO9660: Invalid PVD found, identifier is not 'CD001'\n");
	}
	char* ptr = pvd->volumeidentifier + 31;
	for (; ptr != pvd->volumeidentifier && *ptr == ' '; --ptr)
	{
		// Backpeddle over the trailing spaces in the volume name
		if (*ptr == ' ')
			*ptr = 0;
	}
//printf("parse_PVD here 2 ... \n");
	int j = 0;
	info->volume_name = (char*) malloc(strlen(pvd->volumeidentifier) + 1);
	for (ptr = pvd->volumeidentifier; *ptr; ++ptr)
		info->volume_name[j++] = *ptr;
	// Null-terminate volume name
	info->volume_name[j] = 0;

	info->joliet = 0;
	info->pathtable_lba = pvd->lsb_pathtable_L_lba;
	info->rootextent_lba = pvd->root_directory.extent_lba_lsb;
	info->rootextent_len = pvd->root_directory.data_length_lsb;

	info->root = ParseDirectory(NULL, info, pvd->root_directory.extent_lba_lsb, pvd->root_directory.data_length_lsb);

	return ;
}

void parse_SVD(iso9660* info,u8_t* buffer)
{
	PVD* svd = (PVD*)buffer;
        if (!VERIFY_ISO9660(svd))
	{
		printf("ISO9660: Invalid SVD found, identifier is not 'CD001'\n");
		return;
	}

	int joliet = 0;
	if (svd->escape_seq[0] == '%' && svd->escape_seq[1] == '/')
	{
		switch (svd->escape_seq[2])
		{
			case '@':
				joliet = 1;
			break;
			case 'C':
				joliet = 2;
			break;
			case 'E':
				joliet = 3;
			break;
		}
	}

	if (joliet)
	{
		printf("Joliet extensions found on CD drive %d, UCS-2 Level %d\n", info->drivenumber, joliet);
		info->joliet = joliet;
		info->pathtable_lba = svd->lsb_pathtable_L_lba;
		info->rootextent_lba = svd->root_directory.extent_lba_lsb;
		info->rootextent_len = svd->root_directory.data_length_lsb;
		info->root = ParseDirectory(NULL, info, svd->root_directory.extent_lba_lsb, svd->root_directory.data_length_lsb);
	}
	return;
}


FileSystem* mount_iso9660(int device_num)
{
//	printf("try to mount cdrom, device number = %d\n",device_num);

	FileSystem* filesystem = (FileSystem*) malloc(sizeof(FileSystem));
	memset(filesystem, 0, sizeof(FileSystem));

//printf("mount_iso9660 here 1 ... \n");


	strcpy(filesystem->name, ISO9660_FS);
	filesystem->device_num = device_num;
	filesystem->permission = FILESYSTEM_PREM_READ_ONLY;
	filesystem->readFile = iso_read_file;
	filesystem->getDirectory = iso_get_directory;

//printf("mount_iso9660 here 2 ... \n");
	iso9660* driver_info = iso_entry_point(device_num);	
//printf("mount_iso9660 here 3 ... \n");
	if ( driver_info == NULL )
	{
		printf("Error in trying to mount iso file system, device number = %d\n",device_num);
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

int iso_read_file(void* file, u32_t start_offset, u32_t length, u8_t* buffer)
{

	FS_FileEntry* file_entry = (FS_FileEntry*) file;
	
	u32_t start_lba = file_entry->lbapos + ( start_offset/CD_SECTOR_LEN );
	u32_t first_sect_offset = start_offset % CD_SECTOR_LEN;
	u32_t num_of_sect = length/CD_SECTOR_LEN;
	int remaining_bytes = length%CD_SECTOR_LEN;

//printf("iso_read_file here 0  len = %d\n", length);

//printf("iso_read_file here 1  file_entry->lbapos = %d\n", file_entry->lbapos);
//printf("iso_read_file here 2  start_lba = %d\n", start_lba);	
//printf("iso_read_file here 3  first_sect_offset = %d\n", first_sect_offset);

//printf("iso_read_file here 4  num_of_sect = %d\n", num_of_sect);
//printf("iso_read_file here 5  remaining_bytes = %d\n", remaining_bytes);	


	if ( remaining_bytes )	
	{
		num_of_sect++;
		remaining_bytes = remaining_bytes - CD_SECTOR_LEN - first_sect_offset;
	}
//printf("iso_read_file here 5.5  remaining_bytes = %d\n", remaining_bytes);
	if ( remaining_bytes > 0 )	num_of_sect++;

//printf("iso_read_file here 6  num_of_sect = %d\n", num_of_sect);

//printf("iso_read_file here 6  allocate len = %d\n", num_of_sect * CD_SECTOR_LEN);
	char* data = (char*) malloc(num_of_sect * CD_SECTOR_LEN);

//printf("iso_read_file here 7 data = 0x%x\n", data);
	if ( !ide_read_sectors(file_entry->device, num_of_sect, start_lba, (u32_t)data) )
	{
		printf("ISO9660: Could not read LBA sectors 0x%x+0x%x when reading file !\n", start_lba, num_of_sect);
		if (data)	free(data);
		while(1);
		return NULL;
	}

//printf("iso_read_file here 8 buffer = 0x%x  data+first_sect_offset = 0x%x\n",buffer, data+first_sect_offset);
	memcpy(buffer, data+first_sect_offset, length);
	
//printf("iso_read_file here 9 data = 0x%x\n", data);

	if (data)	free(data);

	

//printf("iso_read_file here 10\n");
	
	return length;
}

void* iso_get_directory(void* t)
{
	FS_Tree* dir_node = (FS_Tree*) t;

	FS_FileEntry* childs = ParseDirectory(dir_node, dir_node->responsible_driver->opaque, dir_node->lbapos, dir_node->size);

	return ((void*) childs);
}



