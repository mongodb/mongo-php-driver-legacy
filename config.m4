PHP_ARG_ENABLE(mongo, whether to enable Mongo extension,
[  --enable-mongo          Enable the MongoDB extension])

PHP_MONGO_CFLAGS="-I@ext_builddir@/util"

AC_DEFUN([MONGO_ADD_DIR], [
  PHP_ADD_BUILD_DIR([$ext_builddir/$1], 1)
  PHP_ADD_INCLUDE([$ext_builddir/$1])
  PHP_ADD_INCLUDE([$ext_srcdir/$1])

])

if test "$PHP_MONGO" != "no"; then
  AC_DEFINE(HAVE_MONGO, 1, [Whether you have Mongo extension])
  PHP_NEW_EXTENSION(mongo, php_mongo.c mongo.c mongoclient.c bson.c cursor.c command_cursor.c cursor_shared.c collection.c db.c io_stream.c log_stream.c contrib/crypto.c contrib/php-json.c contrib/php-ssl.c gridfs/gridfs.c gridfs/gridfs_cursor.c gridfs/gridfs_file.c gridfs/gridfs_stream.c exceptions/exception.c exceptions/connection_exception.c exceptions/duplicate_key_exception.c exceptions/cursor_exception.c exceptions/protocol_exception.c exceptions/cursor_timeout_exception.c exceptions/execution_timeout_exception.c exceptions/gridfs_exception.c exceptions/result_exception.c exceptions/write_concern_exception.c types/bin_data.c types/code.c types/date.c types/db_ref.c types/id.c types/int32.c types/int64.c types/regex.c types/timestamp.c util/log.c util/pool.c mcon/bson_helpers.c mcon/collection.c mcon/connections.c mcon/manager.c mcon/mini_bson.c mcon/parse.c mcon/read_preference.c mcon/str.c mcon/utils.c mcon/contrib/md5.c mcon/contrib/strndup.c api/wire_version.c api/write.c api/batch.c batch/write.c batch/insert.c batch/update.c batch/delete.c, $ext_shared,, $PHP_MONGO_CFLAGS)

  MONGO_ADD_DIR(api)
  MONGO_ADD_DIR(util)
  MONGO_ADD_DIR(exceptions)
  MONGO_ADD_DIR(gridfs)
  MONGO_ADD_DIR(types)
  MONGO_ADD_DIR(batch)
  MONGO_ADD_DIR(contrib)
  MONGO_ADD_DIR(mcon)
  MONGO_ADD_DIR(mcon/contrib)

  PHP_ADD_MAKEFILE_FRAGMENT([$ext_srcdir/Makefile.servers])

  dnl call acinclude func to check endian-ness
  PHP_C_BIGENDIAN
  if test "$ac_cv_c_bigendian_php" = "yes"; then
     CFLAGS="$CFLAGS -DPHP_C_BIGENDIAN"
  fi
  dnl default to little-endian

  case $build_os in
  darwin1*.*.*)
    AC_MSG_CHECKING([whether to compile for recent osx architectures])
    CFLAGS="$CFLAGS -arch i386 -arch x86_64 -mmacosx-version-min=10.5"
    AC_MSG_RESULT([yes])
    ;;
  darwin*)
    AC_MSG_CHECKING([whether to compile for every osx architecture ever])
    CFLAGS="$CFLAGS -arch i386 -arch x86_64 -arch ppc -arch ppc64"
    AC_MSG_RESULT([yes])
    ;;
  esac

fi

PHP_ARG_ENABLE(coverage,  whether to include code coverage symbols,
[  --enable-coverage         Mongo: Enable code coverage symbols, maintainers only!], no, no)

