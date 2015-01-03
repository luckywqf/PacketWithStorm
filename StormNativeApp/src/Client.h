/*
 * Client.h
 *
 *  Created on: Jan 3, 2015
 *      Author: luckywqf
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include "RawPacket.h"

class Client {
private:
	Client();

private:
	static const int HostLen = 256;
	static const int PortLen = 8;
	char host_[HostLen];
	char port_[PortLen];
	int fd_;

	static const int BufferLen = 10240;
	char dataBuf_[BufferLen];
	int dataStart_;
	int currentPos_;
public:
	virtual ~Client();
	static Client* CreateClient(const char *host, const char *port);
	int Connect();
	RawPacket* NextPacket();
	int CheckPacketComplete(char *data, int len);
};

#endif /* CLIENT_H_ */
