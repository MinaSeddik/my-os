

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


int rm(Th_Param* param)
{

	int removed;
	char file_path[MAX_PATH];

	if( param )
	{
		if ( param->argc < 2 )
		{
			printf("rm : Invalid Parameter(s).\n");
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

//printf("rm : I'll call _rm with path = %s\n", file_path);

	removed = _rm(file_path);

	if ( removed < 0 )			printf("rm : Can't remove file %s .\n", file_path);
	if ( removed == -2 )			printf("rm : Directory %s is NOT empty.\n", file_path);

	if (param) 	free(param);

	return 0;
}

