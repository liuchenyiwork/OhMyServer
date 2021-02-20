#include <stdio.h>
#include <iostream>
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
#include <sys/epoll.h>
#include <fcntl.h>
#include <assert.h>
using namespace std;
#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: Chenyi's http/0.1.0\r\n" //定义个人server名称


#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 10

void *accept_request(void *client);//接收请求
void bad_request(int);//未分配
void cat(int, FILE *);//读取服务器上某个文件写到socket
void cannot_execute(int);//处理执行cgi程序时出现的错误
void error_die(const char *);//输出错误信息到perror并退出
void execute_cgi(int, const char *, const char *, const char *);//执行cgi程序
int get_line(int, char *, int);//解析HTML的一行
void headers(int, const char *);//把HTTP响应都头部写到套接字
void not_found(int);//404 no found
void serve_file(int, const char *);//如果是静态文件则直接读取并返回
int startup(u_short *);//开始监听
void unimplemented(int);//发消息说对应方法没有实现
void epollrun(epoll_event* , int , int , int );//epoll捕获到io后执行的操作

/**********************************************************************/
/* 
 * 处理从套接字上监听到到一个HTTP请求，在这里可以很大一部分地体现服务器处理请求流程
 * */
/**********************************************************************/
//  处理监听到的 HTTP 请求 
void *accept_request(void* from_client)
{
	//这个函数在子线程中运行
	//printf("Now in accept_request\n");
	int client = *(int *)from_client;
	char buf[1024];
	int numchars;
	char method[255];
	char url[255];
	char path[512];
	size_t i, j;
	struct stat st;
	int cgi = 0;
	char *query_string = NULL;

	numchars = get_line(client, buf, sizeof(buf));

	i = 0;
	j = 0;

	while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
	{
		//提取其中的请求方式
		method[i] = buf[j];
		i++;
		j++;
	}
	method[i] = '\0';

	if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
	{
		unimplemented(client);
		return NULL;
	}

	if (strcasecmp(method, "POST") == 0)
		cgi = 1;

	i = 0;

	while (ISspace(buf[j]) && (j < sizeof(buf)))
		j++;

	while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
	{
		url[i] = buf[j];
		i++;
		j++;
	}
	url[i] = '\0';

	//GET请求url可能会带有?,有查询参数
	if (strcasecmp(method, "GET") == 0)
	{

		query_string = url;
		while ((*query_string != '?') && (*query_string != '\0'))
			query_string++;

		/* 如果有?表明是动态请求, 开启cgi */
		if (*query_string == '?')
		{
			cgi = 1;
			*query_string = '\0';
			query_string++;
		}
	}

	sprintf(path, "httpdocs%s", url);

	if (path[strlen(path) - 1] == '/')
	{
		strcat(path, "test.html");
	}

	if (stat(path, &st) == -1)//没有找到对应文件
	{
		while ((numchars > 0) && strcmp("\n", buf))//把client中的剩余缓冲读完，什么都不做
			numchars = get_line(client, buf, sizeof(buf));
		not_found(client);
	}
	else//找到了对应文件
	{

		if ((st.st_mode & S_IFMT) == S_IFDIR) //S_IFDIR代表目录
											  //如果请求参数为目录, 自动打开test.html
		{
			strcat(path, "/test.html");
		}

		//是一个可执行文件
		if ((st.st_mode & S_IXUSR) ||
			(st.st_mode & S_IXGRP) ||
			(st.st_mode & S_IXOTH))
			//S_IXUSR:文件所有者具可执行权限
			//S_IXGRP:用户组具可执行权限
			//S_IXOTH:其他用户具可读取权限
			cgi = 1;

		if (!cgi)//不是CGI程序，简单第发送文件
			serve_file(client, path);
		else//是CGI程序，执行CGI程序
			execute_cgi(client, path, method, query_string);
	}

	close(client);
	//printf("connection close....client: %d \n",client);
	return NULL;
}

/**********************************************************************/
/*  
 * 返回给客户端这是个错误请求，HTTP状态码 400 BAD REQUEST.
 * */
/**********************************************************************/
void bad_request(int client)
{
	char buf[1024];
	//发送400
	sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "Content-type: text/html\r\n");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "<P>Your browser sent a bad request, ");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "such as a POST without a Content-Length.\r\n");
	send(client, buf, sizeof(buf), 0);
}

/**********************************************************************/
/* 
 * 读取服务器上某个文件写到socket套接字
 * 
 * */

void cat(int client, FILE *resource)
{
	//发送文件的内容
	char buf[1024];
	fgets(buf, sizeof(buf), resource);
	while (!feof(resource))
	{

		send(client, buf, strlen(buf), 0);
		fgets(buf, sizeof(buf), resource);
	}
}


