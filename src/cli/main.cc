/*
 *   SniffJoke is a software able to confuse the Internet traffic analysis,
 *   developed with the aim to improve digital privacy in communications and
 *   to show and test some securiy weakness in traffic analysis software.
 *   
 *   Copyright (C) 2010 vecna <vecna@delirandom.net>
 *                      evilaliv3 <giovanni.pellerano@evilaliv3.org>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SniffJokeCli.h"

#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

static void sjcli_version(const char *pname)
{
	printf("%s %s\n", "SJCLI", "0.1");
}

#define SNIFFJOKECLI_HELP_FORMAT \
	"sniffjoke_cli --options [command]:\n"\
	" --address <ip>[:port]\tspecify administration IP address [default: 127.0.0.1:8844]\n"\
	" --version\t\tshow sniffjoke version\n"\
	" --help\t\t\tshow this help\n\n"\
	"when sniffjoke is running, you should send commands with a command line argument:\n"\
	" start\t\t\tstart sniffjoke hijacking/injection\n"\
	" stop\t\t\tstop sniffjoke (but remain tunnel interface active)\n"\
	" quit\t\t\tstop sniffjoke, save config, abort the service\n"\
	" saveconf\t\tdump config file\n"\
	" stat\t\t\tget statistics about sniffjoke configuration and network\n"\
	" info\t\t\tget massive info about sniffjoke internet stats\n"\
	" showport\t\tshow TCP ports strongness of injection\n"\
	" set start end value\tset per tcp ports the strongness of injection\n"\
	" \t\t\tthe values are: <heavy|normal|light|none>\n"\
	" \t\t\texample: sniffjoke set 22 80 heavy\n"\
	" clear\t\t\talias to \"set 1 65535 none\"\n"\
	" debug\t\t\t[1-6] change the log debug level\n\n"\
	"\t\t\thttp://www.delirandom.net/sniffjoke\n"

static void sjcli_help()
{
	printf(SNIFFJOKECLI_HELP_FORMAT);
}

static bool parse_command(char **av, uint32_t ac, struct command *sjcmdlist, char *retcmd)
{
	for(uint32_t i = 0; i < ac; ++i) 
	{
		struct command *ptr;
		for(ptr = &sjcmdlist[0]; ptr->cmd != NULL; ++ptr) 
		{
			if (!strcmp(ptr->cmd, av[i])) 
			{
				size_t usedlen = 0;
				snprintf(retcmd, 256, "%s", ptr->cmd);
				if (ptr->related_args + i > ac) {
					sjcli_help();
					exit(-1);
				}
				while (--(ptr->related_args)) {
					usedlen = strlen(retcmd);
					snprintf(&retcmd[usedlen], 256 - usedlen, " %s", av[++i]);
				}
				return true;
			}
		}
	}
	return false;
}


int main(int argc, char **argv)
{
	struct sjcli_cmdline_opts {
		char admin_address[256];
		uint16_t admin_port;
		char cmd_buffer[256];
	} useropt;
	
	memset(&useropt, 0x00, sizeof(useropt));
	snprintf(useropt.admin_address, sizeof(useropt.admin_address), "127.0.0.1");
	useropt.admin_port = 8844;

	struct command sjcli_command[] =
	{
		{ "start",    1 },
		{ "stop",     1 },
		{ "stat",     1 },
		{ "clear",    1 },
		{ "showport", 1 },
		{ "quit",     1 },
		{ "info",     1 },
		{ "saveconf", 1 },
		{ "debug",    2 }, 	/* the log debuglevel */
		{ "set",      4 }, 	/* set start_port end_port value */
		{ NULL,       0	}
	};

	if(!parse_command(argv, argc, sjcli_command, useropt.cmd_buffer)) {
		sjcli_help();
		return -1;
	}

	struct option sjcli_option[] =
	{
		{ "address", required_argument, NULL, 'a' },
		{ "version", no_argument, NULL, 'v' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 }
	};

	int charopt;
	while ((charopt = getopt_long(argc, argv, "c:a:vh", sjcli_option, NULL)) != -1) {
		switch(charopt) {
			case 'c':
				snprintf(useropt.cmd_buffer, sizeof(useropt.cmd_buffer), "%s", optarg);
				break;
			case 'a':
				snprintf(useropt.admin_address, sizeof(useropt.admin_address), "%s", optarg);
				char* port;
				if ((port = strchr(useropt.admin_address, ':')) != NULL) {
					*port = 0x00;
					int checked_port = atoi(++port);

					if (checked_port > 65535 || checked_port < 0)
						goto sniffjokecli_help;

					useropt.admin_port = (uint16_t)checked_port;
				}
				break;
			case 'v':
				sjcli_version(argv[0]);
				return 0;
sniffjokecli_help:
			case 'h':
			default:
				sjcli_help();
				return -1;

			argc -= optind;
			argv += optind;
		}
	}

	SniffJokeCli cli(useropt.admin_address, useropt.admin_port);
	cli.send_command(useropt.cmd_buffer);
}