PHP_ARG_ENABLE(mongo, whether to enable Mongo extension,
[  --enable-mongo   Enable Mongo extension])

if test "$PHP_MONGO" != "no"; then
  AC_DEFINE(HAVE_MONGO, 1, [Whether you have Mongo extension])
  PHP_NEW_EXTENSION(mongo, mongo.c mongo_types.c bson.c cursor.c collection.c db.c gridfs.c, $ext_shared)

  PHP_ADD_EXTENSION_DEP(mongo, date, false)
  PHP_ADD_EXTENSION_DEP(mongo, spl, false)

  case $build_os in
  darwin10.0.0)
    AC_MSG_CHECKING([whether to compile for recent osx architectures])
    CFLAGS="$CFLAGS -arch i386 -arch x86_64"
    AC_MSG_RESULT([yes])
    ;;
  darwin*)
    AC_MSG_CHECKING([whether to compile for every osx architecture ever])
    CFLAGS="$CFLAGS -arch i386 -arch x86_64 -arch ppc -arch ppc64"
    AC_MSG_RESULT([yes])
    ;;
  esac
fi

