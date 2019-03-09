

#include <stdio.h>
#include <stdlib.h>
#include <vga.h>
#include <string.h>
#include <scheduler.h>
#include <memory.h>
#include <thread.h>
#include <filesystem.h>
#include <shell.h>

#define MAX_PATH	1024

char working_dir[MAX_PATH] = "/";

static char file_type[2][2] = { "f", "d"};

#define COMMANDS_LENGTH		13
command commands[COMMANDS_LENGTH] = {
{"date", date 		},
{"reboot", reboot 	},
{"cat", cat 		},
{"touch", touch		},
{"mkdir", mkdir		},
{"rm", rm		},
{"cp", cp		},

{"cd", 	cd 		},
{"pwd", pwd 		},
{"ls", ls 		},
{"cls", cls 		},

{"help", help 		},

{"dummy", NULL 	}	// to solve compilar problem -- invalid opcode

};

extern int sdebug;

int parse_cmd(char line[], char*** argv, int* argc);
int arguments_count(char* line);
void run_cmd(char** argv, int argc, int run_in_background);


int get_cmd(char* cmd)
{
	int j = 0;
	int index = -1;

	if ( !strcmp(cmd,"dummy") )	return index;		// to solve compilar problem -- invalid opcode

	for (j=0;j<COMMANDS_LENGTH;j++)
	{
		if ( !strcmp(cmd,commands[j].cmd) )
		{	
			index = j;
			break;
		}
	}

	return index;
}

void shell()
{


	char line[MAX_CMD_LENGTH];

	int argc;	
	char** argv;
	//int ret = 0;
	int run_in_background = 0;

	while ( 1 ) 	// infinite loop
	{
		
		run_in_background = 0;

		printf("$>");
		getline(line, MAX_CMD_LENGTH);
		


//printf("parse_cmd  before ... \n");
		run_in_background = parse_cmd(line, &argv, &argc);
//printf("parse_cmd  after ... \n");

		if ( run_in_background == -1 )		continue;

		
		if ( argc > 0 )
			run_cmd(argv, argc, run_in_background);



		// free argv
		//printf("argc = %d\n",argc);
		int i;
		for ( i=0;i<argc;i++)	
			if ( argv[i] )
			{
//printf("try to free argv[%d] = 0x%x ... \n", i, argv[i]);
				free (argv[i]);
			}
	
//printf("try to free argv ... \n");
		if ( argv )	free (argv);



	}

}


int parse_cmd(char line[], char*** argv, int* argc)
{
	int run_bg = 0;
	int i = 0;
	int len = strlen(line);
	
	char* cmd = (char*) malloc( len + 1);
	strcpy(cmd, line);
		
	// trim spaces from the end
	i = len - 1;
	while ( i >= 0 && cmd[i] == ' ' )	cmd[i--] = 0;	
	len = strlen(cmd);
	if ( len == 0 )
	{
		if ( cmd ) 	free(cmd);
		*argv = NULL;
		*argc = 0;
		return -1;
	}	
	
	if ( cmd[len-1] == '&' )
	{
		run_bg = 1;
		cmd[--len] = 0;
	}
	
	int args_count = arguments_count(cmd);
	if ( args_count <= 0 ) 
	{
		if ( cmd ) 	free(cmd);
		*argv = NULL;
		*argc = 0;
		return -1;
	}

	char **_argv = (char** ) malloc( args_count * sizeof(char*));
	memset(_argv, 0, args_count * sizeof(char*));
	
	i = 0;
	char* ptr = cmd, *arg;




	// trim spaces from begining
	while ( *ptr && *ptr == ' ')	++ptr;
	arg = ptr;
	while ( *ptr )
	{
		if ( *ptr == ' ' )
		{
			*ptr = 0;
			_argv[i] = (char* ) malloc( strlen(arg) + 1);
//printf("^^^^^^^^^^^^^args = %s, i = %d\n", arg, i);
			strcpy(_argv[i],arg);
			i++;
			arg = ptr + 1;

			// trim spaces between args
			while ( *arg && *arg == ' ' )	ptr = arg++;

		}
		
		++ptr;
	}

//printf("^^^^^^^^^^^^^args = %s, i = %d\n", arg, i);
	len = strlen(arg);
	if ( len )
	{
		_argv[i] = (char* ) malloc( len + 1);
		strcpy(_argv[i],arg);
		i++;
	}


	if ( cmd ) 	free(cmd);

//printf("_______________argv[0] = %x\n", _argv[0]);


	*argv = _argv;
	*argc = i;

	return run_bg;
	
}


