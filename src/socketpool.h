/**
 *  This file is part of the IRCbot project.
 *  Copyright (C) 2007  Michal Kuchta
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _SOCKETPOOL_H
#define _SOCKETPOOL_H 1

typedef struct sSocketPool *SocketPool;
typedef struct sSocket *Socket;
typedef struct sSocketDataNode *SocketDataNode;

typedef void (*socketCallback)(Socket socket);

/**
 * Socketpool for sockets to allow use of more than one socket in application
 */
struct sSocketPool {
	Socket firstSocket;
	Socket lastSocket;
}; // sSocketPool

/**
 * Type of socket data node
 */
typedef enum {
	SN_DATA = 0,					/**< Node contains data to send. */
} SocketDataType;

/**
 * Data node of socket
 */
struct sSocketDataNode {
	void *data;						/**< Pointer to data */
	size_t dataSize;				/**< Size of data */

	SocketDataType type;			/**< Data node type */
	SocketDataNode next;			/**< Next data node */
	SocketDataNode prev;			/**< Previous data node */
};

/**
 * Socket in socketpool
 */
struct sSocket {
	int socketfd;
	socketCallback recvHandler;		/**< Triggered when socket has data
										 to receive. */
	socketCallback sendHandler;		/**< Triggered when all data has been
										 send. */
	socketCallback closedHandler;	/**< Triggered when socket has been
										 closed. This may be used to do
										 some cleanup, immediately after
										 this call is socket removed from
										 socketpool. */
	void *customData;				/**< Custom data that will be available
										 to handlers */

	SocketDataNode sendq_begin;		/**< Send queue start */
	SocketDataNode sendq_end;		/**< Socket queue end */

	Socket next;					/**< Next socket in pool */
	Socket prev;					/**< Previous socket in pool */

	bool shouldBeClosed;			/**< Socket should be closed when all
										 data has been sent. Don't set
										 this manually, use socketpool_close
										 instead. */
	SocketPool pool;				/**< Socketpool that the socket belongs
										 to. */
}; // sSocket

/**
 * Init socketpool
 * @return Initialized socketpool
 */
extern SocketPool socketpool_init(void);

/**
 * Tries to find socket in pool
 * @param pool Socketpool
 * @param socket Socket file descriptor
 * @return Socketpool socket if found, NULL otherwise.
 */
extern Socket socketpool_lookup(SocketPool pool, int socket);

/**
 * Add existing socket to socketpool
 * @param pool Socketpool
 * @param socket Socket file descriptor
 * @param recv Received data handler triggered when socket has data to receive.
 * @param send Send data handler triggered when socket has data to send.
 * @return Socketpool socket
 */
extern Socket socketpool_add(SocketPool pool, int socket, socketCallback recv,
	socketCallback send, socketCallback closed, void *customData);

/**
 * Remove socket from socketpool
 * @param socket Socket file descriptor
 */
extern void socketpool_remove(SocketPool pool, int socket);

/**
 * Sends data through socket
 * @param pool Socketpool
 * @param socket Socket file descriptor
 * @param data Pointer to data to be send.
 * @param dataSize Size of data
 */
extern void socketpool_send(SocketPool pool, int socket, void *data,
	size_t dataSize);

/**
 * Closes socket, but sends all remaining data first.
 * @param pool Socketpool
 * @param socket Socket file descriptor
 */
extern void socketpool_close(SocketPool pool, int socket);

/**
 * Shuts down socket pool. This closes all sockets that are in socketpool,
 * and before closing them, sends all remaining data.
 * @param pool Socketpool
 */
extern void socketpool_shutdown(SocketPool pool);

/**
 * Tests if sockets has some data to read and sends next data node on sockets
 * that doesn't have empty sendq.
 * @param pool Socketpool
 * @param timeout How long to wait to data before give up. In miliseconds.
 */
extern void socketpool_pool(SocketPool pool, long int timeout);

#endif
