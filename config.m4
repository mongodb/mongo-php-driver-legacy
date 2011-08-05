PHP_ARG_ENABLE(mongo, whether to enable Mongo extension,
[  --enable-mongo   Enable Mongo extension])

PHP_MONGO_CFLAGS="-I@ext_builddir@/util"

if test "$PHP_MONGO" != "no"; then
  AC_DEFINE(HAVE_MONGO, 1, [Whether you have Mongo extension])
  PHP_NEW_EXTENSION(mongo, php_mongo.c mongo.c mongo_types.c bson.c cursor.c collection.c db.c gridfs.c util/hash.c util/connect.c util/pool.c util/rs.c util/link.c util/server.c util/log.c util/io.c util/parse.c, $ext_shared,, $PHP_MONGO_CFLAGS)

  PHP_ADD_BUILD_DIR([$ext_builddir/util], 1)
  PHP_ADD_INCLUDE([$ext_builddir/util])
  PHP_ADD_INCLUDE([$ext_srcdir/util])

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

