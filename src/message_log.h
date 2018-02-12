#ifndef __MESSAGE_LOG_H__
#define __MESSAGE_LOG_H__

#include <stdarg.h>

void log_printf(const char *format, ...)__attribute__((format(printf, 1, 2)));

int  LogInit (char *log_dir);

void LogUninit ();

/*
*	LOGE() 在任何情况下都输出，LOGI (只有定义DEBUG 才输出)
*/
#define LOGE(format, ...)  log_printf(format, ##__VA_ARGS__)

#ifdef DEBUG
	#define LOGI(format, ...)  log_printf(format, ##__VA_ARGS__)
#else
	#define LOGI(format, ...) 
#endif

#endif /*__MESSAGE_LOG_H__*/
