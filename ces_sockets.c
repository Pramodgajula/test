//
// ces_sockets.c
//
// A library for simplifying socket operations
//
// $Author: gbetteri $
// $Date: 2003/06/10 12:32:47 $
// $Header: /home/cvsroot/ces/src/ces_sockets/ces_sockets.c,v 1.11 2003/06/10 12:32:47 gbetteri Exp $
// $Revision: 1.11 $
// $Name:  $
//
// $Log: ces_sockets.c,v $
// Revision 1.11  2003/06/10 12:32:47  gbetteri
// Added close and return val if bind failed
//
// Revision 1.10  2003/05/07 14:26:47  jwilbur
// get rid of warning
//
// Revision 1.9  2003/04/17 17:59:56  ctedrow
// Fixed bad form around sw_debug_printf
//
// Revision 1.8  2003/01/29 19:32:39  ctedrow
// Added set_socket_nonblock
//
// Revision 1.7  2002/12/19 15:50:54  ctedrow
// Only fill clientAddress if it has a valid pointer
//
// Revision 1.6  2002/12/06 17:01:36  ctedrow
// Close socket on errors
//
// Revision 1.5  2002/10/29 19:12:44  ctedrow
// Turned off debug statements
//
// Revision 1.4  2002/10/18 16:34:35  jwilbur
// change to udp_create..., added include to ces_util
//
// Revision 1.3  2002/10/17 23:31:47  jwilbur
// added unlink to create_udp...
//
// Revision 1.2  2002/10/17 19:27:44  jwilbur
// added udp socket creation wrappers
//
// Revision 1.1  2002/08/26 15:17:29  ctedrow
// Renamed ces_ from mds_
//
// Revision 1.1  2002/08/20 14:20:31  ctedrow
// Import to new cvs module
//
// Revision 1.6  2002/07/22 12:30:38  ctedrow
// Initial development
//
// Revision 1.5  2002/07/15 19:07:51  ctedrow
// Switched to Unix domain sockets
//
// Revision 1.4  2002/07/12 16:10:12  ctedrow
// Initial development
//
// Revision 1.3  2002/07/11 17:03:07  ctedrow
// Initial development
//
// Revision 1.2  2002/07/11 14:09:16  ctedrow
// Initial development
//
// Revision 1.1  2002/07/10 14:37:37  ctedrow
// Initial revision
//
//
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>

#include "ces_sockets.h"
#include "ces_util.h"
#include "ces_common.h"

//#define sw_debug_printf(fd, fmt,args...)	printf(fmt ,##args)
#define sw_debug_printf(fd, fmt,args...)
/* External variables */
extern UDP_Socket_S swDbgSocket;


#define PROTO_TCP               6  /* from '/etc/protocols' */

/****************************************************************
      ********* DATAGRAM (UDP) SOCKET ROUTINES *********
****************************************************************/
/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		mcast_join
* DESCRIPTION:	Joins a socket to a specified multicast address
* INPUTS: 		fd - a socket descriptor
*				mcastAddr - the multicast address to join
*				ifAddr - the IP address of the local interface to
*					use for the join (this is for multi-homed systems)
* RETURNS:		0/-1
* CONCURRENCY:	Multi-Thread-Safe
</FUNCTION_HEADER>
****************************************************************/
int mcast_join(int fd, unsigned int mcastAddr, unsigned int ifAddr)
{
	struct ip_mreq mreq;
	int rc;

	sw_debug_printf(&swDbgSocket, "mcast_join: IP_ADD_MEMBERSHIP, mcast %lx, ifAddr %lx\n", mcastAddr, ifAddr);
	mreq.imr_multiaddr.s_addr = htonl(mcastAddr);
	mreq.imr_interface.s_addr = htonl(ifAddr);
	rc = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	if(rc)
	{
		sw_debug_printf(&swDbgSocket, "mcast_join: Error: setsockopt IP_ADD_MEMBERSHIP, errno %d\n", errno);
	}
	return(rc);
} /* end of mcast_join */
/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		mcast_set_loopback
* DESCRIPTION:	This routine defines whether multicast packets are
*                 looped back or not.
* INPUTS: 		fd - a socket descriptor
*               loop - 0/1 to enable/disable loopback
* RETURNS:		0/-1
* CONCURRENCY:	Multi-Thread-Safe
</FUNCTION_HEADER>
****************************************************************/
int mcast_set_loopback(int fd, unsigned char loop)
{
	int rc;

	rc = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
	if(rc)
	{
		sw_debug_printf(&swDbgSocket, "mcast_set_loopback: Error: setsockopt IP_MULTICAST_LOOP, errno %d\n", errno);
	}
	return(rc);
} /* end of mcast_set_loopback */
/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		mcast_set_ttl
* DESCRIPTION:	This routine set the time-to-live for multicast packets
*					sent on this socket.
* INPUTS: 		fd - a socket descriptor
*				ttl - the time-to-live
* RETURNS:		0/-1
* CONCURRENCY:	Multi-Thread-Safe
</FUNCTION_HEADER>
****************************************************************/
int mcast_set_ttl(int fd, unsigned char ttl)
{
	int rc;

	rc = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
	if(rc)
	{
		sw_debug_printf(&swDbgSocket, "mcast_set_ttl: Error: setsockopt IP_MULTICAST_TTL, errno %d\n", errno);
	}
	return(rc);
} /* end of mcast_set_ttl */

