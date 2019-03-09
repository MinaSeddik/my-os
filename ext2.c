

#include <filesystem.h>
#include <ext2.h>
#include <ata.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <debug.h>

#define SECTOR_SIZE		512

extern FileSystem* filesystems;

#define BLOCK_TO_SECTOR(info, block_num)		info->start_lba + ( block_num * info->sectors_per_block )
#define BLOCKGROUP_VIA_INODE(info, inode)		((inode - 1) / info->num_of_inodes_per_blockgroup)


u8_t mask[8] = { 0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F };

ext2* ext2_entry_point(int device_num, int part_id, u32_t start_lba, u32_t part_len);
FS_FileEntry* ext2_parse_directory(FS_Tree* node, ext2* info, u32_t dir_inode);
u32_t getBlockNumber(ext2* info, int block_group, int object);
u32_t getBlockNumbyId(ext2* info, u32_t block_index, Inode *InodeEntry);
Inode* getInodeEntry(ext2* info, u32_t inode_num);
u32_t allocate_block(ext2* info);
int free_block(ext2* info, u32_t block_num);

Inode* getInodeEntry(ext2* info, u32_t inode_num)
{
	u32_t inode_table_block = getBlockNumber(info, BLOCKGROUP_VIA_INODE(info, inode_num), BLOCKNUM_INODE_TABLE);
	int inodes_per_block = info->bytes_per_block / info->size_of_inode_structure;
	u32_t block_num = inode_table_block + (inode_num - 1) / inodes_per_block;
	int block_index = (inode_num - 1) % inodes_per_block;

/*
printf("ext2_parse_directory inode_num = %d \n", inode_num);
printf("ext2_parse_directory inode_table_block = %d \n", inode_table_block);
printf("ext2_parse_directory inodes_per_block = %d \n", inodes_per_block);
printf("ext2_parse_directory block_num = %d \n", block_num);
printf("ext2_parse_directory block_index = %d \n", block_index);
*/
	u8_t* content = (u8_t*) malloc(info->bytes_per_block);
	memset(content, 0, info->bytes_per_block);
	if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
	{
		printf("Could not read ext2 inode entry for inode_num = %d!\n", inode_num);
		if (content) 	free(content);
		return NULL;
	}

	Inode *InodeEntry = (Inode*) malloc(sizeof(Inode));
//printf("ext2_parse_directory InodeEntry = %d \n", InodeEntry);
	memcpy(InodeEntry, content + block_index * info->size_of_inode_structure, sizeof(Inode));
//dump_hexa(InodeEntry, 128);

	if (content) 	free(content);

//printf("ext2_parse_directory InodeEntry = %d \n", InodeEntry);
	return InodeEntry;
}
FS_FileEntry* ext2_parse_directory(FS_Tree* node, ext2* info, u32_t dir_inode)
{
	
	FS_FileEntry* list = NULL;

	Inode *InodeDirEntry = getInodeEntry(info, dir_inode);
	if ( !InodeDirEntry )	return NULL;

	u8_t* content = (u8_t*) malloc(info->bytes_per_block);
	Inode *InodeFileEntry = NULL;
	u32_t dir_block_index = 0;
	u32_t block_num = getBlockNumbyId(info, dir_block_index, InodeDirEntry);
	while ( block_num > 0  )
	{
		memset(content, 0, info->bytes_per_block);
		if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
		{
			printf("Could not read ext2 inode entry for dir_node = %d!\n", dir_inode);
			if (content) 	free(content);
			return NULL;
		}	

//dump_hexa(content, 128);
//while(1);
		int offset = 0;
		int end_of_dir = 0;
		u32_t inode_num;
		u16_t entry_len;
		u8_t fileName_len;
		u8_t type_indicator;
		FS_FileEntry* current_entry = NULL;
		while ( !end_of_dir && offset < info->bytes_per_block )
		{
			
			inode_num = *(u32_t*) content;
			entry_len = *(u16_t*) (content + sizeof(u32_t));
			fileName_len = *(u8_t*) (content + sizeof(u32_t) + sizeof(u16_t));

//printf("offset = %d\n", offset);
//printf("entry_len = %d inode_num = %d fileName_len = %d\n", entry_len, inode_num, fileName_len);
			if ( entry_len <= 0 || inode_num < 2 || fileName_len <= 0 )
			{
				end_of_dir = 1;
				break;
			}

			// detect the type of the file
			type_indicator = *(u8_t*) (content + sizeof(u32_t) + sizeof(u16_t) + sizeof(u8_t));
			if ( (type_indicator == EXT2_FILE_REGULAR_FILE || type_indicator == EXT2_FILE_DIRECTORY) && strncmp(content + DIR_FILE_OFFSET, "..", 2) && strncmp(content + DIR_FILE_OFFSET, ".", 1) )
			{
				current_entry = (FS_FileEntry*) malloc(sizeof(FS_FileEntry));
				memset(current_entry, 0, sizeof(FS_FileEntry));
				current_entry->filename = (char*) malloc(fileName_len + 1);
				memset(current_entry->filename, 0, fileName_len + 1);
				strncpy(current_entry->filename, content + DIR_FILE_OFFSET, fileName_len);
				
//printf("name = %s\n", current_entry->filename);
				current_entry->lbapos = inode_num;
				current_entry->device = info->drivenumber;

				if (InodeFileEntry) 	free(InodeFileEntry);
				InodeFileEntry = getInodeEntry(info, inode_num);
				if ( !InodeFileEntry )	break;

				// read size and creation date
				current_entry->size = InodeFileEntry->size_lo;
				
				// ignore creation date for now ........

				if ( type_indicator == EXT2_FILE_DIRECTORY )	// is directory
					current_entry->flags |= FS_DIRECTORY;	// directory file

				current_entry->directory = node;

				// append to the list
				current_entry->next = list;
				list = current_entry;

			}
			content+= entry_len;
			offset+= entry_len;
		} 	// end inner while loop
	

		dir_block_index++;
		block_num = getBlockNumbyId(info, dir_block_index, InodeDirEntry);			

	}
//while(1);
	if (content) 	free(content);
	if (InodeDirEntry) 	free(InodeDirEntry);
	if (InodeFileEntry) 	free(InodeFileEntry);
	
	return list;

}

int appendIndirectBlock(ext2* info, u32_t block_num, u32_t new_block_num)
{
	int ret = 1;
	u8_t* content = (u8_t*) malloc(info->bytes_per_block);

	if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
	{
		printf("Could not read ext2 inode entry for block_num = %d!\n", block_num);
		if (content) 	free(content);
		return NULL;
	}
	
	u32_t* addr_list = (u32_t*) content;
	int i=0;
	for ( i=0;i<info->bytes_per_block/4;i++)
		if ( addr_list[i] == 0 )
		{
			addr_list[i] = new_block_num;
			if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
			{
				printf("Could not read ext2 inode entry for block_num = %d!\n", block_num);
				if (content) 	free(content);
				return -1;
			}
			ret = 0;
			break;
		}
				
	if (content) 	free(content);

	return ret;

}


