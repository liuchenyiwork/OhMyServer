#include <stdio.h>
void serve_file(int, const char *);//如果是静态文件则直接读取并返回
void cat(int, FILE *);//读取服务器上某个文件写到socket
void headers(int, const char *);//把HTTP响应都头部写到套接字