#include "sysutil.h"

//在该模块中完成对socket的初始化
//创建监听套接字
int tcp_server(const char* ip, uint16_t port)
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listen_fd < 0)
        ERR_EXIT("socket error~~\n");

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    //设置套接字属性
    int op = 1;
    //int setsockopt(int socket, int level, int option_name,
    //               const void *option_value, socklen_t option_len);              
    //SOL_SOCKET 基本套接字
    //SO_REFUSEADDR 允许重用本地地址和端口
    int ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op));
    if (ret < 0)
        ERR_EXIT("setsockopt error~~~\n");

    //绑定服务端地址信息
    ret = bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0)
        ERR_EXIT("bind error~~~\n");

    //开始监听
    ret = listen(listen_fd, SOMAXCONN);
    if (ret < 0)
        ERR_EXIT("listen error~~~\n");

    //返回监听套接字的描述符
    return listen_fd;
}

int tcp_client(int port)
{
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        ERR_EXIT("socker error~~\n");
    
    if (port > 0)
    {
        //nobody进程辅助绑定端口
        int on = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
            ERR_EXIT("setsockopt error~~~\n");

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        if(bind(fd, (struct sockaddr*)&address, sizeof(struct sockaddr)) < 0)
            ERR_EXIT("bind 20");
    }
    return fd;
}
const char* statbuf_get_purview(struct stat* sbuf)
{
    // - --- --- ---
    static char purview[] = "----------";
    mode_t mode = sbuf->st_mode;

    switch(mode & S_IFMT)
    {
        case S_IFSOCK:
            purview[0] = 's';
            break;
        case S_IFLNK:
            purview[0] = 'l';//链接文件
            break;
        case S_IFREG:
            purview[0] = '-';//普通文件
            break;
        case S_IFBLK:
            purview[0] = 'b';
            break;
        case S_IFDIR:
            purview[0] = 'd';//目录文件
            break;
        case S_IFCHR:
            purview[0] = 'c';//字符文件
            break;
        case S_IFIFO:
            purview[0] = 'p';//管道文件
            break;
    }
    if (mode & S_IRUSR)
        purview[1] = 'r';
    if (mode & S_IWUSR)
        purview[2] = 'w';
    if (mode & S_IXUSR)
        purview[3] = 'x';

    if (mode & S_IRGRP)
        purview[4] = 'r';
    if (mode & S_IWGRP)
        purview[5] = 'w';
    if (mode & S_IXGRP)
        purview[6] = 'x';

    if (mode & S_IROTH)
        purview[7] = 'r';
    if (mode & S_IWOTH)
        purview[8] = 'w';
    if (mode & S_IXOTH)
        purview[9] = 'x';

    return purview;
}

const char* statbuf_get_date(struct stat* sbuf)
{
    static char datebuf[64] = {0};
    time_t time_file = sbuf->st_mtime;//获取文件最后一次修改时间
    struct tm* ptm = localtime(&time_file);//将时间格式化,或者说格式化一个时间字符串

    //%b月份的简写 %e十进制表示每月的第几天
    // %H 24小时制的小时  %M 十进制表示的分钟数
    strftime(datebuf, 64, "%b %e %H:%M", ptm);
    return datebuf;
}

//发送文件描述符
void send_fd(int sock_fd, int fd)
{
    int ret;
    struct msghdr msg;
    struct cmsghdr *p_cmsg;
    struct iovec vec;
    char cmsgbuf[CMSG_SPACE(sizeof(fd))];
    int *p_fds;
    char sendchar = 0;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);
    p_cmsg = CMSG_FIRSTHDR(&msg);
    p_cmsg->cmsg_level = SOL_SOCKET;
    p_cmsg->cmsg_type = SCM_RIGHTS;
    p_cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
    p_fds = (int*)CMSG_DATA(p_cmsg);
    *p_fds = fd;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;

    vec.iov_base = &sendchar;
    vec.iov_len = sizeof(sendchar);
    ret = sendmsg(sock_fd, &msg, 0);
    if (ret != 1)
        ERR_EXIT("sendmsg");
}

//接收文件描述符
int recv_fd(const int sock_fd)
{
    int ret;
    struct msghdr msg;
    char recvchar;
    struct iovec vec;
    int recv_fd;
    char cmsgbuf[CMSG_SPACE(sizeof(recv_fd))];
    struct cmsghdr *p_cmsg;
    int *p_fd;
    vec.iov_base = &recvchar;
    vec.iov_len = sizeof(recvchar);
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);
    msg.msg_flags = 0;

    p_fd = (int*)CMSG_DATA(CMSG_FIRSTHDR(&msg));
    *p_fd = -1;  
    ret = recvmsg(sock_fd, &msg, 0);
    if (ret != 1)
        ERR_EXIT("recvmsg");

    p_cmsg = CMSG_FIRSTHDR(&msg);
    if (p_cmsg == NULL)
        ERR_EXIT("no passed fd");


    p_fd = (int*)CMSG_DATA(p_cmsg);
    recv_fd = *p_fd;
    if (recv_fd == -1)
        ERR_EXIT("no passed fd");

    return recv_fd;
}

void getLocalip(char* ip)
{
    char host[MAX_HOST_NAME_SIZE] = {0};
    if(gethostname(host, sizeof(host)) < 0)
        ERR_EXIT("getLocalip error~~\n");
    printf("host name = %s\n", host);
    
    struct hostent* ph;
    if ((ph = gethostbyname(host)) == NULL)
    {
        ERR_EXIT("gethostbyname error~~\n");
    }

    strcpy(ip, inet_ntoa(*(struct in_addr*)ph->h_addr));
}


static struct timeval s_cur_time;
long get_time_sec()
{
    if(gettimeofday(&s_cur_time, NULL) < 0)
        ERR_EXIT("get_time_sec error~~\n");

    return s_cur_time.tv_sec;
}
long get_time_usec()
{
    return s_cur_time.tv_usec;
}

void nano_sleep(double sleep_time)
{
    time_t sec = (time_t)sleep_time;
    double decimal = sleep_time - (double)sec;

    struct timespec ts;
    ts.tv_sec = sec;
    ts.tv_nsec = (long)decimal * 1000000000;//纳秒

    int ret;
    do
    {
        ret = nanosleep(&ts, &ts);
    }while (ret == -1 && errno == EINTR); //循环用于预防休眠被信号中断
}





