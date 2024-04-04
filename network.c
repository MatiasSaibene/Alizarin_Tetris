/*
 *                               Alizarin Tetris
 * Low level networking routines.
 *
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */ 

#include "config.h"	/* go autoconf! */
#include "atris.h" 

#include <sys/types.h>
#include <unistd.h>

#if HAVE_NETDB_H
#include <fcntl.h>
#endif

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if HAVE_NETDB_H
#include <netdb.h>
#endif

#include <time.h>

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#else
#if HAVE_WINSOCK_H
#include <winsock.h>
#endif
#endif

char *error_msg = NULL;

/***************************************************************************
 *	Server_AwaitConnection() 
 * Sets up the server side listening socket and awaits connections from the
 * outside world. Returns a socket.
 *********************************************************************PROTO*/
int
Server_AwaitConnection(int port)
{
  static struct sockaddr_in addr;
  static struct hostent *host;
  static int sockListen=0;  
  static int addrLen;
  int val1;
  struct linger val2;
  int sock;

  if (sockListen == 0) {
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(port);
    sockListen=socket(AF_INET,SOCK_STREAM,0);

    error_msg = 0;

    if (sockListen < 0) {
	Debug("unable to create socket: %s\n",error_msg = strerror(errno));
	return -1;
    } 

    val1=1;

#ifdef HAVE_SYS_SOCKET_H
    if (fcntl(sockListen, F_SETFL, O_NONBLOCK)) {
	Debug("unable to make socket non-blocking: %s\n", error_msg = strerror(errno));
	return -1;
    }
#endif

    if (setsockopt(sockListen,SOL_SOCKET,SO_REUSEADDR,
		   (void *)&val1, sizeof(val1))) {
      Debug("WARNING: setsockopt(...,SO_REUSEADDR) failed. You may have to wait a\n"
	      "\tfew minutes before you can try to be the server again.\n");
    }
    if (bind(sockListen, (struct sockaddr *)&addr, sizeof(addr))) {
	Debug("unable to bind socket (for listening): %s\n", error_msg = strerror(errno));
	return -1;
    }
    if (listen(sockListen,16)) {
	Debug("unable to listen on socket: %s\n", error_msg = strerror(errno));
	return -1;
    } 

    addrLen=sizeof(addr);

    Debug("accepting connections on port %d, socketfd %d\n",port,sockListen); 
  }
#if HAVE_SELECT || HAVE_WINSOCK_H
  /* it is possible to select() on a socket for the purpose of doing an
   * accept by putting it in the read group */
  {
      fd_set read_fds;
      struct timeval timeout = {0, 0};
      int retval;

      FD_ZERO(&read_fds);
      FD_SET(sockListen, &read_fds);

      retval = select(sockListen+1, &read_fds, NULL, NULL, &timeout);
      
      if (retval <= 0) {
	  /* no one is there */
	  return -1;
      }
  }
#endif
  sock=accept(sockListen, (struct sockaddr *)&addr,&addrLen);
  if (sock < 0) {
      if (
#ifdef HAVE_SYS_SOCKET_H
	      errno == EWOULDBLOCK || 
#endif
	      errno == EAGAIN)
	  return -1;
      else {
	  Debug("unable to accept on listening socket: %s\n", error_msg = strerror(errno));
	  return -1;
      }
  }

  val2.l_onoff=0; val2.l_linger=0;
  if (setsockopt(sock,SOL_SOCKET,SO_LINGER,(void *)&val2,sizeof(val2)))
      Debug("WARNING: setsockopt(...,SO_LINGER) failed. You may have to wait a few\n"
	      "\tminutes before you can try to be the server again.\n");

  host=gethostbyaddr((void *)&addr.sin_addr,sizeof(struct in_addr),AF_INET);
  if (!host) 
      Debug("connection from some unresolvable IP address, socketfd %d.\n", sock);
  else
      Debug("connection from %s, socketfd %d.\n",host->h_name,sock); 
  return sock;
}

/***************************************************************************
 *	Client_Connect() 
 * Set up client side networking and connect to the server. Returns a socket
 * pointing to the server.
 *
 * On error, -1 is returned.
 *********************************************************************PROTO*/
int
Client_Connect(char *hoststr, int lport)
{
    struct sockaddr_in addr;
    struct hostent *host;
    int mySock;
    short port = lport;
    int retval; 

    error_msg = NULL;

    host=gethostbyname(hoststr);
    if (!host) {
	Debug("unable to resolve [%s]: unknown hostname\n",hoststr);
	error_msg = "unknown hostname";
	return -1;
    }

    Debug("connecting to %s:%d ...\n",host->h_name,port);   

    memset(&addr, 0, sizeof(addr)); 
    addr.sin_family = AF_INET; /*host->h_addrtype;*/
    memcpy(&addr.sin_addr, host->h_addr, host->h_length);
    addr.sin_port=htons(port);
    mySock = socket(AF_INET, SOCK_STREAM, 0);
    if (mySock < 0) {
	Debug("unable to create socket: %s\n",error_msg = strerror(errno));
	return -1;
    }

    retval = connect(mySock, (struct sockaddr *) &addr, sizeof(addr));

    if (retval < 0) {
	Debug("unable to connect: %s\n",error_msg = strerror(errno));
	close(mySock);
	return -1;
    }

    Debug("connected to %s:%d, socketfd %d.\n",host->h_name,port,mySock);
    return mySock;
}

/***************************************************************************
 *	Network_Init() 
 * Call before you try to use any networking functions. Returns 0 on
 * success.
 *********************************************************************PROTO*/
int
Network_Init(void)
{
#if HAVE_WINSOCK_H
    WORD version_wanted = MAKEWORD(1,1);
    WSADATA wsaData;

    if ( WSAStartup(version_wanted, &wsaData) != 0 ) {
	Debug("WARNING: Couldn't initialize Winsock 1.1\n");
	return 1;
    }
    Debug("Winsock 1.1 networking available.\n");
    return 0;
#endif
    /* always works on Unix ... */
    return 0;
}

/***************************************************************************
 *	Network_Quit() 
 * Call when you are done networking. 
 *********************************************************************PROTO*/
void
Network_Quit(void)
{
#if HAVE_WINSOCK_H
    /* Clean up windows networking */
    if ( WSACleanup() == SOCKET_ERROR ) {
	if ( WSAGetLastError() == WSAEINPROGRESS ) {
	    WSACancelBlockingCall();
	    WSACleanup();
	}
    }
#endif
    return;
}
