#ifndef _ISO9660_H
#define _ISO9660_H

// http://wiki.osdev.org/ISO_9660
#include <types.h>


// LBA location of primary volume descriptor on a CD
#define PVD_LBA 	0x10



/* Date structure as defined in the primary volume descriptor */
typedef struct
{
	char year[4];
	char month[2];
	char day[2];
	char hour[2];
	char minute[2];
	char second[2];
	char millisecond[2];
	char tz_offset;
} __attribute__((packed)) PVD_date;


//-------------------------------------------------------------------------------------------------------------------------------------


/* Per-file date entries (they differe from above to save space) */
typedef struct
{
	u8_t years_since_1900;
	u8_t month;
	u8_t day;
	u8_t hour;
	u8_t minute;
	u8_t second;
	char tz_offset;
} __attribute__((packed)) DIRECTORY_date;


/* Directory entry (may refer to a file or another directory) */
typedef struct _ISO9660_File
{
	u8_t length;
	u8_t attribute_length;

	u32_t extent_lba_lsb;		// lba for the directory
	u32_t extent_lba_msb;

	u32_t data_length_lsb;		// Data length
	u32_t data_length_msb;

	DIRECTORY_date recording_date;

	u8_t file_flags;
	u16_t interleave_unit_size;
	u16_t sequence_number_lsb;
	u16_t sequence_number_msb;
	u8_t filename_length;		// file name length

	/* NOTE: Filenames may be longer than 12 characters,
	 * up to filename_length in size. This does not trample
	 * any following structs, because where this happens,
	 * we walk a list of entries by using the length field
	 * which accounts for long filenames.
	 */
	char filename[12];

} __attribute__((packed)) ISO9660_File;



// Primary volume descriptor
typedef struct
{
	u8_t typecode;				// 0x01
	char standardidentifier[5];		// CD001 signature
	u8_t version;
	u8_t unused;
	char systemidentifier[32];
	char volumeidentifier[32];
	char unused2[8];
	u32_t lsb_volumespacesize;
	u32_t msb_volumespacesize;
	//char unused3[32];
	char escape_seq[8];
	char unused3[32-8];
	u16_t lsb_volumesetsize;
	u16_t msb_volumesetsize;
	u16_t lsb_volumeseqno;
	u16_t msb_volumeseqno;
	u16_t lsb_blocksize;
	u16_t msb_blocksize;

	/* OK, whoever thought it was a good idea to have
	 * dual-endianness copies of every value larger than
	 * one byte in the structure needs a kicking.
	 */
	u32_t lsb_pathtablesize;
	u32_t msb_pathtablesize;
	u32_t lsb_pathtable_L_lba;	// the lba of the path table
	u32_t lsb_optpathtable_L_lba;
	u32_t lsb_pathtable_M_lba;
	u32_t lsb_optpathtable_M_lba;

// Note that this is not an LBA address, it is the actual Directory Record, which contains a zero-length Directory Identifier, hence the fixed 34 byte size
	ISO9660_File root_directory;	// Directory entry for the root directory

	char volume_set_id[128];
	char publisher_id[128];
	char data_preparer[128];
	char application_id[128];
	char copyright_file[38];
	char abstract_file[36];
	char bibliographic_file[37];
	PVD_date volume_creation_date;
	PVD_date volume_modification_date;
	PVD_date volume_expire_date;
	PVD_date volume_effective_date;
	u8_t file_structure_version;
	char unused4;
	u8_t application_use[512];
	u8_t reserved[653];
} __attribute__((packed)) PVD;

typedef struct
{
} __attribute__((packed)) SVD;

FileSystem* mount_iso9660(int device_num);


int iso_read_file(void* file, u32_t start, u32_t length, u8_t* buffer);
void* iso_get_directory(void* t);

#endif