/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		mcast_set_if
* DESCRIPTION:	This routine specifies the interface for outgoing
*					multicast packets.
* INPUTS: 		fd - a socket descriptor
*				addr - address of the interface
* RETURNS:		0/-1
* CONCURRENCY:	Multi-Thread-Safe
</FUNCTION_HEADER>
****************************************************************/
int mcast_set_if(int fd, unsigned int addr)
{
	struct in_addr ifAddr;
	int rc;

	ifAddr.s_addr = htonl(addr);
	rc = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &ifAddr, sizeof(ifAddr));
	if(rc)
	{
		sw_debug_printf(&swDbgSocket, "mcast_set_if: Error: setsockopt IP_MULTICAST_IF, errno %d\n", errno);
	}
	return(rc);
} /* end of mcast_set_if */

/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		set_socket_timeout
* DESCRIPTION:	This routine configures a socket to use the timout
*				specified by application
* INPUTS: 		fd - a socket descriptor,
*				tv - Time value structure
* RETURNS:		0/-1
* CONCURRENCY:	SYNCHRONOUS. MULTI_THREAD_SAFE.
</FUNCTION_HEADER>
****************************************************************/
static int set_socket_timeout(int fd, struct timeval tv)
{
	int rc;
	sw_debug_printf(&swDbgSocket, "set_socket_timeout: SO_SNDTIMEO, tv_sec %lx, tv_usec %lx\n", tv.tv_sec, tv.tv_usec);
	rc = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (struct timeval*)&(tv), sizeof(struct timeval));
	if(rc)
	{
		sw_debug_printf(&swDbgSocket, "set_socket_timeout: Error: setsockopt SO_SNDTIMEO , errno %d\n", errno);
	}
	return(rc);
}

