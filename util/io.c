// io.c
/**
 *  Copyright 2009-2011 10gen, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef WIN32
#include <pthread.h>
#endif

#include <php.h>
#include <zend_exceptions.h>

#include "../php_mongo.h"
#include "../bson.h"
#include "../cursor.h"
#include "io.h"
#include "log.h"
#include "pool.h"
#include "rs.h"
#include "link.h"

#if WIN32
HANDLE io_mutex;
#else
static pthread_mutex_t io_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static int get_cursor_header(int sock, mongo_cursor *cursor TSRMLS_DC);
static int get_cursor_body(int sock, mongo_cursor *cursor TSRMLS_DC);
static mongo_cursor* make_persistent_cursor(mongo_cursor *cursor);
static void make_unpersistent_cursor(mongo_cursor *pcursor, mongo_cursor *cursor);

/**
 * Blocks until socket is ready.  Returns FAILURE and throws an exception if
 * something goes wrong, returns SUCCESS if the cursor is ready to be read from.
 */
static int do_timeout(mongo_server *server, int to TSRMLS_DC);

ZEND_EXTERN_MODULE_GLOBALS(mongo);

extern zend_class_entry *mongo_ce_CursorTOException;

/*
 * throws exception on FAILURE
 */
int php_mongo_get_reply(mongo_cursor *cursor, zval *errmsg TSRMLS_DC) {
  int retval = 0;

  LOCK(io);

  retval = php_mongo__get_reply(cursor, errmsg TSRMLS_CC);

  UNLOCK(io);

  return retval;
}

int php_mongo__get_reply(mongo_cursor *cursor, zval *errmsg TSRMLS_DC) {
  int sock;

  mongo_log(MONGO_LOG_IO, MONGO_LOG_FINE TSRMLS_CC, "hearing something");
  sock = cursor->server->socket;

  if (get_cursor_header(sock, cursor TSRMLS_CC) == FAILURE) {
    return FAILURE;
  }

  // check that this is actually the response we want
  if (cursor->send.request_id != cursor->recv.response_to) {
    mongo_log(MONGO_LOG_IO, MONGO_LOG_FINE TSRMLS_CC, "request/cursor mismatch: %d vs %d", cursor->send.request_id, cursor->recv.response_to);

    mongo_cursor_throw(cursor->server, 9 TSRMLS_CC, "request/cursor mismatch: %d vs %d", cursor->send.request_id, cursor->recv.response_to);
    return FAILURE;
  }

  if (FAILURE == get_cursor_body(sock, cursor TSRMLS_CC)) {
#ifdef WIN32
    mongo_cursor_throw(cursor->server, 12 TSRMLS_CC, "WSA error getting database response: %d", WSAGetLastError());
#else
    mongo_cursor_throw(cursor->server, 12 TSRMLS_CC, "error getting database response: %d", strerror(errno));
#endif
    return FAILURE;
  }

  /* if no catastrophic error has happened yet, we're fine, set errmsg to null */
  ZVAL_NULL(errmsg);

  return SUCCESS;
}

static int do_timeout(mongo_server *server, int to TSRMLS_DC) {
  struct timeval timeout;
  int sock = server->socket;

  timeout.tv_sec = to / 1000 ;
  timeout.tv_usec = (to % 1000) * 1000;

  while (1) {
    int status;
    fd_set readfds, exceptfds;

    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    FD_ZERO(&exceptfds);
    FD_SET(sock, &exceptfds);

    status = select(sock+1, &readfds, NULL, &exceptfds, &timeout);

    if (status == -1) {
      // on EINTR, retry
      if (errno == EINTR) {
        continue;
      }

      mongo_cursor_throw(server, 13 TSRMLS_CC, strerror(errno));
      return FAILURE;
    }

    if (FD_ISSET(sock, &exceptfds)) {
      mongo_cursor_throw(server, 17 TSRMLS_CC, "Exceptional condition on socket");
      return FAILURE;
    }

    if (status == 0 && !FD_ISSET(sock, &readfds)) {
      zend_throw_exception_ex(mongo_ce_CursorTOException, 0 TSRMLS_CC,
                              "cursor timed out (timeout: %d, time left: %d:%d, status: %d)",
                              to, timeout.tv_sec, timeout.tv_usec, status);
      return FAILURE;
    }

    // if our descriptor is ready break out
    if (FD_ISSET(sock, &readfds)) {
      break;
    }
  }

  return SUCCESS;
}

/*
 * This method reads the message header for a database response
 * It returns failure or success and throws an exception on failure.
 */
static int get_cursor_header(int sock, mongo_cursor *cursor TSRMLS_DC) {
  int status = 0;
  int num_returned = 0;
  char buf[REPLY_HEADER_LEN];

  // set a timeout
  if (cursor->timeout && cursor->timeout > 0 &&
      do_timeout(cursor->server, cursor->timeout TSRMLS_CC) == FAILURE) {
    return FAILURE;
  }

  status = recv(sock, buf, REPLY_HEADER_LEN, FLAGS);
  // socket has been closed, retry
  if (status == 0) {
    return FAILURE;
  }
  else if (status < INT_32*4) {
    mongo_cursor_throw(cursor->server, 4 TSRMLS_CC, "couldn't get response header");
    return FAILURE;
  }

  // switch the byte order, if necessary
  cursor->recv.length = MONGO_32(*(int*)buf);

  // make sure we're not getting crazy data
  if (cursor->recv.length == 0) {
    mongo_cursor_throw(cursor->server, 5 TSRMLS_CC, "no db response");
    return FAILURE;
  }
  else if (cursor->recv.length < REPLY_HEADER_SIZE) {
    mongo_cursor_throw(cursor->server, 6 TSRMLS_CC,
                       "bad response length: %d, did the db assert?",
                       cursor->recv.length);
    return FAILURE;
  }

  cursor->recv.request_id  = MONGO_32(*(int*)(buf+INT_32));
  cursor->recv.response_to = MONGO_32(*(int*)(buf+INT_32*2));
  cursor->recv.op          = MONGO_32(*(int*)(buf+INT_32*3));
  cursor->flag             = MONGO_32(*(int*)(buf+INT_32*4));
  cursor->cursor_id        = MONGO_64(*(int64_t*)(buf+INT_32*5));
  cursor->start            = MONGO_32(*(int*)(buf+INT_32*5+INT_64));
  num_returned             = MONGO_32(*(int*)(buf+INT_32*6+INT_64));

  if (cursor->recv.response_to > MonGlo(response_num)) {
    MonGlo(response_num) = cursor->recv.response_to;
  }

  // cursor->num is the total of the elements we've retrieved (elements already
  // iterated through + elements in db response but not yet iterated through)
  cursor->num += num_returned;

  // create buf
  cursor->recv.length -= REPLY_HEADER_LEN;

  return SUCCESS;
}

