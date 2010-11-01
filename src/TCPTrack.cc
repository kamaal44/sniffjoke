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
#include "Utils.h"
#include "TCPTrack.h"

#include <dlfcn.h>

#include <algorithm>
using namespace std;

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

HackPacketPoolElem::HackPacketPoolElem(bool* const c, unsigned int index, HackPacket* HackObj) :
	config(c),
	enabled(*c),
	dummy(HackObj),
	track_index(index)
{
}

/* Check if the constructor has make a good job - further checks need to be addedd */
bool HackPacketPool::verifyPluginIntegirty(HackPacket *loaded) 
{
	if(loaded->hackName == NULL) {
		internal_log(NULL, DEBUG_LEVEL, "Hack plugin #%d lack of ->hackName member", loaded->track_index);
		return false;
	}

	if(loaded->hack_frequency == 0) {
		internal_log(NULL, DEBUG_LEVEL, "Hack plugin #%d (%s) lack of ->hack_frequency", 
			loaded->track_index, loaded->hackName);
		return false;
	}

	return true;
}

/*
 * the constructor of HackPacketPool is called once; in the TCPTrack constructor the class member
 * hack_pool is instanced. what we need here is to read the entire plugin list, open and fix the
 * list, keeping track in listOfHacks variable
 *
 *    hack_pool(sjconf->running)
 *
 * (class TCPTrack).hack_pool is the name of the unique HackPacketPool element
 */
HackPacketPool::HackPacketPool(bool *fail, struct sj_config *sjconf) 
{
	FILE *plugfile;

	memset(&listOfHacks, 0x00, sizeof(struct PluginTrack) * MAXPLUGINS);
	n_plugin = 0;

	*fail = false;

	if((plugfile = fopen(PLUGINSENABLER, "r")) == NULL) {
		internal_log(NULL, ALL_LEVEL, "unable to open in reading %s: %s", PLUGINSENABLER, strerror(errno));
		*fail = true;
		return;
	}

	int line = 0;
	do {
		char plugname[SMALLBUF];

		fgets(plugname, SMALLBUF, plugfile);
		line++;

		if(plugname[0] == '#')
			continue;

		/* C's chop() */
		plugname[strlen(plugname) -1] = 0x00; 

		/* 4 is the minimum length of a ?.so plugin */
		if(strlen(plugname) < 4 || feof(plugfile)) {
			internal_log(NULL, VERBOSE_LEVEL, "reading %s: importend %d plugins, matched interruption at line %d",
				PLUGINSENABLER, n_plugin, line);
			break;
		}

		if(n_plugin == MAXPLUGINS -1) {
			internal_log(NULL, ALL_LEVEL, 
				"reached the limits of supported plugins (%d), modify hardcoded-defines.h!", MAXPLUGINS);
			*fail = true;
			/* if you are running more than 32 plugins, you know what to do */
			break;
		}

		void *handler = dlopen(plugname, RTLD_LAZY);
		if(handler == NULL) {
			internal_log(NULL, ALL_LEVEL, "fatal error: unable to load %s: %s", plugname, dlerror());
			*fail = true;
			break;
		}
		internal_log(NULL, DEBUG_LEVEL, "opened %s plugin", plugname);

		listOfHacks[n_plugin].pluginHandler = handler;
		listOfHacks[n_plugin].pluginPath = strdup(plugname);

		constructor_f *X = (constructor_f *) dlsym(handler, "CreateHackObject");
		listOfHacks[n_plugin].fp_CreateHackObj = X; // = (constructor_f *) dlsym(handler, "CreateHackObject");

		destructor_f *Y = (destructor_f *) dlsym(handler, "DeleteHackObject");
		listOfHacks[n_plugin].fp_DeleteHackObj = Y;

		n_plugin++;
	} while(!feof(plugfile));

	if(*fail || !n_plugin) {
		internal_log(NULL, ALL_LEVEL, "loaded correctly %d plugins: FAILURE while loading detected", n_plugin);
		return;
	} else
		internal_log(NULL, ALL_LEVEL, "loaded correctly %d plugins", n_plugin);

	for(int i =0; i < n_plugin; i++)
	{
		bool obviously_true = true;

		HackPacket *loaded = listOfHacks[i].fp_CreateHackObj(i);

		if(!verifyPluginIntegirty(loaded)) {
			internal_log(NULL, ALL_LEVEL, "plugin #%d %s incorret implementation: read the documentation!", 
				loaded->track_index, basename(listOfHacks[i].pluginPath) );
			*fail = true;
			continue;
		} else {
			internal_log(NULL, DEBUG_LEVEL, "plugin #%d/%s implementation accepted",
				loaded->track_index, loaded->hackName);

			listOfHacks[i].selfObj = loaded;
		}

		push_back(HackPacketPoolElem(/* prima c'era il bool del sj_shift_ack_enable, ora, se 
						il plugin è nella lista, o non è tolto in sjconf e quindi
						non presente in listOfHacks, ogni plugin che arriva qui è 
						abilitato di certo. però l'ho lasciato, perché il bool qui ha senso
						per le abilitazioni progressive */ &obviously_true, i, loaded
			)
		);
	}

	/* 
	 * TCPTrack.cc:86: warning: ISO C++ forbids casting between pointer-to-function and pointer-to-object
	 *
	 * THE IS NO WAY TO AVOID IT!
	 * need, for TCPTrack.cc to be compiled without -Werror 
	 */
}