if test "$PHP_COVERAGE" = "yes"; then

  if test "$GCC" != "yes"; then
    AC_MSG_ERROR([GCC is required for --enable-coverage])
  fi
  
  dnl Check if ccache is being used
  case `$php_shtool path $CC` in
    *ccache*[)] gcc_ccache=yes;;
    *[)] gcc_ccache=no;;
  esac

  if test "$gcc_ccache" = "yes" && (test -z "$CCACHE_DISABLE" || test "$CCACHE_DISABLE" != "1"); then
    AC_MSG_ERROR([ccache must be disabled when --enable-coverage option is used. You can disable ccache by setting environment variable CCACHE_DISABLE=1.])
  fi
  
  lcov_version_list="1.5 1.6 1.7 1.9 1.10 1.11"

  AC_CHECK_PROG(LCOV, lcov, lcov)
  AC_CHECK_PROG(GENHTML, genhtml, genhtml)
  PHP_SUBST(LCOV)
  PHP_SUBST(GENHTML)

  if test "$LCOV"; then
    AC_CACHE_CHECK([for lcov version], php_cv_lcov_version, [
      php_cv_lcov_version=invalid
      lcov_version=`$LCOV -v 2>/dev/null | $SED -e 's/^.* //'` #'
      for lcov_check_version in $lcov_version_list; do
        if test "$lcov_version" = "$lcov_check_version"; then
          php_cv_lcov_version="$lcov_check_version (ok)"
        fi
      done
    ])
  else
    lcov_msg="To enable code coverage reporting you must have one of the following LCOV versions installed: $lcov_version_list"      
    AC_MSG_ERROR([$lcov_msg])
  fi

  case $php_cv_lcov_version in
    ""|invalid[)]
      lcov_msg="You must have one of the following versions of LCOV: $lcov_version_list (found: $lcov_version)."
      AC_MSG_ERROR([$lcov_msg])
      LCOV="exit 0;"
      ;;
  esac

  if test -z "$GENHTML"; then
    AC_MSG_ERROR([Could not find genhtml from the LCOV package])
  fi

  PHP_ADD_MAKEFILE_FRAGMENT

  dnl Remove all optimization flags from CFLAGS
  changequote({,})
  CFLAGS=`echo "$CFLAGS" | $SED -e 's/-O[0-9s]*//g'`
  CXXFLAGS=`echo "$CXXFLAGS" | $SED -e 's/-O[0-9s]*//g'`
  changequote([,])

  dnl Add the special gcc flags
  CFLAGS="$CFLAGS -O0 -fprofile-arcs -ftest-coverage"
  CXXFLAGS="$CXXFLAGS -O0 -fprofile-arcs -ftest-coverage"
fi

PHP_ARG_WITH(mongo-sasl, Build with Cyrus SASL support,
[  --with-mongo-sasl[=DIR]     Mongo: Include Cyrus SASL support], no, no)

if test "$PHP_MONGO_SASL" != "no"; then
  AC_MSG_CHECKING(for SASL)
  for i in $PHP_MONGO_SASL /usr /usr/local; do
    if test -f $i/include/sasl/sasl.h; then
      MONGO_SASL_DIR=$i
      AC_MSG_RESULT(found in $i)
      break
    fi
  done

  if test -z "$MONGO_SASL_DIR"; then
    AC_MSG_RESULT(not found)
    AC_MSG_ERROR([sasl.h not found!])
  fi

  PHP_CHECK_LIBRARY(sasl2, sasl_version,
  [
    PHP_ADD_INCLUDE($MONGO_SASL_DIR)
    PHP_ADD_LIBRARY_WITH_PATH(sasl2, $MONGO_SASL_DIR/$PHP_LIBDIR, MONGO_SHARED_LIBADD)
    AC_DEFINE(HAVE_MONGO_SASL, 1, [MONGO SASL support])
  ], [
    AC_MSG_ERROR([MONGO SASL check failed. Please check config.log for more information.])
  ], [
    -L$MONGO_SASL_DIR/$PHP_LIBDIR
  ])
  PHP_SUBST(MONGO_SHARED_LIBADD)
fi
