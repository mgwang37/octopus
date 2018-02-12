#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdlib.h>    /* for exit */
#include <getopt.h>


#include "message_log.h"

#define  LogFileName  "Proxy"

static pthread_spinlock_t log_lock;
static char log_dir[512];
static FILE *f_output = NULL;

static void update ()
{
	static int current_day = 0;
	struct tm  gmtm;
	time_t curTime;
	char full_name[512];

	if (log_dir[0] == 0)
	{
		return;
	}

	time(&curTime);
	localtime_r(&curTime, &gmtm);

	if ((f_output == NULL) || (current_day != gmtm.tm_mday))
	{
		if (f_output)
		{
			fclose (f_output);
			f_output = NULL;
		}
		current_day = gmtm.tm_mday;
		sprintf (full_name, "%s/%s-%d-%04d-%02d-%02d.log", log_dir, LogFileName, getpid(), gmtm.tm_year+1900, gmtm.tm_mon+1, gmtm.tm_mday);

		f_output = fopen (full_name, "w+");
	}
	fprintf (f_output, "[%04d-%02d-%02d %02d:%02d:%02d]:", gmtm.tm_year+1900, gmtm.tm_mon+1, gmtm.tm_mday, gmtm.tm_hour, gmtm.tm_min, gmtm.tm_sec);
}

void log_printf(const char *format, ...)
{
	va_list ap;

	pthread_spin_lock(&log_lock);
	update ();
	va_start(ap, format);
	vfprintf (f_output, format, ap);
	va_end(ap);
	pthread_spin_unlock(&log_lock);
}

int  LogInit (char *log_file_dir)
{
	log_dir[0] = 0;

	if (log_file_dir == NULL)
	{
		f_output = stdout;
	}
	else
	{
		f_output = NULL;
		strcat (log_dir, log_file_dir);
	}

	pthread_spin_init(&log_lock, PTHREAD_PROCESS_PRIVATE);
	return 0;
}

void LogUninit ()
{
	pthread_spin_lock(&log_lock);

	if (log_dir[0] != 0)
	{
		if (f_output)
		{
			fclose (f_output);
			f_output = NULL;
		}
	}
	pthread_spin_unlock(&log_lock);

	pthread_spin_destroy(&log_lock);
}