HackPacketPool::~HackPacketPool() 
{
	/* call the distructor loaded from the plugins */
	while(n_plugin--) 
	{
		listOfHacks[n_plugin].fp_DeleteHackObj(listOfHacks[n_plugin].selfObj);

		if(dlclose(listOfHacks[n_plugin].pluginHandler)) 
			internal_log(NULL, ALL_LEVEL, "unable to close %s plugin: %s", listOfHacks[n_plugin].pluginPath, dlerror());
		else
			internal_log(NULL, DEBUG_LEVEL, "closed handler of %s", listOfHacks[n_plugin].pluginPath);
	}
}

TCPTrack::TCPTrack(UserConf *sjconf) :
	runcopy(sjconf->running),
	youngpacketspresent(false),
	/* fail is the public member used to signal a failure in plugin's loading, 
	 * hack_pool is a "class HackPacketPool", that extend a vector of HackPacketPoolElem */
	hack_pool(&fail, sjconf->running)
{
	/* random pool initialization */
	for (int i = 0; i < ((random() % 40) + 3); i++) 
		srandom((unsigned int)time(NULL) ^ random());
	
	internal_log(NULL, DEBUG_LEVEL, "TCPTrack()");
}

TCPTrack::~TCPTrack(void) 
{
	delete &hack_pool;
	internal_log(NULL, DEBUG_LEVEL, "~TCPTrack()");
}

bool TCPTrack::check_evil_packet(const unsigned char *buff, unsigned int nbyte)
{
	struct iphdr *ip = (struct iphdr *)buff;
 
	if (nbyte < sizeof(struct iphdr) || nbyte < ntohs(ip->tot_len) ) {
		internal_log(NULL, PACKETS_DEBUG, "%s %s: nbyte %s < (struct iphdr) %d || (ip->tot_len) %d", 
			__FILE__, __func__, nbyte, sizeof(struct iphdr), ntohs(ip->tot_len)
		);
		return false;
	}

	if (ip->protocol == IPPROTO_TCP) {
		struct tcphdr *tcp;
		int iphlen;
		int tcphlen;

		iphlen = ip->ihl * 4;

		if (nbyte < iphlen + sizeof(struct tcphdr)) {
			internal_log(NULL, PACKETS_DEBUG, "%s %s: [bad TCP] nbyte %d < iphlen + (struct tcphdr) %d",
				__FILE__, __func__, nbyte, iphlen + sizeof(struct tcphdr)
			);
			return false;
		}

		tcp = (struct tcphdr *)((unsigned char *)ip + iphlen);
		tcphlen = tcp->doff * 4;
		
		if (ntohs(ip->tot_len) < iphlen + tcphlen) {
			internal_log(NULL, PACKETS_DEBUG, "%s %s: [bad TCP][bis] nbyte %d < iphlen + tcphlen %d",
				__FILE__, __func__, nbyte, iphlen + tcphlen
			);
			return false;
		}
	}
	return true;
}

