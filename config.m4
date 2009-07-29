PHP_ARG_ENABLE(mongo, whether to enable Mongo extension,
[  --enable-mongo   Enable Mongo extension])

PHP_ARG_ENABLE(osx, whether to compile for the gaggle of osx architectures,
[  --enable-osx   Compile for the gaggle of osx architectures], no, no)

if test "$PHP_MONGO" != "no"; then
  AC_DEFINE(HAVE_MONGO, 1, [Whether you have Mongo extension])
  PHP_NEW_EXTENSION(mongo, mongo.c mongo_types.c bson.c cursor.c collection.c db.c gridfs.c, $ext_shared)

  PHP_ADD_EXTENSION_DEP(mongo, date, false)
  PHP_ADD_EXTENSION_DEP(mongo, spl, false)

  if test "$PHP_64" != "no"; then
    CFLAGS="$CFLAGS -arch i386 -arch x86_64 -arch ppc -arch ppc64"
  fi
fi

