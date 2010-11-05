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
#include "SessionTrack.h"

SessionTrack::SessionTrack(const Packet &pkt) :
	daddr(pkt.ip->daddr),
	sport(pkt.tcp->source),
	dport(pkt.tcp->dest),
	isn(pkt.tcp->seq),
	packet_number(1),
	shutdown(false)
{}

bool SessionTrackKey::operator<(SessionTrackKey comp) const
{
	if (daddr < comp.daddr) {
		return true;
	} else if (daddr > comp.daddr) {
		return false;
	} else {
		if (sport < comp.sport) {
			return true;
		} else if (sport > comp.sport) {
			return false;
		} else {
			if (dport < comp.dport)
				return true;
			else
				return false;
		}
	}
}

void SessionTrack::selflog(const char *func, const char *lmsg) 
{
	internal_log(NULL, SESSION_DEBUG, "%s sport %d saddr %s dport %u, ISN %x shutdown %s #pkt %d: [%s]",
		func, ntohs(sport), 
		inet_ntoa(*((struct in_addr *)&daddr)),
                ntohl(isn),
		ntohs(dport), 
		shutdown ? "TRUE" : "FALSE",
		packet_number, lmsg
	);
}