/* 
 * 主要处理发生在执行cgi程序时出现的错误
 * */

void cannot_execute(int client)
{
	char buf[1024];
	//发送500
	sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
	send(client, buf, strlen(buf), 0);
}


/* 
 * 把错误信息写到perror并退出
 * */

void error_die(const char *sc)
{
	perror(sc);
	exit(1);
}


/* 
 * 运行cgi程序的处理，也是个主要函数
 * */

//执行cgi动态解析
void execute_cgi(int client, const char *path,
				 const char *method, const char *query_string)
{
	printf("Now in execute_cgi\n");
	char buf[1024];
	int cgi_output[2];
	int cgi_input[2];

	pid_t pid;
	int status;

	int i;
	char c;

	int numchars = 1;
	int content_length = -1;
	//默认字符
	buf[0] = 'A';
	buf[1] = '\0';
	if (strcasecmp(method, "GET") == 0)
	{
		while ((numchars > 0) && strcmp("\n", buf))
		{
			numchars = get_line(client, buf, sizeof(buf));
		}
	}
	else
	{

		numchars = get_line(client, buf, sizeof(buf));
		while ((numchars > 0) && strcmp("\n", buf))
		{
			buf[15] = '\0';
			if (strcasecmp(buf, "Content-Length:") == 0)
				content_length = atoi(&(buf[16]));

			numchars = get_line(client, buf, sizeof(buf));
		}

		if (content_length == -1)
		{
			bad_request(client);
			return;
		}
	}

	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	send(client, buf, strlen(buf), 0);
	if (pipe(cgi_output) < 0)
	{
		cannot_execute(client);
		return;
	}
	if (pipe(cgi_input) < 0)
	{
		cannot_execute(client);
		return;
	}

	if ((pid = fork()) < 0)
	{
		cannot_execute(client);
		return;
	}
	if (pid == 0) /* 子进程: 运行CGI 脚本 */
	{
		char meth_env[255];
		char query_env[255];
		char length_env[255];

		dup2(cgi_output[1], 1);
		dup2(cgi_input[0], 0);

		close(cgi_output[0]); //关闭了cgi_output中的读通道
		close(cgi_input[1]);  //关闭了cgi_input中的写通道

		sprintf(meth_env, "REQUEST_METHOD=%s", method);
		putenv(meth_env);

		if (strcasecmp(method, "GET") == 0)
		{
			//存储QUERY_STRING
			sprintf(query_env, "QUERY_STRING=%s", query_string);
			putenv(query_env);
		}
		else
		{	/* POST */
			//存储CONTENT_LENGTH
			sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
			putenv(length_env);
		}

		execl(path, path, NULL); //执行CGI脚本
		exit(0);
	}
	else
	{
		close(cgi_output[1]);
		close(cgi_input[0]);
		if (strcasecmp(method, "POST") == 0)

			for (i = 0; i < content_length; i++)
			{

				recv(client, &c, 1, 0);

				write(cgi_input[1], &c, 1);
			}

		//读取cgi脚本返回数据

		while (read(cgi_output[0], &c, 1) > 0)
		//发送给浏览器
		{
			send(client, &c, 1, 0);
		}

		//运行结束关闭
		close(cgi_output[0]);
		close(cgi_input[1]);

		waitpid(pid, &status, 0);
	}
}


/* 
 * 读取套接字的一行，把回撤换行等情况都统一为换行符结束
 * */

//解析一行http报文
int get_line(int sock, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;

	while ((i < size - 1) && (c != '\n'))
	{
		n = recv(sock, &c, 1, 0);

		if (n > 0)
		{
			if (c == '\r')
			{

				n = recv(sock, &c, 1, MSG_PEEK);
				if ((n > 0) && (c == '\n'))
					recv(sock, &c, 1, 0);
				else
					c = '\n';
			}
			buf[i] = c;
			i++;
		}
		else
			c = '\n';
	}
	buf[i] = '\0';
	return (i);
}


/* 
 * 把HTTP响应都头部写到套接字
 * */

void headers(int client, const char *filename)
{

	char buf[1024];

	(void)filename; /* could use filename to determine file type */
					//发送HTTP头
	strcpy(buf, "HTTP/1.0 200 OK\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, SERVER_STRING);//SERVER_STRING定义在开头
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
}


/* 
 * 主要处理找不到请求到文件时的情况
*/

void not_found(int client)
{
	char buf[1024];
	//返回404
	sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, SERVER_STRING);//SERVER_STRING定义在开头
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "your request because the resource specified\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "is unavailable or nonexistent.\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}

/* 
 * 调用cat把服务器文件返回给浏览器
 */


