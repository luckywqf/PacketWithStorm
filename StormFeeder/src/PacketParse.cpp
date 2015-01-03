/*
 * PacketParse.cpp
 *
 *  Created on: Dec 30, 2014
 *      Author: luckywqf
 */

#include "packet_def.h"
#include "RawPacket.h"


RawPacket* CreateRawPacket(const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
	const struct sniff_ethernet *ethernet = (struct sniff_ethernet*)(packet);
	if (ethernet->ether_type != 0x0800) { //not ip protocol
		return NULL;
	}

	const struct sniff_ip *ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
	u_int size_ip = IP_HL(ip)*4;
	if (size_ip < 20) {  //Invalid IP header length
		return NULL;
	}

	if (ip->ip_p != IPPROTO_TCP && ip->ip_p != IPPROTO_UDP && ip->ip_p != IPPROTO_ICMP) {
		return NULL;
	}

	u_int  header_len = 0;
	switch (ip->ip_p) {
	case IPPROTO_TCP:
	{
		const struct sniff_tcp *tcp = (struct sniff_tcp*)(packet + SIZE_ETHERNET + size_ip);
		header_len = TH_OFF(tcp)*4;
		if (header_len < 20) {
			return NULL;
		}
	}
		break;
	case IPPROTO_UDP:
	{
		const struct sniff_udp *udp = (struct sniff_udp*)(packet + SIZE_ETHERNET + size_ip);
		header_len = udp->th_len;
		if (udp->th_len < 8) { //printf("   * Invalid UDP header length: %u bytes\n", size_tcp);
			return NULL;
		}
	}
		break;
	case IPPROTO_ICMP:
		header_len = 4;
		break;
	default:
		return NULL;
	}


	const char *payload = (u_char *)(packet + SIZE_ETHERNET + size_ip + header_len);

	RawPacket rp = new RawPacket(pkthdr, packet);
	return rp;
}

