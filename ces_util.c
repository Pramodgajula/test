/*
* util.c
*
* Library of simple utility functions
*
* $Author: jwilbur $
* $Date: 2003/06/16 14:22:46 $
* $Header: /home/cvsroot/ces/src/ces_util/ces_util.c,v 1.19 2003/06/16 14:22:46 jwilbur Exp $
* $Revision: 1.19 $
* $Name:  $
*
*/

/* Standard includes */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

/* CES Project includes */
#include <ces_common.h>
#include <ces_util.h>


//static void print_tm(struct tm *tmPtr)
//{
	//printf("Mon  %d Day %d Year %d\n", tmPtr->tm_mon, tmPtr->tm_mday, tmPtr->tm_year);
	//printf("Hour %d Min %d Sec  %d\n", tmPtr->tm_hour, tmPtr->tm_min, tmPtr->tm_sec);
//}

/***********************************************************************
 * Computes 16 bit checksum of given set of 8-bit data bytes. The data
 * bytes are expected in __u8 array, buff and it's length in cnt. The
 * computed checksum is returned to the application in checksum. Success
 * or Failure of the computation process is the return value of the
 * function.
 ***********************************************************************/

int checksum_compute(unsigned char *buff, unsigned int cnt, unsigned short *checksum)
{
	unsigned int    outer_loop, inner_loop;
	unsigned int    temp, flag;

	temp = 0xFFFF;

	for (outer_loop = 0; outer_loop < cnt; outer_loop++)
	{
		temp = temp ^ buff[outer_loop];

		for (inner_loop = 1; inner_loop <= 8; inner_loop++)
		{
			flag = temp & 0x0001;
			temp = temp >> 1;

			if (flag)
				temp = temp ^ 0xA001;
		}
	}

#ifdef _BIG_ENDIAN_
	*checksum = (short)((temp & 0xFF) << 8) + ((temp & 0xFF00) >> 8);
#else
	*checksum = temp;
#endif

	return 0;
}

/***********************************************************************
 * Verifies whether the 16 bit checksum of given set of 8-bit data
 * bytes is good or bad. The data bytes are  expected in __u8 array
 * buff, with the checksum in last two bytes and the buffer length in
 * cnt. This function returns CHECKSUM_OK or CHECKSUM_BAD as the case
 * may be.
 ***********************************************************************/

int checksum_verify(unsigned char *buff, unsigned int cnt)
{
	unsigned int    outer_loop, inner_loop;
	unsigned int    temp, flag;

	temp = 0xFFFF;

	for (outer_loop = 0; outer_loop < cnt; outer_loop++)
	{
		temp = temp ^ buff[outer_loop];

		for (inner_loop = 1; inner_loop <= 8; inner_loop++)
		{
			flag = temp & 0x0001;
			temp = temp >> 1;

			if (flag)
				temp = temp ^ 0xA001;
		}
	}

	return ((!temp) ? CHECKSUM_OK : CHECKSUM_BAD);
}

/*******************************************************************************
<FUNCTION_HEADER>
Function:    get_sys_uptime
Description: Gets the current system uptime (in seconds).
Inputs:      none
Outputs:		t:  a structure containing the time
Concurrency:	Multi-thread Safe
</FUNCTION_HEADER>
*******************************************************************************/
void get_sys_uptime( unsigned int *time_sec )
{
	FILE	*fp;
	int	seconds=0;
	int	ch;

	fp = fopen( "/proc/uptime", "r" );
	if ( !fp ) {
		return;
	}

	while( 1 ) {
		ch = fgetc( fp );
		if ( ch == '.' ) {
			break;
		}
		seconds = (seconds * 10) + ch - '0';
	}

	fclose( fp );

	*time_sec = seconds;
} /* end of get_sys_uptime */

/*******************************************************************************
<FUNCTION_HEADER>
Function:    uptime_to_str
Description: Converts an uptime to a character string.
Inputs:
*               uptime: number of seconds of uptime
*               str:  receptacle, should be at least 17 characters in size
Returns:		none
Concurrency:	Multi-thread Safe
</FUNCTION_HEADER>
*******************************************************************************/
void uptime_to_str( unsigned int uptime, char *str )
{
int secs;
int mins;
int hours;
int days;

	secs = (uptime % 60);
	mins = (uptime / 60) % 60;
	hours = (uptime / 3600) % 24;
	days = (uptime / (3600*24));

	if ( days < 1 )
	{
		sprintf( str, "%02u:%02u:%02u", hours, mins, secs );
	}
	else
	{
		sprintf( str, "%d days %02u:%02u:%02u", days, hours, mins, secs );
	}
}

