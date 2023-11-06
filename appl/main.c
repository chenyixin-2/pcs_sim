#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <unistd.h>
#include "appl.h"
#include "tool.h"

static void sim_init(void)
{
	mdl.uab = 400.1;
	mdl.ubc = 400.2;
	mdl.uca = 400.3;
}

int DAEMON = 0;
int RUNTYPE = -1;
int VERSION[3] = {2, 0, 5}; /* ONLY VERSION[0] is used */
static const char *s_optstr = "a:b:cdep:s:t:";
int main(int argc, char *argv[])
{
	int ret = 0;
	int i = 0;
	int ch;
	opterr = 0;

	openlog(argv[0], LOG_CONS | LOG_PID, 0);

	syslog(LOG_INFO, "%s,++", __func__);

	for (i = 0; i < argc; i++)
	{
		syslog(LOG_INFO, "%s,arg list,argv[%d]:%s", __func__, i, argv[i]);
	}

	strcpy(mdl.szdev, "Enjoy-100kW");

	while ((ch = getopt(argc, argv, s_optstr)) != -1)
	{
		switch (ch)
		{
		case 'a':
			mdl.adr = atoi(optarg);
			syslog(LOG_INFO, "option a: %s \n", optarg);
			break;
		case 'b':
			strcpy(mdl.szser, optarg);
			syslog(LOG_INFO, "option b: %s\n", optarg);
			break;
		case 'd':
			DAEMON = 1;
			syslog(LOG_INFO, "option d\n");
			break;
		case 'p':
			mdl.mqtt_servport = atoi(optarg);
			syslog(LOG_INFO, "option p : %s\n", optarg);
			break;
		case 's':
			strcpy(mdl.szmqtt_servip, optarg);
			syslog(LOG_INFO, "option s : %s\n", optarg);
			break;
		case 't':
			if (strcmp("appl", optarg) == 0)
			{
				RUNTYPE = 0;
			}
			else if (strcmp("tool", optarg) == 0)
			{
				RUNTYPE = 2;
			}
			else
			{
				RUNTYPE = -1;
			}
			break;
		default:
			syslog(LOG_INFO, "other option :%c\n", ch);
		}
	}

	sim_init();

	if (RUNTYPE == 0)
	{
		appl_main();
	}
	else if (RUNTYPE == 2)
	{
		tool_main();
	}
	else
	{
		syslog(LOG_INFO, "%s, unknown runtype:%d", __func__, RUNTYPE);
	}

	syslog(LOG_INFO, "%s, --", __func__);
	closelog();

	return 0;
}