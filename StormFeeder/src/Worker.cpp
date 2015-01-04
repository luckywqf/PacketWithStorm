/*
 * WorkerLoop.cpp
 *
 *  Created on: 2014��12��29��
 *      Author: luckywqf
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <boost/noncopyable.hpp>
#include <functional>
#include <utility>
#include "RawPacket.h"
#include "Worker.h"


#define LISTENQ     5
#define FDSIZE      1000
#define WORKER_EPOLL (EPOLLIN | EPOLLOUT)// | EPOLLET)


int SpoutWorker::StartWorker(const char* ip, uint16_t port)
{
	strncpy(ip_, ip, sizeof(ip_));
	port_ = port;
	listenfd_ = socket_bind(ip_, port_);
	listen(listenfd_, LISTENQ);

	epollfd_ = epoll_create(FDSIZE);
	if (epollfd_ <= 0) {
		perror("epoll_create failed");
		return -1;
	}
	add_event(listenfd_, EPOLLIN | EPOLLET);

	running_ = true;

	workThread_ = thread(&SpoutWorker::Run, this);
	workThread_.detach();

	return 0;
}

int SpoutWorker::CloseWorker()
{
	running_ = false;
	usleep(1000);
	close(epollfd_);
	return 0;
}

int SpoutWorker::socket_bind(const char* ip,int port)
{
	int  listenfd;
	struct sockaddr_in servaddr;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1) {
		perror("socket error:");
		exit(1);
	}
	int optval = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_pton(AF_INET,ip,&servaddr.sin_addr);
	servaddr.sin_port = htons(port);
	if (bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1) {
		perror("bind error: ");
		exit(1);
	}

	return listenfd;
}

void SpoutWorker::SetClientRoundStart()
{
	for (map<int, Client>::iterator it = clientMap_.begin(); it != clientMap_.end(); ++it) {
		it->second.setSend(false);
	}
}

void SpoutWorker::CheckRound()
{
	bool roundOver = true;
	for (map<int, Client>::iterator it = clientMap_.begin(); it != clientMap_.end(); ++it) {
		if (!it->second.hasSend()) {
			roundOver = false;
			break;
		}
	}

	if (!roundOver) {
		return;
	}
	SetClientRoundStart();
}

void SpoutWorker::SetClientsEvent()
{
	if (PacketContainer::get_mutable_instance().GetSize() <= 0) {
		return;
	}
	for (map<int, Client>::iterator it = clientMap_.begin(); it != clientMap_.end(); ++it) {
		if (!it->second.hasSend()) {
			modify_event(it->first, WORKER_EPOLL);
		}
	}
}

void SpoutWorker::Run()
{
	int ret;
	while(running_) {

		SetClientsEvent();
		ret = epoll_wait(epollfd_, events, EPOLLEVENTS, -1);
		handle_events(events, ret, listenfd_);

		for (map<int, Client>::iterator it = clientMap_.begin();
				it != clientMap_.end() && PacketContainer::get_mutable_instance().GetSize() > 0; ++it) {
			if (it->second.NeedSend()) {
				RawPacket rp = PacketContainer::get_mutable_instance().getPacket();
				do_write(it->first, &rp);
				it->second.setSend(true);
				it->second.setCanSend(false);
				modify_event(it->first, EPOLLIN);
			}
		}

		CheckRound();
	}
}

void SpoutWorker::handle_events(struct epoll_event *events, int num, int listenfd)
{
	int fd;
	for (int i = 0; i < num; i++)
	{
		fd = events[i].data.fd;
		if ((fd == listenfd) && (events[i].events & EPOLLIN)) {
			HandleAccpet(listenfd);
			fprintf(stderr, "accept %d \n", listenfd);
		}

		else if (events[i].events & EPOLLIN) {
			do_read(fd);
		}

		else if (events[i].events & EPOLLOUT){
			HandleWrite(fd);
		}
		else if (events[i].events & EPOLLERR){
			do_error(fd);
			fprintf(stderr, "do_error %d \n", fd);
		}
	}
}

void SpoutWorker::HandleAccpet(int listenfd)
{
	int clifd;
	struct sockaddr_in cliaddr;
	socklen_t  cliaddrlen = 0;
	memset((char*)&cliaddr, 0, sizeof(cliaddr));
	clifd = accept(listenfd,(struct sockaddr*)&cliaddr, &cliaddrlen);
	if (clifd == -1) {
		perror("accpet error:");
	}
	else {
		AddClient(clifd);
	}
}


void SpoutWorker::AddClient(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	add_event(fd, WORKER_EPOLL);

	clientMap_[fd] = Client(fd);
	//printf("accept a new client: %s:%d\n",inet_ntoa(cliaddr.sin_addr),cliaddr.sin_port);
}

void SpoutWorker::RemoveClient(int fd)
{
	clientMap_.erase(fd);
	delete_event(fd, WORKER_EPOLL);
	close(fd);
}

void SpoutWorker::HandleWrite(int fd)
{
	if (clientMap_.count(fd)) {
		clientMap_[fd].setCanSend(true);
	}
}

void SpoutWorker::do_error(int fd)
{
	delete_event(fd, WORKER_EPOLL);
	close(fd);
}

void SpoutWorker::do_read(int fd)
{
	memset(readBuf_, 0, sizeof(readBuf_));
	int nread = read(fd, readBuf_, sizeof(readBuf_));
	if (nread == -1)
	{
		perror("read error:");
		RemoveClient(fd);
	}
	else if (nread == 0)
	{
		fprintf(stderr, "client close.\n");
		RemoveClient(fd);
	}
	else
	{
		//printf("read message is : %s", readBuf_);
		//modify_event(fd, EPOLLOUT);
	}
}

void SpoutWorker::do_write(int fd, RawPacket *packet)
{
	assert(packet);
	char *writeBuf = new char[sizeof(packet->pkthdr_) + packet->pkthdr_.caplen];
	memcpy(writeBuf, (char*)&(packet->pkthdr_), sizeof(packet->pkthdr_));
	memcpy(writeBuf + sizeof(packet->pkthdr_), packet->packet_, packet->pkthdr_.caplen);
	int nwrite = write(fd, writeBuf, sizeof(packet->pkthdr_) + packet->pkthdr_.caplen);
	if (nwrite == -1) {
		perror("write error:");
		RemoveClient(fd);
	}
	delete []writeBuf;
	std::cerr << "do_write " << fd << std::endl;
}

void SpoutWorker::add_event(int fd, int state)
{
	std::cerr << "add_event " << fd << " " << state << std::endl;
	struct epoll_event ev;
	ev.events = state;
	ev.data.fd = fd;
	epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev);
}

void SpoutWorker::delete_event(int fd, int state)
{
	std::cerr << "delete_event " << fd << " " << state << std::endl;
	struct epoll_event ev;
	ev.events = state;
	ev.data.fd = fd;
	epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &ev);
}

void SpoutWorker::modify_event(int fd, int state)
{
	std::cerr << "modify_event " << fd << " " << state << std::endl;
	struct epoll_event ev;
	ev.events = state;
	ev.data.fd = fd;
	epoll_ctl(epollfd_, EPOLL_CTL_MOD,fd,&ev);
}

