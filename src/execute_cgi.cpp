#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../include/excute_cgi.h"
#include "../include/ans.h"
#include "../include/get_line.h"


/*
 * 运行cgi程序的处理，也是个主要函数
 * */

 //执行cgi动态解析
void execute_cgi(int client, const char* path,
	const char* method, const char* query_string) {
	printf("Now in execute_cgi\n");
	char buf[1024];
	int cgi_output[2];	//该管道，cgi的标准输出定向到该管道的cgi_output[1]，cgi从cgi_output[1]写，父进程则从cgi_output[0]读,
	int cgi_input[2];	//该管道，cgi的标准输入定向到，cgi_input[0],即父进程则从cgi_input[1]写，cgi从cgi_input[0]读

	pid_t pid;
	int status;

	int i;
	char c;

	int numchars = 1;
	int content_length = -1;
	//默认字符
	buf[0] = 'A';
	buf[1] = '\0';
	if (strcasecmp(method, "GET") == 0) {
		while ((numchars > 0) && strcmp("\n", buf)) {
			numchars = get_line(client, buf, sizeof(buf));
		}
	}
	else {

		numchars = get_line(client, buf, sizeof(buf));
		while ((numchars > 0) && strcmp("\n", buf)) {
			buf[15] = '\0';
			if (strcasecmp(buf, "Content-Length:") == 0)
				content_length = atoi(&(buf[16]));

			numchars = get_line(client, buf, sizeof(buf));
		}

		if (content_length == -1) {
			bad_request(client);
			return;
		}
	}

	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	send(client, buf, strlen(buf), 0);
	if (pipe(cgi_output) < 0) {
		cannot_execute(client);
		return;
	}
	if (pipe(cgi_input) < 0) {
		cannot_execute(client);
		return;
	}

	if ((pid = fork()) < 0) {//fork一个子进程
		cannot_execute(client);
		return;
	}
	if (pid == 0) /* 子进程: 运行CGI 脚本 */	//子进程中，pid为0
	{
		char meth_env[255];
		char query_env[255];
		char length_env[255];

		dup2(cgi_output[1], 1);		//把用于CGI输出的管道的输出，重定向到标准输出。即cgi直接从标准输出进行写，然后会定向到cgi_out_put[1]，父进程，则从cgi_output[0]读
		dup2(cgi_input[0], 0);		//把用于CGI程序输入的管道的输入，重新定向到标准输入。(即，父进程往cgi_input[1]写，cgi_input[0]被定向到标准输入，所以cgi直接从标准输入读取就可以了)

		close(cgi_output[0]); //关闭了cgi_output中的读通道
		close(cgi_input[1]);  //关闭了cgi_input中的写通道

		sprintf(meth_env, "REQUEST_METHOD=%s", method);
		putenv(meth_env);

		if (strcasecmp(method, "GET") == 0) {
			//存储QUERY_STRING
			sprintf(query_env, "QUERY_STRING=%s", query_string);
			putenv(query_env);
		}
		else {	/* POST */
			//存储CONTENT_LENGTH
			sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
			putenv(length_env);
		}

		execl(path, path, NULL); //执行CGI脚本
		exit(0);
	}
	else {			//父进程中pid是子进程的pid
		close(cgi_output[1]);
		close(cgi_input[0]);
		if (strcasecmp(method, "POST") == 0)

			for (i = 0; i < content_length; i++) {

				recv(client, &c, 1, 0);

				write(cgi_input[1], &c, 1);		//写入到管道cgi_input的输入口1，cgi程序从cgi_input[0]进行读取数据
			}

		//读取cgi脚本返回数据

		while (read(cgi_output[0], &c, 1) > 0)//cgi程序往cgi_output[1]写入数据，父进程则直接从cgi_put[0]读数据
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