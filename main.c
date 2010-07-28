/*
 * Copyright (c) 2010 Kross Windz <krosswindz@gmail.com>
 * All rights reserved.
 */

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "http.h"
#include "logger.h"
#include "sql.h"

#define CONF_LINE_LEN 512

static int parse_config_file (char *);
static void signal_handler (int);
static void usage (char *);
static inline int check_valid_config (void);
static inline int set_defaults (void);
static inline char *strip_comments (char *);
static inline char *strip_whitespace (char *);
#ifdef DEBUG
static void print_config (void);
#endif /* DEBUG */

FILE *log_fp = NULL;
uint8_t log_level = 1;
unsigned int max_thrds = 32;
char *host = NULL;
char *name = NULL;
char *passwd = NULL;
char *user = NULL;
char *ip = NULL;
char *port = NULL;
char *levels = NULL;

static volatile int terminate = 0;

int
main (int argc, char *argv[])
{
	int c = 0;
	char *conffile = NULL;
	int fg = 0;
	char *logfile = NULL;
	struct option opts[] = {
		{.name = "conffile", .has_arg = 1, .flag = NULL, .val = 'c'},
		{.name = "foreground", .has_arg = 0, .flag = NULL, .val = 'f'},
		{.name = "help", .has_arg = 0, .flag = NULL, .val = 'h'},
		{.name = "logfile", .has_arg = 1, .flag = NULL, .val = 'l'},
		{.name = NULL, .has_arg = 0, .flag = NULL, .val = 0}
	};
	pid_t pid;
	int retval = EXIT_SUCCESS;

	while (c != EOF) {
		c = getopt_long (argc, argv, "c:fhl:", opts, NULL);
		switch (c) {
			case EOF:
				break;

			case 'c':
				conffile = strdup (optarg);
				break;

			case 'f':
				fg = 1;
				break;

			case 'l':
				logfile = strdup (optarg);
				break;

			case '?':
			case 'h':
			default:
				usage (argv[0]);
		}
	}

	if (fg == 0) {
		pid = fork ();
		if (pid < 0) {
			printf ("Unable to background as daemon process.\n");
			return EXIT_FAILURE;
		}

		if (pid > 0)
			return EXIT_SUCCESS;

		umask (0);
		if (setsid () == (pid_t) -1) {
			printf ("Unable to create new session for child "
					"process.\n");
			return EXIT_FAILURE;
		}

		fclose (stdin);
		fclose (stdout);
		fclose (stderr);
		signal (SIGCHLD, SIG_IGN);
		signal (SIGPIPE, SIG_IGN);
		signal (SIGHUP, signal_handler);
		signal (SIGINT, signal_handler);
		signal (SIGQUIT, signal_handler);
		signal (SIGTERM, signal_handler);
		if (logfile == NULL) {
			logfile = (char *) malloc (sizeof (char)
					* (strlen (argv[0]) + 5));
			memset (logfile, 0, sizeof (char)
					* (strlen (argv[0]) + 5));
			sprintf (logfile, "%s.log", argv[0]);
		}
	}

	log_fp = stdout;
	if (logfile != NULL) {
		log_fp = fopen (logfile, "a");
		if (log_fp == NULL) {
			printf ("ERROR: Unable to open log file: %s\n",
					logfile);
			return EXIT_FAILURE;
		}
	}

	if (conffile == NULL) {
		conffile = (char *) malloc (sizeof (char)
				* (strlen (argv[0]) + 6));
		memset (conffile, 0, sizeof (char) * (strlen (argv[0]) + 6));
		sprintf (conffile, "%s.conf", argv[0]);
	}

	if (parse_config_file (conffile) != 0) {
		fprintf (log_fp, "ERROR: parsing config file failed.\n");
		retval = EXIT_FAILURE;
		goto cleanup;
	}

	if (set_defaults () != 0) {
		fprintf (log_fp, "ERROR: unable to set defaults.\n");
		retval = EXIT_FAILURE;
		goto cleanup;
	}

	if (check_valid_config () != 0) {
		fprintf (log_fp, "ERROR: bad config file.\n");
		retval = EXIT_FAILURE;
		goto cleanup;
	}

	if (logger_init (levels) != 0) {
		fprintf (log_fp, "ERROR: unable initialize logger.\n");
		retval = EXIT_FAILURE;
		goto cleanup;
	}

#ifdef DEBUG
	print_config ();
#endif /* DEBUG */
	if (http_init () != 0) {
		logger (LOG_ERR, "ERROR: http_init failed.\n");
		retval = EXIT_FAILURE;
		goto cleanup;
	}

	if (sql_init () != 0) {
		logger (LOG_ERR, "ERROR: sql_init failed.\n");
		retval = EXIT_FAILURE;
		goto cleanup;
	}

	while (terminate == 0);
cleanup:
	// Clean up.
	sql_fin ();
	http_fin ();

	// Sync and close the log file.
	fflush (log_fp);
	if (log_fp != stdout) {
		fsync (fileno (log_fp));
		fclose (log_fp);
	}

	if (logfile != NULL)
		free (logfile);

	if (conffile != NULL)
		free (conffile);

	if (host != NULL)
		free (host);

	if (name != NULL)
		free (name);

	if (passwd != NULL)
		free (passwd);

	if (user != NULL)
		free (user);

	if (ip != NULL)
		free (ip);

	if (port != NULL)
		free (port);

	return retval;
}

