

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


int touch(Th_Param* param)
{

	int fd = -1;
	char file_path[MAX_PATH];

	if( param )
	{
		if ( param->argc < 2 )
		{
			printf("touch : Invalid Parameter(s).\n");
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

//printf("touch : I'll call _open with path = %s\n", file_path);

	fd = _open(file_path, _O_WRONLY);

	if ( fd >= 0 )		_close(fd);
	else			printf("touch : Can't Create file %s .\n", file_path);

	if (param) 	free(param);

	return 0;
}

