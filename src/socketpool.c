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

// Standard headers
#include <string.h>
#include <errno.h>
#include <stdbool.h>

// Linux includes
#include <unistd.h>
#include <sys/socket.h>

// My interfaces
#include "io.h"

// This library interface
#include "socketpool.h"

#ifdef max
# undef max
#endif
#define max(a,b) (((a)>(b))?(a):(b))

// Define this to see socketpool debug messages.
//#define SOCKETPOOL_DEBUG 1
#ifdef SOCKETPOOL_DEBUG
# define socketpool_debugmsg(...) printError("socketpool", __VA_ARGS__)
#else
# define socketpool_debugmsg(...)
#endif

/**
 * Init socketpool
 * @return Initialized socketpool
 */
SocketPool socketpool_init(void) {
	SocketPool pool = malloc(sizeof(struct sSocketPool));

	// No sockets in pool by default
	pool->firstSocket = NULL;
	pool->lastSocket = NULL;

	return pool;
} // socketpool_init

/**
 * Tries to find socket in pool
 * @param pool Socketpool
 * @param socket Socket file descriptor
 * @return Socketpool socket if found, NULL otherwise.
 */
Socket socketpool_lookup(SocketPool pool, int socket) {
	Socket s = pool->firstSocket;
	while (s != NULL) {
		if (s->socketfd == socket) {
			return s;
		}

		s = s->next;
	}

	return NULL;
} // socketpool_lookup

/**
 * Add existing socket to socketpool
 * @param pool Socketpool
 * @param socket Socket file descriptor
 * @param recv Received data handler triggered when socket has data to receive.
 * @param send Send data handler triggered when socket has data to send.
 * @param closed Closed handler triggered when socket is closed.
 * @return Socketpool socket
 */
Socket socketpool_add(SocketPool pool, int socket, socketCallback recv,
	socketCallback send, socketCallback closed, void *customData) {

	socketpool_debugmsg("Add socket %d to pool.", socket);
	socketpool_debugmsg("recv: %s, send: %s, closed: %s", (recv != NULL)?"active":"inactive", (send != NULL)?"active":"inactive", (closed != NULL)?"active":"inactive");

	Socket poolsock = socketpool_lookup(pool, socket);
	if (!poolsock) {
		poolsock = malloc(sizeof(struct sSocket));
		poolsock->socketfd = socket;
		poolsock->sendq_begin = NULL;
		poolsock->sendq_end = NULL;
		poolsock->shouldBeClosed = false;
		poolsock->pool = pool;

		poolsock->next = NULL;
		poolsock->prev = pool->lastSocket;
		pool->lastSocket = poolsock;
		if (poolsock->prev != NULL) {
			poolsock->prev->next = poolsock;
		} else {
			pool->firstSocket = poolsock;
		}
	} else {
		socketpool_debugmsg("Socket %d is already in pool, change it's "
			"handlers.", socket);
	}

	poolsock->recvHandler = recv;
	poolsock->sendHandler = send;
	poolsock->closedHandler = closed;
	poolsock->customData = customData;

	return poolsock;
} // socketpool_add

/**
 * Remove socket from socketpool
 * @param socket Socket file descriptor
 */
void socketpool_remove(SocketPool pool, int socket) {
	Socket poolsock = socketpool_lookup(pool, socket);

	socketpool_debugmsg("Remove socket %d from pool.", socket);

	if (poolsock != NULL) {
		// Remove socket from chain
		if (poolsock->prev != NULL) {
			poolsock->prev->next = poolsock->next;
		} else {
			pool->firstSocket = poolsock->next;
		}

		if (poolsock->next != NULL) {
			poolsock->next->prev = poolsock->prev;
		} else {
			pool->lastSocket = poolsock->prev;
		}

		// Free socket
		free(poolsock);
	}
} // socketpool_remove

/**
 * Print content of sendq.
 * @param socket Socket in pool
 */
void socketpool_debugqueue(Socket socket) {
	SocketDataNode node = socket->sendq_begin;
	if (node == NULL) {
		printError("socketpool", "sendq is empty.");
	}
	while (node != NULL) {
		if (node->type == SN_DATA) {
			char *data = malloc((node->dataSize + 1) * sizeof(char));
			memcpy(data, node->data, node->dataSize);
			data[node->dataSize] = '\0';

			printError("socketpool", "Datanode: %s", data);
			free(data);
		} else {
			printError("socketpool", "Closenode.");
		}

		node = node->next;
	}
} // socketpool_debugqueue

/**
 * Add data node into socket sendq
 * @param socket Socket in pool.
 * @param data Pointer to data to be send.
 * @param dataSize Size of data.
 * @param type Node type
 */
