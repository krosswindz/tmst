/*
 * Copyright (c) 2010 Kross Windz <krosswindz@gmail.com>
 * All rights reserved.
 */

#include <arpa/inet.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <microhttpd.h>
#include "config.h"
#include "http.h"
#include "logger.h"
#include "tracker.h"

static int process_request (void *, struct MHD_Connection *, const char *,
		const char *, const char *, const char *, size_t *, void **);
static inline announce_info_t *get_announce_info (struct MHD_Connection *);
static inline uint8_t parse_event (const char *);
#ifdef DEBUG
static int key_val_iterator (void *, enum MHD_ValueKind, const char *,
		const char *);
static inline void print_announce_info (announce_info_t *);
static inline void print_hex (char *str);
#endif /* DEBUG */

static struct MHD_Daemon *daemon = NULL;

void
http_fin (void)
{
	if (daemon != NULL)
		MHD_stop_daemon (daemon);

	return;
}

int
http_init (void)
{
	struct sockaddr_in sock_addr;
	uint16_t tcp_port;

	tcp_port = (uint16_t) atoi ((const char *) port);
	memset (&sock_addr, 0, sizeof (struct sockaddr_in));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = inet_addr (ip);
	sock_addr.sin_port = htons (tcp_port);
	daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, tcp_port, NULL,
			NULL, &process_request, NULL,
			MHD_OPTION_SOCK_ADDR, (struct sockaddr *) &sock_addr,
			MHD_OPTION_THREAD_POOL_SIZE, max_thrds,
			MHD_OPTION_END);
	if (daemon == NULL) {
		debug (LOG_ERR, "ERROR: unable to start http daemon.\n");
		return -1;
	}

	return 0;
}

static int
process_request (void *cls, struct MHD_Connection *conn, const char *url,
		const char *method, const char *version, const char *data,
		size_t *data_size, void **con_cls)
{
	announce_info_t *ai = NULL;
	const char *buf;
	char *info_hash = NULL;
	char *pkey, *req, *ret, *str, *tmp;
	struct MHD_Response *response;
	int ret_val = MHD_YES;

#ifdef DEBUG
	debug (LOG_DBG, "url: %s\n", url);
	MHD_get_connection_values (conn, MHD_GET_ARGUMENT_KIND, 
			key_val_iterator, NULL);
#endif /* DEBUG */
	str = strdup (url);
	if (str == NULL) {
		debug (LOG_ERR, "ERROR: out-of-memory.\n");
		return MHD_NO;
	}

	tmp = str;
	pkey = strtok (tmp, "/");
	req = strtok (NULL, "");
	ret = NULL;
	if (req == NULL) {
		ret = tracker_handle_request (pkey, req, ai, info_hash);
	} else if (strcmp (req, "announce") == 0) {
		ai = get_announce_info (conn);
		ret = tracker_handle_request (pkey, req, ai, info_hash);
	} else if (strcmp (req, "scrape") == 0) {
		buf = MHD_lookup_connection_value (conn, MHD_GET_ARGUMENT_KIND,
				"info_hash");
		if (buf != NULL)
			info_hash = strdup (buf);

		ret = tracker_handle_request (pkey, req, ai, info_hash);
	} else {
		ret = tracker_handle_request (pkey, req, ai, info_hash);
		debug (LOG_DBG, "pkey: %s, req: %s\n", pkey, req);
	}

	free (str);
	response = MHD_create_response_from_data (strlen (ret), (void *) ret,
			MHD_YES, MHD_NO);
	if (response == NULL) {
		free (ret);
		debug (LOG_ERR, "ERROR: Unable to create MHD response.\n");
		return MHD_NO;
	}

	if (MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_TYPE,
				"text/plain") == MHD_NO) {
		debug (LOG_ERR, "ERROR: Unable to set Conntent-Type "
				"text/plain.");
		MHD_destroy_response (response);
		free (ret);
		return MHD_NO;
	}

	if (MHD_add_response_header (response, MHD_HTTP_HEADER_PRAGMA,
				"no-cache") == MHD_NO) {
		debug (LOG_ERR, "ERROR: Unable to set Pragma no-cache.");
		MHD_destroy_response (response);
		free (ret);
		return MHD_NO;
	}

	if (MHD_add_response_header (response, "Keep-Alive", "timeout=15, "
				"max=100") == MHD_NO) {
		debug (LOG_ERR, "ERROR: Unable to set Keep-Alive timeout=15 "
				"max=100.");
		MHD_destroy_response (response);
		free (ret);
		return MHD_NO;
	}

	if (MHD_add_response_header (response, MHD_HTTP_HEADER_CONNECTION,
				"Keep-Alive") == MHD_NO) {
		debug (LOG_ERR, "ERROR: Unable to set Connection Keep-Alive.");
		MHD_destroy_response (response);
		free (ret);
		return MHD_NO;
	}

#ifdef DEBUG
	debug (LOG_DBG, "ret: %s\n", ret);
	MHD_get_response_headers (response, key_val_iterator, NULL);
#endif /* DEBUG */
	ret_val = MHD_queue_response (conn, MHD_HTTP_OK, response);
	MHD_destroy_response (response);

	return ret_val;
}

