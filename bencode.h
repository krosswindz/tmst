/*
 * Copyright (c) 2010 Kross Windz <krosswindz@gmail.com>
 * All rights reserved.
 */

#ifndef __BENCODE_H__
#define __BENCODE_H__

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum BASE {
	BASE_BIN = 2,
	BASE_OCT = 8,
	BASE_DEC = 10,
	BASE_HEX = 16
};

enum BENC_TYPE {
	BENC_TYPE_DCT,
	BENC_TYPE_INT,
	BENC_TYPE_LST,
	BENC_TYPE_STR,
};

typedef struct __benc_data_type benc_data_t;
typedef struct __benc_dict_type benc_dict_t;
typedef struct __benc_list_type benc_list_t;

struct __benc_data_type {
	enum BENC_TYPE type;
	union {
		benc_dict_t *d;
		int64_t i;
		benc_data_t **l;
		char *s;
	} data;
};

struct __benc_dict_type {
	char *key;
	benc_data_t *val;
};

static inline benc_data_t *bdecode (char **, size_t *);
static inline benc_dict_t *bdecode_dct (char **, size_t *);
static inline int64_t bdecode_int (char **, size_t *);
static inline benc_data_t **bdecode_lst (char **, size_t *);
static inline char *bdecode_str (char **, size_t *);
static inline char *bencode_dct (benc_dict_t *);
static inline char *bencode_int (int64_t);
static inline char *bencode_lst (benc_data_t **);
static inline char *bencode_str (char *);
static inline char *lltostr (int64_t, int);
static inline benc_data_t *new_benc_data (enum BENC_TYPE);

static inline benc_data_t *
benc_decode (char *benc_str)
{
	size_t benc_str_len;

	benc_str_len = strlen (benc_str);

	return bdecode (&benc_str, &benc_str_len);
}

static inline char *
benc_encode (benc_data_t *bd)
{
	char *benc_str = NULL;

	if (bd == NULL)
		return NULL;

	switch (bd->type) {
		case BENC_TYPE_DCT: // dictionary
			benc_str = bencode_dct (bd->data.d);
			break;

		case BENC_TYPE_INT: // integer
			benc_str = bencode_int (bd->data.i);
			break;

		case BENC_TYPE_LST: // list
			benc_str = bencode_lst (bd->data.l);
			break;

		case BENC_TYPE_STR: // string
			benc_str = bencode_str (bd->data.s);
			break;

		default:
			return NULL;
	}

	return benc_str;
}

static inline void
free_benc_data (benc_data_t *bd)
{
	uint32_t i;

	if (bd == NULL)
		return;

	switch (bd->type) {
		case BENC_TYPE_DCT: // dictionary
			if (bd->data.d == NULL)
				break;

			for (i = 0; bd->data.d[i].val; i++) {
				free (bd->data.d[i].key);
				free_benc_data (bd->data.d[i].val);
			}

			free (bd->data.d);
			break;

		case BENC_TYPE_INT: // integer
			break;

		case BENC_TYPE_LST: // list
			if (bd->data.l == NULL)
				break;

			for (i = 0; bd->data.l[i]; i++)
				free_benc_data (bd->data.l[i]);

			free (bd->data.l);
			break;

		case BENC_TYPE_STR: // string
			if (bd->data.s)
				free (bd->data.s);

			break;

		default: // invalid.
			break;
	}

	free (bd);

	return;
}

