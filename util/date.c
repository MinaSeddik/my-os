

#include <types.h>

#include <thread.h>
#include <util.h>
#include <stdio.h>
#include <memory.h>
#include <system.h>

static char week_days[8][4] = 
{ "", "Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat" };


static char month[13][4] = 
{ "", "Jan", "Feb", "Mar", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

int date(Th_Param* param)
{
	time_t* now = NULL;
	
	now = (time_t*) malloc(sizeof(time_t));
	local_time(now);

	printf("Date-time : %s %s %d %d:%d:%d %d \n", week_days[now->week_day], month[now->month], now->day, now->hour, now->minute, now->second, (int) (now->year + 2000 ));

	if (now) 	free(now);
	if (param) 	free(param);

	return 0;
}
