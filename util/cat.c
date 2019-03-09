

#include <types.h>

#include <thread.h>
#include <util.h>
#include <stdio.h>
#include <string.h>
#include <vga.h>
#include <memory.h>
#include <system.h>
#include <filesystem.h>

extern char working_dir[MAX_PATH];


int cat(Th_Param* param)
{

	int fd = -1;
	char file_path[MAX_PATH];

	if( param )
	{
		if ( param->argc < 2 )
		{
			printf("cat : Invalid Parameter(s).\n");
			if (param) 	free(param);
			return -1;
		}
		else if ( param->argv[1][0] == '/' )	// absolute path
		{
			strcpy(file_path, param->argv[1]);
		}
		else		// relative path
		{

			strcpy(file_path, working_dir);
			
			if ( strcmp(working_dir, "/") && working_dir[strlen(working_dir)-1] != '/' )
				strcat(file_path, "/");

			strcat(file_path, param->argv[1]);				
		}
		
	}

//printf("cat : I'll call _open with path = %s\n", file_path);

	fd = _open(file_path, _O_RDONLY);
	if ( fd >= 0 )
	{
		int file_size = file_length(fd);

		int data_len = 0;
		if ( file_size )
		{
			char* file_data = (char*) malloc(file_size);
			//memset(file_data, 0, file_size);
			
			data_len = _read(fd, file_data, file_size);

			if ( data_len != file_size )
			{
				printf("cat : Can't read all data of %s.\n", file_path);
				printf("cat : file size = %d , data read length = %d.\n", file_size, data_len);
			}
			else
			{

				int i = 0;
				for(i=0;i<data_len;++i)
				{
					//if ( file_data[i] == '\n' ) 	sleep(4000);
					putch(file_data[i]);
				}

			}

			_close(fd);
			if (file_data) 	free(file_data);
		}
	}
	else
	{
		printf("cat : Can't Open file %s for reading.\n", file_path);
	}

	


/*
printf("cat : test seek.\n", file_path);
fd = _open(file_path, _O_RDONLY);
if ( fd >= 0 )
{
	int size = 5 * 1024;
	char* data = (char*) malloc(size);
	memset(data, 0, size);

	int offset = 10000;

	_lseek(fd, offset, SEEK_SET);
	int len = _read(fd, data, size);
	printf("cat : bytes read = %d.\n", len);

	int j;
	for (j=0;j<len;++j)	
	{
		putch(data[j]);
		//sleep(9000);
	}
		
	_close(fd);

	if (data) free(data);
}
*/
	if (param) 	free(param);

	return 0;
}