int arguments_count(char* line)
{
	int i;
	int count = 0;

	for (i = 0 ;line[i]; ++i)
		if ( line[i] == ' ' )	++count;

	return count+1;
}


void run_cmd(char** argv, int argc, int run_in_background)
{
	int cmd_index = -1;
	FUNC_HANDLER func_handler = NULL;

	cmd_index = get_cmd(argv[0]);
	if ( cmd_index < 0 )
	{
		printf("Command \"%s\" NOT found.\n", argv[0]);	
		return;
	}
	func_handler = commands[cmd_index].func_handler;

	Th_Param* param = (Th_Param *) malloc(sizeof(Th_Param)); 
	param->argc = argc;
	param->argv = argv;

//printf("we 1 ...\n");
	THandler handler = create_thread(func_handler, param);
//printf("we 2 ...\n");

	if ( handler && ! run_in_background )
		block_current_process_waiting_process_to_finish(handler->pid);

//	if ( param ) 	free(param);   // to be free in the end of the function handler
}



int cd(Th_Param* param)
{
	int exists = 0;
	char dir[MAX_PATH];
	char* path = NULL;

	if( param )
	{
		if ( param->argc != 2 )
		{
			printf("cd : Invalid Parameter(s).\n");
		}
		else if ( param->argv[1][0] == '/' )	// absolute path
		{
			strcpy(dir, param->argv[1]);
			path = is_dir_exist(dir);
			if ( ! path )
				printf("cd : %s : No such file or directory.\n", param->argv[1]);
			else
				strcpy(working_dir, path);
		}
		else		// relative path
		{

			strcpy(dir, working_dir);
			
			if ( strcmp(working_dir, "/") && working_dir[strlen(working_dir)-1] != '/' )
				strcat(dir, "/");

			strcat(dir, param->argv[1]);
//printf("cd :  directory = %s\n", dir);
			path = is_dir_exist(dir);

			if ( ! path )
				printf("cd : %s : No such file or directory.\n", param->argv[1]);
			else
				strcpy(working_dir, path);				
		}
		
	}

	if(path)	free(path);

	if (param) 	free(param);
	return 0;
}

int pwd(Th_Param* param)
{
	printf("%s\n", working_dir);

	if (param) 	free(param);

	return 0;
}


int cls(Th_Param* param)
{
	clear_screen();

	if (param) 	free(param);

	return 0;
}


int ls(Th_Param* param)
{

	char dir[MAX_PATH];
	char *path = NULL;
	FS_FileEntry* files = NULL;

	if ( param->argc < 2 )
	{
		strcpy(dir, working_dir);
//		files = list_dir(dir);
	}
	else if ( param->argv[1][0] == '/' )	// absolute path
	{
		strcpy(dir, param->argv[1]);
//		files = list_dir(dir);
	}
	else		// relative path
	{
			
		strcpy(dir, working_dir);
		
		if ( strcmp(working_dir, "/") && working_dir[strlen(working_dir)-1] != '/' )
			strcat(dir, "/");

		strcat(dir, param->argv[1]);
//		path = is_dir_exist(dir);
		
//		if ( path )
//			files = list_dir(path);
			
	}

//	printf("ls : dir = %s\n", dir);
	path = is_dir_exist(dir);
//	printf("ls : path = %s\n", path);


	if ( ! path )
	{
		printf("ls : %s : No such file or directory.\n", dir);
	}
	else
	{
		files = list_dir(path);
		if ( files )
		{
			FS_FileEntry* tmp = files;
			while ( tmp )
			{
				printf("%s\t%d\t%s\n", file_type[tmp->flags], tmp->size, tmp->filename);	
				tmp = tmp->next;
			}
		}
	}

/*
	if ( ! files )
	{
		printf("ls : %s : No such file or directory.\n", dir);
	}
	else
	{
		FS_FileEntry* tmp = files;
		while ( tmp )
		{
			printf("%s\t%s\t%d\t\n", file_type[tmp->flags], tmp->filename, tmp->size);	
			tmp = tmp->next;
		}
	}
*/

	if( path ) 	free(path);

	if (param) 	free(param);
	return 0;
}


int help(Th_Param* param)
{
	printf("cls			Clear Screen.\n");
	printf("ls [path] 		list files in the directory.\n");
	printf("pwd			Print Working Directory.\n");
	printf("cat [file name]		Display content of the file.\n");
	printf("cd [path]		Enter Directory.\n");

	if (param) 	free(param);

	return 0;
}

