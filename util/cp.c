

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


int cp(Th_Param* param)
{

	int src_fd = -1;
	int dest_fd = -1;
	char src_file_path[MAX_PATH];
	char dest_file_path[MAX_PATH];

	if( param )
	{
		if ( param->argc < 3 )
		{
			printf("cp : Invalid Parameter(s).\n");
			if (param) 	free(param);
			return -1;
		}
		
		// for the source file
		if ( param->argv[1][0] == '/' )	// absolute path
		{
			strcpy(src_file_path, param->argv[1]);
		}
		else		// relative path
		{

			strcpy(src_file_path, working_dir);
			
			if ( strcmp(working_dir, "/") && working_dir[strlen(working_dir)-1] != '/' )
				strcat(src_file_path, "/");

			strcat(src_file_path, param->argv[1]);				
		}

		// for the dest file
		if ( param->argv[2][0] == '/' )	// absolute path
		{
			strcpy(dest_file_path, param->argv[2]);
		}
		else		// relative path
		{

			strcpy(dest_file_path, working_dir);
			
			if ( strcmp(working_dir, "/") && working_dir[strlen(working_dir)-1] != '/' )
				strcat(dest_file_path, "/");

			strcat(dest_file_path, param->argv[2]);				
		}

		
	}

	src_fd = _open(src_file_path, _O_RDONLY);
	if ( src_fd < 0 )
	{
		printf("cp : Can't open source file %s.\n", src_file_path);
		if (param) 	free(param);
		return -1;
	}

	int file_size = file_length(src_fd);

	int data_len = 0;
	char* file_data = NULL;
	if ( file_size > 0 )
	{
		file_data = (char*) malloc(file_size);
		//memset(file_data, 0, file_size);

		data_len = _read(src_fd, file_data, file_size);

		if ( data_len != file_size )
		{
			printf("cp : Can't read all data of %s.\n", src_file_path);
			printf("cp : file size = %d , data read length = %d.\n", file_size, data_len);
		}
		else
		{

			dest_fd = _open(dest_file_path, _O_WRONLY);
			if ( dest_fd < 0 )
			{
				printf("cp : Can't open destination file %s.\n", dest_file_path);
				_close(src_fd);
				if (param) 	free(param);
				if (file_data) 	free(file_data);
				return -1;
			}
			
			int written_data = _write(dest_fd, file_data, file_size);
			if ( data_len != file_size )
			{
				printf("cp : Can't write all into %s.\n", dest_file_path);
				printf("cp : file size = %d , data written length = %d.\n", file_size, written_data);
			}
		}
	}

	_close(src_fd);
	_close(dest_fd);

	if (file_data) 	free(file_data);
	if (param) 	free(param);

	return 0;
}