void socketpool_addtosendq(Socket socket, void *data, size_t dataSize,
	SocketDataType type) {

	if (socket == NULL) return;

	SocketDataNode node = malloc(sizeof(struct sSocketDataNode));

	// Copy data to node, because we cannot be sure, that original
	// data pointer will be valid when node is going to be send.
	node->dataSize = dataSize;
	if (dataSize > 0) {
		node->data = malloc(dataSize);
		memcpy(node->data, data, dataSize);
	} else {
		node->data = NULL;
	}

	node->type = type;

	// Add node to sendq

	SocketDataNode sendq = socket->sendq_end;
	while (sendq != NULL) {
		// Find first node with lowest type than currently added
		// one.
		if (sendq->type < node->type) {
			break;
		}
		sendq = sendq->prev;
	}

	if (sendq == NULL) {
		// Add to end of sendq
		node->next = NULL;
		node->prev = socket->sendq_end;
		socket->sendq_end = node;

		if (node->prev != NULL) {
			node->prev->next = node;
		} else {
			socket->sendq_begin = node;
		}
	} else {
		// Add node after found node into sendq.
		node->prev = sendq;
		node->next = sendq->next;

		if (node->next != NULL) {
			node->next->prev = node;
		} else {
			socket->sendq_end = node;
		}

		if (node->prev != NULL) {
			node->prev->next = node;
		} else {
			socket->sendq_begin = node;
		}
	}
} // socketpool_addtosendq

/**
 * Remove node from socket's sendq
 * @param socket Socket in pool
 * @param node Data node in socket's sendq.
 */
void socketpool_removefromsendq(Socket socket, SocketDataNode node) {
	if (socket == NULL || node == NULL) return;

	// Remove node from chain
	if (node->next != NULL) {
		// In the middle of queue
		node->next->prev = node->prev;
	} else {
		// In the end of queue
		socket->sendq_end = node->prev;
	}

	if (node->prev != NULL) {
		// In the middle of queue
		node->prev->next = node->next;
	} else {
		// At the begining of queue
		socket->sendq_begin = node->next;
	}

	// Free node data
	if (node->data != NULL) {
		free(node->data);
	}

	// Free node
	free(node);
} // socketpool_removefromsendq

/**
 * Sends data through socket
 * @param pool Socketpool
 * @param socket Socket file descriptor
 * @param data Pointer to data to be send.
 * @param dataSize Size of data
 */
void socketpool_send(SocketPool pool, int socket, void *data,
	size_t dataSize) {

	Socket poolsock = socketpool_lookup(pool, socket);

	if (poolsock != NULL) {
		socketpool_addtosendq(poolsock, data, dataSize, SN_DATA);
	} else {
		printError("socketpool", "send: Socket %d is not in pool.", socket);
	}
} // socketpool_send

/**
 * Closes socket, but sends all remaining data first.
 * @param pool Socketpool
 * @param socket Socket file descriptor
 */
void socketpool_close(SocketPool pool, int socket) {

	Socket poolsock = socketpool_lookup(pool, socket);
	if (poolsock != NULL) {
		poolsock->shouldBeClosed = true;

		// No data are remaining to be send, it is safe to close the socket.
		if (poolsock->sendq_begin == NULL) {
			socketpool_debugmsg("Close socket %d.", socket);

			// Call the socket closed handler...
			if (poolsock->closedHandler != NULL) {
				poolsock->closedHandler(poolsock);
			}

			close(poolsock->socketfd);
			socketpool_remove(pool, socket);
		} else {
			socketpool_debugmsg("Close socket %d - delayed.", socket);
		}
	} else {
		printError("socketpool", "close: Socket %d is not in pool.", socket);
		close(socket);
	}
} // socketpool_close

/**
 * Send one data node on socket.
 * @param socket Socket in pool
 */
void socketpool_sendnode(Socket socket) {
	if (socket == NULL) return;
	if (socket->sendq_begin == NULL) return;

	SocketDataNode node = socket->sendq_begin;
	ssize_t written = write(socket->socketfd, node->data, node->dataSize);
	if (written < 0) {
		printError("socketpool", "Error when sending data: %s",
			strerror(errno));

		socketpool_close(socket->pool, socket->socketfd);
	} else if ((size_t)written < node->dataSize) {
		// Not all data in node has been sent, don't delete node, but
		// keep it alive, to send remaining data.
		size_t newDataSize = node->dataSize - written;
		void *newData = malloc(newDataSize);
		memcpy(newData, node->data + written, newDataSize);

		free(node->data);
		node->data = NULL;
		node->dataSize = 0;

		if (newData != NULL) {
			node->data = newData;
			node->dataSize = newDataSize;
		} else {
			socketpool_removefromsendq(socket, node);
		}
	} else {
		// Node sent, remove it from sendq.
		socketpool_removefromsendq(socket, node);
	}

	// No data remaining, fire event, if set.
	if (socket->sendq_begin == NULL) {
		if (socket->shouldBeClosed) {
			socketpool_close(socket->pool, socket->socketfd);
		} else if (socket->sendHandler != NULL) {
			socket->sendHandler(socket);
		}
	}
} // socketpool_sendnode