int appendBlockToList(ext2* info, u32_t new_block_num, u32_t inode_num)
{
	int i=0, ret;
	u8_t* content = (u8_t*) malloc(info->bytes_per_block);
	u32_t inode_table_block = getBlockNumber(info, BLOCKGROUP_VIA_INODE(info, inode_num), BLOCKNUM_INODE_TABLE);
	int inodes_per_block = info->bytes_per_block / info->size_of_inode_structure;
	u32_t block_num = inode_table_block + (inode_num - 1) / inodes_per_block, tmp_block;
	int block_index = (inode_num - 1) % inodes_per_block;
	Inode *InodeEntry = NULL;
	if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
	{
		printf("Could not read ext2 inode entry for dir_node = %d!\n", inode_num);
		if (content) 	free(content);
		return -1;
	}
	
	InodeEntry = content + block_index * info->size_of_inode_structure;
	// phase 1, try direct addressess
	for ( i=0;i<12;i++)
		if ( InodeEntry->direct_block[i] == 0 )
		{
			
			InodeEntry->direct_block[i] = new_block_num;
			if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
			{
				printf("Could not read ext2 inode entry for dir_node = %d!\n", inode_num);
				if (content) 	free(content);
				return -1;
			}
			if (content) 	free(content);
			return 0;
		}
	
	u32_t* addr_list = (u32_t*) content;
	u8_t* tmp = (u8_t*) malloc(info->bytes_per_block);


	// phase 2, try a single list
	if ( InodeEntry->singly_indirect_block == 0 )
	{
		tmp_block = allocate_block(info);
		InodeEntry->singly_indirect_block = tmp_block;
		if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
		{
			printf("Could not read ext2 inode entry for dir_node = %d!\n", inode_num);
			if (content) 	free(content);
			if (tmp) 	free(tmp);
			return -1;
		}

		memset(tmp, 0, info->bytes_per_block);
		addr_list = (u32_t*) tmp;
		addr_list[0] = new_block_num;
		if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)tmp) )
		{
			printf("Could not read ext2 inode entry for dir_node = %d!\n", inode_num);
			if (content) 	free(content);
			if (tmp) 	free(tmp);
			return -1;
		}		
		
		if (content) 	free(content);
		if (tmp) 	free(tmp);

		return 0;
	}
	else
	{

		ret = appendIndirectBlock(info,  InodeEntry->singly_indirect_block, new_block_num);
		if ( ret <= 0 )		
		{
			if (content) 	free(content);
			if (tmp) 	free(tmp);		
			return ret;
		}
	}


	// phase 3, try a doubly list
	if ( InodeEntry->doubly_indirect_block == 0 )
	{
		tmp_block = allocate_block(info);
		InodeEntry->doubly_indirect_block = tmp_block;
		if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
		{
			printf("Could not read ext2 inode entry for dir_node = %d!\n", inode_num);
			if (content) 	free(content);
			if (tmp) 	free(tmp);
			return -1;
		}

		int N = 2, k;
		for ( k=0;k<N;k++)
		{
			memset(tmp, 0, info->bytes_per_block);
			addr_list = (u32_t*) tmp;
			addr_list[0] = (k == N-1) ? new_block_num : allocate_block(info);
			if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)tmp) )
			{
				printf("Could not read ext2 inode entry for dir_node = %d!\n", inode_num);
				if (content) 	free(content);
				if (tmp) 	free(tmp);
				return -1;
			}
			tmp_block = addr_list[0];
		}

		if (content) 	free(content);
		if (tmp) 	free(tmp);

		return 0;
	}
	else
	{
		tmp_block = InodeEntry->doubly_indirect_block;
		if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)tmp) )
		{
			printf("Could not read ext2 inode entry for block_index = %d!\n", inode_num);
			if (content) 	free(content);
			if (tmp) 	free(tmp);
			return NULL;
		}

		addr_list = (u32_t*) tmp;
		for ( i=0;i<info->bytes_per_block/4;i++)
		{
			if ( addr_list[i] == 0 )
			{
				addr_list[i] = allocate_block(info);
				if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)tmp) )
				{
					printf("Could not read ext2 inode entry for dir_node = %d!\n", inode_num);
					if (content) 	free(content);
					if (tmp) 	free(tmp);
					return -1;
				}
				tmp_block = addr_list[i];

				memset(tmp, 0, info->bytes_per_block);
				addr_list = (u32_t*) tmp;
				addr_list[0] = new_block_num;
				if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)tmp) )
				{
					printf("Could not read ext2 inode entry for dir_node = %d!\n", inode_num);
					if (content) 	free(content);
					if (tmp) 	free(tmp);
					return -1;
				}
				if (content) 	free(content);
				if (tmp) 	free(tmp);

				return 0;
				
			}
			else
			{

				ret = appendIndirectBlock(info, addr_list[i], new_block_num);
				if ( ret <= 0 )		
				{
					if (content) 	free(content);
					if (tmp) 	free(tmp);		
					return ret;
				}

			}

		}
	}



	// phase 4, try a triply list
	if ( InodeEntry->triply_indirect_block == 0 )
	{
		tmp_block = allocate_block(info);
		InodeEntry->triply_indirect_block = tmp_block;
		if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
		{
			printf("Could not read ext2 inode entry for dir_node = %d!\n", inode_num);
			if (content) 	free(content);
			if (tmp) 	free(tmp);
			return -1;
		}

		int N = 3, k;
		for ( k=0;k<N;k++)
		{
			memset(tmp, 0, info->bytes_per_block);
			addr_list = (u32_t*) tmp;
			addr_list[0] = (k == N-1) ? new_block_num : allocate_block(info);
			if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)tmp) )
			{
				printf("Could not read ext2 inode entry for dir_node = %d!\n", inode_num);
				if (content) 	free(content);
				if (tmp) 	free(tmp);
				return -1;
			}
			tmp_block = addr_list[0];
		}

		if (content) 	free(content);
		if (tmp) 	free(tmp);

		return 0;
	}
	else
	{
		tmp_block = InodeEntry->triply_indirect_block;
		if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)tmp) )
		{
			printf("Could not read ext2 inode entry for block_index = %d!\n", inode_num);
			if (content) 	free(content);
			if (tmp) 	free(tmp);
			return NULL;
		}

		u8_t* tmp2 = (u8_t*) malloc(info->bytes_per_block);
		addr_list = (u32_t*) tmp;
		for ( i=0;i<info->bytes_per_block/4;i++)
		{
			if ( addr_list[i] == 0 )
			{
				addr_list[i] = allocate_block(info);
				if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)tmp) )
				{
					printf("Could not read ext2 inode entry for dir_node = %d!\n", inode_num);
					if (content) 	free(content);
					if (tmp) 	free(tmp);
					if (tmp2) 	free(tmp2);
					return -1;
				}

				tmp_block = addr_list[i];

				int N = 2, k;
				for ( k=0;k<N;k++)
				{
					memset(tmp, 0, info->bytes_per_block);
					addr_list = (u32_t*) tmp;
					addr_list[0] = (k == N-1) ? new_block_num : allocate_block(info);
					if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)tmp) )
					{
						printf("Could not read ext2 inode entry for dir_node = %d!\n", inode_num);
						if (content) 	free(content);
						if (tmp) 	free(tmp);
						return -1;
					}
					tmp_block = addr_list[0];
				}

				if (content) 	free(content);
				if (tmp) 	free(tmp);
				if (tmp2) 	free(tmp2);

				return 0;

			}
			else
			{

				if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, addr_list[i]), (u32_t)tmp2) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", inode_num);
					if (content) 	free(content);
					if (tmp) 	free(tmp);
					if (tmp2) 	free(tmp2);
					return NULL;
				}
				u32_t* tmp_addr_list = (u32_t*) tmp2;
				int j = 0;
				for ( j=0;j<info->bytes_per_block/4;j++)
					if ( tmp_addr_list[j] == 0 )
					{
						tmp_block = addr_list[i];
						tmp_addr_list[j] = allocate_block(info);
						if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)tmp2) )
						{
							printf("Could not read ext2 inode entry for dir_node = %d!\n", inode_num);
							if (content) 	free(content);
							if (tmp) 	free(tmp);
							if (tmp2) 	free(tmp2);
							return -1;
						}

						tmp_block = tmp_addr_list[j];
						memset(tmp2, 0, info->bytes_per_block);
						tmp_addr_list = (u32_t*) tmp2;
						tmp_addr_list[0] = new_block_num;
						if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)tmp2) )
						{
							printf("Could not read ext2 inode entry for dir_node = %d!\n", inode_num);
							if (content) 	free(content);
							if (tmp) 	free(tmp);
							if (tmp2) 	free(tmp2);
							return -1;
						}

						if (content) 	free(content);
						if (tmp) 	free(tmp);
						if (tmp2) 	free(tmp2);

						return 0;

					}
					else
					{

						ret = appendIndirectBlock(info, tmp_addr_list[j], new_block_num);
						if ( ret <= 0 )		
						{
							if (content) 	free(content);
							if (tmp) 	free(tmp);		
							if (tmp2) 	free(tmp2);		
							return ret;
						}
					}
			}

		}
	}


	if (content) 	free(content);
	if (tmp) 	free(tmp);

	return 0;
	
}