/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		udp_setup_socket
* DESCRIPTION:	Creates and configures a UDP socket.
* INPUTS: 		udpSocket->direction - UDP_SEND or UDP_RECV
*				udpSocket->addr - the address to bind or sendto
*				udpSocket->port - the port to bind or sendto
*				udpSocket->localIFAddr - the address of the local interface
*					on which to receive multicast packets.
* RETURNS:		0/-1
* CONCURRENCY:	Multi-Thread-Safe
</FUNCTION_HEADER>
****************************************************************/
int udp_setup_socket(UDP_Socket_S *udpSocket)
{
	int retval = 0;

	//sw_debug_printf(&swDbgSocket, "udp_setup_socket: ENTER\n");

	/* Create the socket */
	udpSocket->fd = socket(udpSocket->localSocket?AF_LOCAL:AF_INET, SOCK_DGRAM, 0);
	if(udpSocket->fd == -1)
	{
		sw_debug_printf(&swDbgSocket, "udp_setup_socket: socket creation failed, errno %d\n", errno);
		return -1;
	}

	if(!udpSocket->localSocket)
	{
		/* EXTERNAL INTERNET SOCKET */

		/* Initialize the address structures */
		memset(&udpSocket->sockAddr, 0, sizeof(udpSocket->sockAddr));
		udpSocket->sin = (struct sockaddr_in *)(&udpSocket->sockAddr);
		udpSocket->sin->sin_family = AF_INET;
		udpSocket->addrSize = sizeof(udpSocket->sockAddr);
		udpSocket->saPtr = &udpSocket->sockAddr;
		udpSocket->rxSin = (struct sockaddr_in *)(&udpSocket->sockAddr);

		/* Use the address given unless it is a receive socket and the address
		   is 224.0.0.0 or higher */
		if((udpSocket->direction == UDP_SEND) || (udpSocket->addr < 0xe0000000))
			udpSocket->sin->sin_addr.s_addr = htonl(udpSocket->addr);
		else
		{
			sw_debug_printf(&swDbgSocket, "udp_setup_socket: use INADDR_ANY for mcast recv\n");
			udpSocket->sin->sin_addr.s_addr = htonl(INADDR_ANY);
		}

		udpSocket->sin->sin_port = htons(udpSocket->port);

		/* If this will be a receive socket, then bind the address and port */
		if(udpSocket->direction == UDP_RECV)
		{
			//sw_debug_printf(&swDbgSocket, "udp_setup_socket: bind recv socket\n");
			if(bind(udpSocket->fd, &udpSocket->sockAddr, sizeof(udpSocket->sockAddr)) < 0)
			{
				sw_debug_printf(&swDbgSocket, "udp_setup_socket: error in bind, errno %d\n", errno);
				close(udpSocket->fd);
				retval = -1;
			}
			/* Join the multicast group if the address is 224.0.0.0 or higher */
			if(udpSocket->addr >= 0xe0000000)
			{
				sw_debug_printf(&swDbgSocket, "udp_setup_socket: join mcast %lx\n", udpSocket->addr);
				if(mcast_join(udpSocket->fd, udpSocket->addr, udpSocket->localIFAddr))
				{
					sw_debug_printf(&swDbgSocket, "udp_setup_socket: error in mcast_join\n");
					close(udpSocket->fd);
					retval = -1;
				}
			}
		}
		/* If this will be a send socket for multicast, then set some options */
		else if(udpSocket->addr >= 0xe0000000)
		{
			/* Turn off multicast loopback so that we don't get our own transmissions */
			sw_debug_printf(&swDbgSocket, "udp_setup_socket: set mcast loopback\n");
			if(mcast_set_loopback(udpSocket->fd, 0))
			{
				sw_debug_printf(&swDbgSocket, "udp_setup_socket: error in mcast_set_loopback for send\n");
				close(udpSocket->fd);
				retval = -1;
			}

			//sw_debug_printf(&swDbgSocket, "udp_setup_socket: set mcast ttl for send\n");
			//mcast_set_ttl(udpSocket->fd, 1);

			//sw_debug_printf(&swDbgSocket, "udp_setup_socket: set mcast IF for send\n");
			//mcast_set_if(udpSocket->fd, INADDR_ANY);
		}
	} /* end of EXTERNAL SOCKET */
	else
	{
		/* INTERNAL LOCAL IPC SOCKET */
		memset(&udpSocket->sun, 0, sizeof(udpSocket->sun));
		udpSocket->sun.sun_family = AF_LOCAL;
		strncpy(udpSocket->sun.sun_path, udpSocket->pathname, sizeof(udpSocket->sun.sun_path)-1);
		udpSocket->saPtr = (struct sockaddr *)&udpSocket->sun;
		udpSocket->addrSize = SUN_LEN(&udpSocket->sun);
		if(udpSocket->direction == UDP_RECV)
		{
			//sw_debug_printf(&swDbgSocket, "udp_setup_socket: fd %d, pathname %s, size %d\n",
				//udpSocket->fd, udpSocket->sun.sun_path, SUN_LEN(&udpSocket->sun));
			if(bind(udpSocket->fd, (struct sockaddr *)&udpSocket->sun, SUN_LEN(&udpSocket->sun)) < 0)
			{
				sw_debug_printf(&swDbgSocket, "udp_setup_socket: error in RECV bind, errno %d\n", errno);
				close(udpSocket->fd);
				retval = -1;
			}
		}
	} /* end of LOCAL SOCKET */
	if(set_socket_timeout(udpSocket->fd, udpSocket->tv))
	{
		sw_debug_printf(&swDbgSocket, "udp_setup_socket: error in set_socket_timeout\n");
		close(udpSocket->fd);
		retval = -1;
	}
	return retval;
} /* end of udp_setup_socket */