bool TCPTrack::check_uncommon_tcpopt(const struct tcphdr *tcp)
{
	unsigned char check;
	/* default: there are not uncommon TCPOPT, and the packets should be stripped off */
	bool ret = false ;

	for (int i = sizeof(struct tcphdr); i < (tcp->doff * 4); i++) 
	{
		check = ((unsigned char *)tcp)[i];

		switch(check) {
			case TCPOPT_TIMESTAMP:
				i += (TCPOLEN_TIMESTAMP +1);
				break;
			case TCPOPT_EOL:
			case TCPOPT_NOP:
				break;
			case TCPOPT_MAXSEG:
			case TCPOPT_WINDOW:
			case TCPOPT_SACK_PERMITTED:
			case TCPOPT_SACK:
			default:
				ret = true; break;
		/* every unknow TCPOPT is keep, only TIMESTAMP, EOL, NOP are stripped off ATM */
		}
	}

	internal_log(NULL, PACKETS_DEBUG,
		"%s %s: sport %d -> dport %d, TCP OPT %s", __FILE__, __func__,
		ntohs(tcp->source), 
		ntohs(tcp->dest),
		ret ? "true" : "false"
	);

	return ret;
	
}

/* 
 * this two functions is required on hacks injection, because that 
 * injection should happens ALWAYS, but give the less possible elements
 * to the attacker for detects sniffjoke working style
 */
bool TCPTrack::percentage(float math_choosed, unsigned int vecna_choosed)
{
	return ((random() % 100) <= ((int)(math_choosed * vecna_choosed) / 100));
}

/*  the variable is used from the sniffjoke routing for decreete the possibility of
 *  an hack happens. this variable are mixed in probabiliy with the session->packet_number, because
 *  the hacks must happens, for the most, in the start of the session (the first 10 packets),
 *  other hacks injection should happen in the randomic mode express in logarithm function.
 */
float TCPTrack::logarithm(int packet_number)
{
	int blah;

	if (packet_number < 20)
		return 150.9;

	if (packet_number > 10000)
		blah = (packet_number / 10000) * 10000;
	else if (packet_number > 1000)
		blah = (packet_number / 1000) * 1000;
	else if (packet_number > 100)
		blah = (packet_number / 100) * 100;
	else
		return 2.2; /* x > 20 && x < 100 */

	if (blah == packet_number)
		return 90.0;
	else
		return 0.08;
}

SessionTrack* TCPTrack::init_sessiontrack(const Packet &pkt) 
{
	/* pkt is the refsyn, SYN packet reference for starting ttl bruteforce */
	SessionTrackKey key = {pkt.ip->daddr, pkt.tcp->source, pkt.tcp->dest};
	SessionTrackMap::iterator it = sex_map.find(key);
	if (it != sex_map.end())
		return &(it->second);
	else {
		if(sex_map.size() == runcopy->max_sex_track) {
			/* if we reach sextrackmax probably we have a lot of dead sessions tracked */
			/* we can make a complete clear() resetting sex_map without problems */
			sex_map.clear();
		}
		return &(sex_map.insert(pair<SessionTrackKey, SessionTrack>(key, pkt)).first->second);
	}
}

void TCPTrack::clear_session(SessionTrackMap::iterator stm_it)
{
	/* 
	 * clear_session don't remove conntrack immediatly, at the first call
	 * set the "shutdown" bool variable, at the second clear it, this
	 * because of double FIN-ACK and RST-ACK happening between both hosts.
	 */
	SessionTrack& st = stm_it->second;
	if (st.shutdown == false) {
		st.selflog(__func__, "shutdown false set to be true");
		st.shutdown = true;
	} else {
		st.selflog(__func__, "shutdown true, deleting session");
		sex_map.erase(stm_it);
	}
}

TTLFocus* TCPTrack::init_ttlfocus(unsigned int daddr) 
{
	TTLFocusMap::iterator it = ttlfocus_map.find(daddr);
	if (it != ttlfocus_map.end())
		return &(it->second);	
	else
		return &(ttlfocus_map.insert(pair<const unsigned int, TTLFocus>(daddr, daddr)).first->second);
}

/* 
 * enque_ttl_probe has not the intelligence to understand if TTL bruteforcing 
 * is required or not more. Is called in different section of code
 */