static int
parse_config_file (char *file)
{
	char *buf, *opt, *val;
	char conf_line [CONF_LINE_LEN];
	FILE *fp;

	fp = fopen (file, "r");
	if (fp == NULL) {
		fprintf (log_fp, "ERROR: Unable to open config file: %s\n",
				file);
		return -1;
	}

	while (fgets (conf_line, CONF_LINE_LEN, fp) != NULL) {
		buf = strip_comments (conf_line);
		buf = strip_whitespace (buf);
		if (*buf == '\0')
			continue;

		opt = strtok (buf, "=");
		val = strtok (NULL, "=");
		if (strcmp (opt, "max_threads") == 0) {
			max_thrds = (unsigned int) atoi ((const char *) val);
			continue;
		}

		if (strcmp (opt, "db_host") == 0) {
			host = strdup (val);
			continue;
		}

		if (strcmp (opt, "db_name") == 0) {
			name = strdup (val);
			continue;
		}

		if (strcmp (opt, "db_password") == 0) {
			passwd = strdup (val);
			continue;
		}

		if (strcmp (opt, "db_user") == 0) {
			user = strdup (val);
			continue;
		}

		if (strcmp (opt, "listen_ip") == 0) {
			ip = strdup (val);
			continue;
		}

		if (strcmp (opt, "listen_port") == 0) {
			port = strdup (val);
			continue;
		}

		if (strcmp (opt, "log_levels") == 0) {
			levels = strdup (val);
			continue;
		}

		fprintf (log_fp, "INFO: conf_line: %s = %s\n", opt, val);
		fprintf (log_fp, "WARNING: Unknown config line.\n");
	}

	return 0;
}

static void
signal_handler (int signo)
{
	switch (signo) {
		case SIGHUP:
		case SIGQUIT:
			logger (LOG_DBG, "INFO: received %s, ignoring.\n",
					strsignal (signo));
			break;

		case SIGINT:
		case SIGTERM:
			logger (LOG_DBG, "INFO: received %s, terminating.\n",
					strsignal (signo));
			terminate = 1;
			break;

		default:
			logger (LOG_ERR, "ERROR: received %s, unhandled.\n",
					strsignal (signo));
	}

	return;
}

static void
usage (char *name)
{
	printf ("usage: %s [options]\n", name);
	printf ("optins:\n\n");
	printf ("\t-c, --conffile=<config file>\n\t\tUse <config file> as the "
			"configuration file.\n");
	printf ("\t-f, --foreground\n\t\tRun in foreground, all output is sent"
			" to stdout.\n");
	printf ("\t-h, --help\n\t\tPrint this help message and then exit.\n");
	printf ("\t-l, --logfile=<log file>\n\t\tUse <log file> for logging."
			"\n");
	exit (EXIT_SUCCESS);
}

static inline int 
check_valid_config (void)
{
	if (name == NULL) {
		fprintf (log_fp, "ERROR: missing database name in config "
				"file.\n");
		return -1;
	}

	if (user == NULL) {
		fprintf (log_fp, "ERROR: missing database user name in config "
				"file.\n");
		return -1;
	}

	if (passwd == NULL) {
		fprintf (log_fp, "ERROR: missing database password in config "
				"file.\n");
		return -1;
	}

	return 0;
}

static inline int
set_defaults (void)
{
	if (ip == NULL) {
		ip = (char *) malloc (sizeof (char) * 8);
		if (ip == NULL) {
			fprintf (log_fp, "ERROR: out-of-memory.\n");
			return -1;
		}

		memset (ip, 0, sizeof (char) * 8);
		sprintf (ip, "0.0.0.0");
	}

	if (port == NULL) {
		port = (char *) malloc (sizeof (char) * 6);
		if (port == NULL) {
			fprintf (log_fp, "ERROR: out-of-memory.\n");
			return -1;
		}

		memset (port, 0, sizeof (char) * 6);
		sprintf (port, "30404");
	}

	if (levels == NULL) {
		levels = (char *) malloc (sizeof (char) * 8);
		if (levels == NULL) {
			fprintf (log_fp, "ERROR: out-of-memroy.\n");
			return -1;
		}

		memset (levels, 0, sizeof (char) * 8);
		sprintf (levels, "LOG_ERR");
	}

	if (host == NULL) {
		host = (char *) malloc (sizeof (char) * 10);
		if (host == NULL) {
			fprintf (log_fp, "ERROR: out-of-memory.\n");
			return -1;
		}

		memset (host, 0, sizeof (char) * 10);
		sprintf (host, "localhost");
	}

	return 0;
}

static inline char *
strip_comments (char *str)
{
	char *c = str;

	while (*c) {
		if (*c == '#') {
			*c = '\0';
			break;
		}

		c++;
	}

	return str;
}
static inline char *
strip_whitespace (char *str)
{
	char *c1, *c2;
	int i;

	while (isspace (*str))
		str++;

	for (i = strlen (str) - 1; i >= 0; i--)
		if (!isspace (str[i]))
			break;

	str[i + 1] = '\0';
	c1 = str;
	c2 = str;
	while ((*c2 = *c1) != '\0') {
		if (isspace (*c1))
			c2--;

		c1++;
		c2++;
	}

	return str;
}

#ifdef DEBUG
static void
print_config (void)
{
	logger (LOG_DBG, "configured parameters:\n");
	logger (LOG_DBG, "max threads: %u\n", max_thrds);
	logger (LOG_DBG, "database host: %s\n", host);
	logger (LOG_DBG, "database name: %s\n", name);
	logger (LOG_DBG, "database passwd: %s\n", passwd);
	logger (LOG_DBG, "database user: %s\n", user);
	logger (LOG_DBG, "bind ip: %s\n", ip);
	logger (LOG_DBG, "bind port: %s\n", port);
	logger (LOG_DBG, "log level: %s\n", levels);

	return;
}
#endif /* DEBUG */
