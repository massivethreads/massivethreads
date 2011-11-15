#ifndef MYTH_IO_PROTO_H_
#define MYTH_IO_PROTO_H_

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

static void myth_io_init(void);
static void myth_io_fini(void);
static void myth_io_worker_init(myth_running_env_t env,myth_io_struct_perenv_t io);
static void myth_io_worker_fini(myth_running_env_t env,myth_io_struct_perenv_t io);

static inline void myth_wait_for_read(int fd,myth_running_env_t env,myth_io_op_t op);
static inline void myth_wait_for_write(int fd,myth_running_env_t env,myth_io_op_t op);

static inline int myth_socket_body (int __domain, int __type, int __protocol);
static inline int myth_connect_body (int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len);
static inline int myth_accept_body (int __fd, __SOCKADDR_ARG __addr,
		   socklen_t *__restrict __addr_len);
static inline int myth_bind_body(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len);
static inline int myth_listen_body (int __fd, int __n);
static inline ssize_t myth_send_body (int __fd, __const void *__buf, size_t __n, int __flags);
static inline ssize_t myth_recv_body (int __fd, void *__buf, size_t __n, int __flags);
static inline int myth_close_body (int __fd);
static inline int myth_fcntl_body (int __fd, int __cmd,va_list vl);
static inline myth_thread_t myth_io_polling(struct myth_running_env *env);
static inline int myth_io_execute(myth_io_op_t op);

#endif /* MYTH_IO_PROTO_H_ */
