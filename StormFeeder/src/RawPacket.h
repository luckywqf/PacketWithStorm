#ifndef RAWPACKETTOJSON_H_
#define RAWPACKETTOJSON_H_

#include <assert.h>
#include <vector>
#include <queue>
#include <mutex>
#include <boost/serialization/singleton.hpp>
#include <pcap.h>
#include "packet_def.h"

using std::queue;
using std::mutex;
using std::lock_guard;
using boost::serialization::singleton;

class RawPacket {
public:
	struct pcap_pkthdr pkthdr_;
	u_char *packet_;

public:
	RawPacket(const struct pcap_pkthdr *pkthdr, const u_char *packet);
	RawPacket(const RawPacket &rp);
	RawPacket(RawPacket &&rp);
	RawPacket& operator=(RawPacket &&rp);
	virtual ~RawPacket() {
		delete []packet_;
		packet_ = NULL;
	}
};


class PacketContainer : public singleton<PacketContainer>
{
private:
	queue<RawPacket> packetQueue_;
	mutex mtx;

public:
	int GetSize()
	{
		lock_guard<mutex> lck (mtx);
		return packetQueue_.size();
	}

	RawPacket getPacket();

	void pushPacket(RawPacket &&packet);
};

#endif /* RAWPACKETTOJSON_H_ */
