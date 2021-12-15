#include <unistd.h>
#include "server/webserver.h"

int main() 
{ 

    WebServer server(
        1314, 3, 60000, false,             /* 端口 ET模式 timeoutMs 优雅退出  */
        3306, "seeker", "seeker", "webserverdb", /* Mysql配置 */
        4, 2, true,0, 1024);             /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */
    //实例化类WebServer对象server,调用类构造函数配置Web服务器端口号、ET触发模式、连接时间、是否优雅退出，
    //配置连接Mysql的端口号、用户名、密码、数据库名
    //配置连接池数量、线程池线程数、日志开关、日志等级、日志异步队列容量
    server.Start();
    //server调用Start()方法启动Web服务器
} 
  