static int get_cursor_body(int sock, mongo_cursor *cursor TSRMLS_DC) {
  if (cursor->buf.start) {
    efree(cursor->buf.start);
  }

  cursor->buf.start = (char*)emalloc(cursor->recv.length);
  cursor->buf.end = cursor->buf.start + cursor->recv.length;
  cursor->buf.pos = cursor->buf.start;

  // finish populating cursor
  return mongo_hear(sock, cursor->buf.pos, cursor->recv.length TSRMLS_CC);
}

/*
 * Low-level send function.
 *
 * Goes through the buffer sending 4K byte batches.
 * On failure, sets errmsg to errno string.
 * On success, returns number of bytes sent.
 * Does not attempt to reconnect nor throw any exceptions.
 *
 * On failure, the calling function is responsible for disconnecting
 */
int _mongo_say(int sock, buffer *buf, zval *errmsg TSRMLS_DC) {
  int sent = 0, total = 0, status = 1;

  mongo_log(MONGO_LOG_IO, MONGO_LOG_FINE TSRMLS_CC, "saying something");

  total = buf->pos - buf->start;

  while (sent < total && status > 0) {
    int len = 4096 < (total - sent) ? 4096 : total - sent;

    status = send(sock, (const char*)buf->start + sent, len, FLAGS);

    if (status == FAILURE) {
      ZVAL_STRING(errmsg, strerror(errno), 1);
      return FAILURE;
    }
    sent += status;
  }

  return sent;
}

int mongo_say(mongo_server *server, buffer *buf, zval *errmsg TSRMLS_DC) {
  if(mongo_util_pool_refresh(server, 0 TSRMLS_CC) == FAILURE) {
    ZVAL_STRING(errmsg, "couldn't get socket to send on", 1);
    return FAILURE;
  }

  if (_mongo_say(server->socket, buf, errmsg TSRMLS_CC) == FAILURE) {
    // try to reconnect, but we can't retry the send regardless
    mongo_util_pool_failed(server TSRMLS_CC);
    return FAILURE;
  }
  return SUCCESS;
}

int mongo_hear(int sock, void *dest, int total_len TSRMLS_DC) {
  int num = 1, received = 0;

  // this can return FAILED if there is just no more data from db
  while(received < total_len && num > 0) {
    int len = 4096 < (total_len - received) ? 4096 : total_len - received;

    // windows gives a WSAEFAULT if you try to get more bytes
    num = recv(sock, (char*)dest, len, FLAGS);

    if (num < 0) {
      return FAILURE;
    }

    dest = (char*)dest + num;
    received += num;
  }
  return received;
}

static mongo_cursor* make_persistent_cursor(mongo_cursor *cursor) {
  mongo_cursor *pcursor;
  int len;

  pcursor = (mongo_cursor*)pemalloc(sizeof(mongo_cursor), 1);
  // copying the whole cursor is easier, but we'll only need certain fields
  memcpy(pcursor, cursor, sizeof(mongo_cursor));

  pcursor->recv.length = cursor->recv.length;
  pcursor->recv.request_id = cursor->recv.request_id;
  pcursor->recv.response_to = cursor->recv.response_to;
  pcursor->recv.op = cursor->recv.op;

  len = cursor->buf.end - cursor->buf.start;
  pcursor->buf.start = (char*)pemalloc(len, 1);
  memcpy(pcursor->buf.start, cursor, len);
  pcursor->buf.pos = pcursor->buf.start;
  pcursor->buf.end = pcursor->buf.start+len;

  return pcursor;
}

/*
 * Copy response fields from a persistent cursor to a normal cursor and then
 * free the persistent cursor.
 */
static void make_unpersistent_cursor(mongo_cursor *pcursor, mongo_cursor *cursor) {
  int len;

  // header
  cursor->recv.length = pcursor->recv.length;
  cursor->recv.request_id = pcursor->recv.request_id;
  cursor->recv.response_to = pcursor->recv.response_to;
  cursor->recv.op = pcursor->recv.op;

  // field populated by the response
  cursor->flag = pcursor->flag;
  cursor->cursor_id = pcursor->cursor_id;
  cursor->start = pcursor->start;
  cursor->num = pcursor->num;

  // the actual response
  len = pcursor->buf.end - pcursor->buf.start;
  cursor->buf.start = (char*)emalloc(len);
  memcpy(cursor->buf.start, pcursor, len);
  cursor->buf.pos = cursor->buf.start;
  cursor->buf.end = cursor->buf.start+len;

  // free persistent cursor
  pefree(pcursor->buf.start, 1);
  pefree(pcursor, 1);
}
