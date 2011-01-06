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

#include "hardcoded-defines.h"

#include "Utils.h"
#include "UserConf.h"
#include "SniffJoke.h"

#include <csignal>
#include <memory>
#include <stdexcept>

#include <getopt.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

Debug debug;
time_t sj_clock;

static auto_ptr<SniffJoke> sniffjoke;

runtime_error sj_runtime_exception(const char* func, const char* file, long line, const char* msg)
{
	stringstream stream;
	stream << "[EXCEPTION] "<< file << "(" << line << ") function: " << func << "()";
	if (msg != NULL)
		stream << " : " << msg;
	return std::runtime_error(stream.str());
}

/* this function is used for read/write configuration and cache files */
FILE *sj_fopen(const char *fname, const char *location, const char *mode) 
{
	char effectivename[LARGEBUF]; /* the two input buffer are MEDIUMBUF */
	const char *nmode;

	snprintf(effectivename, LARGEBUF, "%s.%s", fname, location);

	if(!strcmp(location, DEFAULTLOCATION)) {
		debug.log(VERBOSE_LEVEL, "opening file %s as %s: No location specified", fname, effectivename);
	}

	/* 	communication intra sniffjoke: a "+" mean:
	 -
	 * if the file exists, open in read and write
	 * if not, create it.
	 * ...it's late, maybe exists an easiest way 	*/
	if(strlen(mode) == 1 && mode[0] == '+') {
		if(access(effectivename, R_OK) == R_OK)
			nmode = "r+";
		else 
			nmode = "w+";
	} else {
		nmode = const_cast <char *>(mode);
	}

	return fopen(effectivename, nmode);
}

void init_random()
{
	/* random pool initialization */
	srandom(time(NULL));
	for (uint8_t i = 0; i < ((uint8_t)random() % 10); ++i) 
		srandom(random());
}

void* memset_random(void *s, size_t n)
{
	/* 
	 * highly optimized memset_random
	 * 
	 * long int random(void).
	 * 
	 * long int is variable on different architectures;
	 * for example on linux 64 bit is 8 chars long,
	 * so do a while using single chars its an inefficient choice.
	 * 
	 */

	size_t longint = n / sizeof(long int);
	size_t finalbytes = n % sizeof(long int);
	unsigned char *cp = (unsigned char*)s;

	while (longint-- > 0) {
		*((long int*)cp) = random();
		cp += sizeof(long int);
	}
	
	while (finalbytes-- > 0) {
		*cp = (unsigned char)random();
		++cp;
	}

	return s;
}

void sigtrap(int signal)
{
	sniffjoke->alive = false;
}


static void sj_version(const char *pname)
{
	printf("%s %s\n", SW_NAME, SW_VERSION);
}

#define SNIFFJOKE_HELP_FORMAT \
	"%s [command] or %s --options:\n"\
	" --location <name>\tspecify the network environment (suggested) [default: %s]\n"\
	" --config <filename>\tconfig file [default: %s%s]\n"\
	" --enabler <filename>\tplugins enabler file, modified by the location\n"\
	"\t\t\t[default: %s.$LOCATION_NAME]\n"\
	" --user <username>\tdowngrade priviledge to the specified user [default: %s]\n"\
	" --group <groupname>\tdowngrade priviledge to the specified group [default: %s]\n"\
	" --chroot-dir <dir>\truns chroted into the specified dir [default: %s]\n"\
	" --logfile <file>\tset a logfile, [default: %s%s]\n"\
	" --debug <level 1-6>\tset up verbosoty level [default: %d]\n"\
	"\t\t\t1: suppress log, 2: common, 3: verbose, 4: debug, 5: session 6: packets\n"\
	" --foreground\t\trunning in foreground [default:background]\n"\
	" --admin <ip>[:port]\tspecify administration IP address [default: %s:%d]\n"\
	" --force\t\tforce restart (usable when another sniffjoke service is running)\n"\
	" --version\t\tshow sniffjoke version\n"\
	" --help\t\t\tshow this help\n\n"

static void sj_help(const char *pname, const char optchroot[MEDIUMBUF], const char *defaultbd)
{
	const char *basedir = optchroot[0] ? &optchroot[0] : defaultbd;

	printf(SNIFFJOKE_HELP_FORMAT, 
		pname, pname,
		DEFAULTLOCATION,
		basedir, CONF_FILE,
		PLUGINSENABLER,
		DROP_USER, DROP_GROUP, 
		basedir,
		basedir, LOGFILE,
		DEFAULT_DEBUG_LEVEL, 
		DEFAULT_ADMIN_ADDRESS, DEFAULT_ADMIN_PORT
	);
}

