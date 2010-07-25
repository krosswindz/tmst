/*
 * Copyright (c) 2010 Kross Windz <krosswindz@gmail.com>
 * All rights reserved.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>
#include "config.h"
#include "logger.h"
#include "tcp.h"

#define BACKLOG 64

int serv_sock = -1;

void
tcp_fin (void)
{
	if (serv_sock != -1)
		close (serv_sock);

	return;
}

int
tcp_init (void)
{
	struct sockaddr_in sock_addr;
	int val = 1;

	if ((serv_sock = socket (PF_INET, SOCK_STREAM, 0)) == -1) {
		logger (LOG_ERR, "ERROR: unable to create TCP socket.\n");
		return -1;
	}

	if (setsockopt (serv_sock, SOL_SOCKET, SO_REUSEADDR, &val,
				sizeof (int)) == -1) {
		logger (LOG_ERR, "ERROR: unable to set socket option to reuse"
				"address for bind.\n");
		return -1;
	}

	memset (&sock_addr, 0, sizeof (struct sockaddr_in));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = inet_addr (ip);
	sock_addr.sin_port = htons ((uint16_t) atoi ((const char *) port));
	if (bind (serv_sock, (struct sockaddr *) &sock_addr,
				sizeof (struct sockaddr_in)) == -1) {
		logger (LOG_ERR, "ERROR: unable to bind to %s:%s\n", ip, port);
		close (serv_sock);
		serv_sock = 1;
		return -1;
	}

	if (listen (serv_sock, BACKLOG) == -1) {
		logger (LOG_ERR, "ERROR: unable to listen on %s:%s\n", ip,
				port);
		close (serv_sock);
		serv_sock = 1;
		return -1;
	}

	return 0;
}
