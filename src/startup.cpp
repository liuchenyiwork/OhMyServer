
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "../include/startup.h"
#include "../include/error_die.h"

/*
 * 初始化httpd服务，包括建立套接字，绑定端口，进行监听
 * */
int startup(u_short* port) {

	int httpd = 0, option;
	struct sockaddr_in name;
	//设置http socket
	httpd = socket(PF_INET, SOCK_STREAM, 0); //创建socket
	if (httpd == -1)
		error_die("socket"); //连接失败

	socklen_t optlen;
	optlen = sizeof(option);
	option = 1;
	setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, (void*)&option, optlen); //设置可以使用TIMEWAIT中的连接

	memset(&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_port = htons(*port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(httpd, (struct sockaddr*)&name, sizeof(name)) < 0) //绑定(命名)socket
		error_die("bind");										 //绑定失败
	if (*port == 0)												 /*动态分配一个端口 */
	{
		socklen_t namelen = sizeof(name);
		if (getsockname(httpd, (struct sockaddr*)&name, &namelen) == -1)
			error_die("getsockname");
		*port = ntohs(name.sin_port);
	}

	if (listen(httpd, 5) < 0) //监听socket
		error_die("listen");
	return (httpd); //返回这个监听socket
}