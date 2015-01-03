/*
 * RawPacketToJson.cpp
 *
 *  Created on: 2014��12��29��
 *      Author: luckywqf
 */

#include <string.h>
#include <queue>
#include <mutex>
#include <boost/thread/detail/singleton.hpp>
#include "RawPacket.h"

#define MAX_QUEUE_SIZE 10000

RawPacket::RawPacket(const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
	pkthdr_ = *pkthdr;
	packet_ = new u_char[pkthdr->caplen];
	memcpy(packet_, packet, pkthdr->caplen);
}


RawPacket PacketContainer::getPacket()
{
	lock_guard<mutex> lck (mtx);
	assert(!packetQueue_.empty());
	RawPacket packet = std::move(packetQueue_.front());
	packetQueue_.pop();
	return std::move(packet);
}

void PacketContainer::pushPacket(RawPacket &&packet)
{
	lock_guard<mutex> lck (mtx);
	while (packetQueue_.size() >= MAX_QUEUE_SIZE) {
		packetQueue_.pop();
	}
	packetQueue_.push(packet);
}