static inline benc_data_t *
bdecode (char **str, size_t *str_len)
{
	benc_data_t *bd = NULL;

	if (str_len == 0)
		return NULL;

	switch (**str) {
		case 'd': // dictionary
			bd = new_benc_data (BENC_TYPE_DCT);
			if (bd == NULL)
				break;

			++(*str);
			--(*str_len);
			bd->data.d = bdecode_dct (str, str_len);
			if (**str != 'e') {
				free_benc_data (bd);
				return NULL;
			}

			++(*str);
			--(*str_len);
			break;

		case 'i': // integer
			bd = new_benc_data (BENC_TYPE_INT);
			if (bd == NULL)
				break;

			++(*str);
			--(*str_len);
			bd->data.i = bdecode_int (str, str_len);
			if (**str != 'e') {
				free_benc_data (bd);
				return NULL;
			}

			++(*str);
			--(*str_len);
			break;

		case 'l': // list
			bd = new_benc_data (BENC_TYPE_LST);
			if (bd == NULL)
				break;

			++(*str);
			--(*str_len);
			bd->data.l = bdecode_lst (str, str_len);
			if (**str != 'e') {
				free_benc_data (bd);
				return NULL;
			}

			++(*str);
			--(*str_len);
			break;

		case '0'...'9': // string
			bd = new_benc_data (BENC_TYPE_STR);
			if (bd == NULL)
				break;

			bd->data.s = bdecode_str (str, str_len);
			break;

		default: // invalid
			break;
	}

	return bd;
}

static inline benc_dict_t *
bdecode_dct (char **str, size_t *str_len)
{
	uint32_t i = 0;
	benc_dict_t *dict = NULL;

	while (**str != 'e') {
		dict = (benc_dict_t *) realloc (dict,
				sizeof (benc_dict_t) * (i + 2));
		if (dict == NULL)
			return NULL;

		dict[i].key = bdecode_str (str, str_len);
		dict[i].val = bdecode (str, str_len);
		++i;
	}

	dict[i].val = NULL;

	return dict;
}

static inline int64_t
bdecode_int (char **str, size_t *str_len)
{
	char *end;
	int64_t val;

	val = (int64_t) strtoll (*str, &end, 10);
	*str_len -= (end - *str);
	*str = end;

	return val;
}

static inline benc_data_t **
bdecode_lst (char **str, size_t *str_len)
{
	uint32_t i = 0;
	benc_data_t **list = NULL;

	while (**str != 'e') {
		list = (benc_data_t **) realloc (list,
				sizeof (benc_data_t *) * (i + 2));
		if (list == NULL)
			return NULL;

		list[i] = bdecode (str, str_len);
		++i;
	}

	list[i] = NULL;

	return list;
}

static inline char *
bdecode_str (char **str, size_t *str_len)
{
	int64_t ilen;
	ssize_t slen;
	size_t len;
	char *s = NULL;

	ilen = bdecode_int (str, str_len);
	if (ilen < 0)
		return NULL;

	slen = (ssize_t) ilen;
	if (sizeof (int64_t) != sizeof (ssize_t))
		if (ilen != slen)
			return NULL;

	len = (size_t) slen;
	if (len > *str_len - 1)
		return NULL;

	if (**str != ':')
		return NULL;

	++(*str);
	--(*str_len);
	s = (char *) malloc (sizeof (char) * (len + 1));
	strncpy (s, *str, len);
	s[len] = '\0';
	*str += len;
	*str_len -= len;

	return s;
}

static inline char *
bencode_dct (benc_dict_t *dict)
{
	char *key = NULL, *str = NULL, *val = NULL;
	uint32_t i;
	size_t len = 0;

	if (dict[0].val == NULL)
		return NULL;

	key = bencode_str (dict[0].key);
	if (key == NULL)
		return NULL;

	val = benc_encode (dict[0].val);
	if (val == NULL) {
		free (key);
		return NULL;
	}

	len = strlen (key) + strlen (val) + 1;
	str = (char *) malloc (sizeof (char) * len);
	if (str == NULL) {
		free (val);
		free (key);
		return NULL;
	}

	sprintf (str, "%s%s", key, val);
	free (key);
	free (val);
	key = val = NULL;
	len = strlen (str) + 1;
	for (i = 1; dict[i].val; i++) {
		key = bencode_str (dict[i].key);
		if (key == NULL) {
			free (str);
			return NULL;
		}

		val = benc_encode (dict[i].val);
		if (val == NULL) {
			free (key);
			free (str);
			return NULL;
		}

		len += strlen (key) + strlen (val);
		str = (char *) realloc (str, sizeof (char) * len);
		if (str == NULL) {
			free (val);
			free (key);
			return NULL;
		}

		strcat (str, key);
		strcat (str, val);
		free (key);
		free (val);
		key = val = NULL;
	}

	val = str;
	len = strlen (str) + 3;
	str = (char *) malloc (sizeof (char) * len);
	if (str == NULL) {
		free (val);
		return NULL;
	}

	sprintf (str, "d%se", val);
	free (val);

	return str;
}