static inline announce_info_t *
get_announce_info (struct MHD_Connection *conn)
{
	announce_info_t *ai;
	const union MHD_ConnectionInfo *ci;
	const char *tmp;

	ai = (announce_info_t *) malloc (sizeof (announce_info_t));
	if (ai == NULL) {
		debug (LOG_ERR, "ERROR: out-of-memory.\n");
		return NULL;
	}

	memset (ai, 0, sizeof (announce_info_t));
	tmp = MHD_lookup_connection_value (conn, MHD_GET_ARGUMENT_KIND,
			"info_hash");
	if (tmp != NULL)
		ai->info_hash = strdup (tmp);

	tmp = MHD_lookup_connection_value (conn, MHD_GET_ARGUMENT_KIND,
			"peer_id");
	if (tmp != NULL)
		ai->peer_id = strdup (tmp);

	tmp = MHD_lookup_connection_value (conn, MHD_GET_ARGUMENT_KIND,
			"ip");
	if (tmp != NULL)
		ai->ip = strdup (tmp);

	if (ai->ip == NULL) {
		ci = MHD_get_connection_info (conn,
				MHD_CONNECTION_INFO_CLIENT_ADDRESS);
		ai->ip = strdup (inet_ntoa (ci->client_addr->sin_addr));
	}

	tmp = MHD_lookup_connection_value (conn, MHD_GET_ARGUMENT_KIND,
			"port");
	if (tmp != NULL)
		ai->port = (uint16_t) atoi (tmp);

	tmp = MHD_lookup_connection_value (conn, MHD_GET_ARGUMENT_KIND,
			"uploaded");
	if (tmp != NULL)
		ai->uploaded = (int64_t) atoll (tmp);

	tmp = MHD_lookup_connection_value (conn, MHD_GET_ARGUMENT_KIND,
			"downloaded");
	if (tmp != NULL)
		ai->downloaded = (int64_t) atoll (tmp);

	tmp = MHD_lookup_connection_value (conn, MHD_GET_ARGUMENT_KIND,
			"uploaded");
	if (tmp != NULL)
		ai->left = (int64_t) atoll (tmp);

	tmp = MHD_lookup_connection_value (conn, MHD_GET_ARGUMENT_KIND,
			"corrupt");
	if (tmp != NULL)
		ai->corrupt = (int64_t) atoll (tmp);

	tmp = MHD_lookup_connection_value (conn, MHD_GET_ARGUMENT_KIND,
			"compact");
	if (tmp != NULL)
		ai->compact = (uint8_t) atoi (tmp);

	tmp = MHD_lookup_connection_value (conn, MHD_GET_ARGUMENT_KIND,
			"no_peer_id");
	if (tmp != NULL)
		ai->no_peer_id = (uint8_t) atoi (tmp);

	tmp = MHD_lookup_connection_value (conn, MHD_GET_ARGUMENT_KIND,
			"event");
	if (tmp != NULL)
		ai->event = parse_event (tmp);

	tmp = MHD_lookup_connection_value (conn, MHD_GET_ARGUMENT_KIND,
			"numwant");
	if (tmp != NULL)
		ai->numwant = (int32_t) atoi (tmp);

	tmp = MHD_lookup_connection_value (conn, MHD_GET_ARGUMENT_KIND,
			"key");
	if (tmp != NULL)
		ai->key = strdup (tmp);

	tmp = MHD_lookup_connection_value (conn, MHD_GET_ARGUMENT_KIND,
			"tracker_id");
	if (tmp != NULL)
		ai->tracker_id = strdup (tmp);

	return ai;
}

	static inline uint8_t
parse_event (const char *event)
{
	if (strcmp (event, "completed") == 0)
		return 1;

	if (strcmp (event, "started") == 0)
		return 2;

	if (strcmp (event, "stopped") == 0)
		return 3;

	return 0;
}

#ifdef DEBUG
static int
key_val_iterator (void *cls, enum MHD_ValueKind kind, const char *key,
		const char *value)
{
	debug (LOG_DBG, "%s = %s\n", key, value);
	return MHD_YES;
}

static inline void
print_announce_info (announce_info_t *ai)
{
	debug (LOG_DBG, "announce info:\n");
	debug (LOG_DBG, "info_hash:\n");
	print_hex (ai->info_hash);
	debug (LOG_DBG, "peer_id: %c%c%c%c%c%c%c%c\n", ai->peer_id[0],
			ai->peer_id[1], ai->peer_id[2], ai->peer_id[3],
			ai->peer_id[4], ai->peer_id[5], ai->peer_id[6],
			ai->peer_id[7]);
	print_hex (ai->peer_id + 8);
	debug (LOG_DBG, "ip: %s\n", ai->ip);
	debug (LOG_DBG, "port: %" PRIu16" \n", ai->port);
	debug (LOG_DBG, "uploaded: %" PRId64 "\n", ai->uploaded);
	debug (LOG_DBG, "downloaded: %" PRId64 "\n", ai->downloaded);
	debug (LOG_DBG, "left: %" PRId64 "\n", ai->left);
	debug (LOG_DBG, "corrupt: %" PRId64 "\n", ai->corrupt);
	debug (LOG_DBG, "compat: %" PRIu8 "\n", ai->compact);
	debug (LOG_DBG, "no_peer_id: %" PRIu8 "\n", ai->no_peer_id);
	debug (LOG_DBG, "event: %" PRIu8 "\n", ai->event);
	debug (LOG_DBG, "numwant: %" PRId32 "\n", ai->numwant);
	debug (LOG_DBG, "key: %s\n", ai->key);
	debug (LOG_DBG, "tracker_id: %s\n", ai->tracker_id);

	return;
}

static void
print_hex (char *str)
{
	char *out, *tmp;
	size_t i;

	if (strlen (str) != 20)
		debug (LOG_DBG, "strlen: %zu\n", strlen (str));

	out = (char *) malloc (sizeof (char) * (2 * strlen (str) + 1));
	memset (out, 0, sizeof (char) * (2 * strlen (str) + 1));
	tmp = out;
	for (i = 0; i < strlen (str); i++) {
		sprintf (tmp, "%0hhx", str[i]);
		tmp += 2;
	}

	debug (LOG_DBG, "%s\n", out);
	free (out);

	return;
}
#endif /* DEBUG */