/****************************************************************
      ********* STREAM (TCP) SOCKET ROUTINES *********
****************************************************************/

/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		tcp_check_connection
* DESCRIPTION:	Checks the errno macro to see why an operation
* 				failed.
* INPUTS: 		NONE
* RETURNS:		0 = broken connection; 1 = not broken
* CONCURRENCY:	SYNCHRONOUS. MULTI_THREAD_SAFE.
</FUNCTION_HEADER>
****************************************************************/
unsigned int tcp_check_connection(void)
{
int i = 0;
//const char *msgs[] = { "", "Broken pipe", "Net Unreachable", "Connection Reset",
//						"Socket not Connected", "Host Unreachable" };

	switch(errno)
	{
		case EPIPE:
			i = 1; break;
		case ENETUNREACH:
			i = 2; break;
		case ECONNRESET:
			i = 3; break;
		case ENOTCONN:
			i = 4; break;
		case EHOSTUNREACH:
			i = 5; break;
	}
//	if(i)
//	{
//		sw_debug_printf(&swDbgSocket, "tcp: ERROR: %s\n", msgs[i]);
//	}
	return (i ? 1 : 0);
}  /* end of tcp_check_connection */

/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		tcp_start_server
* DESCRIPTION:	Creates and starts a listening socket,
* 				binds ANY ADDRESS, and begins listening.
* INPUTS: 		listenfd - pointer to a socket descriptor
*               localPort - the local port to bind to
*               backlog - the number of simultaneous connections
*               localSocket - if set, PF_LOCAL is used to keep the socket local and private
* RETURNS:		0/-1
* CONCURRENCY:	SYNCHRONOUS. MULTI_THREAD_SAFE.
</FUNCTION_HEADER>
****************************************************************/
int tcp_start_server(int localPort, int backlog, unsigned int localSocket, tcp_service_S *tcp_service)
{
	struct sockaddr_in *sin;
	struct sockaddr servaddr;
	struct sockaddr_un sun;
	struct sockaddr *sa;
	int addrSize = 0;
	int *listenfd;
	char *pathname = (char *)tcp_service->sock_path;

	unlink(pathname);
	listenfd = &(tcp_service->fds[TCP_LISTENER]);

	/* Create the socket */
	*listenfd = socket(localSocket?AF_LOCAL:AF_INET, SOCK_STREAM, localSocket?0:PROTO_TCP);
	if(*listenfd == -1)
	{
		sw_debug_printf(&swDbgSocket, "tcp: socket creation failed, errno %d\n", errno);
		return -1;
	}

	/* Change the sockopt to guard against restart problems */
	set_socket_ReUseAddr(*listenfd);

	/* Initialize the address structures */
	if(!localSocket)
	{
		memset(&servaddr, 0, sizeof(servaddr));
		sin = (struct sockaddr_in *)(&servaddr);
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = htonl(INADDR_ANY);
		sin->sin_port = htons(localPort);
		sa = &servaddr;
		addrSize = sizeof(servaddr);
		sw_debug_printf(&swDbgSocket, "tcp: Using socket port %d\n", localPort);
	}
	else
	{
		memset(&sun, 0, sizeof(sun));
		sun.sun_family = AF_LOCAL;
		strncpy(sun.sun_path, pathname, sizeof(sun.sun_path)-1);
		//sw_debug_printf(&swDbgSocket, "tcp: setup the unix domain socket, <%s>\n", sun.sun_path);
		sa = (struct sockaddr *)&sun;
		addrSize = SUN_LEN(&sun);
	}

	//printf("%s : Binding to local address\n",__FUNCTION__);
	/* Bind an address/port to the socket */
	//sw_debug_printf(&swDbgSocket, "tcp: bind the socket\n");
	if( (bind(*listenfd, sa, addrSize)) < 0)
	{
		perror("Bind failed:\n");
		sw_debug_printf(&swDbgSocket, "tcp: bind failed, errno %d\n", errno);
		close(*listenfd);
		return -1;
	}
	/* Begin listening on the socket/port */
	if( (listen(*listenfd, backlog)) < 0)
	{
		sw_debug_printf(&swDbgSocket, "tcp: listen failed, errno %d\n", errno);
		close(*listenfd);
		return -1;
	}
	return 0;
}  /* end of tcp_start_server */

