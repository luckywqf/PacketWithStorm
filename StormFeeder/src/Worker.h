/*
 * Worker.h
 *
 *  Created on: Dec 30, 2014
 *      Author: luckywqf
 */

#ifndef WORKER_H_
#define WORKER_H_

#include <thread>
#include <map>
#include <sys/epoll.h>

#define MAXSIZE     1024
#define EPOLLEVENTS 1000

using std::map;
using std::thread;

class Client {
public:
	Client() {
		Client(0);
	}

	Client(int fd) {
		fd_ = fd;
		hasSend_ = false;
		canSend_ = true;//can send when start
	}

	bool NeedSend() {
		return !hasSend_ && canSend_;
	}

	bool hasSend() {
		return hasSend_;
	}

	void setSend(bool haveSend) {
		hasSend_ = haveSend;
		std::cerr << fd_ << " send=" << hasSend_ << std::endl;
	}

	void setCanSend(bool canSend) {
		canSend_ = canSend;
	}

private:
	int fd_;
	bool hasSend_;
	bool canSend_;
};


class RawPacket;
class SpoutWorker : boost::noncopyable{
private:
	char ip_[32];
	uint16_t port_;
	int listenfd_;
	int epollfd_;
	struct epoll_event events[EPOLLEVENTS];
	char readBuf_[MAXSIZE];

	thread workThread_;
	bool running_;
	map<int, Client> clientMap_;

public:
	int StartWorker(const char* ip, uint16_t port);

	int CloseWorker();

private:
	int socket_bind(const char* ip,int port);

	void SetClientRoundStart();

	void CheckRound();
	void SetClientsEvent();
	void Run();

	void handle_events(struct epoll_event *events, int num, int listenfd);

	void HandleAccpet(int listenfd);

	void AddClient(int fd);

	void RemoveClient(int fd);

	void HandleWrite(int fd);

	void do_error(int fd);

	void do_read(int fd);

	void do_write(int fd, RawPacket *packet);

	void add_event(int fd, int state);

	void delete_event(int fd, int state);

	void modify_event(int fd, int state);
};





#endif /* WORKER_H_ */