u32_t getBlockNumbyId(ext2* info, u32_t block_index, Inode *InodeEntry)
{

	int blocks_in_indirectBlocks = info->bytes_per_block / 4;
	u32_t tmp_block, block_num;

	u8_t* content = (u8_t*) malloc(info->bytes_per_block);
	switch(block_index)
	{
		case 0://		block_num = InodeEntry->direct_block_0;		break;
		case 1://		block_num = InodeEntry->direct_block_1;		break;
		case 2://		block_num = InodeEntry->direct_block_2;		break;
		case 3://		block_num = InodeEntry->direct_block_3;		break;
		case 4://		block_num = InodeEntry->direct_block_4;		break;
		case 5://		block_num = InodeEntry->direct_block_5;		break;
		case 6://		block_num = InodeEntry->direct_block_6;		break;
		case 7://		block_num = InodeEntry->direct_block_7;		break;
		case 8://		block_num = InodeEntry->direct_block_8;		break;
		case 9://		block_num = InodeEntry->direct_block_9;		break;
		case 10://	block_num = InodeEntry->direct_block_10;	break;
		case 11://	block_num = InodeEntry->direct_block_11;	break;
			block_num = InodeEntry->direct_block[block_index];
			break;
		default :
			if ( block_index < 12 + blocks_in_indirectBlocks )	// inside sigle indirect block
			{
				block_index-= 12;
				memset(content, 0, info->bytes_per_block);
				u32_t* addr_list = (u32_t*) content;
				if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, InodeEntry->singly_indirect_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return NULL;
				}
				block_num = addr_list[block_index];
			}
			else if ( block_index < 12 + blocks_in_indirectBlocks + blocks_in_indirectBlocks * blocks_in_indirectBlocks )	// doubly indirect block
			{
				block_index = ( block_index - ( 12 + blocks_in_indirectBlocks)) / blocks_in_indirectBlocks;
				int indirect_block_index = ( block_index - ( 12 + blocks_in_indirectBlocks)) % blocks_in_indirectBlocks;
				memset(content, 0, info->bytes_per_block);
				u32_t* addr_list = (u32_t*) content;
				if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, InodeEntry->doubly_indirect_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return NULL;
				}
				tmp_block = addr_list[block_index];

				memset(content, 0, info->bytes_per_block);
				addr_list = (u32_t*) content;
				if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return NULL;
				}
				block_num = addr_list[indirect_block_index];

			}
			else if ( block_index < 12 + blocks_in_indirectBlocks + blocks_in_indirectBlocks * blocks_in_indirectBlocks + blocks_in_indirectBlocks * blocks_in_indirectBlocks * blocks_in_indirectBlocks )	// tribly indirect block
			{
				block_index = ( block_index - ( 12 + blocks_in_indirectBlocks + blocks_in_indirectBlocks * blocks_in_indirectBlocks)) / ( blocks_in_indirectBlocks * blocks_in_indirectBlocks);
				int indirect_block_index_1 = ( block_index - ( 12 + blocks_in_indirectBlocks + blocks_in_indirectBlocks * blocks_in_indirectBlocks)) / blocks_in_indirectBlocks;
				int indirect_block_index_2 = ( block_index - ( 12 + blocks_in_indirectBlocks + blocks_in_indirectBlocks * blocks_in_indirectBlocks)) % blocks_in_indirectBlocks;
				memset(content, 0, info->bytes_per_block);
				u32_t* addr_list = (u32_t*) content;
				if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, InodeEntry->triply_indirect_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return NULL;
				}
				tmp_block = addr_list[block_index];
				memset(content, 0, info->bytes_per_block);
				addr_list = (u32_t*) content;
				if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return NULL;
				}
				tmp_block = addr_list[indirect_block_index_1];
				memset(content, 0, info->bytes_per_block);
				addr_list = (u32_t*) content;
				if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return NULL;
				}
				block_num = addr_list[indirect_block_index_2];
			}

		}	// end swich stmt

	if (content) 	free(content);

	return block_num;
}


