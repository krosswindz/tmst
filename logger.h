/*
 * Copyright (c) 2010 Kross Windz <krosswindz@gmail.com>
 * All rights reserved.
 */

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum LOG_LEVELS {
	LOG_ERR = 1,
	LOG_DBG = 1 << 1,
	LOG_INFO = 1 << 2,
	LOG_WARN = 1 << 3
};

extern FILE *log_fp;
extern uint8_t log_level;

#define logger_unimplemented() do { \
	logger (LOG_ERR, "UNIMPLEMENTED: %s_%d, %s\n", __FILE__, __LINE__, \
			__FUNCTION__); \
} while (0)

static inline void
logger (uint8_t level, char *args, ...)
{
	va_list ap;

	if (log_fp == NULL)
		return;

	if ((level & log_level) == 0)
		return;

	va_start (ap, args);
	vfprintf (log_fp, args, ap);
	va_end (ap);
	fflush (log_fp);

	return;
}

static inline int
logger_init (char *levels)
{
	char *level, *ptr, *token;

	ptr = strdup (levels);
	token = ptr;
	log_level = 0;
	while (1) {
		level = strsep (&token, "|");
		if (level == NULL)
			break;

		if (strcmp ("LOG_ERR", level) == 0)
			log_level |= LOG_ERR;

		if (strcmp ("LOG_DBG", level) == 0)
			log_level |= LOG_DBG;

		if (strcmp ("LOG_INFO", level) == 0)
			log_level |= LOG_INFO;

		if (strcmp ("LOG_WARN", level) == 0)
			log_level |= LOG_WARN;
	}

	if (ptr != NULL)
		free (ptr);

	return 0;
}

#endif /* __LOGGER_H__ */
