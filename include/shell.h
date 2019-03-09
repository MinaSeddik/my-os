#ifndef _SHELL_H
#define _SHELL_H

#include <types.h>
#include <util.h>
#include <thread.h>


#define MAX_CMD_LENGTH		100

//#define COMMANDS_LENGTH		7

typedef int (* FUNC_HANDLER)(Th_Param* param);

int cd(Th_Param* param);
int pwd(Th_Param* param);
int ls(Th_Param* param);
int cls(Th_Param* param);
int help(Th_Param* param);


typedef struct _command
{
	char cmd[MAX_CMD_LENGTH];
	FUNC_HANDLER func_handler;
}command;

void shell();


int get_cmd(char* cmd);


#endif