int freeBlockbyId(ext2* info, u32_t block_index, int inode_num)
{
	
	u8_t* content = (u8_t*) malloc(info->bytes_per_block);
	u32_t inode_table_block = getBlockNumber(info, BLOCKGROUP_VIA_INODE(info, inode_num), BLOCKNUM_INODE_TABLE);
	int inodes_per_block = info->bytes_per_block / info->size_of_inode_structure;
	u32_t block_num = inode_table_block + (inode_num - 1) / inodes_per_block, tmp_block;
	int blockIndex = (inode_num - 1) % inodes_per_block;
	Inode *InodeEntry = NULL;
	if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
	{
		printf("Could not read ext2 inode entry for inode_num = %d!\n", inode_num);
		if (content) 	free(content);
		return -1;
	}
	
	InodeEntry = content + blockIndex * info->size_of_inode_structure;
	int blocks_in_indirectBlocks = info->bytes_per_block / 4;

	switch(block_index)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
			InodeEntry->direct_block[block_index] = 0;
			if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
			{
				printf("Could not read ext2 inode entry for inode_num = %d!\n", inode_num);
				if (content) 	free(content);
				return -1;
			}
			break;
		default :
			if ( block_index < 12 + blocks_in_indirectBlocks )	// inside sigle indirect block
			{
				block_index-= 12;
				memset(content, 0, info->bytes_per_block);
				u32_t* addr_list = (u32_t*) content;
				if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, InodeEntry->singly_indirect_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return -1;
				}
				addr_list[block_index] = 0;
				if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, InodeEntry->singly_indirect_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return -1;
				}
			}
			else if ( block_index < 12 + blocks_in_indirectBlocks + blocks_in_indirectBlocks * blocks_in_indirectBlocks )	// doubly indirect block
			{
				block_index = ( block_index - ( 12 + blocks_in_indirectBlocks)) / blocks_in_indirectBlocks;
				int indirect_block_index = ( block_index - ( 12 + blocks_in_indirectBlocks)) % blocks_in_indirectBlocks;
				memset(content, 0, info->bytes_per_block);
				u32_t* addr_list = (u32_t*) content;
				if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, InodeEntry->doubly_indirect_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return -1;
				}
				addr_list[block_index] = 0;
				if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, InodeEntry->doubly_indirect_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return -1;
				}

				memset(content, 0, info->bytes_per_block);
				addr_list = (u32_t*) content;
				if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return -1;
				}
				addr_list[indirect_block_index] = 0;
				if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return -1;
				}
			}
			else if ( block_index < 12 + blocks_in_indirectBlocks + blocks_in_indirectBlocks * blocks_in_indirectBlocks + blocks_in_indirectBlocks * blocks_in_indirectBlocks * blocks_in_indirectBlocks )	// tribly indirect block
			{
				block_index = ( block_index - ( 12 + blocks_in_indirectBlocks + blocks_in_indirectBlocks * blocks_in_indirectBlocks)) / ( blocks_in_indirectBlocks * blocks_in_indirectBlocks);
				int indirect_block_index_1 = ( block_index - ( 12 + blocks_in_indirectBlocks + blocks_in_indirectBlocks * blocks_in_indirectBlocks)) / blocks_in_indirectBlocks;
				int indirect_block_index_2 = ( block_index - ( 12 + blocks_in_indirectBlocks + blocks_in_indirectBlocks * blocks_in_indirectBlocks)) % blocks_in_indirectBlocks;
				memset(content, 0, info->bytes_per_block);
				u32_t* addr_list = (u32_t*) content;
				if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, InodeEntry->triply_indirect_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return -1;
				}
				addr_list[block_index] = 0;
				if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, InodeEntry->triply_indirect_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return -1;
				}
				memset(content, 0, info->bytes_per_block);
				addr_list = (u32_t*) content;
				if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return -1;
				}
				addr_list[indirect_block_index_1] = 0;
				if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return -1;
				}

				memset(content, 0, info->bytes_per_block);
				addr_list = (u32_t*) content;
				if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return -1;
				}
				addr_list[indirect_block_index_2] = 0;
				if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_index = %d!\n", block_index);
					if (content) 	free(content);
					return -1;
				}
			}

		}	// end swich stmt

	if (content) 	free(content);

	return block_num;
}


u32_t getBlockNumber(ext2* info, int block_group, int object)
{
	Block_Group_Descriptor* pgd = (Block_Group_Descriptor*) ( info->group_desc_table + block_group * sizeof(Block_Group_Descriptor));


//printf("getBlockNumber sizeof(Block_Group_Descriptor) = %d \n", sizeof(Block_Group_Descriptor));
/*
printf("getBlockNumber block_bitmap = %d \n", pgd->block_bitmap);
printf("getBlockNumber inode_bitmap = %d \n", pgd->inode_bitmap);
printf("getBlockNumber start_block_inodeTable = %d \n", pgd->start_block_inodeTable);
printf("getBlockNumber free_blocks = %d \n", pgd->free_blocks);
printf("getBlockNumber free_inodes = %d \n", pgd->free_inodes);
*/


	u32_t block_num = -1;
	switch( object )
	{
		case BLOCKNUM_BLOCK_BITMAP:
			block_num = pgd->block_bitmap;
			break;
		case BLOCKNUM_INODE_BITMAP:
			block_num = pgd->inode_bitmap;
			break;
		case BLOCKNUM_INODE_TABLE:
			block_num = pgd->start_block_inodeTable;
			break;
	}

	return block_num; 
}


ext2* ext2_entry_point(int device_num, int part_id, u32_t start_lba, u32_t part_len)
{

	ext2* driver_info = (ext2*) malloc(sizeof(ext2));
	memset(driver_info , 0, sizeof(ext2));
	driver_info->drivenumber = device_num;
	driver_info->partition_id = part_id;
	driver_info->start_lba = start_lba;
	driver_info->partition_len = part_len;		// len in sectors units

	Ext2_SupperBlock* superblock = (Ext2_SupperBlock*) malloc(sizeof(Ext2_SupperBlock));
	memset(superblock , 0, sizeof(Ext2_SupperBlock));

	if ( !ide_read_sectors(device_num, 2, start_lba + 2, (u32_t)superblock) )
	{
		printf("Could not read ext2 Supper block!\n");
		if (driver_info) 	free(driver_info);
		if (superblock) 	free(superblock);
		return NULL;
	}

	if ( superblock->ext2_signature == EXT2_SUPPERBLOCK_SIGNATURE )
	{

/*
		printf("total_inodes = %d\n", superblock->total_inodes);
		printf("total_blocks = %d\n", superblock->total_blocks);
		printf("total_unalloc_blocks = %d\n", superblock->total_unalloc_blocks);
		printf("total_unalloc_inodes = %d\n", superblock->total_unalloc_inodes);
		printf("supperblock_block_number = %d\n", superblock->supperblock_block_number);
		printf("block_size = %d\n", superblock->block_size);
		printf("supperblock_reserved_blocks = %d\n", superblock->supperblock_reserved_blocks);
		printf("num_of_blocks_per_blockgroup = %d\n", superblock->num_of_blocks_per_blockgroup);
		printf("num_of_inodes_per_blockgroup = %d\n", superblock->num_of_inodes_per_blockgroup);
		printf("first_non_reserved_inode = %d\n", superblock->first_non_reserved_inode);
		printf("sys_id_string = %s\n", superblock->ext2_signature);
		printf("signature = 0x%x\n\n", superblock->ext2_signature);
*/
		driver_info->supperblock_block_number = superblock->supperblock_block_number;
		driver_info->bytes_per_sector = SECTOR_SIZE;
		driver_info->bytes_per_block = 1024 << superblock->block_size;
		driver_info->sectors_per_block = driver_info->bytes_per_block / SECTOR_SIZE;
		driver_info->reserved_blocks = superblock->supperblock_reserved_blocks;
		driver_info->total_inodes = superblock->total_inodes;
		driver_info->total_blocks = superblock->total_blocks;
		driver_info->num_of_blocks_per_blockgroup = superblock->num_of_blocks_per_blockgroup;
		driver_info->num_of_inodes_per_blockgroup = superblock->num_of_inodes_per_blockgroup;
		driver_info->first_non_reserved_inode = superblock->first_non_reserved_inode;	
		driver_info->size_of_inode_structure = superblock->size_of_inode_structure;

		int blockgroup_count_by_block = (int) round( ( (double )driver_info->total_blocks / driver_info->num_of_blocks_per_blockgroup ), 0);
		int blockgroup_count_by_inode = (int) round( ( (double )driver_info->total_inodes / driver_info->num_of_inodes_per_blockgroup ), 0);

		if ( blockgroup_count_by_block != blockgroup_count_by_inode )	
			printf("WARNING : on mounting ext2 file system { blockgroup_count_by_block != blockgroup_count_by_inode } \n");
		driver_info->blockgroup_count = max(blockgroup_count_by_block, blockgroup_count_by_inode);

//printf("driver_info->blockgroup_count = %d\n", driver_info->blockgroup_count);


		//===================================================
		//===================================================
		// please comment this line, this line to solve bochs emulation problem
		//===================================================
		driver_info->blockgroup_count = 11;
		//===================================================
		//===================================================
		//===================================================



		int NumOfBlocksinGroupDesc = driver_info->blockgroup_count * sizeof(Block_Group_Descriptor) / driver_info->bytes_per_block;
		if ( driver_info->blockgroup_count * sizeof(Block_Group_Descriptor) % driver_info->bytes_per_block )	++NumOfBlocksinGroupDesc;
//printf("driver_info->sectors_per_block = %d\n", driver_info->sectors_per_block);
//printf("NumOfBlocksinGroupDesc = %d\n", NumOfBlocksinGroupDesc);

		driver_info->root_dir_inode = ROOT_DIRECTORY_INODE_NUMBER;
		driver_info->group_desc_table = (u8_t*) malloc(driver_info->bytes_per_block * NumOfBlocksinGroupDesc);
		memset(driver_info->group_desc_table, 0, driver_info->bytes_per_block);

		if ( !ide_read_sectors(device_num, driver_info->sectors_per_block * NumOfBlocksinGroupDesc, BLOCK_TO_SECTOR(driver_info, driver_info->supperblock_block_number + 1), (u32_t)driver_info->group_desc_table) )
		{
			printf("Could not read ext2 Block Group Descriptor Table!\n");
			if (driver_info->group_desc_table) 	free(driver_info->group_desc_table);
			if (driver_info) 	free(driver_info);
			if (superblock) 	free(superblock);
			return NULL;
		}


// trace descriptor table

//u32_t inode_table_block = getBlockNumber(driver_info, 0, BLOCKNUM_INODE_TABLE);
//printf("inode_table_block [0] = %d\n",inode_table_block);
//inode_table_block = getBlockNumber(driver_info, 1, BLOCKNUM_INODE_TABLE);
//printf("inode_table_block [1] = %d\n",inode_table_block);
//inode_table_block = getBlockNumber(driver_info, 2, BLOCKNUM_INODE_TABLE);
//printf("inode_table_block [2] = %d\n",inode_table_block);

//dump_hexa(driver_info->group_desc_table, 256);
		driver_info->superblock = superblock;
		driver_info->root = ext2_parse_directory(NULL, driver_info, driver_info->root_dir_inode);

	}
	else
	{
		printf("Invalid ext2 Supper block!\n");
		if (driver_info) 	free(driver_info);
		if (superblock) 	free(superblock);
		return NULL;	
	}	

	return driver_info;
}

