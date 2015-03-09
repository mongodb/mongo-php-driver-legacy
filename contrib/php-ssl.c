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


#include "php-ssl.h"

int php_mongo_matches_wildcard_name(const char *subjectname, const char *certname) /* {{{ */
{
    char *wildcard = NULL;
    int prefix_len, suffix_len, subject_len;

    if (strcasecmp(subjectname, certname) == 0) {
        return SUCCESS;
    }

    /* wildcard, if present, must only be present in the left-most component */
    if (!(wildcard = strchr(certname, '*')) || memchr(certname, '.', wildcard - certname)) {
        return FAILURE;
    }

    /* 1) prefix, if not empty, must match subject */
    prefix_len = wildcard - certname;
    if (prefix_len && strncasecmp(subjectname, certname, prefix_len) != 0) {
        return FAILURE;
    }

    suffix_len = strlen(wildcard + 1);
    subject_len = strlen(subjectname);
    if (suffix_len <= subject_len) {
        /* 2) suffix must match
         * 3) no . between prefix and suffix
         **/
        if ( strcasecmp(wildcard + 1, subjectname + subject_len - suffix_len) == 0 &&
            memchr(subjectname + prefix_len, '.', subject_len - suffix_len - prefix_len) == NULL) {
			return SUCCESS;
		}
    }

    return FAILURE;
}
/* }}} */

int php_mongo_matches_san_list(X509 *peer, const char *subject_name) /* {{{ */
{
	int i, len;
	unsigned char *cert_name = NULL;
	char ipbuffer[64];

	GENERAL_NAMES *alt_names = X509_get_ext_d2i(peer, NID_subject_alt_name, 0, 0);
	int alt_name_count = sk_GENERAL_NAME_num(alt_names);

	for (i = 0; i < alt_name_count; i++) {
		GENERAL_NAME *san = sk_GENERAL_NAME_value(alt_names, i);

		if (san->type == GEN_DNS) {
			ASN1_STRING_to_UTF8(&cert_name, san->d.dNSName);
			if ((size_t)ASN1_STRING_length(san->d.dNSName) != strlen((const char*)cert_name)) {
				OPENSSL_free(cert_name);
				/* prevent null-byte poisoning*/
				continue;
			}

			/* accommodate valid FQDN entries ending in "." */
			len = strlen((const char*)cert_name);
			if (len && strcmp((const char *)&cert_name[len-1], ".") == 0) {
				cert_name[len-1] = '\0';
			}

			if (php_mongo_matches_wildcard_name(subject_name, (const char *)cert_name) == SUCCESS) {
				OPENSSL_free(cert_name);
				return SUCCESS;
			}
			OPENSSL_free(cert_name);
		} else if (san->type == GEN_IPADD) {
			if (san->d.iPAddress->length == 4) {
				sprintf(ipbuffer, "%d.%d.%d.%d",
					san->d.iPAddress->data[0],
					san->d.iPAddress->data[1],
					san->d.iPAddress->data[2],
					san->d.iPAddress->data[3]
				);
				if (strcasecmp(subject_name, (const char*)ipbuffer) == 0) {
					return SUCCESS;
				}
			}
			/* No, we aren't bothering to check IPv6 addresses. Why?
			 * Because IP SAN names are officially deprecated and are
			 * not allowed by CAs starting in 2015. Deal with it.
			 */
		}
	}

	return FAILURE;
}
/* }}} */

int php_mongo_matches_common_name(X509 *peer, const char *subject_name TSRMLS_DC) /* {{{ */
{
	char buf[1024];
	X509_NAME *cert_name;
	int cert_name_len;

	cert_name = X509_get_subject_name(peer);
	cert_name_len = X509_NAME_get_text_by_NID(cert_name, NID_commonName, buf, sizeof(buf));

	if (cert_name_len == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to locate peer certificate CN");
	} else if ((size_t) cert_name_len != strlen(buf)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Peer certificate CN=`%.*s' is malformed", cert_name_len, buf);
	} else if (php_mongo_matches_wildcard_name(subject_name, buf) == SUCCESS) {
		return SUCCESS;
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Peer certificate CN=`%.*s' did not match expected CN=`%s'", cert_name_len, buf, subject_name);
	}

	return FAILURE;
}
/* }}} */