int main(int argc, char **argv)
{
	
	/* 
	 * set the default values in the configuration struct
	 * we have only constant length char[] and booleans
	 */
	struct sj_cmdline_opts useropt;
	memset(&useropt, 0x00, sizeof(useropt));
	
	struct option sj_option[] =
	{
		{ "config", required_argument, NULL, 'f' },
		{ "enabler", required_argument, NULL, 'e' },
		{ "user", required_argument, NULL, 'u' },
		{ "group", required_argument, NULL, 'g' },
		{ "chroot-dir", required_argument, NULL, 'c' },
		{ "debug", required_argument, NULL, 'd' },
		{ "logfile", required_argument, NULL, 'l' },
		{ "location", required_argument, NULL, 'o' },
		{ "admin", required_argument, NULL, 'a' },		
		{ "foreground", no_argument, NULL, 'x' },
		{ "force", no_argument, NULL, 'r' },
		{ "version", no_argument, NULL, 'v' },
		{ "only-plugin", required_argument, NULL, 'p' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 }
	};

	int charopt;
	while ((charopt = getopt_long(argc, argv, "f:e:u:g:c:d:l:o:a:xrp:vs:h", sj_option, NULL)) != -1) {
		switch(charopt) {
			case 'o':
				snprintf(useropt.location, sizeof(useropt.location), "%s", optarg);
				break;
			case 'f':
				snprintf(useropt.cfgfname, sizeof(useropt.cfgfname), "%s", optarg);
				break;
			case 'e':
				snprintf(useropt.enabler, sizeof(useropt.enabler), "%s", optarg);
				break;
			case 'u':
				snprintf(useropt.user, sizeof(useropt.user), "%s", optarg);
				break;
			case 'g':
				snprintf(useropt.group, sizeof(useropt.group), "%s", optarg);
				break;
			case 'c':
				snprintf(useropt.chroot_dir, sizeof(useropt.chroot_dir) -1, "%s", optarg);
				/* this fix it's useful if the useropt path lacks the ending slash */
				if (useropt.chroot_dir[strlen(useropt.chroot_dir) -1] != '/')
					useropt.chroot_dir[strlen(useropt.chroot_dir)] = '/';
				break;
			case 'd':
				useropt.debug_level = atoi(optarg);
				if (useropt.debug_level < 1 || useropt.debug_level > 6)
					goto sniffjoke_help;
				break;
			case 'l':
				snprintf(useropt.logfname, sizeof(useropt.logfname), "%s", optarg);
				snprintf(useropt.logfname_packets, sizeof(useropt.logfname_packets), "%s%s", optarg, SUFFIX_LF_PACKETS);
				snprintf(useropt.logfname_sessions, sizeof(useropt.logfname_sessions), "%s%s", optarg, SUFFIX_LF_SESSIONS);
				break;
			case 'a':
				snprintf(useropt.admin_address, sizeof(useropt.admin_address), "%s", optarg);
				char* port;
				if ((port = strchr(useropt.admin_address, ':')) != NULL) {
					*port = 0x00;
					int checked_port = atoi(++port);

					if (checked_port > PORTNUMBER || checked_port < 0)
						goto sniffjoke_help;

					useropt.admin_port = (uint16_t)checked_port;
				}
				break;
			case 'x':
				useropt.go_foreground = true;
				break;
			case 'r':
				useropt.force_restart = true;
				break;
			case 'p':
				snprintf(useropt.onlyplugin, sizeof(useropt.onlyplugin), "%s", optarg);
				break;
			case 'v':
				sj_version(argv[0]);
				return 0;
sniffjoke_help:
			case 'h':
			default:
				sj_help(argv[0], useropt.chroot_dir, CHROOT_DIR);
				return -1;

			argc -= optind;
			argv += optind;
		}
	}
	
	/* someone has made a "sniffjoke typo" */
	if (argc > 1 && argv[1][0] != '-') {
		sj_help(argv[0], useropt.chroot_dir, CHROOT_DIR);
		return -1;
	}

	init_random();

	try {
		sniffjoke = auto_ptr<SniffJoke> (new SniffJoke(useropt));
		sniffjoke->run();
	
	} catch (runtime_error &exception) {
		debug.log(ALL_LEVEL, "Runtime exception, going shutdown: %s", exception.what());
		
		sniffjoke.reset();
		return 0;
	}
}