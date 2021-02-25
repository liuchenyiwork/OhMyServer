
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include "../include/accept_request.h"
#include "../include/get_line.h"
#include "../include/ans.h"
#include "../include/serve_file.h"
#include "../include/excute_cgi.h"


#define ISspace(x) isspace((int)(x))
/*
 * 处理从套接字上监听到到一个HTTP请求，在这里可以很大一部分地体现服务器处理请求流程
 *
*/
//  处理监听到的 HTTP 请求 
void* accept_request(void* from_client) {
	//这个函数在子线程中运行
	//printf("Now in accept_request\n");
	int client = *(int*)from_client;
	char buf[1024];
	int numchars;
	char method[255];
	char url[255];
	char path[512];
	size_t i, j;
	struct stat st;
	int cgi = 0;
	char* query_string = NULL;

	numchars = get_line(client, buf, sizeof(buf));

	i = 0;
	j = 0;

	while (!ISspace(buf[j]) && (i < sizeof(method) - 1)) {
		//提取其中的请求方式
		method[i] = buf[j];
		i++;
		j++;
	}
	method[i] = '\0';

	if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
		unimplemented(client);
		return NULL;
	}

	if (strcasecmp(method, "POST") == 0)
		cgi = 1;

	i = 0;

	while (ISspace(buf[j]) && (j < sizeof(buf)))
		j++;

	while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))) {
		url[i] = buf[j];
		i++;
		j++;
	}
	url[i] = '\0';

	//GET请求url可能会带有?,有查询参数
	if (strcasecmp(method, "GET") == 0) {

		query_string = url;
		while ((*query_string != '?') && (*query_string != '\0'))
			query_string++;

		/* 如果有?表明是动态请求, 开启cgi */
		if (*query_string == '?') {
			cgi = 1;
			*query_string = '\0';
			query_string++;
		}
	}

	sprintf(path, "httpdocs%s", url);

	if (path[strlen(path) - 1] == '/') {
		strcat(path, "test.html");
	}

	if (stat(path, &st) == -1) {//没有找到对应文件
		while ((numchars > 0) && strcmp("\n", buf))//把client中的剩余缓冲读完，什么都不做
			numchars = get_line(client, buf, sizeof(buf));
		not_found(client);
	}
	else//找到了对应文件
	{

		if ((st.st_mode & S_IFMT) == S_IFDIR) 		{//如果请求参数为目录, 自动打开test.htmlS_IFDIR代表目录
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