/**
 * Shuts down socket pool. This closes all sockets that are in socketpool,
 * and before closing them, sends all remaining data.
 * @param pool Socketpool
 */
void socketpool_shutdown(SocketPool pool) {
	// Don't create read fd_set, we want only to send all remaining data...
	fd_set wrsock;

	printError("socketpool", "Sending all remaining data on sockets.");

	// Set all sockets to close
	Socket socket = pool->firstSocket;
	Socket next;
	while (socket != NULL) {
		next = socket->next;
		socketpool_close(pool, socket->socketfd);
		socket = next;
	}

	bool allEmpty = false;
	do {
		allEmpty = true;
		FD_ZERO(&wrsock);

		Socket socket = pool->firstSocket;
		int highestSocket = 0;

		// Add sockets that has something to say to queue.
		Socket next;
		while (socket != NULL) {
			next = socket->next;
			if (socket->sendq_begin != NULL) {
				allEmpty = false;

				FD_SET(socket->socketfd, &wrsock);
				highestSocket = max(highestSocket, socket->socketfd);
			} else {
				// This shouldn't happen.
				socketpool_close(pool, socket->socketfd);
			}

			socket = next;
		}

		// If at least one socket has something to say
		if (!allEmpty) {
			ssize_t wrsocks =
				select(highestSocket + 1, NULL, &wrsock, NULL, NULL);

			if (wrsocks < 0) {
				printError("socketpool", "select error: %s", strerror(errno));
			} else if (wrsocks > 0) {
				socket = pool->firstSocket;
				Socket next;
				while (socket != NULL) {
					next = socket->next;

					// Remove socket handlers, at shutdown, we don't want to
					// know that socket don't have data to send.
					socket->sendHandler = NULL;

					if (FD_ISSET(socket->socketfd, &wrsock) &&
						socket->sendq_begin != NULL) {

						socketpool_sendnode(socket);
					}

					socket = next;
				}
			}
		}
	} while (!allEmpty);

	free(pool);
} // socketpool_shutdown

/**
 * Tests if sockets has some data to read and sends next data node on sockets
 * that doesn't have empty sendq.
 * @param pool Socketpool
 * @param timeout How long to wait to data before give up. In miliseconds.
 */
void socketpool_pool(SocketPool pool, long int timeout) {
	// Read and write socket sets
	fd_set rdsock, wrsock;
	FD_ZERO(&rdsock);
	FD_ZERO(&wrsock);

	// Fill in socket sets
	Socket socket = pool->firstSocket;
	int highestSocket = 0;
	while (socket != NULL) {
		highestSocket = max(highestSocket, socket->socketfd);

		FD_SET(socket->socketfd, &rdsock);

		if (socket->sendq_begin != NULL) {
			FD_SET(socket->socketfd, &wrsock);
		}

		socket = socket->next;
	}

	struct timeval tv = {
		.tv_sec = timeout / 1000,
		.tv_usec = 0
	};
	tv.tv_usec = (timeout - tv.tv_sec * 1000) * 1000;

	int socks = select(highestSocket + 1, &rdsock, &wrsock, NULL, &tv);
	if (socks < 0) {
		// Don't print that socketpool was interrupted, because it isn't
		// error.
		if (errno != EINTR) {
			printError("socketpool", "select error: %s", strerror(errno));
		}
	} else if (socks > 0) {
		socket = pool->firstSocket;
		Socket next;
		while (socket != NULL) {
			next = socket->next;

			// Must do this, because in read handler socket can be closed,
			// but it won't be closed if has still some data to read.
			bool hasSomethingToSend = socket->sendq_begin != NULL;

			// Socket has something to receive
			if (FD_ISSET(socket->socketfd, &rdsock)) {
				socketpool_debugmsg("Socket %d has something to receive.", socket->socketfd);
				if (socket->recvHandler != NULL) {
					socket->recvHandler(socket);
				} else {
					socketpool_debugmsg("Socket %d has no recv handler set.",
						socket->socketfd);
				}
			}

			// Socket can send
			if (hasSomethingToSend && FD_ISSET(socket->socketfd, &wrsock)) {
				socketpool_sendnode(socket);
			}

			socket = next;
		}
	}
} // socketpool_pool
