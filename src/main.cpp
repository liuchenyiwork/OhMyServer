#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "accept_request.h"
#include "execute_cgi.h"
#include "serve_file.h"
#include "startup.h"
#include "get_line.h"
#include "error_die.h"
#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: Chenyi's http/0.1.0\r\n" //定义个人server名称

using namespace std;

int main(int argc, char *argv[])
{
	u_short port = 0;
	if (argc <= 1)
	{ //无参数，端口号使用默认的4396
		cout << "using default port : 4396..." << endl;
		port = 4396;
	}
	else
	{
		port = atoi(argv[1]); //获取监听的端口号
	}

	int server_sock = -1;
	int client_sock = -1;
	struct sockaddr_in client_name;
	socklen_t client_name_len = sizeof(client_name);
	pthread_t newthread;
	server_sock = startup(&port);

	printf("http server_sock is %d\n", server_sock);
	printf("http running on port %d\n", port);
	while (1)
	{

		client_sock = accept(server_sock,
							 (struct sockaddr *)&client_name,
							 &client_name_len); //阻塞地使用accept接收一个连接

		printf("New connection....  ip: %s , port: %d\n", inet_ntoa(client_name.sin_addr), ntohs(client_name.sin_port));
		if (client_sock == -1)
			error_die("accept");
		//accept_request(&client_sock);
		//创建一个子线程来处理连接，处理连接使用的是accept_request函数。(newthread是新线程的标识符)
		if (pthread_create(&newthread, NULL, accept_request, (void *)&client_sock) != 0)
			perror("pthread_create");
	}
	close(server_sock);

	return (0);

	return 0;
}
