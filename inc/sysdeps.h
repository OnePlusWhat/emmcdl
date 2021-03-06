/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* this file contains system-dependent definitions used by EMMCDL
 * they're related to threads, sockets and file descriptors
 */
#ifndef _EMMCDL_SYSDEPS_H
#define _EMMCDL_SYSDEPS_H

#ifdef __CYGWIN__
#  undef _WIN32
#endif

/*
 * TEMP_FAILURE_RETRY is defined by some, but not all, versions of
 * <unistd.h>. (Alas, it is not as standard as we'd hoped!) So, if it's
 * not already defined, then define it here.
 */
#ifndef TEMP_FAILURE_RETRY
/* Used to retry syscalls that can return EINTR. */
#define TEMP_FAILURE_RETRY(exp) ({         \
    typeof (exp) _rc;                      \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })
#endif

#ifdef _WIN32

#include <ctype.h>
#include <direct.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <sys/stat.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include "fdevent.h"

#define OS_PATH_SEPARATOR '\\'
#define OS_PATH_SEPARATOR_STR "\\"
#define ENV_PATH_SEPARATOR_STR ";"

typedef CRITICAL_SECTION          emmcdl_mutex_t;

#define  EMMCDL_MUTEX_DEFINE(x)     emmcdl_mutex_t   x

/* declare all mutexes */
/* For win32, emmcdl_sysdeps_init() will do the mutex runtime initialization. */
#define  EMMCDL_MUTEX(x)   extern emmcdl_mutex_t  x;
#include "mutex_list.h"

extern void  emmcdl_sysdeps_init(void);

static __inline__ void emmcdl_mutex_lock( emmcdl_mutex_t*  lock )
{
    EnterCriticalSection( lock );
}

static __inline__ void  emmcdl_mutex_unlock( emmcdl_mutex_t*  lock )
{
    LeaveCriticalSection( lock );
}

typedef  void*  (*emmcdl_thread_func_t)(void*  arg);

typedef  void (*win_thread_func_t)(void*  arg);

static __inline__ bool emmcdl_thread_create(emmcdl_thread_func_t func, void* arg) {
    uintptr_t tid = _beginthread((win_thread_func_t)func, 0, arg);
    return (tid != static_cast<uintptr_t>(-1L));
}

static __inline__  unsigned long emmcdl_thread_id()
{
    return GetCurrentThreadId();
}

static __inline__ void  close_on_exec(int  fd)
{
    /* nothing really */
}

#define  lstat    stat   /* no symlinks on Win32 */

#define  S_ISLNK(m)   0   /* no symlinks on Win32 */

static __inline__  int    emmcdl_unlink(const char*  path)
{
    int  rc = unlink(path);

    if (rc == -1 && errno == EACCES) {
        /* unlink returns EACCES when the file is read-only, so we first */
        /* try to make it writable, then unlink again...                  */
        rc = chmod(path, _S_IREAD|_S_IWRITE );
        if (rc == 0)
            rc = unlink(path);
    }
    return rc;
}
#undef  unlink
#define unlink  ___xxx_unlink

static __inline__ int  emmcdl_mkdir(const char*  path, int mode)
{
	return _mkdir(path);
}
#undef   mkdir
#define  mkdir  ___xxx_mkdir

extern int  emmcdl_open(const char*  path, int  options);
extern int  emmcdl_creat(const char*  path, int  mode);
extern int  emmcdl_read(int  fd, void* buf, int len);
extern int  emmcdl_write(int  fd, const void*  buf, int  len);
extern int  emmcdl_lseek(int  fd, int  pos, int  where);
extern int  emmcdl_shutdown(int  fd);
extern int  emmcdl_close(int  fd);

static __inline__ int  unix_close(int fd)
{
    return close(fd);
}
#undef   close
#define  close   ____xxx_close

extern int  unix_read(int  fd, void*  buf, size_t  len);

#undef   read
#define  read  ___xxx_read

static __inline__  int  unix_write(int  fd, const void*  buf, size_t  len)
{
    return write(fd, buf, len);
}
#undef   write
#define  write  ___xxx_write

static __inline__ int  emmcdl_open_mode(const char* path, int options, int mode)
{
    return emmcdl_open(path, options);
}

static __inline__ int  unix_open(const char*  path, int options,...)
{
    if ((options & O_CREAT) == 0)
    {
        return  open(path, options);
    }
    else
    {
        int      mode;
        va_list  args;
        va_start( args, options );
        mode = va_arg( args, int );
        va_end( args );
        return open(path, options, mode);
    }
}
#define  open    ___xxx_unix_open


/* normally provided by <cutils/misc.h> */
extern void*  load_file(const char*  pathname, unsigned*  psize);

