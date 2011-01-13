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

#ifndef SJ_SESSIONTRACK_H
#define SJ_SESSIONTRACK_H

#include "Utils.h"
#include "Packet.h"

#include <map>
#include <memory>

using namespace std;

class SessionTrack {
public:
	time_t access_timestamp; /* access timestamp used to decretee expiry */

	uint32_t daddr;
	uint16_t sport;
	uint16_t dport;
	uint32_t packet_number;

	SessionTrack(const Packet &);
	~SessionTrack();
	
	/* utilities */
	void selflog(const char *, const char *) const;
	/* no personal buffer used in selflog, maybe in the future */
};

class SessionTrackKey {
public:
	uint32_t daddr;
	uint16_t sport;
	uint16_t dport;
	bool operator<(SessionTrackKey) const;
};

class SessionTrackMap : public map<const SessionTrackKey, SessionTrack*> {
public:
	SessionTrackMap();
        ~SessionTrackMap();
	SessionTrack& getSessionTrack(const Packet &);
        void manage();        
};

#endif /* SJ_SESSIONTRACK_H */