void TCPTrack::enque_ttl_probe(const Packet &delayed_syn_pkt, TTLFocus& ttlfocus)
{
	/* 
	 * the first packet (the SYN) is used as starting point
	 * in the enque_ttl_burst to generate the series of 
	 * packets able to detect the number of hop distance 
	 * between our peer and the remote peer. the packet
	 * is lighty modify (ip->id change) and checksum fixed
	 */
	 
	if(!ttlfocus.isProbeIntervalPassed(clock))
		return;

	if (analyze_ttl_stats(ttlfocus))
		return;
	
	/* create a new packet; the copy is done to keep refsyn ORIGINAL */
	Packet *injpkt = new Packet(delayed_syn_pkt);
	injpkt->mark(TTLBFORCE, SEND, INNOCENT);

	/* 
	 * if TTL expire and is generated and ICMP TIME EXCEEDED,
	 * the iphdr is preserved and the tested_ttl found
	 */
	ttlfocus.sent_probe++;
	injpkt->ip->ttl = ttlfocus.sent_probe;
	injpkt->tcp->source = ttlfocus.puppet_port;
	injpkt->tcp->seq = htonl(ttlfocus.rand_key + ttlfocus.sent_probe);
	injpkt->ip->id = (ttlfocus.rand_key % 64) + ttlfocus.sent_probe;

	p_queue.insert(HIGH, *injpkt);
	
	ttlfocus.scheduleNextProbe();

	snprintf(injpkt->debugbuf, LARGEBUF, "Injecting probe %d [exp %d min work %d]",
		ttlfocus.sent_probe, ttlfocus.expiring_ttl, ttlfocus.min_working_ttl
	);
	injpkt->selflog(__func__, injpkt->debugbuf);
}

bool TCPTrack::analyze_ttl_stats(TTLFocus &ttlfocus)
{
	if (ttlfocus.sent_probe == runcopy->max_ttl_probe) {
		ttlfocus.status = TTL_UNKNOWN;
		return true;
	}
	return false;
}

void TCPTrack::analyze_incoming_ttl(Packet &pkt)
{
	TTLFocusMap::iterator it = ttlfocus_map.find(pkt.ip->saddr);
	TTLFocus *ttlfocus;

	if (it != ttlfocus_map.end()) 
	{
		ttlfocus = &(it->second);
		if (ttlfocus->status == TTL_KNOWN && ttlfocus->synack_ttl != pkt.ip->ttl) 
		{
			/* probably a topology change has happened - we need a solution wtf!!  */
			snprintf(pkt.debugbuf, LARGEBUF, 
				"net topology change! #probe %d [exp %d min work %d synack ttl %d]",
				ttlfocus->sent_probe, ttlfocus->expiring_ttl, 
				ttlfocus->min_working_ttl, ttlfocus->synack_ttl
			);
			pkt.selflog(__func__, pkt.debugbuf);
		}
	}
}


Packet* TCPTrack::analyze_incoming_icmp(Packet &timeexc)
{
	const struct iphdr *badiph;
	const struct tcphdr *badtcph;
	TTLFocusMap::iterator ttlfocus_map_it;

	badiph = (struct iphdr *)((unsigned char *)timeexc.icmp + sizeof(struct icmphdr));
	badtcph = (struct tcphdr *)((unsigned char *)badiph + (badiph->ihl * 4));

	ttlfocus_map_it = ttlfocus_map.find(badiph->daddr);
	if (ttlfocus_map_it != ttlfocus_map.end() && badiph->protocol == IPPROTO_TCP) {
		TTLFocus *ttlfocus = &(ttlfocus_map_it->second);
		unsigned char expired_ttl = badiph->id - (ttlfocus->rand_key % 64);
		unsigned char exp_double_check = ntohl(badtcph->seq) - ttlfocus->rand_key;

		if (ttlfocus->status != TTL_KNOWN && expired_ttl == exp_double_check) {
			ttlfocus->received_probe++;

			if (expired_ttl > ttlfocus->expiring_ttl) {
				snprintf(ttlfocus->debugbuf, LARGEBUF, "good TTL: recv %d", expired_ttl);
				ttlfocus->selflog(__func__, ttlfocus->debugbuf);
				ttlfocus->expiring_ttl = expired_ttl;
			}
			else  {
				snprintf(ttlfocus->debugbuf, LARGEBUF, "BAD TTL!: recv %d", expired_ttl);
				ttlfocus->selflog(__func__, ttlfocus->debugbuf);
			}
		}
		p_queue.remove(timeexc);
		delete &timeexc;
		return NULL;
	}
	
	return &timeexc;
}

