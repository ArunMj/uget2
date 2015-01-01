/*
 *
 *   Copyright (C) 2012-2015 by C.H. Huang
 *   plushuang.tw@gmail.com
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  ---
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of portions of this program with the
 *  OpenSSL library under certain conditions as described in each
 *  individual source file, and distribute linked combinations
 *  including the two.
 *  You must obey the GNU Lesser General Public License in all respects
 *  for all of the code used other than OpenSSL.  If you modify
 *  file(s) with this exception, you may extend this exception to your
 *  version of the file(s), but you are not obligated to do so.  If you
 *  do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source
 *  files in the program, then also delete it here.
 *
 */

#include <string.h>
#include <UgDefine.h>
#include <UgThread.h>
#include <UgStdio.h>
#include <UgSocket.h>

#if defined _WIN32 || defined _WIN64
//#include <ws2tcpip.h>       // socklen_t, XXXXaddrinfo()
//static int  ws2_init_count = 0;
#define  ug_sleep       Sleep
#else
#include <netdb.h>          // struct addrinfo
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#define  ug_sleep(millisecond)    usleep (millisecond * 1000)
#endif // _WIN32 || _WIN64

int  ug_socket_connect (SOCKET fd, const char* addr, const char* port_or_serv)
{
	struct addrinfo  hints;
	struct addrinfo* result;
	struct addrinfo* cur;
	socklen_t  len;
	int        type;

	len = sizeof (type);
	if (getsockopt (fd, SOL_SOCKET, SO_TYPE, (char*) &type, &len) == -1)
		return SOCKET_ERROR;

	// AI_PASSIVE     // Socket address will be used in bind() call
	// AI_CANONNAME   // Return canonical name in first ai_canonname
	// AI_NUMERICHOST // Nodename must be a numeric address string
	// AI_NUMERICSERV // Servicename must be a numeric port number
	// get addrinfo
	memset (&hints, 0, sizeof (hints));
//	hints.ai_family = AF_INET;
	hints.ai_socktype = type;
//	hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE;
	if (getaddrinfo (addr, port_or_serv, &hints, &result) != 0)
		return SOCKET_ERROR;

	/*
	struct sockaddr_in	saddr;
	saddr.sin_family = family;
	saddr.sin_port = htons (80);
	saddr.sin_addr.S_un.S_addr = inet_addr ("127.0.0.1");
	connect (fd, (struct sockaddr *) &saddr, sizeof(saddr));
	*/
	for (cur = result;  cur;  cur = cur->ai_next) {
		if (connect (fd, result->ai_addr, result->ai_addrlen) == 0)
			break;
	}
	if (cur == NULL) {
		freeaddrinfo (result);
		return SOCKET_ERROR;
	}

	freeaddrinfo (result);
	return fd;
}

#if !(defined _WIN32 || defined _WIN64)
int  ug_socket_connect_unix (SOCKET fd, const char* path, int path_len)
{
	struct sockaddr_un  saddr;

	saddr.sun_family = AF_UNIX;
	saddr.sun_path [sizeof (saddr.sun_path) -1] = 0;

	if (path_len == -1) {
		strncpy (saddr.sun_path, path, sizeof (saddr.sun_path) -1);
		if (connect (fd, (struct sockaddr *) &saddr, sizeof(saddr)) < 0)
			return SOCKET_ERROR;
	}
	else {
		if (path_len > sizeof (saddr.sun_path) -1)
			path_len = sizeof (saddr.sun_path) -1;
		memcpy (saddr.sun_path, path, path_len);
		// abstract socket names (begin with 0) are not null terminated
		if (path_len > 0 && path[0] != 0)
			saddr.sun_path[path_len++] = 0;
		if (connect (fd, (struct sockaddr *) &saddr, sizeof (saddr.sun_family) + path_len) < 0)
			return SOCKET_ERROR;
	}

	return fd;
}

#endif // ! (_WIN32 || _WIN64)

// ------------------------------------
// Server API

int  ug_socket_listen (SOCKET fd, const char* addr, const char* port_or_serv,
                       int backlog)
{
	struct addrinfo  hints;
	struct addrinfo* result;
	struct addrinfo* cur;
	socklen_t  len;
	int        type;

	len = sizeof (type);
	if (getsockopt (fd, SOL_SOCKET, SO_TYPE, (char*) &type, &len) == -1)
		return SOCKET_ERROR;

	// AI_PASSIVE     // Socket address will be used in bind() call
	// AI_CANONNAME   // Return canonical name in first ai_canonname
	// AI_NUMERICHOST // Nodename must be a numeric address string
	// AI_NUMERICSERV // Servicename must be a numeric port number
	// get addrinfo
	memset (&hints, 0, sizeof (hints));
//	hints.ai_family = AF_INET;
	hints.ai_socktype = type;
//	hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE;
	if (getaddrinfo (addr, port_or_serv, &hints, &result) != 0)
		return SOCKET_ERROR;

	/*
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons (80);
	saddr.sin_addr.S_un.S_addr = inet_addr ("127.0.0.1");
	bind (fd, (struct sockaddr *)&saddr, sizeof (saddr));
	*/

	for (cur = result;  cur;  cur = cur->ai_next) {
		if (bind (fd, result->ai_addr, result->ai_addrlen) == 0)
			break;
	}
	if (cur == NULL)
		goto error_exit;

	if (listen (fd, backlog) == -1)
		goto error_exit;

	freeaddrinfo (result);
	return fd;

error_exit:
	freeaddrinfo (result);
	return SOCKET_ERROR;
}