/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		tcp_set_socket_nolinger
* DESCRIPTION:	This routine configures a connection-oriented socket
*                to not linger when it is closed.
* INPUTS: 		fd - a socket descriptor
* RETURNS:		0/-1
* CONCURRENCY:	SYNCHRONOUS. MULTI_THREAD_SAFE.
</FUNCTION_HEADER>
****************************************************************/
int tcp_set_socket_nolinger(int fd)
{
	long rc;
	struct linger socketLinger;
	unsigned int size;
	int level = SOL_SOCKET, optname = SO_LINGER;

	//printf("tcp: tcp_set_socket_nolinger\n");
	if( (rc = getsockopt(fd, level, optname, (void *)(&socketLinger), &size)) != 0)
	{
		sw_debug_printf(&swDbgSocket, "tcp: getsockopt failed (errno %x)\n", errno);
	}
	else
	{
		//sw_debug_printf(&swDbgSocket, "tcp: getsockopt - onoff = %d, linger time = %d\n", socketLinger.l_onoff, socketLinger.l_linger);
	}

	size = sizeof(struct linger);
	socketLinger.l_onoff = 0;
	socketLinger.l_linger = 0;
	if( (rc = setsockopt(fd, level, optname, (void *)(&socketLinger), size)) != 0)
	{
		sw_debug_printf(&swDbgSocket, "tcp: setsockopt failed (errno %x)\n", errno);
		return -1;
	}
	return 0;
} /* end of tcp_set_socket_nolinger */

/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		tcp_close_listening_socket
* DESCRIPTION:	This routine closes the server's listening socket
* INPUTS: 		listenfd - the open listening socket
* RETURNS:		none
* CONCURRENCY:	SYNCHRONOUS. MULTI_THREAD_SAFE.
</FUNCTION_HEADER>
****************************************************************/
void tcp_close_listening_socket(int listenfd)
{
	if(listenfd != -1)
	{
		tcp_set_socket_nolinger(listenfd);
		sw_debug_printf(&swDbgSocket, "tcp: closing listening socket\n");
		close(listenfd);
	}
} /* end of tcp_close_listening_socket */

/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		tcp_check_for_connection
* DESCRIPTION:	Checks if a connection has been requested by the client.
* INPUTS: 		listenfd - the listening socket descriptor
*				clientAddress - receptacle for the address of the connected client
*				connfd - receptacle for the connected socket descriptor
*				delay - timeout in msec to wait for a connection.  Note: if
*					delay = 0, this routine returns immediately; if delay = -1,
*					this routine will block indefinitely.
* RETURNS:		0 = no connection / 1 = connection
* CONCURRENCY:	SYNCHRONOUS. MULTI_THREAD_SAFE.
</FUNCTION_HEADER>
****************************************************************/
unsigned int tcp_check_for_connection(int listenfd, unsigned int *clientAddress, int *connfd, int delay, unsigned int localSocket)
{
	struct sockaddr_in *sin;
	struct sockaddr conncliaddr;
	fd_set readfds;
	unsigned int connclilen;
	int err;
	struct timeval tv = { 0, 0 };
	struct timeval *tvPtr = 0;
	unsigned int retval = 0;

	/* Set up for the select call */
	FD_ZERO(&readfds);
	FD_SET(listenfd, &readfds);

	/* Check if a datagram is available */
	if(delay >= 0)
	{
	    tv.tv_sec = delay / 1000;
    	tv.tv_usec = (delay % 1000) * 1000;
		tvPtr = &tv;
	}

	err=select(listenfd+1, &readfds, 0, 0, tvPtr);
	/* err == 0  Timeout
	 * err <  0  Error
	 * err >  0  The socket has data
	 */
	if(err == 0)
	{
		printf("%s : Select timedout!!!\n",__FUNCTION__);
		//printf("tcp: Check for connection timeout\n");
	}
	else if(err < 0)
	{
		printf("%s : Connection error\n",__FUNCTION__);
		//printf("tcp: Check For Connection Error: select() failed (errno %x)\n", errno);
	}
	else if(FD_ISSET(listenfd, &readfds))
	{
		connclilen = sizeof(conncliaddr);
		if(localSocket)
			*connfd = accept(listenfd, 0, 0);
		else
			*connfd = accept(listenfd, &conncliaddr, &connclilen);
		if( *connfd < 0)
		{
			sw_debug_printf(&swDbgSocket, "tcp: accept error, errno %d\n", errno);
		}
		else
		{
			sin = (struct sockaddr_in *)(&conncliaddr);
			if(clientAddress)
				*clientAddress = sin->sin_addr.s_addr;
			retval = 1;
		}
	}
	return(retval);
}  /* end of tcp_check_for_connection */

