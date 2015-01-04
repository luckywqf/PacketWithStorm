#include <iostream>
#include <stdexcept>

#include "PacketSpout.h"

#define TEST 1

#ifndef TEST

int main(int argc, char *argv[])
{
	PacketSpout ps;
	ps.Run();
	return 0;
}

#else

int main(int argc, char *argv[])
{
	Client *client;
	client = Client::CreateClient("127.0.0.1", "125");
	client->Connect();
	assert(client);
	while(true) {
		RawPacket *rp = client->NextPacket();
		if (rp == NULL) {
			std::cerr << "connection close or exception" << std::endl;
			return -1;
		}
		Json::Value j_token = ParsePacket(rp);
		std::cout << j_token.toStyledString() << std::endl;
	}
	return 0;
}

#endif