FileSystem* mount_ext2(int device_num, int part_id, int start_lba, int part_len)
{

	printf("Try to mount ext2 partition # %d, device number = %d\n",part_id, device_num);

	FileSystem* filesystem = (FileSystem*) malloc(sizeof(FileSystem));
	memset(filesystem, 0, sizeof(FileSystem));

	strcpy(filesystem->name, EXT2_FS);
	filesystem->device_num = device_num;
	filesystem->permission = FILESYSTEM_PREM_READ_WRITE;

	filesystem->readFile = ext2_read_file;
	filesystem->writeFile = ext2_write_file;
	filesystem->getDirectory = ext2_get_directory;		// to list directory
	filesystem->createFile = ext2_create_ext_entry;
	filesystem->removeFile = ext2_remove_ext_entry;

	fat32* driver_info = ext2_entry_point(device_num, part_id, start_lba, part_len);	

	if ( driver_info == NULL )
	{
		printf("Error in trying to mount ext2 file system, device number = %d\n",device_num);
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

u32_t allocate_block(ext2* info)
{
	Block_Group_Descriptor* pgd = NULL;
	int i, group_desc = -1;

	for (i=0;i<info->blockgroup_count;i++)
	{	
		pgd = (Block_Group_Descriptor*) info->group_desc_table + i * sizeof(Block_Group_Descriptor);
		if ( pgd->free_blocks )
		{
			group_desc = i;
			break;
		}
	}
	
	if ( group_desc == -1 ) return -1;
	u8_t* content = (u8_t*) malloc(info->bytes_per_block);
	memset(content, 0, info->bytes_per_block);
	int block_num = pgd->block_bitmap;

	// read block bitmap
	if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
	{
		printf("Could not read ext2 block entry for block_bitmap = %d!\n", block_num);
		if (content) 	free(content);
		return -1;
	}	

	int byte_index, byte_offset;
	int free_block = 0;
	Ext2_SupperBlock* sp = (Ext2_SupperBlock*) info->superblock;
//dump_hexa(content, 128);
	for (i=1;i<=sp->num_of_blocks_per_blockgroup;++i)
	{
		byte_index = (i-1) / 8;
		byte_offset = (i-1) % 8;
//printf("------ byte_index = %d, byte_offset = %d	[%2x]\n", byte_index, byte_offset, content[byte_index]);
		if ( !(( content[byte_index] >> byte_offset ) & 1) )	
		{
			free_block = i;	
			content[byte_index] = content[byte_index] | ( 1 << byte_offset );
			if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
			{
				printf("Could not write ext2 block entry for block_bitmap = %d!\n", block_num);
				if (content) 	free(content);
				return -1;
			}
			break;
		}
	}
	free_block+= group_desc * info->num_of_blocks_per_blockgroup;

	pgd->free_blocks--;
	sp->total_unalloc_blocks--;

	syncronize_superblock(info, sp);
	syncronize_descriptorTable(info);
	
	if (content) 	free(content);

	return free_block;
}

int free_block(ext2* info, u32_t block_num)
{
	Block_Group_Descriptor* pgd = NULL;
	Ext2_SupperBlock* sp = (Ext2_SupperBlock*) info->superblock;

	int group_desc = block_num / sp->num_of_blocks_per_blockgroup;

	pgd = (Block_Group_Descriptor*) info->group_desc_table + group_desc * sizeof(Block_Group_Descriptor);
	block_num%= sp->num_of_inodes_per_blockgroup;
	int byte_index = (block_num-1) / 8;
	int byte_offset = (block_num-1) % 8;

	u8_t* content = (u8_t*) malloc(info->bytes_per_block);
	memset(content, 0, info->bytes_per_block);
	int block_bmp_num = pgd->inode_bitmap;

	// read block bitmap
	if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_bmp_num), (u32_t)content) )
	{
		printf("Could not read ext2 block entry for block_bitmap = %d!\n", block_num);
		if (content) 	free(content);
		return -1;
	}	

	content[byte_index]&= mask[byte_offset];

	if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_bmp_num), (u32_t)content) )
	{
		printf("Could not write ext2 inode entry for inode_bitmap = %d!\n", block_num);
		if (content) 	free(content);
		return -1;
	}

	pgd->free_blocks++;
	sp->total_unalloc_blocks++;

	syncronize_superblock(info, sp);
	syncronize_descriptorTable(info);
	
	if (content) 	free(content);

	return free_block;
}


