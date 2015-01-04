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

RawPacket::RawPacket(const RawPacket &rp)
{
	pkthdr_ = rp.pkthdr_;
	packet_ = new u_char[rp.pkthdr_.caplen];
	memcpy(packet_, rp.packet_, rp.pkthdr_.caplen);
}

RawPacket::RawPacket(RawPacket &&rp)
{
	pkthdr_ = rp.pkthdr_;
	packet_ = rp.packet_;
	memset((char*)&rp.pkthdr_, 0, sizeof(rp.pkthdr_));
	rp.packet_ = NULL;
}

RawPacket& RawPacket::operator=(RawPacket &&rp)
{
	if (&rp != this) {
		delete []packet_;

		pkthdr_ = rp.pkthdr_;
		packet_ = rp.packet_;
		memset((char*)&rp.pkthdr_, 0, sizeof(rp.pkthdr_));
		rp.packet_ = NULL;
	}
	return *this;
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
	packetQueue_.push(std::move(packet));
}
