#ifndef __ANS_H
#define __ANS_H

#define SERVER_STRING "Server: Chenyi's http/0.1.0\r\n" //定义个人server名称

void headers(int, const char *);//把HTTP响应都头部写到套接字
void bad_request(int);//未分配
void unimplemented(int);//发消息说对应方法没有实现
void not_found(int);//404 no found
void cannot_execute(int);//处理执行cgi程序时出现的错误

#endif