Packet* TCPTrack::analyze_incoming_synack(Packet &synack)
{
	TTLFocusMap::iterator it = ttlfocus_map.find(synack.ip->saddr);
	TTLFocus *ttlfocus;

	/* NETWORK is src: dest port and source port inverted and saddr are used, 
	 * source is put as last argument (puppet port)
	 */

	if (it != ttlfocus_map.end()) {
		
		ttlfocus = &(it->second);

		snprintf(synack.debugbuf, LARGEBUF, "puppet %d Incoming SYN/ACK", ntohs(ttlfocus->puppet_port));
		synack.selflog(__func__, synack.debugbuf);

		if (synack.tcp->dest == ttlfocus->puppet_port) {
			unsigned char discern_ttl =  ntohl(synack.tcp->ack_seq) - ttlfocus->rand_key - 1;

			ttlfocus->received_probe++;
			ttlfocus->status = TTL_KNOWN;

			if (ttlfocus->min_working_ttl > discern_ttl && discern_ttl <= ttlfocus->sent_probe) { 
				ttlfocus->min_working_ttl = discern_ttl;
				ttlfocus->expiring_ttl = discern_ttl - 1;
				ttlfocus->synack_ttl = synack.ip->ttl;
			}

			snprintf(ttlfocus->debugbuf, LARGEBUF, "discerned TTL %d", discern_ttl);
			ttlfocus->selflog(__func__, ttlfocus->debugbuf);

			/* 
			* this code flow happens only when the SYN ACK is received, due to
			* a SYN send from the "puppet port". this kind of SYN is used only
			* for discern TTL, and this mean a REFerence-SYN packet is present in
			* the packet queue. Now that ttl has been detected, the real SYN could
			* be send.
			*/
		
			mark_real_syn_packets_SEND(synack.ip->saddr);
			p_queue.remove(synack);
			delete &synack;
			return NULL;
		}
				
		/* 
		 * connect(3, {sa_family=AF_INET, sin_port=htons(80), 
		 * sin_addr=inet_addr("89.186.95.190")}, 16) = 
		 * -1 EHOSTUNREACH (No route to host)
		 *
		 * sadly, this happens when you try to use the real syn. for
		 * this reason I'm using encoding in random sequence and a
		 * fake source port (puppet port)
		 *
		 * anyway, every SYN/ACK received is passed to the hosts, so
		 * our kernel should RST/ACK the unrequested connect.
		 */

	}
	
	return &synack;
}

Packet* TCPTrack::analyze_incoming_rstfin(Packet &rstfin)
{
	SessionTrackKey key = {rstfin.ip->saddr, rstfin.tcp->dest, rstfin.tcp->source};
	SessionTrackMap::iterator stm_it = sex_map.find(key);

	if (stm_it != sex_map.end()) {
		rstfin.selflog(__func__, "Session found");
		clear_session(stm_it);
	} else {
		rstfin.selflog(__func__, "Session not found!");
	}

	return &rstfin;
}

void TCPTrack::manage_outgoing_packets(Packet &pkt)
{
	TTLFocus *ttlfocus;
	SessionTrackKey key = {pkt.ip->daddr, pkt.tcp->source, pkt.tcp->dest};
	SessionTrackMap::iterator sex_map_it;
	SessionTrack *session;

	/* 
	 * session get return an existing session or even NULL, 
	 */
	if (pkt.tcp->syn) {
		init_sessiontrack(pkt);
		ttlfocus = init_ttlfocus(pkt.ip->daddr);

		pkt.selflog(__func__, "incoming SYN");

		/* if sniffjoke had not yet the minimum working ttl, continue the starting probe */
		if (ttlfocus->status == TTL_BRUTALFORCE) {
			ttlfocus->selflog(__func__, "SYN retrasmission, keep pkt");
			pkt.status = KEEP; 
			return;
		}
	}

	sex_map_it = sex_map.find(key);

	if (sex_map_it != sex_map.end()) 
	{
		session = &(sex_map_it->second);
		session->packet_number++;
		if (pkt.tcp->fin || pkt.tcp->rst) 
		{
			pkt.selflog(__func__, "handling closing flags");
			clear_session(sex_map_it);
			   
		} else {
			/* update_session_stat(xml_stat_root, ct); */

			/* a closed or shutdown session don't require to be hacked */
			pkt.selflog(__func__, "injecting pkt in queue");
			inject_hack_in_queue(pkt, session);		
		}
	}
}

void TCPTrack::mark_real_syn_packets_SEND(unsigned int daddr)
{
	Packet *pkt = p_queue.get(ANY_STATUS, ANY_SOURCE, TCP, false);
	while (pkt != NULL) {
		if (pkt->tcp->syn && pkt->ip->daddr == daddr) {
			pkt->selflog(__func__, "the orig SYN shift from keep to send");
			pkt->status = SEND;
		}
		pkt = p_queue.get(ANY_STATUS, ANY_SOURCE, TCP, true);
	}
}