/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		tcp_drop_client
* DESCRIPTION:
* INPUTS: 		NONE
* RETURNS:		none
* CONCURRENCY:	SYNCHRONOUS. MULTI_THREAD_SAFE.
</FUNCTION_HEADER>
****************************************************************/
void tcp_drop_client(int *connfd)
{
	if(*connfd != -1)
	{
		//sw_debug_printf(&swDbgSocket, "tcp: Dropping connected client\n");
		tcp_set_socket_nolinger(*connfd);
		close(*connfd);
		*connfd = -1;
	}
}  /* end of tcp_drop_client */

/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		tcp_receive_msg
* DESCRIPTION:	Receives a single datagram on the socket
* 				connected to a foriegn client.
* INPUTS: 		buffer - receptacle for the data
* 				buflen - size of the buffer
* RETURNS:		-1 on error OR number of bytes received
* CONCURRENCY:	SYNCHRONOUS. MULTI_THREAD_SAFE.
</FUNCTION_HEADER>
****************************************************************/
int tcp_receive_msg(tcp_service_S* tcp_service)
{
	fd_set readfds;
	int  err, rc;

	int  *connfd;
	unsigned int *serverConnected;
	char *buffer = NULL;
	unsigned int buflen;
	struct timeval tv = { 0, 0 };

	/* Initialize the Variables */
	serverConnected = &(tcp_service->socket_connected);
	connfd = &(tcp_service->fds[tcp_service->sock_cnt]);
	buffer = tcp_service->buffer;
	buflen = tcp_service->buffer_size;

	if(!*serverConnected)
	{
		sw_debug_printf(&swDbgSocket, "tcp: recvmsg, server not connected\n");
		return -1;
	}
	/* Set up for the select call */
	FD_ZERO(&readfds);
	FD_SET(*connfd, &readfds);

	/* Check if a datagram is available */
	tv.tv_sec = 0;        /* wait for 100ms */
    tv.tv_usec = 10000;

	/* Check if a datagram is available */
	err=select(FD_SETSIZE, &readfds, 0, 0, &tv);
	/* err == 0  Timeout
	 * err <  0  Error
	 * err >  0  The socket has data
	 */
	if(err == 0)
	{
		//printf("tcp: Recv timeout\n");
		return 0;
	}
	else if(err < 0)
	{
		//printf("tcp: Error: select() failed (errno %x)\n", errno);
		*serverConnected = tcp_check_connection() ? FALSE:TRUE;
		if(!*serverConnected)
			return -1;
	}
	if( (rc = recv(*connfd, buffer, buflen, 0)) < 0)
	{
		*serverConnected = tcp_check_connection() ? FALSE:TRUE;
		if(!*serverConnected)
			return -1;
	}
	else if(rc == 0)
	{
		//sw_debug_printf(&swDbgSocket, "tcp: recv returned zero!  Connection closed, errno %d\n", errno);
		*serverConnected = tcp_check_connection();
	}
	return rc;
}  /* end of tcp_receive_msg */