/* normally provided by <cutils/sockets.h> */
extern int socket_loopback_client(int port, int type);
extern int socket_network_client(const char *host, int port, int type);
extern int socket_network_client_timeout(const char *host, int port, int type,
                                         int timeout);
extern int socket_loopback_server(int port, int type);
extern int socket_inaddr_any_server(int port, int type);

/* normally provided by "fdevent.h" */

#define FDE_READ              0x0001
#define FDE_WRITE             0x0002
#define FDE_ERROR             0x0004
#define FDE_DONT_CLOSE        0x0080

typedef void (*fd_func)(int fd, unsigned events, void *userdata);

fdevent *fdevent_create(int fd, fd_func func, void *arg);
void     fdevent_destroy(fdevent *fde);
void     fdevent_install(fdevent *fde, int fd, fd_func func, void *arg);
void     fdevent_remove(fdevent *item);
void     fdevent_set(fdevent *fde, unsigned events);
void     fdevent_add(fdevent *fde, unsigned events);
void     fdevent_del(fdevent *fde, unsigned events);
void     fdevent_loop();

static __inline__ void  emmcdl_sleep_ms( int  mseconds )
{
    Sleep( mseconds );
}

extern int  emmcdl_socket_accept(int  serverfd, struct sockaddr*  addr, socklen_t  *addrlen);

#undef   accept
#define  accept  ___xxx_accept

extern int  emmcdl_setsockopt(int  fd, int  level, int  optname, const void*  optval, socklen_t  optlen);

#undef   setsockopt
#define  setsockopt  ___xxx_setsockopt

static __inline__  int  emmcdl_socket_setbufsize( int   fd, int  bufsize )
{
    int opt = bufsize;
    return emmcdl_setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const void*)&opt, sizeof(opt));
}

static __inline__ void  disable_tcp_nagle( int  fd )
{
    int  on = 1;
    emmcdl_setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const void*)&on, sizeof(on));
}

extern int  emmcdl_socketpair( int  sv[2] );

static __inline__  char*  emmcdl_dirstart( const char*  path )
{
    char*  p  = strchr(path, '/');
    char*  p2 = strchr(path, '\\');

    if ( !p )
        p = p2;
    else if ( p2 && p2 > p )
        p = p2;

    return p;
}

static __inline__  char*  emmcdl_dirstop( const char*  path )
{
    char*  p  = strrchr(path, '/');
    char*  p2 = strrchr(path, '\\');

    if ( !p )
        p = p2;
    else if ( p2 && p2 > p )
        p = p2;

    return p;
}

static __inline__  int  emmcdl_is_absolute_host_path( const char*  path )
{
    return isalpha(path[0]) && path[1] == ':' && path[2] == '\\';
}

#else /* !_WIN32 a.k.a. Unix */

#ifdef ANDROID
#include "fdevent.h"
#include <cutils/sockets.h>
#include <cutils/misc.h>
#endif
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <unistd.h>

#define OS_PATH_SEPARATOR '/'
#define OS_PATH_SEPARATOR_STR "/"
#define ENV_PATH_SEPARATOR_STR ":"

typedef  pthread_mutex_t          emmcdl_mutex_t;

#define  EMMCDL_MUTEX_INITIALIZER    PTHREAD_MUTEX_INITIALIZER
#define  emmcdl_mutex_init           pthread_mutex_init
#define  emmcdl_mutex_lock           pthread_mutex_lock
#define  emmcdl_mutex_unlock         pthread_mutex_unlock
#define  emmcdl_mutex_destroy        pthread_mutex_destroy

#define  EMMCDL_MUTEX_DEFINE(m)      emmcdl_mutex_t   m = PTHREAD_MUTEX_INITIALIZER

#define  emmcdl_cond_t               pthread_cond_t
#define  emmcdl_cond_init            pthread_cond_init
#define  emmcdl_cond_wait            pthread_cond_wait
#define  emmcdl_cond_broadcast       pthread_cond_broadcast
#define  emmcdl_cond_signal          pthread_cond_signal
#define  emmcdl_cond_destroy         pthread_cond_destroy

/* declare all mutexes */
#define  EMMCDL_MUTEX(x)   extern emmcdl_mutex_t  x;
#ifdef ANDROID
#include "mutex_list.h"
#endif

static __inline__ void  close_on_exec(int  fd)
{
    fcntl( fd, F_SETFD, FD_CLOEXEC );
}

static __inline__ int  unix_open(const char*  path, int options,...)
{
    if ((options & O_CREAT) == 0)
    {
        return  TEMP_FAILURE_RETRY( open(path, options) );
    }
    else
    {
        int      mode;
        va_list  args;
        va_start( args, options );
        mode = va_arg( args, int );
        va_end( args );
        return TEMP_FAILURE_RETRY( open( path, options, mode ) );
    }
}