/* 
 * inject_hack_in_queue is one of the core function in sniffjoke:
 *
 * the hacks are, for the most, two kinds.
 *
 * one kind require the knowledge of exactly hop distance between the two end points, to forge
 * packets able to expire an hop before the destination IP addres, and inject in the
 * stream some valid TCP RSQ, TCP FIN and fake sequenced packet.
 *
 * the other kind of attack work forging packets with bad details, wishing the sniffer ignore
 * those irregularity and poison the connection tracking: injection of RST with bad checksum;
 * bad checksum FIN packet; bad checksum fake SEQ; valid reset with bad sequence number ...
 *
 */
void TCPTrack::inject_hack_in_queue(Packet &orig_pkt, const SessionTrack *session)
{
	vector<HackPacketPoolElem>::iterator it;
	HackPacketPoolElem *hppe;
	
	HackPacketPool applicable_hacks = hack_pool;
	
	/* SELECT APPLICABLE HACKS */
	for ( it = applicable_hacks.begin(); it != applicable_hacks.end(); it++ ) {
		hppe = &(*it);
		hppe->enabled &= *(hppe->config);
		hppe->enabled &= hppe->dummy->Condition(orig_pkt);
		hppe->enabled &= percentage(logarithm(session->packet_number), hppe->dummy->hack_frequency);
	}

	/* -- RANDOMIZE HACKS APPLICATION */
	random_shuffle( applicable_hacks.begin(), applicable_hacks.end() );

	/* -- FINALLY, SEND THE CHOOSEN PACKET(S) */
	for ( it = applicable_hacks.begin(); it != applicable_hacks.end(); it++ ) 
	{
		/* must be moved in the do/while loop based on HackPacket->num_pkt_gen */
		Packet *injpkt;

		hppe = &(*it);
		if(!hppe->enabled) 
			continue;

		injpkt = hppe->dummy->createHack(orig_pkt);

		/* we trust in the external developer, but is required a safety check by sniffjoke :) */
		if(!injpkt->SelfIntegrityCheck(hppe->dummy->hackName)) {
			internal_log(NULL, ALL_LEVEL, "invalid packet generated by hack %s", hppe->dummy->hackName);
			delete injpkt;
			continue;
		}
		injpkt->mark(LOCAL, SEND);
		snprintf(injpkt->debugbuf, MEDIUMBUF, "inj from %s", hppe->dummy->hackName);
		injpkt->selflog(__func__, injpkt->debugbuf);

		switch(injpkt->position) {
			case ANTICIPATION:
				p_queue.insert_before(*injpkt, orig_pkt);
				break;
			case POSTICIPATION:
				p_queue.insert_after(*injpkt, orig_pkt);
				break;
			case ANY_POSITION:
				if(random() % 2)
					p_queue.insert_before(*injpkt, orig_pkt);
				else
					p_queue.insert_after(*injpkt, orig_pkt);
				break;
		}
	}
}

/* 
 * Last_pkt_fix is the last modification applied to packets.
 * Modification involve only TCP packets coming from TUNNEL.
 * On others packs, treated as INNOCENT, get only fixed the IP/TCP CHECKSUM.
 * They could be:
 * 
 *   PRESCRIPTION: will EXPIRE BEFORE REACHING destination (due to ttl modification)
 * 			could be: ONLY HACK PACKETS
 *   GUILTY:       will BE DISCARDED by destination (due to some error introduction)
 *                      at the moment the only error applied is the invalidation tcp checksum
 *                      could be: ONLY HACK PACKETS 
 *   INNOCENT      will REACH the destination
 *                      could be: REAL PACKETS
 *                                HACK PACKETS (that will be discarded by destination)
 */