#if !(defined _WIN32 || defined _WIN64)

SOCKET ug_socket_listen_unix (SOCKET fd, const char* path, int path_len, int backlog)
{
	struct sockaddr_un  saddr;

	saddr.sun_family = AF_UNIX;
	saddr.sun_path [sizeof (saddr.sun_path) -1] = 0;
	if (path_len == -1) {
		strncpy (saddr.sun_path, path, sizeof (saddr.sun_path) -1);
		// delete socket file before server start
//		ug_unlink (saddr.sun_path);
		if (bind (fd, (struct sockaddr*) &saddr, sizeof (saddr)) == -1)
			return SOCKET_ERROR;
	}
	else {
		if (path_len > sizeof (saddr.sun_path) -1)
			path_len = sizeof (saddr.sun_path) -1;
		memcpy (saddr.sun_path, path, path_len);
		// abstract socket names (begin with 0) are not null terminated
		if (path_len > 0 && path[0] != 0)
			saddr.sun_path[path_len++] = 0;
		if (bind (fd, (struct sockaddr*) &saddr, sizeof (saddr.sun_family) + path_len) == -1)
			return SOCKET_ERROR;
	}

	if (listen (fd, backlog) == -1)
		return SOCKET_ERROR;

	return fd;
}

#endif // ! (_WIN32 || _WIN64)

// ------------------------------------
// UgSocketServer

static UG_THREAD_RETURN_TYPE server_thread (UgSocketServer* server);

UgSocketServer* ug_socket_server_new (SOCKET server_fd)
{
	UgSocketServer*  server;

	server = ug_malloc0 (sizeof (UgSocketServer));
	server->ref_count = 1;
	server->socket = server_fd;
	server->stopped = TRUE;
	server->stopping = FALSE;
	server->client_addr_len = sizeof (server->client_addr);
	return server;
}

void  ug_socket_server_ref (UgSocketServer* server)
{
	server->ref_count++;
}

void  ug_socket_server_unref (UgSocketServer* server)
{
	if (--server->ref_count == 0) {
		if (server->destroy.func)
			server->destroy.func (server->destroy.data);
		shutdown (server->socket, 0);
		closesocket (server->socket);
		ug_free (server);
	}
}

void  ug_socket_server_close (UgSocketServer* server)
{
	if (server->socket != -1) {
		closesocket (server->socket);
		server->socket = -1;
	}
}

void  ug_socket_server_set_receiver (UgSocketServer* server,
                                     UgSocketServerFunc func,
                                     void* data)
{
	server->receiver.func  = func;
	server->receiver.data  = data;
}

int   ug_socket_server_start (UgSocketServer* server)
{
	if (server->receiver.func == NULL)
		return FALSE;

	if (server->stopped) {
		server->stopped = FALSE;
		server->stopping = FALSE;
		ug_socket_server_ref (server);
		ug_thread_create (&server->thread,
				(UgThreadFunc) server_thread, server);
		ug_thread_unjoin (&server->thread);
		return TRUE;
	}
	return FALSE;
}

void  ug_socket_server_stop (UgSocketServer* server)
{
	if (server->stopped == FALSE) {
		server->stopping = TRUE;
//		ug_thread_join (&server->thread);
	}
}

static UG_THREAD_RETURN_TYPE server_thread (UgSocketServer* server)
{
	struct    timeval  timeout;
	int       client_fd;
	int       result;

	timeout.tv_sec = 2;
	timeout.tv_usec = 0;

	while (server->stopping == FALSE) {
		FD_ZERO (&server->read_fds);
		FD_SET (server->socket, &server->read_fds);

		result = select (server->socket + 1, &server->read_fds,
		        NULL, NULL, &timeout);
		if (server->stopping || result < 0)
			break;
		// select() time limit expired if result == 0
		if (result == 0) {
			ug_sleep (500);
			continue;
		}

		client_fd = accept (server->socket,
				(struct sockaddr *) &server->client_addr,
				&server->client_addr_len);

		server->receiver.func (server, client_fd,
		                       server->receiver.data);
	}

	server->stopping = FALSE;
	server->stopped = TRUE;
	ug_socket_server_unref (server);
	return UG_THREAD_RETURN_VALUE;
}
