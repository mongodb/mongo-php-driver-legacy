Copyright 2009-2014 MongoDB, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.



Portions of this code base are borrowed from other projects under the following
licenses:

contrib/php-json.c:

    /*
      +----------------------------------------------------------------------+
      | PHP Version 5                                                        |
      +----------------------------------------------------------------------+
      | Copyright (c) 1997-2014 The PHP Group                                |
      +----------------------------------------------------------------------+
      | This source file is subject to version 3.01 of the PHP license,      |
      | that is bundled with this package in the file LICENSE, and is        |
      | available through the world-wide-web at the following url:           |
      | http://www.php.net/license/3_01.txt                                  |
      | If you did not receive a copy of the PHP license and are unable to   |
      | obtain it through the world-wide-web, please send a note to          |
      | license@php.net so we can mail you a copy immediately.               |
      +----------------------------------------------------------------------+
      | Author: Omar Kilani <omar@php.net>                                   |
      +----------------------------------------------------------------------+
    */

mcon/contrib/md5.c:

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
     * (This is a heavily cut-down "BSD license".)
     *
     * This differs from Colin Plumb's older public domain implementation in that
     * no exactly 32-bit integer data type is required (any 32-bit or wider
     * unsigned integer data type will do), there's no compile-time endianness
     * configuration, and the function prototypes match OpenSSL's.  No code from
     * Colin Plumb's implementation has been reused; this comment merely compares
     * the properties of the two independent implementations.
     *
     * The primary goals of this implementation are portability and ease of use.
     * It is meant to be fast, but not as fast as possible.  Some known
     * optimizations are not included to reduce source code size and avoid
     * compile-time configuration.
     */

mcon/contrib/strndup.c:

    /*
     * Copyright (c) 1988, 1993
     *      The Regents of the University of California.  All rights reserved.
     *
     * Redistribution and use in source and binary forms, with or without
     * modification, are permitted provided that the following conditions
     * are met:
     * 1. Redistributions of source code must retain the above copyright
     *    notice, this list of conditions and the following disclaimer.
     * 2. Redistributions in binary form must reproduce the above copyright
     *    notice, this list of conditions and the following disclaimer in the
     *    documentation and/or other materials provided with the distribution.
     * 4. Neither the name of the University nor the names of its contributors
     *    may be used to endorse or promote products derived from this software
     *    without specific prior written permission.
     *
     * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
     * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
     * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
     * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
     * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
     * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
     * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
     * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
     * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
     * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
     * SUCH DAMAGE.
     */
