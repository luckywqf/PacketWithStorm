/*
Author: Sasa Petrovic (montyphyton@gmail.com)
Copyright (c) 2012, University of Edinburgh
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SPLIT_SENTENCE_H
#define SPLIT_SENTENCE_H

#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
#include <arpa/inet.h>
#include "json/json.h"
#include "Storm.h"
#include "RawPacket.h"
#include "Client.h"


using namespace storm;
using std::string;
string HardAddressToString(unsigned char *address) {
	char hardAddress[16];
	sprintf(hardAddress, "%X-%X-%X-%X-%X-%X", address[0], address[1], address[2],
			address[3], address[4], address[5]);
	return hardAddress;
}

Json::Value ParsePacket(RawPacket *rp)
{
	Json::Value root;
	root["time"] = (long long)rp->pkthdr_.ts.tv_sec;
	root["caplen"] = rp->pkthdr_.caplen;

	unsigned char *packet = rp->packet_;
	const struct sniff_ethernet *ethernet = (struct sniff_ethernet*)(packet);
	root["ether_type"] = ethernet->ether_type;
	root["ether_src"] = HardAddressToString((unsigned char*)ethernet->ether_shost);
	root["ether_dst"] = HardAddressToString((unsigned char*)ethernet->ether_dhost);

	if (ethernet->ether_type != 0x0800) { //not ip protocol
		return root;
	}

	const struct sniff_ip *ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
	u_int size_ip = IP_HL(ip)*4;
	if (size_ip < 20) {  //Invalid IP header length
		return root;
	}

	char ipdotdec[32];
	root["ip_type"] = ip->ip_p;
	inet_ntop(AF_INET, (void *)&ip->ip_src, ipdotdec, sizeof(ipdotdec));
	root["ip_src"] = ipdotdec;
	inet_ntop(AF_INET, (void *)&ip->ip_dst, ipdotdec, sizeof(ipdotdec));
	root["ip_dst"] = ipdotdec;
	if (ip->ip_p != IPPROTO_TCP && ip->ip_p != IPPROTO_UDP && ip->ip_p != IPPROTO_ICMP) {
		return root;
	}

//	u_int  header_len = 0;
//	switch (ip->ip_p) {
//	case IPPROTO_TCP:
//	{
//		const struct sniff_tcp *tcp = (struct sniff_tcp*)(packet + SIZE_ETHERNET + size_ip);
//		header_len = TH_OFF(tcp)*4;
//		if (header_len < 20) {
//			return root;
//		}
//	}
//		break;
//	case IPPROTO_UDP:
//	{
//		const struct sniff_udp *udp = (struct sniff_udp*)(packet + SIZE_ETHERNET + size_ip);
//		header_len = udp->th_len;
//		if (udp->th_len < 8) { //printf("   * Invalid UDP header length: %u bytes\n", size_tcp);
//			return root;
//		}
//	}
//		break;
//	case IPPROTO_ICMP:
//		header_len = 4;
//		break;
//	default:
//		return root;
//	}
//
//
//	const char *payload = (u_char *)(packet + SIZE_ETHERNET + size_ip + header_len);
//
//	RawPacket rp = new RawPacket(pkthdr, packet);
	return root;
}

class PacketSpout : public storm::Spout
{
private:
	Client *client;

public:
	virtual void Initialize(Json::Value conf, Json::Value context) {
		client = Client::CreateClient("127.0.0.1", "125");
		client->Connect();
	}

	// Read the next tuple and write it to stdout.
	virtual void NextTuple() {
		assert(client);
		RawPacket *rp = client->NextPacket();
		if (rp == NULL) {
			std::cerr << "connection close or exception" << std::endl;
			exit(-1);
		}

		Json::Value j_token = ParsePacket(rp);
		//j_token.append(tokens[i]);
		Tuple t(j_token);
		Emit(t);
	}
};


#endif
