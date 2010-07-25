/*
 * Copyright (c) 2010 Kross Windz <krosswindz@gmail.com>
 * All rights reserved.
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <stdint.h>

enum LOG_LEVELS {
	LOG_ERR = 1,
	LOG_INFO = 1 << 1,
	LOG_WARN = 1 << 2
};

extern FILE *log_fp;
extern uint8_t log_level;

static inline int
log_init (char *levels)
{
	fprintf (log_fp, "UNIMPLEMENTED: %s_%d\n", __FILE__, __LINE__);
	fprintf (log_fp, "Log level: %s\n", levels);
	return -1;
}

#endif /* __LOG_H__ */
