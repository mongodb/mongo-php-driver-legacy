#ifndef __MCON_UTILS_H__
#define __MCON_UTILS_H__

#include "types.h"

char *mongo_server_create_hashed_password(char *username, char *password);
char *mongo_server_create_hash(mongo_server_def *server_def);
int mongo_server_split_hash(char *hash, char **host, int *port, char **database, char **username, char **auth_hash, int *pid);
char *mongo_server_hash_to_server(char *hash);
int mongo_server_hash_to_pid(char *hash);

/*
 * This is an OpenSSL-compatible implementation of the RSA Data Security, Inc.
 * MD5 Message-Digest Algorithm (RFC 1321).
 *
 * Homepage:
 * http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
 *
 * Author:
 * Alexander Peslyak, better known as Solar Designer <solar at openwall.com>
 *
 * This software was written by Alexander Peslyak in 2001.  No copyright is
 * claimed, and the software is hereby placed in the public domain.
 * In case this attempt to disclaim copyright and place the software in the
 * public domain is deemed null and void, then the software is
 * Copyright (c) 2001 Alexander Peslyak and it is hereby released to the
 * general public under the following terms:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 *
 * See utils.c for more information.
 */
 
/* Any 32-bit or wider unsigned integer data type will do */
typedef unsigned int MD5_u32plus;
 
typedef struct {
	MD5_u32plus lo, hi;
	MD5_u32plus a, b, c, d;
	unsigned char buffer[64];
	MD5_u32plus block[16];
} MD5_CTX;
 
char *mongo_util_md5_hex(char *hash, int hash_length);

#endif