void TCPTrack::last_pkt_fix(Packet &pkt)
{
	const TTLFocus *ttlfocus;
	TTLFocusMap::iterator ttlfocus_map_it;

	if (pkt.proto != TCP) {
		return;
	} else if (pkt.source != TUNNEL && pkt.source != LOCAL) {
		pkt.fixIpTcpSum();
		return;
	}
	
	/* 1st check: WHAT VALUE OF TTL GIVE TO THE PACKET ? */
	ttlfocus_map_it = ttlfocus_map.find(pkt.ip->daddr);

	if (ttlfocus_map_it != ttlfocus_map.end())
		ttlfocus = &(ttlfocus_map_it->second);
	else
		ttlfocus = NULL;

	if (ttlfocus != NULL && ttlfocus->status != TTL_UNKNOWN) 
	{
		if (pkt.wtf == PRESCRIPTION) 
			pkt.ip->ttl = ttlfocus->expiring_ttl - (random() % 5);
		else	/* GUILTY or INNOCENT */
			pkt.ip->ttl = ttlfocus->min_working_ttl + (random() % 5);

		pkt.selflog(__func__, "TTL available");
	} 
	else {
		if (pkt.wtf == PRESCRIPTION)
			pkt.wtf = GUILTY;

		pkt.ip->ttl = STARTING_ARB_TTL + (random() % 100);
		pkt.selflog(__func__, "TTL not available");
	}	
	/* end 1st check */
	
	/* START FIXME START FIXME START FIXME START FIXME START FIXME START FIXME START */ 

	/* 2nd check: WHAT KIND OF INJECTIONS CAN WE INJECT IP/TCP OPTIONS INTO THE PACKET */
	
	/* AT THE MOMENT THE IMPLEMENTED IP/TCP OPTIONS COULD LEAD PKTS TO BE DISCARDED BY DESTIONATION
	 * SO AT THE MOMENT WE APPLY THEM TO EVIL PKTS ONLY.
	 * IN FUTURE WE PROBABLY WILL HAVE ALSO INTERESTING INJECTIONS THAT COULD LEAD THE SNIFFER TO FAIL WITHOUT
	 * DISTURB THE SEXION.
	 * SO WE WILL IMPLEMENT SOMETHING LIKE:

		if(pkt.evilbit == EVIL) { 
			* real packet;
			* we MUST select injections that assicure that the packet
			* can arrive to destination without been discarded.
			* 
		} else {
			* 
			* hack packet, INNOCENT, PRESCRIPTION or GUILTY it WILL be discarded;
			* we CAN select one of all injections.
			*
		}
	*/

	if(pkt.evilbit == EVIL) {

		/* 2nd check: CAN WE INJECT IP/TCP OPTIONS INTO THE PACKET ? */
		if (runcopy->SjH__inject_ipopt && (pkt.injection == ANY_INJECTION || pkt.injection == IP_INJECTION)) {
			if (percentage(15, 100)) {
				pkt.SjH__inject_ipopt();
			}
		}

		if (runcopy->SjH__inject_tcpopt && (pkt.injection == ANY_INJECTION || pkt.injection == TCP_INJECTION)) {
			if (!check_uncommon_tcpopt(pkt.tcp)) {
				if (percentage(25, 100)) {
					pkt.SjH__inject_tcpopt();
				}
			}
		}
	}
	/* end 2nd check */
	
	/* END FIXME END FIXME END FIXME END FIXME END FIXME END FIXME END FIXME END */

	/* 3rd check: GOOD CHECKSUM or BAD CHECKSUM ? */
	pkt.fixIpTcpSum();
	if (pkt.wtf == GUILTY)
		pkt.tcp->check ^= (0xd34d * (unsigned short)random() + 1);
	/* end 3nd check */
}

/* the packet is add in the packet queue for be analyzed in a second time */
bool TCPTrack::writepacket(const source_t source, const unsigned char *buff, int nbyte)
{
	Packet *pkt;
	
	if (check_evil_packet(buff, nbyte) == false)
		return false;

	/* 
	 * the packet_id is required because the OS should generate 
	 * duplicate SYN when didn't receive the expected answer. 
	 *
	 * this happens when the first SYN is blocked for TTL bruteforce
	 * routine. 
	 *
	 * the nbyte is added with the max ip/tcp option injection because
	 * the packets options could be modified by last_pkt_fix
	 */

	pkt = new Packet(buff, nbyte);
	pkt->mark(source, YOUNG, INNOCENT);
	
	/* 
	 * the packet from the tunnel are put with lower priority and the
	 * hack-packet, injected from sniffjoke, are put in the higher one.
	 * when the software loop for in p_queue.get(status, source, proto) the 
	 * forged packet are sent before the originals one.
	 */
	p_queue.insert(LOW, *pkt);
	
	youngpacketspresent = true;
	
	return true;
}

Packet* TCPTrack::readpacket()
{
	Packet *pkt = p_queue.get(SEND, ANY_SOURCE, ANY_PROTO, false);
	if (pkt != NULL) {
		p_queue.remove(*pkt);
		if (runcopy->sj_run == true)
			last_pkt_fix(*pkt);
	}
	return pkt;
}