u32_t allocate_inode(ext2* info, int is_dir)
{
	Block_Group_Descriptor* pgd = NULL;
	int i, group_desc = -1;

	for (i=0;i<info->blockgroup_count;i++)
	{	
		pgd = (Block_Group_Descriptor*) info->group_desc_table + i * sizeof(Block_Group_Descriptor);
		if ( pgd->free_inodes )
		{
			group_desc = i;
			break;
		}
	}
	
	if ( group_desc == -1 ) return -1;
	u8_t* content = (u8_t*) malloc(info->bytes_per_block);
	memset(content, 0, info->bytes_per_block);
	int block_num = pgd->inode_bitmap;

	// read inode bitmap
	if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
	{
		printf("Could not read ext2 inode entry for inode_bitmap = %d!\n", block_num);
		if (content) 	free(content);
		return -1;
	}	

	int byte_index, byte_offset;
	int free_inode = 0;
	Ext2_SupperBlock* sp = (Ext2_SupperBlock*) info->superblock;
//dump_hexa(content, 128);
	for (i=1;i<=sp->num_of_inodes_per_blockgroup;++i)
	{
		byte_index = (i-1) / 8;
		byte_offset = (i-1) % 8;
//printf("------ byte_index = %d, byte_offset = %d	[%2x]\n", byte_index, byte_offset, content[byte_index]);
		if ( !(( content[byte_index] >> byte_offset ) & 1) )	
		{
			free_inode = i;	
			content[byte_index] = content[byte_index] | ( 1 << byte_offset );
			if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
			{
				printf("Could not write ext2 inode entry for inode_bitmap = %d!\n", block_num);
				if (content) 	free(content);
				return -1;
			}
			break;
		}
	}
	free_inode+= group_desc * info->num_of_inodes_per_blockgroup;

	pgd->free_inodes--;
	if ( is_dir )	pgd->dir_count++;

	sp->total_unalloc_inodes--;

	syncronize_superblock(info, sp);
	syncronize_descriptorTable(info);
	
	if (content) 	free(content);

	return free_inode;
}


int free_inode(ext2* info, int inode_num, int is_dir)
{
	Block_Group_Descriptor* pgd = NULL;
	Ext2_SupperBlock* sp = (Ext2_SupperBlock*) info->superblock;

	int group_desc = inode_num / sp->num_of_inodes_per_blockgroup;

	pgd = (Block_Group_Descriptor*) info->group_desc_table + group_desc * sizeof(Block_Group_Descriptor);
	inode_num%= sp->num_of_inodes_per_blockgroup;
	int byte_index = (inode_num-1) / 8;
	int byte_offset = (inode_num-1) % 8;

	u8_t* content = (u8_t*) malloc(info->bytes_per_block);
	memset(content, 0, info->bytes_per_block);
	int block_num = pgd->inode_bitmap;

	// read inode bitmap
	if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
	{
		printf("Could not read ext2 inode entry for inode_bitmap = %d!\n", block_num);
		if (content) 	free(content);
		return -1;
	}	

	content[byte_index]&= mask[byte_offset];

	if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
	{
		printf("Could not write ext2 inode entry for inode_bitmap = %d!\n", block_num);
		if (content) 	free(content);
		return -1;
	}

	pgd->free_inodes++;
	if ( is_dir )	pgd->dir_count--;

	sp->total_unalloc_inodes++;

	syncronize_superblock(info, sp);
	syncronize_descriptorTable(info);
	
	if (content) 	free(content);

	return 0;
}


void syncronize_superblock(ext2* info, Ext2_SupperBlock* superblock)
{
	if ( !ide_write_sectors(info->drivenumber, 2, info->start_lba + 2, (u32_t)superblock) )
		printf("Could not Syncronize ext2 Supper block!\n");	
}

void syncronize_descriptorTable(ext2* info)
{
	int NumOfBlocksinGroupDesc = info->blockgroup_count * sizeof(Block_Group_Descriptor) / info->bytes_per_block;
	if ( info->blockgroup_count * sizeof(Block_Group_Descriptor) % info->bytes_per_block )	++NumOfBlocksinGroupDesc;

	if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block * NumOfBlocksinGroupDesc, BLOCK_TO_SECTOR(info, info->supperblock_block_number + 1), (u32_t)info->group_desc_table) )
		printf("Could not read ext2 Block Group Descriptor Table!\n");
	
}

int ext2_create_ext_entry(void* t)
{
	FS_FileEntry* file = (FS_FileEntry*) t;
	FS_Tree* dir_node = file->directory;
	ext2* info = (ext2*) dir_node->responsible_driver->opaque;

//printf("try to allocate inode ... !\n");


	u32_t file_inode = allocate_inode(info, file->flags);
	if ( file_inode == -1 ) 	return -1;
	file->lbapos = file_inode;

//	printf("Allocate ext entry inode = %d!\n", file_inode);

	// create entry in inode table
	u32_t inode_table_block = getBlockNumber(info, BLOCKGROUP_VIA_INODE(info, file_inode), BLOCKNUM_INODE_TABLE);
	int inodes_per_block = info->bytes_per_block / info->size_of_inode_structure;
	u32_t block_num = inode_table_block + (file_inode - 1) / inodes_per_block;
	int block_index = (file_inode - 1) % inodes_per_block;

	u8_t* content = (u8_t*) malloc(info->bytes_per_block);
	memset(content, 0, info->bytes_per_block);
	if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
	{
		printf("Could not read ext2 inode entry for inode_num = %d!\n", file_inode);
		if (content) 	free(content);
		return -1;
	}
	Inode *InodeEntry = (Inode*) (content + block_index * info->size_of_inode_structure);
	memset(InodeEntry, 0, sizeof(Inode));

	// next, if dir allocate block
	InodeEntry->permission = INODE_REGILAR_FILE;
	if ( file->flags )
	{
		u32_t tmp_block = InodeEntry->direct_block[0] = allocate_block(info);
//printf("Allocate ext entry block = %d!\n", InodeEntry->direct_block[0]);
		InodeEntry->size_lo = info->bytes_per_block;
		InodeEntry->permission = INODE_DIRECTORY;

		u8_t* tmp = (u8_t*) malloc(info->bytes_per_block);
		memset(tmp, 0, info->bytes_per_block);

		// current directory .
		*(u32_t*) tmp = file_inode;
		*(u16_t*) (tmp + sizeof(u32_t)) = 12;
		*(u8_t*) (tmp + sizeof(u32_t) + sizeof(u16_t)) = 1;	// file length '.'
		*(u8_t*) (tmp + sizeof(u32_t) + sizeof(u16_t) + sizeof(u8_t)) = EXT2_FILE_DIRECTORY;
		*(u8_t*) (tmp + DIR_FILE_OFFSET ) = '.';

		// parent directory ..
		*(u32_t*) (tmp + 12 )= dir_node->lbapos;
		*(u16_t*) (tmp + 12 + sizeof(u32_t)) = info->bytes_per_block - 12;
		*(u8_t*) (tmp + 12 + sizeof(u32_t) + sizeof(u16_t)) = 2;	// file length '..'
		*(u8_t*) (tmp + 12 + sizeof(u32_t) + sizeof(u16_t) + sizeof(u8_t)) = EXT2_FILE_DIRECTORY;
		*(u8_t*) (tmp + 12 + DIR_FILE_OFFSET ) = '.';
		*(u8_t*) (tmp + 12 + DIR_FILE_OFFSET + 1) = '.';

		if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, tmp_block), (u32_t)tmp) )
		{
			printf("Could not read ext2 inode entry for tmp_block = %d!\n", tmp_block);
			if (content) 	free(content);
			if(tmp)		free(tmp);
			return -1;
		}

		if(tmp)		free(tmp);

	}
	//InodeEntry->permission|= 0x0040 | 0x0080 | 0x0020 ;	// read write for the owner, read for group
	if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
	{
		printf("Could not read ext2 inode entry for inode_num = %d!\n", file_inode);
		if (content) 	free(content);
		return -1;
	}

 

	// finally, create record in parent directory
	int fileName_len = strlen(file->filename);
	/*		inode number	entry_len	filenameLen    type	      fileName length	*/		
	int new_entry_len = sizeof(u32_t) + sizeof(u16_t) + sizeof(u8_t) + sizeof(u8_t) + fileName_len;
	if ( new_entry_len % 4 )
		new_entry_len = new_entry_len + ( 4 - (new_entry_len % 4));
	int offset, done = 0, dir_block_index = 0, end_of_dir, name_len, actual_entry_len;
	u16_t entry_len;
	Inode *InodeDirEntry = getInodeEntry(info, dir_node->lbapos /* inode number of the parent directory */ );
	block_num = getBlockNumbyId(info, dir_block_index, InodeDirEntry);

