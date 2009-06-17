PHP_ARG_ENABLE(mongo, whether to enable Mongo extension,
[  --enable-mongo   Enable Mongo extension])

PHP_ARG_ENABLE(64, whether to compile for 64-bit architecture,
[  --enable-64   Compile for 64-bit architecture], no, no)

if test "$PHP_MONGO" != "no"; then
  AC_DEFINE(HAVE_MONGO, 1, [Whether you have Mongo extension])
  PHP_NEW_EXTENSION(mongo, mongo.c mongo_types.c bson.c cursor.c collection.c db.c gridfs.c, $ext_shared)

  PHP_ADD_EXTENSION_DEP(mongo, date, false)
  PHP_ADD_EXTENSION_DEP(mongo, spl, false)

  if test "$PHP_64" != "no"; then
    CFLAGS="$CFLAGS -m64"
    LDFLAGS="$LDFLAGS -m64"
  fi
fi

