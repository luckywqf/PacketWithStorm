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
	return handle;
}

int main(int argc, char *argv[])
{
	bool running = true;

	SpoutWorker worker;
	worker.StartWorker("0.0.0.0", 125);

	pcap_t *handle = StartCapture();
	assert(handle);
	while (running) {
		pcap_loop(handle, -1, PcapHandler, NULL);
	}

	pcap_close(handle);
	return(0);
}