//printf("Allocate ext block_num = %d!\n", block_num);
	while ( block_num > 0 && !done )
	{
		memset(content, 0, info->bytes_per_block);
		if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
		{
			printf("Could not read ext2 inode entry for block_num = %d!\n", block_num);
			if (content) 	free(content);
			return -1;
		}	
//dump_hexa(content, 128);
//while(1);
		offset = 0;
		u8_t* ptr = content;
		while ( offset < info->bytes_per_block )
		{
			entry_len = *(u16_t*) (ptr + sizeof(u32_t));

			name_len = *(u8_t*)(ptr + sizeof(u32_t) + sizeof(u16_t));
			actual_entry_len = sizeof(u32_t) + sizeof(u16_t) + sizeof(u8_t) + sizeof(u8_t) + name_len;
			if ( actual_entry_len % 4 )
				actual_entry_len = actual_entry_len + ( 4 - (actual_entry_len % 4));

//printf("entry_len = %d   actual_entry_len = %d\n",entry_len ,actual_entry_len);
//printf("entry_len = %d   offset = %d\n",entry_len ,offset);

			if ( entry_len + offset >= info->bytes_per_block )	// reach to end of directory
			{
				// check if there is a room for the new entry
				if ( info->bytes_per_block - offset >= new_entry_len )	// ok
				{
					// fix size of the previous entry
					*(u16_t*) (ptr + sizeof(u32_t)) = actual_entry_len;
					ptr+= actual_entry_len;
					offset+= actual_entry_len;

					u8_t type = ( file->flags ) ? EXT2_FILE_DIRECTORY : EXT2_FILE_REGULAR_FILE; 
					*((u32_t*) ptr) = file_inode;
					*((u16_t*) (ptr + sizeof(u32_t))) = info->bytes_per_block - offset;//new_entry_len;
					*((u8_t*) (ptr + sizeof(u32_t) + sizeof(u16_t))) = (u8_t) fileName_len;
					*((u8_t*) (ptr + sizeof(u32_t) + sizeof(u16_t) + sizeof(u8_t))) = type;
					strncpy(ptr + DIR_FILE_OFFSET, file->filename, fileName_len);

					if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
					{
						printf("Could not write ext2 inode entry for block_num = %d!\n", block_num);
						if (content) 	free(content);
						return -1;
					}

					done = 1;
				}
				break;
			}
			
			ptr+= entry_len;
			offset+= entry_len;
	
		}	// end inner while loop

		dir_block_index++;
		block_num = getBlockNumbyId(info, dir_block_index, InodeDirEntry);		

	}	// end outer while loop
				
	if ( !done )	// need to allocate new block and append it
	{
		block_num = allocate_block(info);
//printf("Allocate newwwww block = %d!\n", block_num);
		memset(content, 0, info->bytes_per_block);
		if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
		{
			printf("Could not read ext2 inode entry for block_num = %d!\n", block_num);
			if (content) 	free(content);
			return NULL;
		}
		u8_t type = ( file->flags ) ? EXT2_FILE_DIRECTORY : EXT2_FILE_REGULAR_FILE; 
		*((u32_t*) content) = file_inode;
		*((u16_t*) (content + sizeof(u32_t))) = info->bytes_per_block;//new_entry_len;
		*((u8_t*) (content + sizeof(u32_t) + sizeof(u16_t))) = (u8_t) fileName_len;
		*((u8_t*) (content + sizeof(u32_t) + sizeof(u16_t) + sizeof(u8_t))) = type;
		strncpy(content + DIR_FILE_OFFSET, file->filename, fileName_len);

		if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
		{
			printf("Could not write ext2 inode entry for block_num = %d!\n", block_num);
			if (content) 	free(content);
			return -1;
		}

		// append the block to the file's blocks
		appendBlockToList(info, block_num, dir_node->lbapos);

	}

	if (content) 	free(content);

	return 0;
}

int ext2_remove_ext_entry(void* t)
{
	FS_FileEntry* file = (FS_FileEntry*) t;
	FS_Tree* dir_node = file->directory;
	ext2* info = (ext2*) dir_node->responsible_driver->opaque;

	Inode *InodeFileEntry = getInodeEntry(info, file->lbapos /* inode number of the file */);
	if ( !InodeFileEntry )	return 0;

	u32_t next_block;
	int file_block_index = 0;

	next_block = getBlockNumbyId(info, file_block_index++, InodeFileEntry);

	while ( next_block )
	{
		free_block(info, next_block);
		
		next_block = getBlockNumbyId(info, file_block_index++, InodeFileEntry);		
	}


	// delete the file from parent directory
	int offset, done = 0, dir_block_index = 0;
	u8_t* content = (u8_t*) malloc(info->bytes_per_block);
	u16_t entry_len, removed_entry_len;
	u16_t* prev_entry_len;
	u8_t filename_len = strlen(file->filename), entry_name_len;
	Inode *InodeDirEntry = getInodeEntry(info, dir_node->lbapos /* inode number of the parent directory */ );
	u32_t block_num = getBlockNumbyId(info, dir_block_index, InodeDirEntry);
	u8_t* target_entry_offset = NULL;
	int entry_offset;
	while ( block_num > 0 && !done )
	{
		memset(content, 0, info->bytes_per_block);
		if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
		{
			printf("Could not read ext2 inode entry for block_num = %d!\n", block_num);
			if (content) 	free(content);
			return -1;
		}	
//dump_hexa(content, 128);
//while(1);
		offset = 0;
		u8_t* ptr = content;
		while ( offset < info->bytes_per_block )
		{
			entry_len = *(u16_t*) (ptr + sizeof(u32_t));

			entry_name_len = *(u8_t*)(ptr + sizeof(u32_t) + sizeof(u16_t));
	
			if( !target_entry_offset && entry_name_len == filename_len && !strncmp(file->filename, ptr + DIR_FILE_OFFSET, filename_len) )
			{
//printf("entry found ....\n");
				target_entry_offset = ptr;
				removed_entry_len = entry_len;
				entry_offset = offset;
			}
			else if ( target_entry_offset && entry_len + offset >= info->bytes_per_block)	// adjust last entry
			{
				*(u16_t*) (ptr + sizeof(u32_t)) = (*(u16_t*) (ptr + sizeof(u32_t))) + removed_entry_len;
				memmove(target_entry_offset ,target_entry_offset + removed_entry_len, info->bytes_per_block - (entry_offset + removed_entry_len));

				
//printf("moving mid entry....\n");
				if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_num = %d!\n", block_num);
					if (content) 	free(content);
					return -1;
				}
				break;
			}


//printf("entry_len = %d   actual_entry_len = %d\n",entry_len ,actual_entry_len);

			// test if it is the first and last enty in the directory, i.e the dir contains only 1 entry
			if ( target_entry_offset && entry_len >= info->bytes_per_block && offset == 0 )
			{
				// free the whole block
				free_block(info, block_num);
				freeBlockbyId(info, dir_block_index, dir_node->lbapos);
				done = 1;
				break;
			}
			else if( target_entry_offset && entry_len + offset >= info->bytes_per_block )	// the last entry
			{
				*prev_entry_len = *prev_entry_len + entry_len;
//printf("moving last entry....entry_len = %d offset = %d\n", entry_len ,offset);
				if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
				{
					printf("Could not read ext2 inode entry for block_num = %d!\n", block_num);
					if (content) 	free(content);
					return -1;
				}
				break;
			}
			
			prev_entry_len = ptr + sizeof(u32_t);
			ptr+= entry_len;
			offset+= entry_len;
	
		}	// end inner while loop

		dir_block_index++;
		block_num = getBlockNumbyId(info, dir_block_index, InodeDirEntry);		

	}	// end outer while loop

	

