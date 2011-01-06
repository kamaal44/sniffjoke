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

#ifndef SJ_SNIFFJOKE_H
#define SJ_SNIFFJOKE_H

#include "Utils.h"
#include "UserConf.h"
#include "Process.h"
#include "NetIO.h"

#include <csignal>
#include <cstdio>
#include <memory>

#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

using namespace std;

class SniffJoke {
public:
	bool alive;
	SniffJoke(struct sj_cmdline_opts &);
	~SniffJoke();
	void run();

private:
	sj_cmdline_opts &opts;
	UserConf userconf;
	Process proc;
	auto_ptr<NetIO> mitm;
	auto_ptr<HackPool> hack_pool;
	auto_ptr<SessionTrackMap> sessiontrack_map;
	auto_ptr<TTLFocusMap> ttlfocus_map;
	auto_ptr<TCPTrack> conntrack;

	/* after detach:
	 *     service_pid in the root process [the pid of the user process]
	 *                 in the user process [0]
	 */
	pid_t service_pid;
	
	int admin_socket;
	int admin_socket_flags_blocking;
	int admin_socket_flags_nonblocking;

	void debug_setup(FILE *) const;
	void debug_cleanup();
	void server_root_cleanup();
	void server_user_cleanup();
	void admin_socket_setup();
	void admin_socket_handle();
	int recv_command(int sock, char *databuf, int bufsize, struct sockaddr *from, FILE *error_flow, const char *usermsg);	
	bool parse_port_weight(char *weightstr, Strength *Value);
};

#endif /* SJ_SNIFFJOKE_H */