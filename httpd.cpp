#include <stdio.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <assert.h>
#include "include/accept_request.h"
#include "include/error_die.h"
#include "include/startup.h"
#include "include/epoll_helper.h"

using namespace std;

#define MAX_EVENT_NUMBER 1024
//#define BUFFER_SIZE 10
static unsigned long long times = 0;
int main(void) {
	int listenfd = -1;
	u_short port = 4396; //默认监听端口号 port 为4396

	int connfd = -1;
	struct sockaddr_in client_name;
	socklen_t client_name_len = sizeof(client_name);
	pthread_t newthread;

	//开启监听服务器，并且返回对应的文件描述符
	listenfd = startup(&port);//server_sock等同于listenfd
	printf("http server_sock is %d\n", listenfd);
	printf("http running on port %d\n", port);

	//code of epoll
	epoll_event events[MAX_EVENT_NUMBER];//创建epoll并指定epoll事件表大小
	int epollfd = epoll_create(5);//
	assert(epollfd != -1);
	addfd(epollfd, listenfd, false);//把listenfd注册以ET(true)方式注册到内核事件表epollfd中

	while (1) {
		//code for epoll
		cout << "----------------" << endl;

		cout << "epoll_wait is ready..." << endl;

		int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);

		if (ret < 0) {
			cout << "epoll failure" << endl;
			break;
		}

		//epoll choose
		for (int i = 0;i < ret;i++) {
			int eventfd = events[i].data.fd;
			if (eventfd == listenfd) {//是tcp监听事件
				cout << "第" << times++ << "次接收连接" << endl;
				connfd = accept(listenfd, (struct sockaddr*)&client_name, &client_name_len);
				if (connfd == -1)
					error_die("accept");
				if (pthread_create(&newthread, NULL, accept_request, (void*)&connfd) != 0)
					perror("pthread_create");
			}
		}
	}
	close(listenfd);
	return (0);
}