static __inline__ int  emmcdl_open_mode( const char*  pathname, int  options, int  mode )
{
    return TEMP_FAILURE_RETRY( open( pathname, options, mode ) );
}


static __inline__ int  emmcdl_open( const char*  pathname, int  options )
{
    int  fd = TEMP_FAILURE_RETRY( open( pathname, options ) );
    if (fd < 0)
        return -1;
    close_on_exec( fd );
    return fd;
}
#undef   open
#define  open    ___xxx_open

static __inline__ int  emmcdl_shutdown(int fd)
{
    return shutdown(fd, SHUT_RDWR);
}
#undef   shutdown
#define  shutdown   ____xxx_shutdown

static __inline__ int  emmcdl_close(int fd)
{
    return close(fd);
}
#undef   close
#define  close   ____xxx_close


static __inline__  int  emmcdl_read(int  fd, void*  buf, size_t  len)
{
    return TEMP_FAILURE_RETRY( read( fd, buf, len ) );
}

#undef   read
#define  read  ___xxx_read

static __inline__  int  emmcdl_write(int  fd, const void*  buf, size_t  len)
{
    return TEMP_FAILURE_RETRY( write( fd, buf, len ) );
}
#undef   write
#define  write  ___xxx_write

static __inline__ int   emmcdl_lseek(int  fd, int  pos, int  where)
{
    return lseek(fd, pos, where);
}
#undef   lseek
#define  lseek   ___xxx_lseek

static __inline__  int    emmcdl_unlink(const char*  path)
{
    return  unlink(path);
}
#undef  unlink
#define unlink  ___xxx_unlink

static __inline__  int  emmcdl_creat(const char*  path, int  mode)
{
    int  fd = TEMP_FAILURE_RETRY( creat( path, mode ) );

    if ( fd < 0 )
        return -1;

    close_on_exec(fd);
    return fd;
}
#undef   creat
#define  creat  ___xxx_creat

static __inline__ int  emmcdl_socket_accept(int  serverfd, struct sockaddr*  addr, socklen_t  *addrlen)
{
    int fd;

    fd = TEMP_FAILURE_RETRY( accept( serverfd, addr, addrlen ) );
    if (fd >= 0)
        close_on_exec(fd);

    return fd;
}

#undef   accept
#define  accept  ___xxx_accept

#define  unix_read   emmcdl_read
#define  unix_write  emmcdl_write
#define  unix_close  emmcdl_close

typedef void*  (*emmcdl_thread_func_t)( void*  arg );

static __inline__ bool emmcdl_thread_create(emmcdl_thread_func_t start, void* arg) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_t thread;
    errno = pthread_create(&thread, &attr, start, arg);
    return (errno == 0);
}

static __inline__  int  emmcdl_socket_setbufsize( int   fd, int  bufsize )
{
    int opt = bufsize;
    return setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));
}

static __inline__ void  disable_tcp_nagle(int fd)
{
    int  on = 1;
    setsockopt( fd, IPPROTO_TCP, TCP_NODELAY, (void*)&on, sizeof(on) );
}

static __inline__ int  emmcdl_setsockopt( int  fd, int  level, int  optname, const void*  optval, socklen_t  optlen )
{
    return setsockopt( fd, level, optname, optval, optlen );
}

#undef   setsockopt
#define  setsockopt  ___xxx_setsockopt

static __inline__ int  unix_socketpair( int  d, int  type, int  protocol, int sv[2] )
{
    return socketpair( d, type, protocol, sv );
}

static __inline__ int  emmcdl_socketpair( int  sv[2] )
{
    int  rc;

    rc = unix_socketpair( AF_UNIX, SOCK_STREAM, 0, sv );
    if (rc < 0)
        return -1;

    close_on_exec( sv[0] );
    close_on_exec( sv[1] );
    return 0;
}

#undef   socketpair
#define  socketpair   ___xxx_socketpair

static __inline__ void  emmcdl_sleep_ms( int  mseconds )
{
    usleep( mseconds*1000 );
}

static __inline__ int  emmcdl_mkdir(const char*  path, int mode)
{
    return mkdir(path, mode);
}
#undef   mkdir
#define  mkdir  ___xxx_mkdir

static __inline__ void  emmcdl_sysdeps_init(void)
{
}

static __inline__ char*  emmcdl_dirstart(const char*  path)
{
    return strchr((char*)path, '/');
}

static __inline__ char*  emmcdl_dirstop(const char*  path)
{
    return strrchr((char*)path, '/');
}

static __inline__  int  emmcdl_is_absolute_host_path( const char*  path )
{
    return path[0] == '/';
}

static __inline__ unsigned long emmcdl_thread_id()
{
    return (unsigned long)pthread_self();
}

#endif /* !_WIN32 */

#endif /* _EMMCDL_SYSDEPS_H */