/*******************************************************************************
<FUNCTION_HEADER>
Function:    get_time
Description: Gets the current system time.
\              Note: the month will range from 0 to 11!
Inputs:      none
Outputs:		t:  a structure containing the time
Concurrency:	Multi-thread Safe
</FUNCTION_HEADER>
*******************************************************************************/
void get_time( struct tm *tmPtr )
{
struct tm *tempTM;
time_t	timeT;
struct timeval tv;

	// get the current time
	memset(tmPtr, 0, sizeof(struct tm));
	if(gettimeofday( &tv, NULL ))
	{
		//printf("get_time: gettimeofday failed (errno %x)\n", errno);
	}
	else
	{
		// convert the calendar time (pure seconds) into broken-down time
		timeT = tv.tv_sec;
		tempTM = gmtime(&timeT);
		memcpy(tmPtr, tempTM, sizeof(struct tm));
	}
} /* end of get_time */


/*******************************************************************************
<FUNCTION_HEADER>
Function:    set_time
Description: Sets the given system time.
Inputs:      t:  a structure containing the time
Outputs:		none
Returns:     0 on success, -1 on failure
Concurrency:	Multi-thread Safe
</FUNCTION_HEADER>
*******************************************************************************/
int set_time( struct tm *tmPtr )
{
time_t	timeT;
struct timeval tv;
int rc;

	// convert the broken-down time into calendar time
	//printf("set_time: ENTER\n");
	//print_tm(tmPtr);
	timeT = mktime( tmPtr );
	if(timeT == -1)
	{
		//printf("set_time: mktime failed\n");
		return -1;
	}
	else
	{
		// convert to the correct structure for setting the time
		tv.tv_usec = 0;
		tv.tv_sec = timeT;
		//printf("set_time: tv.tv_sec %ld\n", tv.tv_sec);
		// set the new time
		rc = settimeofday( &tv, NULL );
		if(rc)
		{
			//printf("set_time: settimeofday failed (errno %x)\n", errno);
			return -1;
		}
	}

	return 0;
} /* end of set_time */

/*******************************************************************************
<FUNCTION_HEADER>
Function:      convert_uptime_to_tm
Description:   Converts a specific uptime value into a tm struct
Inputs:
*              uptime: an uptime value to convert
*              t:  receptacle for the converted time
Returns:       none
Concurrency:   Multi-thread Safe
</FUNCTION_HEADER>
*******************************************************************************/
void convert_uptime_to_tm( unsigned int uptime, struct tm *tmPtr)
{
time_t timeT;
unsigned int currentUptime;
struct tm *tempTM;

	/* get the current date/time */
	time(&timeT);
	/* get the current uptime */
	get_sys_uptime( &currentUptime);

	/* Calculate the delta uptime (how long ago it occurred) */
	currentUptime -= uptime;
	/* Decrease the current date/time by the delta uptime */
	timeT -= currentUptime;
	tempTM = gmtime(&timeT);
	memcpy(tmPtr, tempTM, sizeof(struct tm));
} /* end of convert_uptime_to_tm */

const char month_str[12][4] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
/*******************************************************************************
<FUNCTION_HEADER>
Function:    date_to_str
Description: Converts a date to a character string based on the date format.
Inputs:
*              time: date and time structure
*              fmt:  date format
*              str:  output string
Outputs:		none
Concurrency:	Multi-thread Safe
</FUNCTION_HEADER>
*******************************************************************************/
void date_to_str( struct tm *tmPtr, DateFormat_E fmt, char *str )
{
	if(tmPtr->tm_mon > 11)
		return;
	switch(fmt)
	{

		case DF_GEN:
			sprintf( str, "%02d %s %04d", tmPtr->tm_mday, &month_str[tmPtr->tm_mon][0], tmPtr->tm_year + 1900 );
			break;

		case DF_EUR:
			sprintf( str, "%02d-%02d-%04d", tmPtr->tm_mday, tmPtr->tm_mon+1, tmPtr->tm_year + 1900 );
			break;
		default:
			sprintf( str, "%02d-%02d-%04d", tmPtr->tm_mon+1, tmPtr->tm_mday, tmPtr->tm_year + 1900 );
			break;

	} /* end of switch fmt */
} /* end of date_to_str */


