/*
 * Copyright (c) 2010 Kross Windz <krosswindz@gmail.com>
 * All rights reserved.
 */

#include "bencode.h"
#include "logger.h"
#include "tracker.h"

static inline char *
bad_request (void)
{
	benc_data_t *bd = NULL;
	benc_dict_t *dict = NULL;
	char *str = NULL;

	dict = (benc_dict_t *) malloc (sizeof (benc_dict_t) * 2);
	if (dict == NULL)
		return NULL;

	dict[1].val = NULL;
	dict[0].key = strdup ("failure reason");
	bd = new_benc_data (BENC_TYPE_STR);
	if (bd == NULL) {
		free (dict[0].key);
		free (dict);
		return NULL;
	}

	bd->data.s = strdup ("Bad request, unsupported request from client.");
	if (bd->data.s == NULL) {
		free (bd);
		free (dict[0].key);
		free (dict);
		return NULL;
	}

	dict[0].val = bd;
	bd = new_benc_data (BENC_TYPE_DCT);
	if (bd == NULL) {
		free_benc_data (dict[0].val);
		free (dict[0].key);
		free (dict);
		return NULL;
	}

	bd->data.d = dict;
	str = benc_encode (bd);
	free_benc_data (bd);

	return str;
}

static inline char *
missing_pass_key (void)
{
	benc_data_t *bd = NULL;
	benc_dict_t *dict = NULL;
	char *str = NULL;

	dict = (benc_dict_t *) malloc (sizeof (benc_dict_t) * 2);
	if (dict == NULL)
		return NULL;

	dict[1].val = NULL;
	dict[0].key = strdup ("failure reason");
	bd = new_benc_data (BENC_TYPE_STR);
	if (bd == NULL) {
		free (dict[0].key);
		free (dict);
		return NULL;
	}

	bd->data.s = strdup ("Missing passkey, re-download torrent from "
			"forum.");
	if (bd->data.s == NULL) {
		free (bd);
		free (dict[0].key);
		free (dict);
		return NULL;
	}

	dict[0].val = bd;
	bd = new_benc_data (BENC_TYPE_DCT);
	if (bd == NULL) {
		free_benc_data (dict[0].val);
		free (dict[0].key);
		free (dict);
		return NULL;
	}

	bd->data.d = dict;
	str = benc_encode (bd);
	free_benc_data (bd);

	return str;
}

char *
tracker_handle_request (char *pkey, char *req, announce_info_t *ai,
		char *info_hash)
{
	if ((strcmp (pkey, "announce") == 0)
			|| (strcmp (pkey, "scrape") == 0)) {
		return missing_pass_key ();
	}

	if ((strcmp (req, "announce") != 0) && (strcmp (req, "scrape") != 0)) {
		return bad_request ();
	}

	debug_unimplemented ();
	return NULL;
}
