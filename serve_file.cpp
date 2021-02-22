#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include "get_line.h"
#include "serve_file.h"
#include "ans.h"


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
		
	}
	fclose(resource); //关闭文件句柄
}