/*******************************************************************************
<FUNCTION_HEADER>
Function:     time_to_str
Description:  Converts a time to a character string based on the date format.
Inputs:
*             time: date and time structure
*             str:  output string
Outputs:      none
Concurrency:  Multi-thread Safe
</FUNCTION_HEADER>
*******************************************************************************/
void time_to_str( struct tm *tmPtr, char *str )
{
	if (( tmPtr->tm_sec > 60 ) || ( tmPtr->tm_min > 60 ) || ( tmPtr->tm_hour > 23 )) {
		return;
	}
	/*<Bugzilla><1495><29-Sep-2008><KK> Updated the code in order to display the time as HH:MM:SS*/
	sprintf( str, "%02u:%02u:%02u", tmPtr->tm_hour, tmPtr->tm_min, tmPtr->tm_sec);
} /* end of time_to_str */

/*******************************************************************************
<FUNCTION_HEADER>
Function:    str_to_date
Description: Converts a character string to a date based on the date format.
Inputs:
*              time: date and time structure
*              fmt:  date format
*              str:  input string
Returns:		0 on success
Concurrency:	Multi-thread Safe
</FUNCTION_HEADER>
*******************************************************************************/
int str_to_date( char *str, struct tm *tmPtr, DateFormat_E fmt )
{
char *mon = 0;
char *day = 0;
char *year = 0;
int len,i;

	//printf("str_to_date: ENTER %s\n", str);
	/* Check the input args */
	if((str == NULL) || (tmPtr == NULL))
	{
		//printf("str_to_date: bad pointer\n");
		return -1;
	}
	/* Convert string to use only /'s */
	for(mon = str; *mon != '\0'; mon++)
	{
		if( (*mon == '-') || (*mon == ' ') )
			*mon = '/';
	}

	/* Get pointers to the day, month, and year */
	switch(fmt)
	{
		case DF_GEN:	// dd/MON/yyyy
		case DF_EUR:		// dd/mm/yyyy
			day = str;
			mon = strstr(str, "/");
			if(mon == NULL)
			{
				//printf("str_to_date: couldn't find month\n");
				return -1;
			}
			*mon = '\0';
			mon++;
			year = strstr(mon, "/");
			break;
		case DF_USA:		// mm/dd/yyyy
			mon = str;
			day = strstr(str, "/");
			if(day == NULL)
			{
				//printf("str_to_date: couldn't find day\n");
				return -1;
			}
			*day = '\0';
			day++;
			year = strstr(day, "/");
			break;
		default:
			break;
	}
	/* Check the validity of the year string */
	if(year == NULL)
	{
		//printf("str_to_date: couldn't find year\n");
		return -1;
	}
	*year = '\0';
	year++;
	//printf("str_to_date: %s %s %s#\n", day, mon, year);
	len = strlen(year);
	if(len != 4)
	{
		//printf("str_to_date: invalid year (%d)\n", len);
		return -1;
	}
	/* Convert the day and year */
	tmPtr->tm_mday = atoi(day);
	tmPtr->tm_year = atoi(year);
	/* Convert the month according to the date format */
	if(fmt == DF_GEN)
	{
		for(i=0;i<12;i++)
			if(!strcasecmp(mon, month_str[i]))
				break;
		if(i > 11)
		{
			//printf("str_to_date: invalid month\n");
			return -1;
		}
		tmPtr->tm_mon = i;
	}
	else

	tmPtr->tm_mon = atoi(mon) - 1;

	/* Check for Valid Day , Year, and Month */
	if( (tmPtr->tm_mon > 11) )
	{
		//printf("##Invalid Month\n");
		return -1;
	}

	if ( (tmPtr->tm_year < 1970) || (tmPtr->tm_year > 2037) )
	{
		//printf("##Invalid Year\n");
		return -1;
	}
	tmPtr->tm_year -= 1900;

	switch (tmPtr->tm_mon)
	{
		case 1: //February has 28 Days
			{
			int LEAPYEAR = FALSE;
			if((tmPtr->tm_year % 4 == 0 && tmPtr->tm_year % 100 != 0) || tmPtr->tm_year % 400 == 0)
				LEAPYEAR = TRUE;
			if(LEAPYEAR)
			{
				if(tmPtr->tm_mday > 29 || tmPtr->tm_mday < 1)
				{
					//printf("##Invalid Day\n");
					return -1;
				}
			}
			else
			{
				if(tmPtr->tm_mday > 28 || tmPtr->tm_mday < 1)
				{
					//printf("##Invalid Day\n");
					return -1;
				}
			}
			break;
			}
		case 3:
		case 5:
		case 8:
		case 10:
			if(tmPtr->tm_mday > 30 || tmPtr->tm_mday < 1)
			{
				//printf("##Invalid Day\n");
				return -1;
			}
			break;
		default:
			if(tmPtr->tm_mday > 31 || tmPtr->tm_mday < 1)
			{
				//printf("##Invalid Day\n");
				return -1;
			}
			break;
	}

	//printf("str_to_date: %d / %d / %d\n", tmPtr->tm_mon, tmPtr->tm_mday, tmPtr->tm_year);
	return 0;
} /* end of str_to_date */


