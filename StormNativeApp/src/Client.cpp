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

}

Client::~Client() {
	// TODO Auto-generated destructor stub
}

Client* Client::CreateClient(char *host, char *port)
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
	int sfd, s, j;

	/* Obtain address(es) matching host/port */

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          /* Any protocol */

	s = getaddrinfo(host_, port_, &hints, &result);
	if (s != 0) {
	   fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
	   exit(EXIT_FAILURE);
	}

	/* getaddrinfo() returns a list of address structures.
	  Try each address until we successfully connect(2).
	  If socket(2) (or connect(2)) fails, we (close the socket
	  and) try the next address. */

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
	len -= header->caplen;
	return len;
}

RawPacket* Client::NextPacket()
{
	ssize_t nread;
	while(true) {
		nread = read(fd_, dataBuf_ + currentPos_, BufferLen - currentPos_);
		if (nread <= 0) {
		   perror("read");
		   return NULL;
		}
		currentPos_ += nread;
		int leftLen = CheckPacketComplete(dataBuf_ + dataStart_, currentPos_ - dataStart_);
		if (leftLen >= 0) {
			RawPacket *packet = new RawPacket(dataBuf_ + dataStart_, dataBuf_ + dataStart_ + sizeof(struct pcap_pkthdr));
			if (BufferLen - currentPos_ < 1600) { //max pdu length
				currentPos_ = 0; // move to starter
			}
			dataStart_ = currentPos_;
			return packet;
		}

//		else if (currentPos_ - dataStart_ + >= BufferLen - 100) {
//
//		}
	}
}