/* 
 * this is the "second time", the received packet are assigned in a tracked TCP session,
 * for understand which kind of mangling should be apply. maybe not all packets is sent 
 * immediatly, this happens when sniffjoke require some time (and some packets) for
 * detect the hop distance between the remote peer.
 *
 * as defined in sniffjoke.h, the "status" variable could have these status:
 * SEND (packet marked as sendable)
 * KEEP (packet to keep and wait)
 * YOUNG (packet received, here analyzed for the first time)
 *
 * analyze_packets_queue is called from the main.cc poll() block
 */
void TCPTrack::analyze_packets_queue()
{
	Packet *pkt;
	TTLFocusMap::iterator ttlfocus_map_it;
	TTLFocus *ttlfocus;
	
	clock_gettime(CLOCK_REALTIME, &clock);

	if(youngpacketspresent == false)
		goto analyze_keep_packets;
	else
		youngpacketspresent = false;

	pkt = p_queue.get(YOUNG, NETWORK, ICMP, false);
	while (pkt != NULL) {
		
		analyze_incoming_ttl(*pkt);
		
		/* 
		 * a TIME_EXCEEDED packet should contains informations
		 * for discern HOP distance from a remote host
		 */
		if (pkt->icmp->type == ICMP_TIME_EXCEEDED) {
			pkt = analyze_incoming_icmp(*pkt);
		}

		pkt = p_queue.get(YOUNG, NETWORK, ICMP, true);
	}

	/* 
	 * incoming TCP. sniffjoke algorithm open/close sessions and detect TTL
	 * lists analyzing SYN+ACK and FIN|RST packet
	 */
	pkt = p_queue.get(YOUNG, NETWORK, TCP, false);
	while (pkt != NULL) {
		
		analyze_incoming_ttl(*pkt);

		if (pkt->tcp->syn && pkt->tcp->ack)
			pkt = analyze_incoming_synack(*pkt);

		if (pkt != NULL && pkt->status == YOUNG && (pkt->tcp->rst || pkt->tcp->fin))
			pkt = analyze_incoming_rstfin(*pkt);   

		pkt = p_queue.get(YOUNG, NETWORK, TCP, true);
	}

	/* outgoing TCP packets ! */
	pkt = p_queue.get(YOUNG, TUNNEL, TCP, false);
	while (pkt != NULL) {

		/* no hacks required for this destination port */
		if (runcopy->portconf[ntohs(pkt->tcp->dest)] == NONE) {
			pkt->status = SEND; 
			continue;
		}

		/* 
		 * create/close session, check ttlfocus and start new discovery, 
		 * this function contains the core functions of sniffjoke: 
		 * enque_ttl_probe and inject_hack_in_queue 
		 *
		 * those packets had ttlfocus set inside
		 */
		manage_outgoing_packets(*pkt);

		pkt = p_queue.get(YOUNG, TUNNEL, TCP, true);
	}

	/* all YOUNG packets must be sent immediatly */
	pkt = p_queue.get(YOUNG, ANY_SOURCE, ANY_PROTO, false);
	while (pkt != NULL) {
		pkt->status = SEND;
		pkt = p_queue.get(YOUNG, ANY_SOURCE, ANY_PROTO, true);
	}

analyze_keep_packets:

	pkt = p_queue.get(KEEP, TUNNEL, TCP, false);
	while (pkt != NULL) {
		ttlfocus_map_it = ttlfocus_map.find(pkt->ip->daddr);
		if (ttlfocus_map_it == ttlfocus_map.end())
			 check_call_ret("unforeseen bug: ttlfocus == NULL in TCPTrack.cc, contact the package mantainer, sorry. analyze_packet_queue", 0, -1, true);
		
		ttlfocus = &(ttlfocus_map_it->second);
		if (ttlfocus->status == TTL_BRUTALFORCE)  {
			pkt->selflog(__func__, "ttl status BForce, pkt KEEP");
			enque_ttl_probe(*pkt, *ttlfocus);
		}
		pkt = p_queue.get(KEEP, TUNNEL, TCP, true);
	}
}

/*
 * this function set SEND stats to all packets, is used when sniffjoke must not 
 * mangle the packets 
 */
void TCPTrack::force_send(void)
{
	Packet *pkt = p_queue.get(false);
	while (pkt != NULL) {
		pkt->status = SEND;
		pkt = p_queue.get(true);
	}
}