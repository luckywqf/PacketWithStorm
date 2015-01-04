//============================================================================
// Name        : packetAnalysis.cpp
// Author      : luckywqf
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <pcap.h>
#include <stdio.h>
#include <iostream>
#include <queue>
#include "packet_def.h"
#include "RawPacket.h"
#include "Worker.h"
using namespace std;

void PcapHandler(u_char *args, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
	PacketContainer::get_mutable_instance().pushPacket(RawPacket(pkthdr, packet));
}

pcap_t* StartCapture()
{
	pcap_t *handle;			/* Session handle */
	char *dev;			/* The device to sniff on */
	char errbuf[PCAP_ERRBUF_SIZE];	/* Error string */
	//struct bpf_program fp;		/* The compiled filter */
	//char filter_exp[] = "port 23";	/* The filter expression */
	bpf_u_int32 mask;		/* Our netmask */
	bpf_u_int32 net;		/* Our IP */
	struct pcap_pkthdr header;	/* The header that pcap gives us */
	const u_char *packet;		/* The actual packet */

	/* Define the device */
	dev = pcap_lookupdev(errbuf);
	if (dev == NULL) {
		fprintf(stderr, "Couldn't find default device: %s\n", errbuf);
		return NULL;
	}
	/* Find the properties for the device */
	if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
		fprintf(stderr, "Couldn't get netmask for device %s: %s\n", dev, errbuf);
		net = 0;
		mask = 0;
	}
	/* Open the session in promiscuous mode */
	handle = pcap_open_live(dev, BUFSIZ, 1, 0, errbuf);
	if (handle == NULL) {
		fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
		return NULL;
	}
	/* Compile and apply the filter */
//	if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
//		fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
//		return(2);
//	}
//	if (pcap_setfilter(handle, &fp) == -1) {
//		fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
//		return(2);
//	}
}

int main(int argc, char *argv[])
{
	bool running = true;

	SpoutWorker worker;
	worker.StartWorker("127.0.0.1", 125);

	pcap_t *handle = StartCapture();
	assert(handle);
	while (running) {
		pcap_loop(handle, 65536, PcapHandler, NULL);
	}

	pcap_close(handle);
	return(0);
}