/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		tcp_send_msg
* DESCRIPTION:	Sends a single datagram to the socket
* 				connected to a foriegn client.
* INPUTS: 		buffer - buffer containing the data to send
* 				buflen - number of bytes to send
* RETURNS:		0/-1
* CONCURRENCY:	SYNCHRONOUS. MULTI_THREAD_SAFE.
</FUNCTION_HEADER>
****************************************************************/
int tcp_send_msg(char *buffer, unsigned int buflen, unsigned int *serverConnected, int *connfd)
{
int rc;
	if(!*serverConnected)
		return -1;
	if( (rc = send(*connfd, buffer, buflen, 0)) < 0)
	{
		*serverConnected = tcp_check_connection() ? FALSE:TRUE;
		if(!*serverConnected)
		{
			sw_debug_printf(&swDbgSocket, "tcp: sendmsg, server not connected\n");
			return -1;
		}
	}
	return 0;
}  /* end of tcp_send_msg */

/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		tcp_start_client
* DESCRIPTION:	Creates and connects a client socket
* INPUTS: 		fd - receptacle for a socket descriptor
*               addr - the address to connect to
*               port - the port to connect to
* RETURNS:		0/-1
* CONCURRENCY:	SYNCHRONOUS. MULTI_THREAD_SAFE.
</FUNCTION_HEADER>
****************************************************************/
int tcp_start_client(unsigned int addr, int port, unsigned int localSocket, gal_socket_client_S* gal_socket_client)
{
	struct sockaddr servAddr;
	struct sockaddr_in *sin;
	struct sockaddr_un sun;
	struct sockaddr *sa;
	int addrSize = 0;
	int *fd;
	char *pathname = (char *)gal_socket_client->sock_path;

	fd = &(gal_socket_client->fd);


	*fd = socket(localSocket?AF_LOCAL:AF_INET, SOCK_STREAM, 0);
	if(*fd == -1)
		return -1;
	if(!localSocket)
	{
		memset(&servAddr, 0, sizeof(servAddr));
		sin = (struct sockaddr_in *)(&servAddr);
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = htonl(addr);
		sin->sin_port = htons(port);
		sa = &servAddr;
		addrSize = sizeof(servAddr);
	}
	else
	{
		memset(&sun, 0, sizeof(sun));
		sun.sun_family = AF_LOCAL;
		strncpy(sun.sun_path, pathname, sizeof(sun.sun_path)-1);
		//sw_debug_printf(&swDbgSocket, "tcp: setup the unix domain socket, <%s>\n", sun.sun_path);
		sa = (struct sockaddr *)&sun;
		addrSize = SUN_LEN(&sun);
	}
	//printf("%s : Connecting\n",__FUNCTION__);
	if(set_socket_timeout(*fd, gal_socket_client->tv))
	{
		sw_debug_printf(&swDbgSocket, "udp_setup_socket: error in set_socket_timeout\n");
		close(*fd);
		return -1;
	}
	return(connect(*fd, sa, addrSize));
} /* end of start_tcp_client */

/****************************************************************
      ********** GENERAL SOCKET ROUTINES *********
****************************************************************/

/****************************************************************
<FUNCTION_HEADER>
* FUNCTION:		set_socket_ReUseAddr
* DESCRIPTION:	This routine configures a socket to allow it to
*                reuse an address.
* INPUTS: 		fd - a socket descriptor
* RETURNS:		0/-1
* CONCURRENCY:	SYNCHRONOUS. MULTI_THREAD_SAFE.
</FUNCTION_HEADER>
****************************************************************/
int set_socket_ReUseAddr(int fd)
{
	long rc;
	int reuse;
	unsigned int size;
	int level = SOL_SOCKET, optname = SO_REUSEADDR;

	//printf("set_socket_ReUseAddr\n");
	size = sizeof(reuse);
	if( (rc = getsockopt(fd, level, optname, (void *)(&reuse), &size)) != 0)
	{
		sw_debug_printf(&swDbgSocket, "tcp: getsockopt failed (errno %x)\n", errno);
	}
	else
	{
		//printf("tcp: getsockopt - reuse = %d\n", reuse);
	}

	size = sizeof(reuse);
	reuse = 1;
	if( (rc = setsockopt(fd, level, optname, (void *)(&reuse), size)) != 0)
	{
		sw_debug_printf(&swDbgSocket, "tcp: setsockopt failed (errno %x)\n", errno);
		return -1;
	}
	return 0;
} /* end of set_socket_ReUseAddr */

