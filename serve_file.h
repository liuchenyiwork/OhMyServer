
#ifndef __SERVE_FILE_H
#define __SERVE_FILE_H

void cat(int, FILE *);//读取服务器上某个文件写到socket
void serve_file(int, const char *);//如果是静态文件则直接读取并返回

#endif