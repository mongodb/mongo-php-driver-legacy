/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Wez Furlong <wez@thebrainroom.com>                          |
  |          Daniel Lowrey <rdlowrey@php.net>                            |
  |          Chris Wright <daverandom@php.net>                           |
  +----------------------------------------------------------------------+
*/

#ifndef MONGO_CONTRIB_SSL_H
#define MONGO_CONTRIB_SSL_H

#ifdef PHP_WIN32
# include "config.w32.h"
#else
# include <php_config.h>
#endif

#ifdef HAVE_OPENSSL_EXT

#include "php.h"
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

int php_mongo_matches_wildcard_name(const char *subjectname, const char *certname);
int php_mongo_matches_san_list(X509 *peer, const char *subject_name);
int php_mongo_matches_common_name(X509 *peer, const char *subject_name TSRMLS_DC);
time_t php_mongo_asn1_time_to_time_t(ASN1_UTCTIME * timestr TSRMLS_DC);

#endif /* HAVE_OPENSSL_EXT */
#endif 

