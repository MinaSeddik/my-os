

#include <types.h>
#include <system.h>

#include <stdio.h>


#define CMOS_ADDR	0X70
#define CMOS_DATA	0X71



#define SECOND_INDEX_REG	0x00
#define MINUTE_INDEX_REG	0x02
#define HOUR_INDEX_REG		0x04
#define WEEKDAY_INDEX_REG	0x06
#define DAYMONTH_INDEX_REG	0x07
#define MONTH_INDEX_REG		0x08
#define YEAR_INDEX_REG		0x09
#define CENTURY_INDEX_REG	0x32

int get_update_in_progress_flag();
u8_t get_RTC_value(int reg, int date_format);


void local_time(time_t* time)
{

	outportb(CMOS_ADDR, 0x0B);
	u8_t date_format = inportb(CMOS_DATA);
	int binary_format = date_format & 0x04;
	
	//printf("Date-time : %x %x\n",date_format, binary_format);

	while (get_update_in_progress_flag());


	time->second = get_RTC_value(SECOND_INDEX_REG, binary_format);
	time->minute = get_RTC_value(MINUTE_INDEX_REG, binary_format);
	time->hour = 	get_RTC_value(HOUR_INDEX_REG, binary_format);

	time->week_day = get_RTC_value(WEEKDAY_INDEX_REG, binary_format);

	time->day = get_RTC_value(DAYMONTH_INDEX_REG, binary_format);
	time->month = get_RTC_value(MONTH_INDEX_REG, binary_format);
	time->year = get_RTC_value(YEAR_INDEX_REG, binary_format);

	time->century = get_RTC_value(CENTURY_INDEX_REG, binary_format);

/*
	printf("Date-time : second = %d\n",second);
	printf("Date-time : minute = %d\n",minute);
	printf("Date-time : hour = %d\n",hour);
	printf("Date-time : week_day = %d\n",week_day);
	printf("Date-time : day = %d\n",day);
	printf("Date-time : month = %d\n",month);
	printf("Date-time : year = %d\n",year);
	printf("Date-time : century = %d\n",century);
*/
	//printf("Date-time : %d/%d/%d %d:%d:%d cent = %d\n", day, month, year, hour, minute, second, century );

}

u8_t get_RTC_value(int reg, int date_format) 	// date_format = 1 => binary format
{						// date_format = 0 => bcd format
	outportb(CMOS_ADDR, reg);
	u8_t value = inportb(CMOS_DATA);

//printf("reg : 0x%x  = 0x%x\n", reg, value);
	return (date_format) ? value : ( (value / 16) * 10) + (value & 0xF);
}


int get_update_in_progress_flag() 
{
      outportb(CMOS_ADDR, 0x0A);
      return ( inportb(CMOS_DATA) & 0x80 );
}