/*******************************************************************************
<FUNCTION_HEADER>
Function:    str_to_time
Description: Converts a character string to a time.
Inputs:
*              time: date and time structure
*              str:  input string
Returns:		0 on success
Concurrency:	Multi-thread Safe
</FUNCTION_HEADER>
*******************************************************************************/
int str_to_time( char *str, struct tm *tmPtr )
{
char *hr, *min, *sec;

	if((str == NULL) || (tmPtr == NULL))
		return -1;
	hr = str;
	min = strstr(str, ":");
	if(min == NULL)
		return -1;
	*min = '\0';
	min++;
	sec = strstr(min, ":");
	if(sec != NULL)
	{
		*sec = '\0';
		sec++;
		tmPtr->tm_sec = atoi(sec);
	}
	tmPtr->tm_hour = atoi(hr);
	tmPtr->tm_min = atoi(min);
	if(tmPtr->tm_hour >= 24 || tmPtr->tm_min >=60 || tmPtr->tm_sec >=60)
		return -1;
	//printf("str_to_time: %d:%d:%d\n", tmPtr->tm_hour, tmPtr->tm_min, tmPtr->tm_sec);
	return 0;
} /* end of str_to_time */

/****************************************************************
<FUNCTION_HEADER>
FUNCTION:       tv_sub
DESCRIPTION:	Subtracts two timeval's. out = out - in
INPUTS: 		out, in
RETURNS:		none
CONCURRENCY:	multi-thread safe
</FUNCTION_HEADER>
****************************************************************/
void tv_sub(struct timeval *out, struct timeval *in)
{
	if((out->tv_usec -= in->tv_usec) < 0) {
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
} /* end of tv_sub */

/*******************************************************************************
<FUNCTION_HEADER>
Function:     ms_wait
Description:  Waits a given number of milliseconds.
Inputs:       ms: number of milliseconds to wait
Returns:      none
Concurrency:  Multi-thread Safe
</FUNCTION_HEADER>
*******************************************************************************/
void ms_wait( int ms )
{
	struct timeval tv, startTime, curTime;
	int status;

	tv.tv_sec = (ms / 1000);
	tv.tv_usec = (ms % 1000) * 1000;

	gettimeofday(&startTime,0);

	do
	{
		tv.tv_sec = (ms / 1000);
		tv.tv_usec = (ms % 1000) * 1000;
		gettimeofday(&curTime,0);
		tv_sub(&curTime, &startTime);
		if(( (curTime.tv_sec*1000) + curTime.tv_usec/1000) > ms)
			return;
		tv_sub(&tv, &curTime);

		status = select( 0, NULL, NULL, NULL, &tv );

	} while((status == -1) && (errno == EINTR));
} /* end of ms_wait */

/*******************************************************************************
<FUNCTION_HEADER>
FUNCTION:		swap16
DESCRIPTION:	Swaps the bytes of a 16-bit value
INPUTS:         val - the 16-bit word to swap
OUTPUTS:		none
RETURNS:		the swapped value
</FUNCTION_HEADER>
*******************************************************************************/
unsigned short swap16(unsigned short val)
{
	return ( ((val >> 8)&0x00ff) | ((val << 8)&0xff00) );
}


/****************************************************************
<FUNCTION_HEADER>
FUNCTION:		check_fd_read
DESCRIPTION:	Checks if data is available on the given file descriptor
INPUTS:
*  fd    - a file descriptor
*  delay - timeout in msec.
*  wait  - if TRUE, make sure we don't return on all signals until a timeout occurs
*
*					If delay = 0,  the call will return immediately
*					If delay = -1, the call will block indefinitely
RETURNS:
*               -1 - on error
*                0 - on timeout
*                1 - datagram available
CONCURRENCY:	Multi-thread
</FUNCTION_HEADER>
****************************************************************/
int check_fd_read(int fd, int delay, int wait)
{
fd_set readfds;
int err;
struct timeval tv = { 0, 0 };
struct timeval *tvPtr = 0;
int retval = 0;
struct timeval startTime, curTime;

	/* Set up for the select call */
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	/* Check if a datagram is available */
	if(delay >= 0)
	{
	  tv.tv_sec = delay / 1000;
    tv.tv_usec = (delay % 1000) * 1000;
		tvPtr = &tv;
	}

	if(wait) {
	  gettimeofday(&startTime,0);
	  do
	  {
		  if(delay >= 0) {
		    tv.tv_sec = (delay / 1000);
		    tv.tv_usec = (delay % 1000) * 1000;
		    gettimeofday(&curTime,0);
		    tv_sub(&curTime, &startTime);
		    if( ((curTime.tv_sec*1000) + ((curTime.tv_usec+500)/1000)) > delay){
			    err = 0;
			    break;
		    }
		    tv_sub(&tv, &curTime);
	    }
		  err = select( fd+1, &readfds, 0, 0, tvPtr );
	  } while((err == -1) && (errno == EINTR));

	} else { //wait = FALSE
    err = select( fd+1, &readfds, 0, 0, tvPtr );
	}

	if(err == 0) {
	  retval = 0;
	}
	else if(err < 0) {
		retval = -1;
	}
	else {
		retval = 1;
	}
	return(retval);
} /* end of check_fd_read */

/****************************************************************
<FUNCTION_HEADER>
FUNCTION:		check_fds_read
DESCRIPTION:	Checks if a data is available on the given file descriptors
INPUTS:
*  fds     - pointer to an array of file descriptors
*  numFDs  - the number of file descriptors in the array
*  results - pointer to the results
*  delay   - timeout in msec, or -1 to block
*  wait    - if TRUE, make sure we don't return on all signals until a timeout occurs
RETURNS:
*               -1 - on error
*               0  - on timeout
*               >0 representing the number of descriptors which have data available
CONCURRENCY:	Multi-thread
</FUNCTION_HEADER>
****************************************************************/
int check_fds_read(int *fds, int numFDs, int *results, int delay, int wait)
{
fd_set readfds;
int err;
struct timeval tv = { 0, 0 };
struct timeval *tvPtr = 0;
int retval = 0;
int maxFD = 0;
int i;
struct timeval startTime, curTime;

	memset(results, 0, sizeof(int) * numFDs);

	/* Set up for the select call */
	FD_ZERO(&readfds);
	for(i = 0; i < numFDs; i++)
	{
		if(fds[i] == -1)
			continue;
		FD_SET(fds[i], &readfds);
		if(fds[i] > maxFD)
			maxFD = fds[i];
	}

	if(delay >= 0)
	{
	  tv.tv_sec = delay / 1000;
    tv.tv_usec = (delay % 1000) * 1000;
		tvPtr = &tv;
	}


	if(wait) {
	  gettimeofday(&startTime,0);
	  do
	  {
      if(delay >= 0) {
		    tv.tv_sec = (delay / 1000);
		    tv.tv_usec = (delay % 1000) * 1000;
		    gettimeofday(&curTime,0);
		    tv_sub(&curTime, &startTime);

		    if( ((curTime.tv_sec*1000) + ((curTime.tv_usec+500)/1000)) > delay){
			    err = 0;
			    break;
		    }
		    tv_sub(&tv, &curTime);
		  }
	    err = select(maxFD + 1, &readfds, 0, 0, tvPtr);
	  } while((err == -1) && (errno == EINTR));

	} else {  //wait = FALSE
    err = select(maxFD + 1, &readfds, 0, 0, tvPtr);
	}

	if(err == 0){		// timeout
		retval = 0;
	}
	else if(err < 0){	// error
		retval = -1;
	}
	else				// data available
	{
		retval = 0;

		for(i = 0; i < numFDs; i++)
		{
			if(fds[i] == -1){
				continue;
			}
			if(FD_ISSET(fds[i], &readfds))
			{
				results[i] = 1;
				retval++;
			}
		}
	}
	return(retval);
} /* end of check_fds_read */




/****************************************************************
<FUNCTION_HEADER>
FUNCTION:		convert_ip_address_string
DESCRIPTION:	Converts a string of the format ##.##.##.## to a
*								long int.  The form of the long int is 0xaabbccdd
*								where aa is the first value before the first period
*								and dd is the last value after the last period.  bb
*								and cc are the middle values.
INPUTS: 		ipaddress is a null terminated string of the form described above.
RETURNS:	Returns the ip address as a long int as described above.  If an error
*						occurs (such as an invalid ip address) the return value is 0.
CONCURRENCY:	Multi-thread
</FUNCTION_HEADER>
****************************************************************/
long convert_ip_address_string( char* ipaddress )
{
  char ip[32];
  int N = strlen(ipaddress);
  int i = N;
  long retVal = 0;

  if(strlen(ipaddress) > 15) {
    return(0);
  }

  strcpy(ip, ipaddress);  //save local copy to preserve original

   //look for last dot
  while(ip[--i] != '.') {if(i<0) return(0);}
  retVal |= (atoi(&ip[i+1]) & 0xFF) << 0;
  ip[i] = '\0';

  //look for next to last dot
  while(ip[--i] != '.') {if(i<0) return(0);}
  retVal |= (atoi(&ip[i+1]) & 0xFF) << 8;
  ip[i] = '\0';

  //look for first dot
  while(ip[--i] != '.') {if(i<0) return(0);}
  retVal |= (atoi(&ip[i+1]) & 0xFF) << 16;
  ip[i] = '\0';

  //get first digit
  retVal |= (atoi(ip) & 0xFF) << 24;

  return(retVal);
}


/****************************************************************
<FUNCTION_HEADER>
FUNCTION:		convert_ip_to_string
DESCRIPTION:	Converts an ip address which is in the format of
*								0xaabbccdd to a string in the form of ##.##.##.##
*
*						There is no checking for length
*						in this function.  Make sure buff is of sufficient length
*						(at least 16 characters to accomodate ###.###.###.###
*
INPUTS: 		ip is a long of the form described above.
RETURNS:	Returns the pointer to buff.
CONCURRENCY:	Multi-thread
</FUNCTION_HEADER>
****************************************************************/
char* convert_ip_to_string( long ip, char* buff  )
{
	int a = (ip >> 24) & 0xFF;
	int b = (ip >> 16) & 0xFF;
	int c = (ip >>  8) & 0xFF;
	int d =  ip & 0xFF;
	sprintf(buff, "%d.%d.%d.%d", a, b, c, d);
	return(buff);
}

/****************************************************************
<FUNCTION_HEADER>
FUNCTION:		convert_hwaddr_to_string
DESCRIPTION:	Converts a hardware address which is in the format of
\					0xaabbccdd to a string in the form of ##:##:##:##:##:##
\
\				There is no checking for length
\				in this function.  Make sure buff is of sufficient length
\				(at least 16 characters to accomodate ##:##:##:##:##:##)
\
INPUTS: 		hwAddr - pointer to an array of unsigned chars
RETURNS:	Returns the pointer to buff.
CONCURRENCY:	Multi-thread
</FUNCTION_HEADER>
****************************************************************/
char* convert_hwaddr_to_string( unsigned char *hwAddr, char* buff  )
{
	sprintf(buff, "%02x:%02x:%02x:%02x:%02x:%02x",
			*(hwAddr), *(hwAddr+1), *(hwAddr+2), *(hwAddr+3), *(hwAddr+4), *(hwAddr+5));
	return(buff);
}


/****************************************************************
<FUNCTION_HEADER>
PUBLIC FUNCTION: int get_unique_filename(char* dir, char* buff)

DESCRIPTION:
	This function creates a unique file in the specified directory.  The
	file is created, but not opened.  It will be the responsibility
	of the calling process to remove the file when finished with it.

	NOTE that there is no checking on the length of buff.  It must be
	at least length(dir) + length(prefix) + 10 characters long.

	(uses the mkstemp function)

Example:
	err = get_unique_filename("/tmp", "prog_", buff);

INPUTS:
*				dir 		= directory where file should be located (can be NULL)
*				prefix	= prefix of the file.  For example, if you are creating
\									a temporary socket for the programming process, you
\									may want to have a prefix of "prog_" (can be NULL)
*				buff 		= a buff which will contain the full path and name of the
\									newly created file (upon success)

RETURNS:
	Returns 0 if error occured or 1 if a unique file has been
	created.

CONCURRENCY:
	Multi-thread
</FUNCTION_HEADER>
****************************************************************/
int get_unique_filename(char* dir, char* prefix, char* buff)
{
	int t, val, retVal = 0;

	strcpy(buff, "");
	if(dir) {
		if(dir[0] != '/') {
			strcat(buff,"/");
		}
		strcat(buff, dir);
		t = strlen(buff);
		if(t>0) {
			if(buff[t-1] == '/') {
				buff[t-1] = '\0';
			}
		}
	}
	strcat(buff, "/");

	if(prefix) {
		strcat(buff, prefix);
	}
	strcat(buff, "XXXXXX");
	val = mkstemp(buff);
	if(val >= 0) {
	  close(val);
		remove(buff);
		retVal = 1;
	} else {
		strcpy(buff, "");
		retVal = 0;
	}

	return(retVal);
}

/*******************************************************************************
<FUNCTION_HEADER>
FUNCTION:     read_dev_urandom
DESCRIPTION:  Opens the /dev/urandom device, reads the specified number of bytes
\                 and close the file.
INPUTS:
*             length - number of bytes to read (inherently, the size of buf)
*             buf - receptacle for the data
RETURNS:      0 on success, -1 on error
CONCURRENCY:  MULTI_THREAD_SAFE
</FUNCTION_HEADER>
*******************************************************************************/
int read_dev_urandom(unsigned char *buf, int length)
{
int fd;
int retval = 0;

	/* Open the device */
	fd = open("/dev/urandom", O_RDONLY);
	if(fd < 0)
	{
		retval = -1;
	}
	else
	{
		/* Read the requested data */
		retval = read(fd, buf, length);
		/* Close the device */
		close(fd);
	}
	return retval;
} /* end of read_dev_urandom */

/*
*
* $Log: ces_util.c,v $
* Revision 1.19  2003/06/16 14:22:46  jwilbur
* added parameter to check_fd(s)_read to force waiting
*
* Revision 1.18  2003/06/13 19:17:07  jwilbur
* docs
*
* Revision 1.17  2003/06/13 18:42:20  jwilbur
* fixed problem in check_fd functions where select was used wrong
*
* Revision 1.16  2003/06/04 13:50:28  jwilbur
* return(buff)
*
* Revision 1.15  2003/05/22 15:52:02  ctedrow
* Added hwaddr_to_string
*
* Revision 1.14  2003/04/18 15:12:02  awright
* moved set_console_baud to menu_util
*
* Revision 1.13  2003/03/25 22:19:52  pruhland
* set_time returns int, 0 on success, -1 on failure
*
* Revision 1.12  2003/03/05 15:32:45  pruhland
* console uses console device
*
* Revision 1.11  2003/01/06 13:01:03  ctedrow
* Fixed set_time; Added str to date/time
*
* Revision 1.10  2003/01/02 14:27:52  ctedrow
* Added uptime_to_str
*
* Revision 1.9  2002/12/20 20:01:29  ctedrow
* Added a check for invalid month
*
* Revision 1.8  2002/12/19 19:50:08  ctedrow
* Added date and time APIs
*
* Revision 1.7  2002/12/19 15:51:33  ctedrow
* Added set_console_baud; Updated documentation
*
* Revision 1.6  2002/11/05 20:10:40  ctedrow
* Added read_dev_urandom
*
* Revision 1.5  2002/10/16 18:24:14  jwilbur
* added get unique filename function
*
* Revision 1.4  2002/09/26 20:12:48  jwilbur
* *** empty log message ***
*
* Revision 1.3  2002/09/18 16:23:57  jwilbur
* another function name change
*
* Revision 1.2  2002/09/18 16:17:46  jwilbur
* added IP/string conversions
*
* Revision 1.1  2002/08/26 15:18:19  ctedrow
* Renamed from mds_ to ces_
*
* Revision 1.1  2002/08/20 14:22:01  ctedrow
* Import to new cvs module
*
* Revision 1.4  2002/07/22 12:30:38  ctedrow
* Initial development
*
* Revision 1.3  2002/07/12 16:10:12  ctedrow
* Initial development
*
* Revision 1.2  2002/07/11 17:03:07  ctedrow
* Initial development
*
* Revision 1.1  2002/07/11 14:05:05  ctedrow
* Initial development
*
*/


