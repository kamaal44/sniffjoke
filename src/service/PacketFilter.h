/*
 *   SniffJoke is a software able to confuse the Internet traffic analysis,
 *   developed with the aim to improve digital privacy in communications and
 *   to show and test some securiy weakness in traffic analysis software.
 *
 *   Copyright (C) 2011 vecna <vecna@delirandom.net>
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

#include "Utils.h"
#include "Packet.h"

#include <set>

#include <stdint.h>

using namespace std;

class FilterEntry
{
public:
    uint16_t ip_id;
    uint16_t ip_totallen;
    uint32_t ip_saddr;
    uint32_t ip_daddr;

    FilterEntry(uint16_t, uint16_t, uint32_t, uint32_t);
    FilterEntry(const Packet &);
    bool operator<(FilterEntry) const;
};

class FilterMultiset
{
private:
    uint32_t timeout_len;
    uint32_t manage_timeout;
    multiset<FilterEntry> fm[2];
    multiset<FilterEntry> &first;
    multiset<FilterEntry> &second;

public:
    FilterMultiset(void);
    ~FilterMultiset(void);
    bool checkEntry(const FilterEntry &);
    void addEntry(const FilterEntry &);
    void manage();
};

class PacketFilter
{
private:
    FilterMultiset filter_multiset;

    bool filterICMPErrors(const Packet &pkt);

public:
    void addFilter(const Packet& pkt);
    bool matchFilter(const Packet& pkt);
};