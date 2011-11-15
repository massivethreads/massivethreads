#include <assert.h>

#include "myth_desc.h"
#include "myth_worker.h"
#include "myth_io.h"

#include "myth_io_proto.h"

#include "myth_worker_func.h"
#include "myth_io_func.h"

myth_fd_map_t g_fd_map;

int g_recvfrom_cnt,g_sendto_cnt;
int g_n_recvfrom_cnt,g_n_sendto_cnt;

#ifdef MYTH_WRAP_IO

int socket (int __domain, int __type, int __protocol)
{
	return myth_socket_body(__domain,__type,__protocol);
}
int connect (int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len)
{
	return myth_connect_body(__fd,__addr,__len);
}
int accept (int __fd, __SOCKADDR_ARG __addr,
		   socklen_t *__restrict __addr_len)
{
	return myth_accept_body(__fd,__addr,__addr_len);
}
int listen (int __fd, int __n)
{
	return myth_listen_body(__fd,__n);
}
int bind(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len)
{
	return myth_bind_body(__fd,__addr,__len);
}
int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout)
{
	return myth_select_body(nfds,readfds,writefds,exceptfds,timeout);
}
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen)
{
	return myth_sendto_body(sockfd,buf,len,flags,dest_addr,addrlen);
}
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen)
{
	return myth_recvfrom_body(sockfd,buf,len,flags,src_addr,addrlen);
}
ssize_t send (int __fd, __const void *__buf, size_t __n, int __flags)
{
	return myth_send_body(__fd,__buf,__n,__flags);
}
ssize_t recv (int __fd, void *__buf, size_t __n, int __flags)
{
	return myth_recv_body(__fd,__buf,__n,__flags);
}
int close (int __fd)
{
	return myth_close_body(__fd);
}
int fcntl (int __fd, int __cmd, ...)
{
	int ret;
	va_list vl;
	va_start(vl,__cmd);
	ret=myth_fcntl_body(__fd,__cmd,vl);
	va_end(vl);
	return ret;
}

#endif