static inline char *
bencode_int (int64_t i)
{
	char *str = NULL, *tmp = NULL;

	tmp = lltostr (i, BASE_DEC);
	if (tmp == NULL)
		return NULL;

	str = (char *) malloc (sizeof (char) * (strlen (tmp) + 3));
	if (str == NULL) {
		free (tmp);
		return NULL;
	}

	sprintf (str, "i%se", tmp);

	return str;
}

static inline char *
bencode_lst (benc_data_t **list)
{
	uint32_t i = 0;
	size_t len = 0;
	char *str = NULL, *tmp = NULL;

	if (list[0] == NULL)
		return NULL;

	tmp = benc_encode (list[0]);
	if (tmp == NULL)
		return NULL;

	str = tmp;
	tmp = NULL;
	len = strlen (str) + 1;
	for (i = 1; list[i]; i++) {
		tmp = benc_encode (list[i]);
		if (tmp == NULL) {
			free (str);
			return NULL;
		}

		len += strlen (tmp);
		str = (char *) realloc (str, sizeof (char) * len);
		if (str == NULL)
			break;

		strcat (str, tmp);
		free (tmp);
		tmp = NULL;
	}

	tmp = str;
	len = strlen (str) + 3;
	str = (char *) malloc (sizeof (char) * len);
	if (str == NULL) {
		free (tmp);
		return NULL;
	}

	sprintf (str, "l%se", tmp);
	free (tmp);

	return str;
}

static inline char *
bencode_str (char *s)
{
	char *len = NULL, *str = NULL;

	len = lltostr ((int64_t) strlen (s), BASE_DEC);
	if (len == NULL)
		return NULL;

	str = (char *) malloc (sizeof (char) * (strlen (len) + strlen (s) + 2));
	if (str == NULL) {
		free (len);
		return NULL;
	}

	sprintf (str, "%s:%s", len, s);

	return str;
}

static inline char *
lltostr (int64_t num, int base)
{
	char buf[21];
	char *c1, *c2;
	int64_t digit;
	uint8_t i = 0;
	uint8_t sign = 0;

	if ((base == BASE_DEC) && (num < 0)) {
		num = -num;
		sign = 1;
	}

	do {
		digit = num % base;
		if (digit < 10)
			buf[i++] = digit + '0';
		else
			buf[i++] = digit + 'A' - 10;

		num = num / base;
	} while (num > 0);

	if (sign == 1)
		buf[i++] = '-';

	buf[i] = '\0';
	if (*buf == '\0')
		return NULL;

	// Reverse the string.
	c1 = buf;
	c2 = buf + strlen (buf) - 1;
	while (c2 > c1) {
		*c1 ^= *c2;
		*c2 ^= *c1;
		*c1 ^= *c2;
		++c1;
		--c2;
	}

	return (strdup (buf));
}

static inline benc_data_t *
new_benc_data (enum BENC_TYPE type)
{
	benc_data_t *bd;

	bd = (benc_data_t *) malloc (sizeof (benc_data_t));
	if (bd == NULL)
		return NULL;

	memset (bd, 0, sizeof (benc_data_t));
	bd->type = type;

	return bd;
}
#endif /* __BENCODE_H__ */