/****************************************************************
<FUNCTION_HEADER>
FUNCTION:		set_socket_nonblock
DESCRIPTION:	This routine configures a socket to be non-blocking
INPUTS: 		fd - a socket descriptor
RETURNS:		0/-1
CONCURRENCY:	SYNCHRONOUS. MULTI_THREAD_SAFE.
</FUNCTION_HEADER>
****************************************************************/
int set_socket_nonblock(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
} /* end of set_socket_nonblock */

/****************************************************************
<FUNCTION_HEADER>
FUNCTION:       int create_udp_receive_socket(char* name, UDP_Socket_S* socket)
DESCRIPTION:    Create a simple udp_listening socket
INPUTS:
*               name = socket name
*               socket = a structure to be filled in
RETURNS:        1 if successful, 0 if not
CONCURRENCY:	SYNCHRONOUS. MULTI_THREAD_SAFE.
</FUNCTION_HEADER>
****************************************************************/
int create_udp_receive_socket(udp_service_S *udp_service)
{
	int cntr;
	UDP_Socket_S* udp_socket = NULL;
	char* name = (char *)udp_service->sock_path;

	udp_socket = &(udp_service->udp_sock);

	memset(udp_socket, 0, sizeof(UDP_Socket_S));
	udp_socket->localSocket 	= TRUE;
	udp_socket->direction 		= UDP_RECV;
	udp_socket->pathname 		= name;


	/* Default the Time OUT for UDP receive Socket */
//	udp_socket->tv.tv_sec 		= udp_service->tv.tv_sec;
//	udp_socket->tv.tv_usec 		= udp_service->tv.tv_usec;

	unlink(udp_socket->pathname);
	cntr = 0;

	while( (udp_setup_socket(udp_socket)) &&
			 	 (cntr++ < 10) ) {
		ms_wait(1000);
	}
	if(cntr >= 10) {
		udp_socket->fd = -1;
		udp_service->fd = udp_socket->fd;
		printf("Socket creation failed \n");
	  return(0);
	}
	udp_service->fd = udp_socket->fd;
	return(1);
}

/****************************************************************
<FUNCTION_HEADER>
FUNCTION: int create_udp_send_socket(char* name, UDP_Socket_S* socket);
DESCRIPTION:	Create a simple udp_listening socket
INPUTS: 		name = socket name
*							socket = a structure to be filled in
RETURNS:	1 if successful, 0 if not
CONCURRENCY:	SYNCHRONOUS. MULTI_THREAD_SAFE.
</FUNCTION_HEADER>
****************************************************************/
int create_udp_send_socket(gal_socket_client_S *gal_socket_client)
{
	int cntr;
	UDP_Socket_S* udp_socket = NULL;
	char* name = (char *)gal_socket_client->sock_path;

	udp_socket = &(gal_socket_client->udp_sock);

	memset(udp_socket, 0, sizeof(UDP_Socket_S));
	udp_socket->localSocket 	= TRUE;
	udp_socket->direction 		= UDP_SEND;
	udp_socket->pathname 		= name;

	udp_socket->tv.tv_sec 		= gal_socket_client->tv.tv_sec;
	udp_socket->tv.tv_usec 		= gal_socket_client->tv.tv_usec;

	cntr = 0;
	while((udp_setup_socket(udp_socket)) &&
			 	 (cntr++ < 10) ) {
		ms_wait(1000);
	}
	if(cntr >= 10) {
		udp_socket->fd = -1;
		gal_socket_client->fd = udp_socket->fd;
	  return(0);
	}
	gal_socket_client->fd = udp_socket->fd;
	return(1);
}
