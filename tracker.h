/*
 * Copyright (c) 2010 Kross Windz <krosswindz@gmail.com>
 * All rights reserved.
 */

#ifndef __TRACKER_H__
#define __TRACKER_H__

#include <inttypes.h>

typedef struct __announce_info_type {
	char *info_hash;
	char *peer_id;
	char *ip;
	uint16_t port;
	int64_t uploaded;
	int64_t downloaded;
	int64_t left;
	int64_t corrupt;
	uint8_t compact;
	uint8_t no_peer_id;
	uint8_t event;
	int32_t numwant;
	char *key;
	char *tracker_id;
} announce_info_t;

char *tracker_handle_request (char *, char *, announce_info_t *, char *);
#endif /* __TRACKER_H__ */
