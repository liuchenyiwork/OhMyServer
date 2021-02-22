#include <fcntl.h>
#include <sys/epoll.h>
#include "epoll_helper.h"

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