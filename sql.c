/*
 * Copyright (c) 2010 Kross Windz <krosswindz@gmail.com>
 * All rights reserved.
 */

#include <mysql.h>
#include "config.h"
#include "logger.h"
#include "sql.h"

static MYSQL *sql_conn = NULL;

void
sql_fin (void)
{
	if (sql_conn != NULL)
		mysql_close (sql_conn);

	sql_conn = NULL;

	return;
}

int
sql_init (void)
{
	sql_conn = mysql_init (NULL);
	if (sql_conn == NULL) {
		logger (LOG_ERR, "ERROR: out-of-memory.\n");
		return -1;
	}

	if (mysql_real_connect (sql_conn, host, user, passwd, name, 0, NULL, 0)
			== NULL) {
		logger (LOG_ERR, "ERROR: faild to connect to database, error: "
				"%s\n", mysql_error (sql_conn));
		return -1;
	}

	return 0;
}
