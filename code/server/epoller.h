
#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>
class Epoller {
public:
    explicit Epoller(int maxEvent = 1024);

    ~Epoller();

    bool AddFd(int fd, uint32_t events);

    bool ModFd(int fd, uint32_t events);

    bool DelFd(int fd);

    int Wait(int timeoutMs = -1);

    int GetEventFd(size_t i) const;

    uint32_t GetEvents(size_t i) const;
        
private:
    int epollFd_;

    std::vector<struct epoll_event> events_;    
};

#endif //EPOLLER_H

/*
epoll维护了一个全局的事件集合，调用epoll_ctl()通过epoll句柄添加、修改、删除事件集合中的某个元素。
*/