//printf("De-allocate ext entry inode = %d!\n", file->lbapos);
	free_inode(info, file->lbapos /* inode number of the file */, file->flags);
//dump_hexa(content, 128);
	if (InodeDirEntry)	free(InodeDirEntry);
	if (InodeFileEntry) 	free(InodeFileEntry); 
	if (content) 	free(content);

	return 0;
}

int ext2_read_file(void* file, u32_t start, u32_t length, u8_t* buffer)
{
	FS_FileEntry* file_entry = (FS_FileEntry*) file;
	ext2* info = (ext2*) file_entry->directory->responsible_driver->opaque;

	int remaining_bytes = length, block_count = 0, data_len, read_bytes = 0;
	u32_t file_block_index = start / info->bytes_per_block;
	int file_block_offset =  start % info->bytes_per_block;

	int len = length;
	if ( file_block_offset )
	{
		block_count++;
		len = len - (info->bytes_per_block - file_block_offset);
	}
	block_count+= len / info->bytes_per_block;
	if ( len % info->bytes_per_block ) block_count++;	

	Inode *InodeFileEntry = getInodeEntry(info, file_entry->lbapos /* inode number of the file */);
	if ( !InodeFileEntry )	return -1;

	u8_t* content = (u8_t*) malloc(info->bytes_per_block);
	u32_t next_block = getBlockNumbyId(info, file_block_index, InodeFileEntry);
	
	while( remaining_bytes > 0 && block_count > 0 && next_block > 0 )
	{

		memset(content, 0, info->bytes_per_block);
		if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, next_block), (u32_t)content) )
		{
			printf("Could not read ext2 inode entry for file_inode = %d!\n", next_block);
			if (content) 	free(content);
			return -1;
		}

		data_len = info->bytes_per_block - file_block_offset;
		if ( remaining_bytes < data_len )	data_len = remaining_bytes;
		memcpy(buffer+read_bytes, content+file_block_offset, data_len);

		read_bytes+= data_len;
		remaining_bytes-= data_len;
		file_block_offset = 0;

		// get next block
		next_block = getBlockNumbyId(info, ++file_block_index, InodeFileEntry);
		block_count--;
	} 

	if (InodeFileEntry) 	free(InodeFileEntry);
	if (content) 	free(content);

	return read_bytes;
}

int ext2_write_file(void* file, u32_t start_offset, u32_t length, u8_t* buffer)
{
	FS_FileEntry* file_entry = (FS_FileEntry*) file;
	ext2* info = (ext2*) file_entry->directory->responsible_driver->opaque;


	Inode *InodeFileEntry = getInodeEntry(info, file_entry->lbapos /* inode number of the file */);
	if ( !InodeFileEntry )	return -1;

//printf("inode = %d offset = %d length = %d\n", file_entry->lbapos, start_offset, length);
	u32_t next_block;
	int remaining_bytes = length, block_count = 0, data_len,  writtenbytes = 0;

	if( file_entry->size == 0 )	// the file is empty
	{
		next_block = allocate_block(info);
//		printf("ext2_write_file after allocation next_block = %d .\n", next_block);
		appendBlockToList(info, next_block, file_entry->lbapos);
	}

	u8_t* content = (u8_t*) malloc(info->bytes_per_block);
	int block_index = start_offset / info->bytes_per_block;
	int block_offset = start_offset % info->bytes_per_block;
	if( block_index )
		next_block = getBlockNumbyId(info, block_index, InodeFileEntry);
	
	
	if ( block_offset )
	{

		if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, next_block), (u32_t)content) )
		{
			printf("Could not read ext2 inode entry for file_block = %d!\n", next_block);
			if (content) 	free(content);
			return -1;
		}

		data_len = info->bytes_per_block - block_offset;
		if ( remaining_bytes < data_len )	data_len = remaining_bytes;
		memcpy(content + block_offset, buffer+writtenbytes, data_len);
		writtenbytes+= data_len;
		remaining_bytes-= data_len;
		block_offset = 0;
		if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, next_block), (u32_t)content) )
		{
			printf("Could not read ext2 inode entry for file_block = %d!\n", next_block);
			if (content) 	free(content);
			return -1;
		}
		
		// get next block
		next_block = getBlockNumbyId(info, ++block_index, InodeFileEntry);
	}

	block_count = remaining_bytes / info->bytes_per_block;
	if ( remaining_bytes % info->bytes_per_block ) block_count++;
	
	while( remaining_bytes > 0 && block_count > 0 )
	{

		// check if we reach to the end of the file
		if ( !next_block )	// it is the last block in the file chain
		{
			next_block = allocate_block(info);
			//printf("try to aloc next_block = %d \n", next_block);

			// add it to the linked blocks of the file
			appendBlockToList(info, next_block, file_entry->lbapos);
		}

		data_len = remaining_bytes < info->bytes_per_block ? remaining_bytes : info->bytes_per_block;
		memcpy(content, buffer+writtenbytes, data_len);
		writtenbytes+= data_len;
		remaining_bytes-= data_len;

		if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, next_block), (u32_t)content) )
		{
			printf("Could not read ext2 inode entry for file_block = %d!\n", next_block);
			if (content) 	free(content);
			return -1;
		}

		next_block = getBlockNumbyId(info, ++block_index, InodeFileEntry);

		block_count--;
	}


	// update file size
	file_entry->size+= writtenbytes;
	int inode_num = file_entry->lbapos;
	u32_t inode_table_block = getBlockNumber(info, BLOCKGROUP_VIA_INODE(info, inode_num), BLOCKNUM_INODE_TABLE);
	int inodes_per_block = info->bytes_per_block / info->size_of_inode_structure;
	u32_t block_num = inode_table_block + (inode_num - 1) / inodes_per_block;
	block_index = (inode_num - 1) % inodes_per_block;

	if ( !ide_read_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
	{
		printf("Could not read ext2 inode entry for inode_num = %d!\n", inode_num);
		if (content) 	free(content);
		return -1;
	}

	Inode *InodeEntry = (Inode*) (content + block_index * info->size_of_inode_structure);
	InodeEntry->size_lo+= writtenbytes;

	if ( !ide_write_sectors(info->drivenumber, info->sectors_per_block, BLOCK_TO_SECTOR(info, block_num), (u32_t)content) )
	{
		printf("Could not read ext2 inode entry for inode_num = %d!\n", inode_num);
		if (content) 	free(content);
		return -1;
	}

	if (InodeFileEntry) 	free(InodeFileEntry);	
	if (content)	free(content);	
//printf("writtenbytes = %d  ", writtenbytes);
	return writtenbytes;
}


void* ext2_get_directory(void* t)
{
	FS_Tree* dir_node = (FS_Tree*) t;

	FS_FileEntry* childs = ext2_parse_directory(dir_node, dir_node->responsible_driver->opaque, dir_node->lbapos /* inode number */ );

	return ((void*) childs);
}



