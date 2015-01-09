/*
 * Client.cpp
 *
 *  Created on: Jan 3, 2015
 *      Author: luckywqf
 */

#include "Client.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

Client::Client() {
	// TODO Auto-generated constructor stub

	dataStart_ = 0;
	currentPos_ = 0;
}

Client::~Client() {
	// TODO Auto-generated destructor stub
}

Client* Client::CreateClient(const char *host, const char *port)
{
	if (host == NULL || strlen(host) > HostLen ||
			port == NULL || strlen(port) > PortLen) {
		return NULL;
	}
	Client *cl = new Client();
	strcpy(cl->host_, host);
	strcpy(cl->port_, port);
	return cl;
}

int Client::Connect()
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;


	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          /* Any protocol */

	s = getaddrinfo(host_, port_, &hints, &result);
	if (s != 0) {
	   fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
	   exit(EXIT_FAILURE);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
	   sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	   if (sfd == -1)
		   continue;

	   if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
		   break;                  /* Success */

	   close(sfd);
	}

	if (rp == NULL) {               /* No address succeeded */
	   fprintf(stderr, "Could not connect\n");
	   exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);           /* No longer needed */
	fd_ = sfd;
	return 0;
}


//if the packet isn't complete, return < 0
//else return the next packet length
//
int Client::CheckPacketComplete(char *data, int len)
{
	len -= sizeof(struct pcap_pkthdr);
	if (len < 0) {
		return len;
	}

	struct pcap_pkthdr *header = (struct pcap_pkthdr*)data;
	assert(header->caplen <= 65535);
	len -= header->caplen;
	return len;
}

RawPacket* Client::NextPacket()
{
	static uint64_t receiveLen = 0;
	static uint64_t dealLen = 0;
	ssize_t nread;
	while(true) {
		int leftLen = CheckPacketComplete(dataBuf_ + dataStart_, currentPos_ - dataStart_);
		if (leftLen >= 0) {
			RawPacket *packet = new RawPacket((struct pcap_pkthdr *)(dataBuf_ + dataStart_), (unsigned char*)(dataBuf_ + dataStart_ + sizeof(struct pcap_pkthdr)));
			int packetLen = sizeof(struct pcap_pkthdr) + packet->pkthdr_.caplen;
			if (BufferLen - currentPos_ < 8192) { //max pdu length
				int leftStart = dataStart_ + packetLen;
				memcpy(dataBuf_, dataBuf_ + leftStart, leftLen);
				currentPos_ = leftLen; // move to starter
				dataStart_ = 0;
			} else {
				dataStart_ += packetLen;
			}

//			std::cerr << "leftLen=" << leftLen << ", packetLen=" << packetLen << ", dataStart_=" << dataStart_
//					<< ", currentPos_=" << currentPos_ << ", receiveLen="<< receiveLen << std::endl;
			std::cerr << dealLen << " " << packet->pkthdr_.caplen << std::endl;
			dealLen += packetLen;
			return packet;
		} else if (currentPos_ >= BufferLen) {
			int len = currentPos_ - dataStart_;
			memcpy(dataBuf_, dataBuf_ + dataStart_, currentPos_ - dataStart_);
			currentPos_ = len; // move to starter
			dataStart_ = 0;
		}


		nread = read(fd_, dataBuf_ + currentPos_, BufferLen - currentPos_);
		if (nread < 0) {
		   perror("read");
		   return NULL;
		}
		currentPos_ += nread;
		receiveLen += nread;
	}
}