//如果不是CGI文件，也就是静态文件，直接读取文件返回给请求的http客户端
void serve_file(int client, const char *filename)
{
	FILE *resource = NULL;
	int numchars = 1;
	char buf[1024];
	buf[0] = 'A';
	buf[1] = '\0';
	while ((numchars > 0) && strcmp("\n", buf))
	{
		numchars = get_line(client, buf, sizeof(buf)); /* read & discard headers */
	}

	//打开文件
	resource = fopen(filename, "r");
	if (resource == NULL)
		not_found(client);
	else
	{
		headers(client, filename);
		cat(client, resource);
		cout<<"serve file : "<<filename<<endl;
	}
	fclose(resource); //关闭文件句柄
}

/* 
 * 初始化httpd服务，包括建立套接字，绑定端口，进行监听
 * */

//启动服务端
int startup(u_short *port)
{

	int httpd = 0, option;
	struct sockaddr_in name;
	//设置http socket
	httpd = socket(PF_INET, SOCK_STREAM, 0); //创建socket
	if (httpd == -1)
		error_die("socket"); //连接失败

	socklen_t optlen;
	optlen = sizeof(option);
	option = 1;
	setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, (void *)&option, optlen); //设置可以使用TIMEWAIT中的连接

	memset(&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_port = htons(*port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0) //绑定(命名)socket
		error_die("bind");										 //绑定失败
	if (*port == 0)												 /*动态分配一个端口 */
	{
		socklen_t namelen = sizeof(name);
		if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
			error_die("getsockname");
		*port = ntohs(name.sin_port);
	}

	if (listen(httpd, 5) < 0) //监听socket
		error_die("listen");
	return (httpd); //返回这个监听socket
}



//将文件描述符设置为非阻塞的
int setnonblocking( int fd )
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}

//将文件描述符fd上的EPOLLIN注册到epolldf指示的epoll内核事件表中，参数enable_et指定是否对df启用et模式
void addfd( int epollfd, int fd, bool enable_et )
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN; //监听数据可读事件
    if( enable_et )//设置ET模式
    {
        event.events |= EPOLLET;
    }
    //epollfd：epoll事件表
    //EPOLL_CTL_ADD：操作模式
    //event：描述事件类型。
    epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );//添加fd及其对应需要监听的事件到注册表中
    setnonblocking( fd );//将文件描述符设置为非阻塞
}

void unimplemented(int client)
{
	char buf[1024];
	//发送501说明相应方法没有实现
	sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, SERVER_STRING);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</TITLE></HEAD>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}

void epollrun(epoll_event* events, int number, int epollfd, int server_sock){

	//epoll code here
	for(int i=0;i<number;i++){//遍历每一个事件
		int sockfd=events[i].data.fd;
		if(sockfd==server_sock){//是监听事件
			cout<<"server_sock is "<<server_sock<<endl;
			int client_sock = -1;
			struct sockaddr_in client_name;
			socklen_t client_name_len = sizeof(client_name);
			pthread_t newthread;
			client_sock = accept(server_sock,
							 (struct sockaddr *)&client_name,
							 &client_name_len); //阻塞地使用accept接收一个连接
		

			//printf("accept new connection....  ip: %s , port: %d-------------------\n", inet_ntoa(client_name.sin_addr), ntohs(client_name.sin_port));
			cout<<"accept new connection... ip:"<<inet_ntoa(client_name.sin_addr)<<", port: "<<ntohs(client_name.sin_port)<<endl;
			if (client_sock == -1)
				error_die("accept");
			//accept_request(&client_sock);
			//创建一个子线程来处理连接，处理连接使用的是accept_request函数。(newthread是新线程的标识符)
			if (pthread_create(&newthread, NULL, accept_request, (void *)&client_sock) != 0)
				perror("pthread_create");

			addfd(epollfd,server_sock,false);//false代表lt模式
		}
	}
}


int main(void)
{
	int server_sock = -1;
	u_short port = 4396; //默认监听端口号 port 为4396

	server_sock = startup(&port);//server_sock等同于listenfd
	printf("http server_sock is %d\n", server_sock);
	printf("http running on port %d\n", port);

	//code of epoll
	epoll_event events[MAX_EVENT_NUMBER];//创建epoll并指定epoll事件表大小
	int epollfd=epoll_create(5);//
	assert( epollfd != -1 );
	addfd( epollfd, server_sock, false );//把listenfd注册以ET(true)方式注册到内核事件表epollfd中

	while (1)
	{
		//code if epoll
		cout<<"----------------"<<endl;
		cout<<"epoll_wait is ready..."<<endl;
		cout<<"----------------"<<endl;
		int ret=epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
		if(ret<0){
			printf("epoll failure\n");
			break;
		}
		cout<<"epoll ok, events numbers:"<<ret<<endl;
		epollrun(events,ret,epollfd,server_sock);//使用epoll处理监听到的事件 
	}
	close(server_sock);
	return (0);
}
