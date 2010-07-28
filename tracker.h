/*
 * Copyright (c) 2010 Kross Windz <krosswindz@gmail.com>
 * All rights reserved.
 */

#ifndef __TRACKER_H__
#define __TRACKER_H__

#include <stdint.h>

typedef struct __announce_info_type {
	char *info_hash;
	char *peer_id;
	char *ip;
	uint16_t port;
	uint64_t uploaded;
	uint64_t downloaded;
	uint64_t left;
	uint64_t corrupt;
	uint8_t compact;
	uint8_t no_peer_id;
	uint8_t event;
	uint32_t numwant;
	char *key;
	char *tracker_id;
} announce_info_t;

#endif /* __TRACKER_H__ */
